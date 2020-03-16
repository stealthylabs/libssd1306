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

// old versions did not have this
#if (FREETYPE_MAJOR == 2 && FREETYPE_MINOR < 10)
static const char *FT_Error_String(FT_Error err)
{
    #undef __FTERRORS_H__
    #define FT_ERRORDEF( e, v, s )  case e: return s;
    #define FT_ERROR_START_LIST     switch (err) {
    #define FT_ERROR_END_LIST       }
    #include FT_ERRORS_H
    return "(Unknown error)";
}
#endif

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

static const char *ssd1306_fontface_paths[SSD1306_FONT_MAX + 1] = {
    "/usr/share/fonts/truetype/freefont/FreeMono.ttf",
    "/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf",
    "/usr/share/fonts/truetype/freefont/FreeMonoOblique.ttf",
    "/usr/share/fonts/truetype/freefont/FreeMonoBoldOblique.ttf",
    "/usr/share/fonts/truetype/ttf-bitstream-vera/Vera.ttf",
    "/usr/share/fonts/truetype/ttf-bitstream-vera/VeraBd.ttf",
    "/usr/share/fonts/truetype/ttf-bitstream-vera/VeraIt.ttf",
    "/usr/share/fonts/truetype/ttf-bitstream-vera/VeraBI.ttf",
    NULL
};
static const char *ssd1306_fontface_names[SSD1306_FONT_MAX + 1] = {
    "SSD1306_FONT_VERA",
    "SSD1306_FONT_VERA_BOLD",
    "SSD1306_FONT_VERA_ITALIC",
    "SSD1306_FONT_VERA_BOLDITALIC",
    "SSD1306_FONT_FREEMONO",
    "SSD1306_FONT_FREEMONO_BOLD",
    "SSD1306_FONT_FREEMONO_ITALIC",
    "SSD1306_FONT_FREEMONO_BOLDITALIC",
    "SSD1306_FONT_CUSTOM"
};

struct ssd1306_font_ {
    FT_Library lib;
    FT_Face faces[SSD1306_FONT_MAX];
    ssd1306_lock_t _lock;
};

static void ssd1306_font_destroy(ssd1306_font_t *font, ssd1306_err_t *err);

