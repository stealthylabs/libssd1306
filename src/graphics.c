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
#ifdef HAVE_FREETYPE2
#include <ft2build.h>
#include FT_FREETYPE_H
#else
#error "Freetype2 required for compiling this file"
#endif
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
#ifndef SSD1306_ERR_GET_ERRFP
#define SSD1306_ERR_GET_ERRFP(P) ((P) != NULL && (P)->err_fp != NULL) ? (P)->err_fp : stderr;
#endif

#ifdef HAVE_PTHREAD
    #include <pthread.h>
    typedef pthread_mutex_t ssd1306_lock_t;
    #define SSD1306_LOCK(A) pthread_mutex_lock((A))
    #define SSD1306_UNLOCK(A) pthread_mutex_unlock((A))
    #define SSD1306_LOCK_DESTROY(A) pthread_mutex_destroy((A))
    #define SSD1306_LOCK_CREATE(A,B) \
    do { \
        pthread_mutexattr_t mattr;\
        pthread_mutexattr_init(&mattr);\
        if ((B)) \
            pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE); \
        pthread_mutex_init((A), &mattr);\
        pthread_mutexattr_destroy(&mattr); \
    } while (0)
#else
    typedef void * ssd1306_lock_t;
    #define SSD1306_LOCK(A) do{} while(0)
    #define SSD1306_UNLOCK(A) do{} while(0)
    #define SSD1306_LOCK_DESTROY(A) do{} while(0)
    #define SSD1306_LOCK_CREATE(A,B) do{} while(0)
#endif

struct ssd1306_font_ {
    FT_Library lib;
    ssd1306_lock_t _lock;
};

static void ssd1306_font_destroy(ssd1306_font_t *font, ssd1306_err_t *err);

static ssd1306_font_t *ssd1306_font_create(ssd1306_err_t *err)
{
    FILE *err_fp = SSD1306_ERR_GET_ERRFP(err);
    ssd1306_font_t *font = calloc(1, sizeof(ssd1306_font_t));
    if (font) {
        SSD1306_LOCK_CREATE(&(font->_lock), true);// allow recursive lock
        FT_Error ferr = FT_Init_FreeType(&(font->lib));
        if (ferr) {
            fprintf(err_fp, "ERROR: Freetype FT_Init_FreeType() error: %d\n", ferr);
            font->lib = NULL;
            ssd1306_font_destroy(font, err);
            font = NULL;
        }
        return font;
    } else {
        fprintf(err_fp, "ERROR: Out of memory allocating %zu bytes\n", sizeof(ssd1306_font_t));
        return NULL;
    }
}

static void ssd1306_font_destroy(ssd1306_font_t *font, ssd1306_err_t *err)
{
    if (font) {
        if (font->lib) {
            SSD1306_LOCK(&(font->_lock));
            // cleanup
            FT_Error ferr = FT_Done_FreeType(font->lib);
            if (ferr) {
                FILE *err_fp = SSD1306_ERR_GET_ERRFP(err);   
                fprintf(err_fp, "WARN: Freetype FT_Done_FreeType() error: %d\n", ferr);
            }
            font->lib = NULL;
            SSD1306_UNLOCK(&(font->_lock));
        }
        SSD1306_LOCK_DESTROY(&(font->_lock));
        memset(font, 0, sizeof(*font));
        free(font);
        font = NULL;
    }
}

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
    FILE *err_fp = SSD1306_ERR_GET_ERRFP(err);
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

int ssd1306_framebuffer_bitdump_custom(const ssd1306_framebuffer_t *fbp,
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

int ssd1306_framebuffer_put_pixel(ssd1306_framebuffer_t *fbp, uint8_t x, uint8_t y, bool color)
{
    SSD1306_FB_BAD_PTR_RETURN(fbp, -1);
    uint8_t *fb = fbp->buffer;
#ifdef DEBUG
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
    // we do not use xor here since if a pixel is filled, and we fill it again
    // it should stay filled.
#ifdef DEBUG
    uint8_t old_byte = fb[idx];
#endif
    if (color) {// color the bit.
        fb[idx] |= ch;
    } else {// clear the bit
        fb[idx] &= ((~ch) & 0xFF);
    }
#ifdef DEBUG
    fprintf(err_fp, "DEBUG: idx: %zu ch: 0x%x fb[%zu]: Before: 0x%x After: 0x%x\n",
            idx, ch, idx, old_byte, fb[idx]);
#endif
    return 0;
}

int8_t ssd1306_framebuffer_get_pixel(ssd1306_framebuffer_t *fbp, uint8_t x, uint8_t y)
{
    SSD1306_FB_BAD_PTR_RETURN(fbp, -1);
    uint8_t *fb = fbp->buffer;
#ifdef DEBUG
    FILE *err_fp = SSD1306_FB_GET_ERRFP(fbp);
    fprintf(err_fp, "DEBUG: w: %zu h: %zu, x: %zu, y: %zu\n", fbp->width, fbp->height, x, y);
#endif
    x = x % fbp->width; // if x > fbp->width, rotate screen
    y = y % fbp->height;// if y > fbp->height, rotate
    // find the byte to edit first
    uint8_t bit = x % 8; // the position of the bit right to left
    size_t idx = ((size_t)((x - bit) / 8));
    idx = idx + (y * (size_t)(fbp->width / 8)); // find the correct row
    int8_t ch = (fb[idx] >> (7 - bit)) & 0x01;
#ifdef DEBUG
    fprintf(err_fp, "DEBUG: idx: %zu byte: 0x%x bit: %x\n", idx, fb[idx], ch);
#endif
    return ch;
}
