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
#include <ssd1306_i2c.h>

#ifndef SSD1306_I2C_BAD_PTR_RETURN
#define SSD1306_I2C_BAD_PTR_RETURN(P,RC) do { \
  if (!(P != NULL && P->gddram_buffer != NULL && P->gddram_buffer_len > 0)) { \
    return RC; \
  } \
} while (0)
#endif // SSD1306_I2C_BAD_PTR_RETURN

int ssd1306_i2c_framebuffer_hexdump(const ssd1306_i2c_t *oled)
{
    SSD1306_I2C_BAD_PTR_RETURN(oled, -1);
    FILE *err_fp = SSD1306_I2C_GET_ERRFP(oled);
    uint8_t * const fb = &(oled->gddram_buffer[1]);
    size_t fblen = oled->gddram_buffer_len - 1;
    size_t rows = oled->height;
    size_t cols = oled->width / 8;
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

int ssd1306_i2c_framebuffer_bitdump(const ssd1306_i2c_t *oled,
            char zerobit, char onebit, bool use_space)
{
    SSD1306_I2C_BAD_PTR_RETURN(oled, -1);
    FILE *err_fp = (oled != NULL && oled->err.err_fp != NULL) ? oled->err.err_fp : stderr;
    uint8_t * const fb = &(oled->gddram_buffer[1]);
    size_t fblen = oled->gddram_buffer_len - 1;
    size_t rows = oled->height;
    size_t cols = oled->width / 8;
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

int ssd1306_i2c_framebuffer_draw_bricks(ssd1306_i2c_t *oled)
{
    SSD1306_I2C_BAD_PTR_RETURN(oled, -1);
    uint8_t * const fb = &(oled->gddram_buffer[1]);
    size_t fblen = oled->gddram_buffer_len - 1;
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

int ssd1306_i2c_framebuffer_draw_pixel(ssd1306_i2c_t *oled, uint8_t x, uint8_t y, bool clear)
{
    SSD1306_I2C_BAD_PTR_RETURN(oled, -1);
#if DEBUG
    FILE *err_fp = SSD1306_I2C_GET_ERRFP(oled);
    fprintf(err_fp, "DEBUG: w: %zu h: %zu, x: %zu, y: %zu\n", oled->width, oled->height, x, y);
#endif
    x = x % oled->width; // if x > oled->width, rotate screen
    y = y % oled->height;// if y > oled->height, rotate
    // find the byte to edit first
    uint8_t bit = x % 8; // the position of the bit right to left
    size_t idx = ((size_t)((x - bit) / 8));
    idx = idx + (y * (size_t)(oled->width / 8)); // find the correct row
    uint8_t * const fb = &(oled->gddram_buffer[1]);
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
