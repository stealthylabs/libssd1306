/*
 * COPYRIGHT: 2020. Stealthy Labs LLC.
 * DATE: 2020-01-15
 * SOFTWARE: libssd1306-i2c
 * LICENSE: Refer license file
 */
#include <ssd1306_config.h>
#if HAVE_FEATURES_H
#include <features.h>
#endif
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <ssd1306_graphics.h>

#ifndef SSD1306_FB_BAD_PTR_RETURN
#define SSD1306_FB_BAD_PTR_RETURN(P,RC) do { \
  if (!(P != NULL && (P)->buffer != NULL && (P)->len > 0)) { \
    return RC; \
  } \
} while (0)
#endif // SSD1306_FB_BAD_PTR_RETURN
#ifndef SSD1306_FB_GET_ERRFP
#define SSD1306_FB_GET_ERRFP(P) ((P) != NULL && (P)->err != NULL && (P)->err->err_fp != NULL) ? (P)->err->err_fp : stderr;
#endif

const char *ssd1306_fb_version(void)
{
    return PACKAGE_VERSION;
}

ssd1306_err_t *ssd1306_err_create(FILE *fp)
{
    if (fp == NULL)
        fp = stderr;
    ssd1306_err_t *err = calloc(1, sizeof(ssd1306_err_t));
    if (!err) {
        fprintf(fp, "ERROR: Out of memory allocating %zu bytes\n", sizeof(ssd1306_err_t));
        return NULL;
    }
    err->err_fp = fp;
    err->errnum = 0;
    err->errlen = 256;
    err->errbuf = calloc(1, err->errlen);
    if (!err->errbuf) {
        fprintf(fp, "ERROR: Failed to allocate memory of size %zu bytes\n",
                err->errlen);
        free(err);
        return NULL;
    }
    SSD1306_ATOMIC_ZERO(&(err->_ref));
    SSD1306_ATOMIC_INCREMENT(&(err->_ref));
    return err;
}

void ssd1306_err_destroy(ssd1306_err_t *err)
{
    if (err) {
        int zero = 0;
        SSD1306_ATOMIC_DECREMENT(&(err->_ref));
        if (SSD1306_ATOMIC_IS_EQUAL(&(err->_ref), &zero)) {
            if (err->errbuf) {
                free(err->errbuf);
                err->errbuf = NULL;
            }
            if (err->err_fp != NULL && err->err_fp != stderr) {
                fclose(err->err_fp);
            }
            memset(err, 0, sizeof(*err));
            free(err);
            err = NULL;
        }
    }
}

ssd1306_framebuffer_t *ssd1306_framebuffer_create(uint8_t width, uint8_t height, ssd1306_err_t *err)
{
    FILE *err_fp = (err != NULL && err->err_fp != NULL) ? err->err_fp : stderr;
    if (width == 0 || height == 0) {
        fprintf(err_fp, "ERROR: Width: %zd Height: %zd cannot be zero\n", width, height);
        return NULL;
    }
    ssd1306_framebuffer_t *fbp = calloc(1, sizeof(ssd1306_framebuffer_t));
    if (!fbp) {
        fprintf(err_fp, "ERROR: Failed to allocate memory of size %zu bytes\n", sizeof(*fbp));
        return NULL;
    }
    int rc = 0;
    do {
        fbp->width = width;
        fbp->height = height;
        fbp->err = err;
        SSD1306_ERR_REF_INC(err);
        fbp->len = sizeof(uint8_t) * (fbp->width * fbp->height) / 8;
        fbp->buffer = calloc(1, fbp->len);
        if (!fbp->buffer) {
            fprintf(err_fp, "ERROR: Failed to allocate memory of size %zu bytes\n", fbp->len);
            fbp->buffer = NULL;
            rc = -1;
            break;
        }
    } while (0);
    if (rc < 0) {
        ssd1306_framebuffer_destroy(fbp);
        fbp = NULL;
    }
    return fbp;
}

void ssd1306_framebuffer_destroy(ssd1306_framebuffer_t *fbp)
{
    if (fbp) {
        ssd1306_err_destroy(fbp->err);
        fbp->err = NULL;
        if (fbp->buffer) {
            free(fbp->buffer);
            fbp->buffer = NULL;
        }
        memset(fbp, 0, sizeof(*fbp));
        free(fbp);
        fbp = NULL;
    }
}