static ssd1306_font_t *ssd1306_font_create(ssd1306_err_t *err)
{
    FILE *err_fp = SSD1306_ERR_GET_ERRFP(err);
    ssd1306_font_t *font = calloc(1, sizeof(ssd1306_font_t));
    if (font) {
        SSD1306_LOCK_CREATE(&(font->_lock), true);// allow recursive lock
        FT_Error ferr = 0;
        do {
            ferr = FT_Init_FreeType(&(font->lib));
            if (ferr) {
                fprintf(err_fp, "ERROR: Freetype FT_Init_FreeType() error: %d (%s)\n", ferr, FT_Error_String(ferr));
                font->lib = NULL;
                break;
            }
            for (uint32_t idx = SSD1306_FONT_DEFAULT; idx < SSD1306_FONT_MAX; ++idx) {
                font->faces[idx] = NULL;
                ferr = FT_New_Face(font->lib, ssd1306_fontface_paths[idx], 0,
                                    &(font->faces[idx]));
                if (ferr) {
                    fprintf(err_fp, "ERROR: FreeType FT_New_Face(%s => %s) error: %d (%s)\n",
                            ssd1306_fontface_names[idx], ssd1306_fontface_paths[idx],
                            ferr, FT_Error_String(ferr));
                    break;
                }
            }
            if (ferr)
                break;
        } while (0);
        if (ferr != 0) {
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
        FILE *err_fp = SSD1306_ERR_GET_ERRFP(err);
        if (font->lib) {
            SSD1306_LOCK(&(font->_lock));
            FT_Error ferr = 0;
            for (uint32_t idx = 0; idx < SSD1306_FONT_MAX; ++idx) {
                if (font->faces[idx]) {
                    ferr = FT_Done_Face(font->faces[idx]);
                    if (ferr) {
                        fprintf(err_fp, "WARN: Freetype FT_Done_Face(%s) error: %d (%s)\n",
                                ssd1306_fontface_paths[idx], ferr, FT_Error_String(ferr));
                    }
                    font->faces[idx] = NULL;
                }
            }
            // cleanup
            ferr = FT_Done_FreeType(font->lib);
            if (ferr) {
                fprintf(err_fp, "WARN: Freetype FT_Done_FreeType() error: %d (%s)\n",
                        ferr, FT_Error_String(ferr));
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

static ssize_t ssd1306_font_render_string(ssd1306_font_t *font, ssd1306_err_t *err,
        ssd1306_fontface_t font_idx, uint8_t font_size, const char *str, size_t slen,
        uint16_t x, uint16_t y, ssd1306_framebuffer_t *fbp)
{
    FILE *err_fp = SSD1306_ERR_GET_ERRFP(err);
    if (font && font_idx < SSD1306_FONT_MAX && fbp && str && slen > 0) {
        ssize_t rc = 0;
        SSD1306_LOCK(&(font->_lock));
        do {
            FT_Face face = font->faces[font_idx];
            if (face) {
                FT_GlyphSlot slot = face->glyph;
                FT_Vector pen;
                pen.x = x;
                pen.y = y;
                FT_Error ferr;
                ferr = FT_Set_Char_Size(face, 0, font_size * 64, 300, 300);
                if (ferr) {
                    fprintf(err_fp, "ERROR: FreeType FT_Set_Char_Size(%s, %d) error: %d (%s)\n",
                            ssd1306_fontface_paths[font_idx], font_size, ferr, FT_Error_String(ferr));
                    rc = -1;
                    break;
                }
                for (size_t idx = 0; idx < slen; ++idx) {
                    FT_Set_Transform(face, NULL, &pen);
                    // these are the same
                    if (0) {
                        FT_UInt glyph_idx = FT_Get_Char_Index(face, str[idx]);
                        ferr = FT_Load_Glyph(face, glyph_idx, FT_LOAD_DEFAULT);
                        if (ferr) {
                            fprintf(err_fp, "WARN: Freetype FT_Load_Glyph() error: %d (%s)\n",
                                    ferr, FT_Error_String(ferr));
                            continue; // ignore error
                        }
                        ferr = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
                        if (ferr) {
                            fprintf(err_fp, "WARN: Freetype FT_Render_Glyph() error: %d (%s)\n",
                                    ferr, FT_Error_String(ferr));
                            continue; // ignore error
                        }
                    } else {
                        ferr = FT_Load_Char(face, str[idx], FT_LOAD_RENDER);
                        if (ferr) {
                            fprintf(err_fp, "WARN: Freetype FT_Load_Char(%c) error: %d (%s)\n",
                                    str[idx], ferr, FT_Error_String(ferr));
                            continue; // ignore error
                        }
                    }
                    //draw bitmap
                    FT_Bitmap *bmap = &slot->bitmap;
                    FT_Int x_bmap = slot->bitmap_left;
                    FT_Int y_bmap = fbp->height - slot->bitmap_top;
                    FT_Int xmax_bmap = x_bmap + bmap->width;
                    FT_Int ymax_bmap = y_bmap + bmap->rows;
                    for (FT_Int i = x_bmap, p = 0; i < xmax_bmap; ++i, ++p) {
                        for (FT_Int j = y_bmap, q = 0; j < ymax_bmap; ++j, ++q) {
                            if (i < 0 || j < 0 || i >= fbp->width || j >= fbp->height)
                                continue;
                            ssd1306_framebuffer_put_pixel(fbp, (uint8_t)(i & 0xFF),
                                    (uint8_t)(j & 0xFF),
                                    bmap->buffer[q * bmap->width + p]);
                        }
                    }
                    // advance position
                    pen.x += slot->advance.x;
                    pen.y += slot->advance.y;
                }
                rc = (ssize_t)slen;
            } else {
                fprintf(err_fp, "ERROR: Font %s does not have a face pointer\n",
                        ssd1306_fontface_names[font_idx]);
                rc = -1;
                break;
            }
        } while (0);
        SSD1306_UNLOCK(&(font->_lock));
        return rc;
    } else {
        fprintf(err_fp, "ERROR: Invalid font inputs given\n");
    }
    return -1;
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
        fbp->len = sizeof(uint8_t) * (fbp->width * fbp->height) / 8;
        fbp->buffer = calloc(1, fbp->len);
        if (!fbp->buffer) {
            fprintf(err_fp, "ERROR: Failed to allocate memory of size %zu bytes\n", fbp->len);
            fbp->buffer = NULL;
            rc = -1;
            break;
        }
        fbp->font = ssd1306_font_create(fbp->err);
        if (!fbp->font) {
            fprintf(err_fp, "ERROR: Failed to create font object, exiting\n");
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
        if (fbp->font) {
            ssd1306_font_destroy(fbp->font, fbp->err);
            fbp->font = NULL;
        }
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
    for (size_t y = 0; y < fbp->height; ++y) {
        fprintf(err_fp, "%04zX ", y);
        uint8_t bit = 0;
        uint8_t z = 0;
        for (size_t x = 0; x < fbp->width; ++x) {
            char ch = ssd1306_framebuffer_get_pixel(fbp, x, y);
            if (ch) { // fill the correct bit
                z |= (1 << (bit & 7));
            }
            if (bit % 8 == 7) {
                fprintf(err_fp, "%02X ", z);
            }
            bit++;
        }
        fprintf(err_fp, "\n");
    }
    return 0;
}

int ssd1306_framebuffer_bitdump_custom(const ssd1306_framebuffer_t *fbp,
            char zerobit, char onebit, bool use_space)
{
    SSD1306_FB_BAD_PTR_RETURN(fbp, -1);
    FILE *err_fp = SSD1306_FB_GET_ERRFP(fbp);
    if (!isprint(zerobit)) {
        zerobit = '.';
    }
    if (!isprint(onebit)) {
        onebit = '|';
    }
    for (size_t y = 0; y < fbp->height; ++y) {
        fprintf(err_fp, "%04zX ", y);
        for (size_t x = 0; x < fbp->width; ++x) {
            char ch = ssd1306_framebuffer_get_pixel(fbp, x, y) ? onebit : zerobit;
            fprintf(err_fp, "%c", ch);
            if (x % 8 == 7 && use_space) {
                fprintf(err_fp, "%c", ' ');
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
    uint8_t w = fbp->width;
    uint8_t h = fbp->height;
    // based on the page arrangement in GDDRAM as per the datasheet
    if (x >= 0 && x < w && y >= 0 && y < h) {
        // this allows rotation!!
    //    x = w - x - 1;
    //    y = h - y - 1;
    } else {
        return -1;
    }
    if (color) {
        fbp->buffer[x + (y / 8) * w] |= (1 << (y & 7));
    } else {
        fbp->buffer[x + (y / 8) * w] &= ~(1 << (y & 7));
    }
    return 0;
}

int ssd1306_framebuffer_invert_pixel(ssd1306_framebuffer_t *fbp, uint8_t x, uint8_t y)
{
    SSD1306_FB_BAD_PTR_RETURN(fbp, -1);
    uint8_t w = fbp->width;
    uint8_t h = fbp->height;
    if (x >= 0 && x < w && y >= 0 && y < h) {
        fbp->buffer[x + (y / 8) * w] ^= (1 << (y & 7));
        return 0;
    }
    return -1;
}

int8_t ssd1306_framebuffer_get_pixel(const ssd1306_framebuffer_t *fbp, uint8_t x, uint8_t y)
{
    SSD1306_FB_BAD_PTR_RETURN(fbp, -1);
    uint8_t w = fbp->width;
    uint8_t h = fbp->height;
    if (x >= 0 && x < w && y >= 0 && y < h) {
        return fbp->buffer[x + (y / 8) * w] & (1 << (y & 7));
    }
    return -1;
}

ssize_t ssd1306_framebuffer_draw_text(ssd1306_framebuffer_t *fbp,
                const char *str, size_t slen,
                uint8_t x, uint8_t y, ssd1306_fontface_t fontface,
                uint8_t font_size)
{
    if (fontface < SSD1306_FONT_CUSTOM) {
        return ssd1306_framebuffer_draw_text_extra(fbp, str, slen, x, y, fontface, font_size, NULL, 0);
    } else {
        FILE *err_fp = SSD1306_FB_GET_ERRFP(fbp);
        fprintf(err_fp, "ERROR: Fontface cannot be %s in %s(). use %s_extra()\n",
                ssd1306_fontface_names[(size_t)fontface], __func__, __func__);
        return -1;
    }
}

ssize_t ssd1306_framebuffer_draw_text_extra(ssd1306_framebuffer_t *fbp,
                const char *str, size_t slen,
                uint8_t x, uint8_t y, ssd1306_fontface_t fontface,
                uint8_t font_size,
                const ssd1306_graphics_options_t *opts, size_t num_opts)
{
    if (fbp && str) {
        if (slen == 0) {
            slen = strlen(str);
        }
        //TODO: handle options
        return ssd1306_font_render_string(fbp->font, fbp->err, fontface,
                font_size, str, slen, (uint16_t)x, (uint16_t)y, fbp);
    }
    return -1;
}