int ssd1306_framebuffer_hexdump(const ssd1306_framebuffer_t *fbp)
{
    SSD1306_FB_BAD_PTR_RETURN(fbp, -1);
    FILE *err_fp = SSD1306_FB_GET_ERRFP(fbp);
    const uint8_t *fb = fbp->buffer;
    size_t fblen = fbp->len;
    size_t rows = fbp->height;
    size_t cols = fbp->width / 8;
    fprintf(err_fp, "DEBUG: No. of rows: %zu cols: %zu\n", rows, cols);
    for (size_t i = 0; i < rows; ++i) {
        fprintf(err_fp, "%04zX ", i);
        for (size_t j = 0; j < cols; ++j) {
            if ((i * cols + j) < fblen)
                fprintf(err_fp, "%02X ", fb[i * cols + j]);
            else
                fprintf(err_fp, "   ");
        }
        fprintf(err_fp, " \n");
    }
    return 0;
}

int ssd1306_framebuffer_bitdump(const ssd1306_framebuffer_t *fbp,
            char zerobit, char onebit, bool use_space)
{
    SSD1306_FB_BAD_PTR_RETURN(fbp, -1);
    FILE *err_fp = SSD1306_FB_GET_ERRFP(fbp);
    const uint8_t *fb = fbp->buffer;
    size_t fblen = fbp->len;
    size_t rows = fbp->height;
    size_t cols = fbp->width / 8;
    if (!isprint(zerobit)) {
        zerobit = '.';
    }
    if (!isprint(onebit)) {
        onebit = '|';
    }
    fprintf(err_fp, "DEBUG: No. of cols: %zu rows: %zu\n", cols, rows);
    for (size_t i = 0; i < rows; ++i) {
        fprintf(err_fp, "%04zX ", i);
        for (size_t j = 0; j < cols; ++j) {
            if ((i * cols + j) < fblen) {
                for (int8_t k = 7; k >= 0; k--) {
                    uint8_t ch = fb[i * cols + j];
                    ch = ((ch >> k) & 0x01) ? onebit : zerobit;
                    fprintf(err_fp, "%c", ch);
                }
                if (use_space) {
                    fprintf(err_fp, "%c", ' ');
                }
            } else {
                if (use_space) {
                    fprintf(err_fp, "        ");// 8 spaces
                }
            }
        }
        fprintf(err_fp, "\n");
    }
    return 0;
}

int ssd1306_framebuffer_clear(ssd1306_framebuffer_t *fbp)
{
    SSD1306_FB_BAD_PTR_RETURN(fbp, -1);
    memset(fbp->buffer, 0, fbp->len);
    return 0;
}

int ssd1306_framebuffer_draw_bricks(ssd1306_framebuffer_t *fbp)
{
    SSD1306_FB_BAD_PTR_RETURN(fbp, -1);
    uint8_t *fb = fbp->buffer;
    size_t fblen = fbp->len;
    for (size_t i = 0; i < fblen; ++i) {
        if (i % 1) {
            fb[i] = 0xFF;
        }
        if (i % 3) {
            fb[i] = 0x7F;
        }
        if (i % 5) {
            fb[i] = 0x3F;
        }
    }
    return 0;
}

int ssd1306_framebuffer_draw_pixel(ssd1306_framebuffer_t *fbp, uint8_t x, uint8_t y, bool clear)
{
    SSD1306_FB_BAD_PTR_RETURN(fbp, -1);
    uint8_t *fb = fbp->buffer;
    size_t fblen = fbp->len;
#if DEBUG
    FILE *err_fp = SSD1306_FB_GET_ERRFP(fbp);
    fprintf(err_fp, "DEBUG: w: %zu h: %zu, x: %zu, y: %zu\n", fbp->width, fbp->height, x, y);
#endif
    x = x % fbp->width; // if x > fbp->width, rotate screen
    y = y % fbp->height;// if y > fbp->height, rotate
    // find the byte to edit first
    uint8_t bit = x % 8; // the position of the bit right to left
    size_t idx = ((size_t)((x - bit) / 8));
    idx = idx + (y * (size_t)(fbp->width / 8)); // find the correct row
    uint8_t ch = (0x80 >> bit);
#if DEBUG
    fprintf(err_fp, "DEBUG: idx: %zu ch: %x\n", idx, ch);
#endif
    // we do not use xor here since if a pixel is filled, and we fill it again
    // it should stay filled.
    if (clear) {// clear the bit.
        fb[idx] &= ((~ch) & 0xFF);
    } else {// fill the bit
        fb[idx] |= ch;
    }
    return 0;
}
