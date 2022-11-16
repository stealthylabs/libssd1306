/*
 * COPYRIGHT: 2020. Stealthy Labs LLC.
 * DATE: 2020-01-15
 * SOFTWARE: libssd1306-i2c
 * LICENSE: Refer license file
 */
#include <ssd1306_config.h>
#ifdef LIBSSD1306_HAVE_FEATURES_H
#include <features.h>
#endif
#ifdef LIBSSD1306_HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef LIBSSD1306_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef LIBSSD1306_HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef LIBSSD1306_HAVE_STRING_H
#include <string.h>
#endif
#ifdef LIBSSD1306_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef LIBSSD1306_HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef LIBSSD1306_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef LIBSSD1306_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef LIBSSD1306_HAVE_MATH_H
#include <math.h>
#endif

#if LIBSSD1306_HAVE_DECL_STRERROR_R
// do nothing
#else
// rewrite it
#warning "strerror_r is reentrant. strerror is not, so removing usage of strerror_r"
#define strerror_r(A,B,C) do {} while (0)
#endif
#ifdef LIBSSD1306_HAVE_FREETYPE2
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

#ifdef LIBSSD1306_HAVE_PTHREAD
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
    "/usr/share/fonts/truetype/ttf-bitstream-vera/Vera.ttf",
    "/usr/share/fonts/truetype/ttf-bitstream-vera/VeraBd.ttf",
    "/usr/share/fonts/truetype/ttf-bitstream-vera/VeraIt.ttf",
    "/usr/share/fonts/truetype/ttf-bitstream-vera/VeraBI.ttf",
    "/usr/share/fonts/truetype/freefont/FreeMono.ttf",
    "/usr/share/fonts/truetype/freefont/FreeMonoBold.ttf",
    "/usr/share/fonts/truetype/freefont/FreeMonoOblique.ttf",
    "/usr/share/fonts/truetype/freefont/FreeMonoBoldOblique.ttf",
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

static int ssd1306_font_render_string(ssd1306_framebuffer_t *fbp,
        const char *font_file, ssd1306_fontface_t font_idx, uint8_t font_size,
        const void *str, size_t slen, size_t chsz,
        uint16_t x, uint16_t y,
        int16_t rotation_degrees, uint8_t rotate_pixel,
        ssd1306_framebuffer_box_t *bbox)
{
    FILE *err_fp = SSD1306_FB_GET_ERRFP(fbp);
    if (fbp && fbp->font && (font_idx < SSD1306_FONT_MAX || font_file) && str && slen > 0) {
        ssd1306_font_t *font = fbp->font;
        int rc = 0;
        FT_Face face = NULL;
        bool free_the_face = false;
        SSD1306_LOCK(&(font->_lock));
        do {
            if (bbox) {
                bbox->top = bbox->left = bbox->right = bbox->bottom = 0;
            }
            if (font_idx < SSD1306_FONT_MAX) {
                face = font->faces[font_idx];
                free_the_face = false;
            } else if (font_file) {
                if (access(font_file, R_OK) < 0) {
                    int serrno = errno;
                    char serrbuf[256];
                    memset(serrbuf, 0, sizeof(serrbuf));
                    strerror_r(serrno, serrbuf, sizeof(serrbuf));
                    serrbuf[255] = '\0';
                    fprintf(err_fp, "ERROR: Tried reading '%s'. Error: %s(%d)\n", font_file,
                                    serrbuf, serrno);
                    rc = -1;
                    break;
                } 
                FT_Error ferr = FT_New_Face(font->lib, font_file, 0, &face);
                if (ferr) {
                    fprintf(err_fp, "ERROR: FreeType FT_New_Face(%s => %s) error: %d (%s)\n",
                            ssd1306_fontface_names[SSD1306_FONT_CUSTOM], font_file,
                            ferr, FT_Error_String(ferr));
                    break;
                }
                free_the_face = true;
            }
            if (face) {
                FT_Error ferr;
                FT_GlyphSlot slot = face->glyph;
                FT_Matrix transformer = { 0 };
                FT_Vector pen = { 0 };
                FT_Select_Charmap(face, FT_ENCODING_UNICODE);
                ferr = FT_Set_Char_Size(face, 0, font_size * 64, 300, 300);
                if (ferr) {
                    fprintf(err_fp, "ERROR: FreeType FT_Set_Char_Size(%s, %d) error: %d (%s)\n",
                            ssd1306_fontface_paths[font_idx], font_size, ferr, FT_Error_String(ferr));
                    rc = -1;
                    break;
                }
                if (rotation_degrees) {
                    // convert to radians. multiply by pi/180
                    double angle = (M_PI * (double)rotation_degrees) / 180.0;
                    // taken from FreeType tutorial example
                    transformer.xx = (FT_Fixed)(cos(angle) * 0x10000L);
                    transformer.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
                    transformer.yx = (FT_Fixed)(sin(angle) * 0x10000L);
                    transformer.yy = (FT_Fixed)(cos(angle) * 0x10000L);
                }
                for (size_t idx = 0; idx < slen; ++idx) {
                    if (rotation_degrees) {
                        // rotation transform
                        FT_Set_Transform(face, &transformer, &pen);
                    } else {
                        // no/identity transform
                        FT_Set_Transform(face, NULL, &pen);
                    }
                    FT_ULong cc;
                    if (chsz == sizeof(char)) {
                        // ascii
                        const uint8_t *astr = (const uint8_t *)str;
                        cc = astr[idx];
                    } else {
                        // utf32
                        const uint32_t *astr = (const uint32_t *)str;
                        cc = astr[idx];
                    }
                    // these are the same
                    if (0) {
                        FT_UInt glyph_idx = FT_Get_Char_Index(face, cc);
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
                        ferr = FT_Load_Char(face, cc, FT_LOAD_RENDER);
                        if (ferr) {
                            fprintf(err_fp, "WARN: Freetype FT_Load_Char(0x%x) error: %d (%s)\n",
                                    cc, ferr, FT_Error_String(ferr));
                            continue; // ignore error
                        }
                    }
                    //draw bitmap
                    FT_Bitmap *bmap = &slot->bitmap;
                    FT_Int x_bmap = x + slot->bitmap_left;
                    FT_Int y_bmap = y - slot->bitmap_top;
                    FT_Int xmax_bmap = x_bmap + bmap->width;
                    FT_Int ymax_bmap = y_bmap + bmap->rows;
                    // find the max height of the font
                    if (bbox) {
                        if (idx == 0) { // set top and left if not set
                            bbox->top = ((uint8_t)(x_bmap & 0xFF));
                            if (bbox->top >= fbp->width)
                                bbox->top = fbp->width;
                            bbox->left = ((uint8_t)(y_bmap & 0xFF));
                            if (bbox->left >= fbp->height)
                                bbox->left = fbp->height;
                        }
                        // choose the max right/bottom since characters in the
                        // middle might have a lower bottom than just the end
                        // characters. like a y or g
                        uint8_t right = ((uint8_t)(xmax_bmap & 0xFF)) - 1;
                        if (right >= fbp->width)
                            right = fbp->width;
                        if (bbox->right < right)
                            bbox->right = right;
                        uint8_t bottom = ((uint8_t)(ymax_bmap & 0xFF)) - 1;
                        if (bottom >= fbp->height)
                            bottom = fbp->height;
                        if (bbox->bottom < bottom)
                            bbox->bottom = bottom;
                    }
                    for (FT_Int i = x_bmap, p = 0; i < xmax_bmap; ++i, ++p) {
                        for (FT_Int j = y_bmap, q = 0; j < ymax_bmap; ++j, ++q) {
                            if (i < 0 || j < 0 || i >= fbp->width || j >= fbp->height)
                                continue;
                            ssd1306_framebuffer_put_pixel_rotation(fbp, (uint8_t)(i & 0xFF),
                                    (uint8_t)(j & 0xFF),
                                    bmap->buffer[q * bmap->width + p],
                                    rotate_pixel);
                        }
                    }
                    // advance position
                    pen.x += slot->advance.x;
                    pen.y += slot->advance.y;
                }
                rc = 0;
            } else {
                fprintf(err_fp, "ERROR: Font %s does not have a face pointer\n",
                        ssd1306_fontface_names[font_idx]);
                rc = -1;
                break;
            }
        } while (0);
        if (free_the_face && face) {
            FT_Error ferr = FT_Done_Face(face);
            if (ferr) {
                fprintf(err_fp, "WARN: Freetype FT_Done_Face(%s) error: %d (%s)\n",
                        font_file ? font_file : "unknown font file", ferr, FT_Error_String(ferr));
            }
        }
        SSD1306_UNLOCK(&(font->_lock));
        return rc;
    } else {
        fprintf(err_fp, "ERROR: Invalid font inputs given\n");
    }
    return -1;
}

const char *ssd1306_fb_version(void)
{
    return LIBSSD1306_PACKAGE_VERSION;
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
            char zerobit, char onebit, bool use_space, bool use_color)
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
            int8_t pixel = ssd1306_framebuffer_get_pixel(fbp, x, y);
            if (pixel) {// red
                fprintf(err_fp, use_color ? "\x1b[31m%c\x1b[0m" : "%c", onebit);
            } else {// green
                fprintf(err_fp, use_color ? "\x1b[34m%c\x1b[0m" : "%c", zerobit);
            }
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

int ssd1306_framebuffer_put_pixel_rotation(ssd1306_framebuffer_t *fbp,
        uint8_t x, uint8_t y, bool color, uint8_t rotation_flag)
{
    SSD1306_FB_BAD_PTR_RETURN(fbp, -1);
    uint8_t w = fbp->width;
    uint8_t h = fbp->height;
    // based on the page arrangement in GDDRAM as per the datasheet
    if (x >= 0 && x < w && y >= 0 && y < h) {
        switch (rotation_flag) {
        uint8_t tmp;
        case 1: // 90deg rotation
            tmp = x; x = y; y = tmp; // swap x&y
            x = w - x - 1;
            break;
        case 2: // 180deg rotation
            x = w - x - 1;
            y = h - y - 1;
            break;
        case 3: // -90deg rotation
            tmp = x; x = y; y = tmp; // swap x&y
            y = h - y - 1;
            break;
        default: break; // no rotation
        }
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

static void ssd1306_framebuffer_draw_text_options_handler(
            const ssd1306_graphics_options_t *opts, size_t num_opts,
            const char **font_file_ptr,
            uint8_t *rotate_pixel_ptr,
            int16_t *rotation_degrees_ptr,
            FILE *err_fp
        )
{
    const char *font_file = NULL;
    uint8_t rotate_pixel = 0;
    int16_t rotation_degrees = 0;
    if (opts != NULL && num_opts > 0) {
        for(size_t i = 0; i < num_opts; ++i) {
            switch (opts[i].type) {
                case SSD1306_OPT_FONT_FILE:
                    if (opts[i].value.font_file != NULL && font_file == NULL) {
                        font_file = opts[i].value.font_file;
                    }
                    break;
                case SSD1306_OPT_ROTATE_FONT:
                    if (opts[i].value.rotation_degrees != 0) {
                        rotation_degrees = opts[i].value.rotation_degrees;
                    }
                    break;
                case SSD1306_OPT_ROTATE_PIXEL:
                    if (opts[i].value.rotation_degrees % 90 == 0) {
                        switch ((opts[i].value.rotation_degrees % 360)) {
                            case 90: rotate_pixel = 1; break;
                            case 180: rotate_pixel = 2; break;
                            case 270: rotate_pixel = 3; break;
                            default: rotate_pixel = 0; break;
                        }
                    } else {
                        fprintf(err_fp, "WARN: SSD1306_OPT_ROTATE_PIXEL only accepts rotation_degrees in multiples of 90\n");
                        rotate_pixel = 0;
                    }
                    break;
                default:
                    /* ignore */
                    break;
            }
        }
    }
    if (font_file_ptr)
        *font_file_ptr = font_file;
    if (rotate_pixel_ptr)
        *rotate_pixel_ptr = rotate_pixel;
    if (rotation_degrees)
        *rotation_degrees_ptr = rotation_degrees;
}

#ifdef LIBSSD1306_HAVE_UNISTR_H
ssize_t ssd1306_framebuffer_draw_text_extra(ssd1306_framebuffer_t *fbp,
                const char *str, size_t slen,
                uint8_t x, uint8_t y, ssd1306_fontface_t fontface,
                uint8_t font_size,
                const ssd1306_graphics_options_t *opts, size_t num_opts,
                ssd1306_framebuffer_box_t *bbox)
{
    return ssd1306_framebuffer_draw_text_utf8(fbp, (const uint8_t *)str, slen, x, y, fontface,
                    font_size, opts, num_opts, bbox);
}

ssize_t ssd1306_framebuffer_draw_text_utf8(ssd1306_framebuffer_t *fbp,
                const uint8_t *str, size_t slen,
                uint8_t x, uint8_t y, ssd1306_fontface_t fontface,
                uint8_t font_size,
                const ssd1306_graphics_options_t *opts, size_t num_opts,
                ssd1306_framebuffer_box_t *bbox)
{
    FILE *err_fp = SSD1306_FB_GET_ERRFP(fbp);
    if (fbp && str) {
        if (slen == 0) {
            slen = u8_strlen(str);
            if (slen == 0) {
                fprintf(err_fp, "WARN: input string in UTF-18 does not have a length, cannot proceed\n");
                return -1;
            }
        }
        // check for UTF-8
        const uint8_t *sptr = u8_check(str, slen); 
        if (sptr != NULL) {
            size_t vlen = sptr - str;
            fprintf(err_fp, "WARN: input string in UTF-8 is not well formed. Starting string location 0x%x Malformed string location 0x%x Valid length: %zu Input length: %zu\n", str, sptr, vlen, slen);
            // update slen to use vlen
            slen = vlen;
            if (vlen == 0) {
                fprintf(err_fp, "WARN: input string in UTF-8 does not have a length, cannot proceed\n");
                return -1;
            }
        }
        size_t slen32 = 0;
        uint32_t *str32 = u8_to_u32((const uint8_t *)str, slen, NULL, &slen32);
        if (!str32 || slen32 == 0) {
            int serrno = errno;
            char serrbuf[256];
            memset(serrbuf, 0, sizeof(serrbuf));
            strerror_r(serrno, serrbuf, sizeof(serrbuf));
            serrbuf[255] = '\0';
            fprintf(err_fp, "ERROR: Failed to convert UTF-8 to UTF-32 string for drawing. Error: %s\n", serrbuf);
            return -1;
        }
        int rc = ssd1306_framebuffer_draw_text_utf32(fbp, str32, slen32, 
                        x, y, fontface, font_size, opts, num_opts, bbox);
        // do not forget to dealloc
        free(str32);
        return rc;
    }
    return -1;
}
ssize_t ssd1306_framebuffer_draw_text_utf16(ssd1306_framebuffer_t *fbp,
                const uint16_t *str, size_t slen,
                uint8_t x, uint8_t y, ssd1306_fontface_t fontface,
                uint8_t font_size,
                const ssd1306_graphics_options_t *opts, size_t num_opts,
                ssd1306_framebuffer_box_t *bbox)
{
    FILE *err_fp = SSD1306_FB_GET_ERRFP(fbp);
    if (fbp && str) {
        if (slen == 0) {
            slen = u16_strlen(str);
            if (slen == 0) {
                fprintf(err_fp, "WARN: input string in UTF-16 does not have a length, cannot proceed\n");
                return -1;
            }
        }
        // check for UTF-16
        const uint16_t *sptr = u16_check(str, slen); 
        if (sptr != NULL) {
            size_t vlen = sptr - str;
            fprintf(err_fp, "WARN: input string in UTF-16 is not well formed. Starting string location 0x%x Malformed string location 0x%x Valid length: %zu Input length: %zu\n", str, sptr, vlen, slen);
            // update slen to use vlen
            slen = vlen;
            if (vlen == 0) {
                fprintf(err_fp, "WARN: input string in UTF-16 does not have a length, cannot proceed\n");
                return -1;
            }
        }
        size_t slen32 = 0;
        uint32_t *str32 = u16_to_u32(str, slen, NULL, &slen32);
        if (!str32 || slen32 == 0) {
            int serrno = errno;
            char serrbuf[256];
            memset(serrbuf, 0, sizeof(serrbuf));
            strerror_r(serrno, serrbuf, sizeof(serrbuf));
            serrbuf[255] = '\0';
            fprintf(err_fp, "ERROR: Failed to convert UTF-16 to UTF-32 string for drawing. Error: %s\n", serrbuf);
            return -1;
        }
        int rc = ssd1306_framebuffer_draw_text_utf32(fbp, str32, slen32, 
                        x, y, fontface, font_size, opts, num_opts, bbox);
        // do not forget to dealloc
        free(str32);
        return rc;
    }
    return -1;
}

ssize_t ssd1306_framebuffer_draw_text_utf32(ssd1306_framebuffer_t *fbp,
                const uint32_t *str, size_t slen,
                uint8_t x, uint8_t y, ssd1306_fontface_t fontface,
                uint8_t font_size,
                const ssd1306_graphics_options_t *opts, size_t num_opts,
                ssd1306_framebuffer_box_t *bbox)
{
    FILE *err_fp = SSD1306_FB_GET_ERRFP(fbp);
    if (fbp && str) {
        if (slen == 0) {
            slen = u32_strlen(str);
            if (slen == 0) {
                fprintf(err_fp, "WARN: input string in UTF-32 does not have a length, cannot proceed\n");
                return -1;
            }
        }
        // check for UTF-32
        const uint32_t *sptr = u32_check(str, slen); 
        if (sptr != NULL) {
            size_t vlen = sptr - str;
            fprintf(err_fp, "WARN: input string in UTF-32 is not well formed. Starting string location 0x%x Malformed string location 0x%x Valid length: %zu Input length: %zu\n", str, sptr, vlen, slen);
            // update slen to use vlen
            slen = vlen;
            if (vlen == 0) {
                fprintf(err_fp, "WARN: input string in UTF-32 does not have a length, cannot proceed\n");
                return -1;
            }
        }
        // handle options
        const char *font_file = NULL;
        uint8_t rotate_pixel = 0;
        int16_t rotation_degrees = 0;
        ssd1306_framebuffer_draw_text_options_handler(opts, num_opts, &font_file,
                        &rotate_pixel, &rotation_degrees, err_fp);
        if (fontface >= SSD1306_FONT_CUSTOM) {
            if (font_file != NULL) {
                return ssd1306_font_render_string(fbp,
                        font_file, SSD1306_FONT_CUSTOM, font_size,
                        str, slen, sizeof(uint32_t), (uint16_t)x, (uint16_t)y,
                        rotation_degrees, rotate_pixel, bbox);
            } else {
                fprintf(err_fp, "ERROR: If using %s then you need to use SSD1306_OPT_FONT_FILE\n",
                        ssd1306_fontface_names[SSD1306_FONT_CUSTOM]);
                return -1;
            }
        } else {
            return ssd1306_font_render_string(fbp,
                NULL, fontface, font_size,
                str, slen, sizeof(uint32_t), (uint16_t)x, (uint16_t)y,
                rotation_degrees, rotate_pixel, bbox);
        }
    }
    return -1;
}

#else /* if the user did not compile with libunistring */

ssize_t ssd1306_framebuffer_draw_text_extra(ssd1306_framebuffer_t *fbp,
                const char *str, size_t slen,
                uint8_t x, uint8_t y, ssd1306_fontface_t fontface,
                uint8_t font_size,
                const ssd1306_graphics_options_t *opts, size_t num_opts,
                ssd1306_framebuffer_box_t *bbox)
{
    FILE *err_fp = SSD1306_FB_GET_ERRFP(fbp);
    if (fbp && str) {
        if (slen == 0) {
            slen = strlen(str);
        }
        // handle options
        const char *font_file = NULL;
        uint8_t rotate_pixel = 0;
        int16_t rotation_degrees = 0;
        ssd1306_framebuffer_draw_text_options_handler(opts, num_opts, &font_file,
                        &rotate_pixel, &rotation_degrees, err_fp);
        if (fontface >= SSD1306_FONT_CUSTOM) {
            if (font_file != NULL) {
                return ssd1306_font_render_string(fbp,
                        font_file, SSD1306_FONT_CUSTOM, font_size,
                        str, slen, sizeof(char), (uint16_t)x, (uint16_t)y,
                        rotation_degrees, rotate_pixel, bbox);
            } else {
                fprintf(err_fp, "ERROR: If using %s then you need to use SSD1306_OPT_FONT_FILE\n",
                        ssd1306_fontface_names[SSD1306_FONT_CUSTOM]);
                return -1;
            }
        } else {
            return ssd1306_font_render_string(fbp,
                NULL, fontface, font_size,
                str, slen, sizeof(char), (uint16_t)x, (uint16_t)y,
                rotation_degrees, rotate_pixel, bbox);
        }
    }
    return -1;
}

#endif /* LIBSSD1306_HAVE_UNISTR_H */

ssize_t ssd1306_framebuffer_draw_text(ssd1306_framebuffer_t *fbp,
                const char *str, size_t slen,
                uint8_t x, uint8_t y, ssd1306_fontface_t fontface,
                uint8_t font_size, ssd1306_framebuffer_box_t *bbox)
{
    if (fontface < SSD1306_FONT_CUSTOM) {
        return ssd1306_framebuffer_draw_text_extra(fbp, str, slen, x, y,
                    fontface, font_size, NULL, 0, bbox);
    } else {
        FILE *err_fp = SSD1306_FB_GET_ERRFP(fbp);
        fprintf(err_fp, "ERROR: Fontface cannot be %s in %s(). use %s_extra()\n",
                ssd1306_fontface_names[(size_t)fontface], __func__, __func__);
        return -1;
    }
}

int ssd1306_framebuffer_draw_line(ssd1306_framebuffer_t *fbp,
            uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool color)
{
    SSD1306_FB_BAD_PTR_RETURN(fbp, -1);
    uint8_t w = fbp->width;
    uint8_t h = fbp->height;
    if (y0 > y1) {
        // swap to approach calcs from lower point to higher point
        uint8_t tmp = y0;
        y0 = y1;
        y1 = tmp;
        tmp = x0;
        x0 = x1;
        x1 = tmp;
    }
    int delta_x = ((int)x1) - ((int)x0);
    int delta_y = ((int)y1) - ((int)y0);
    if (delta_x == 0) {
        if (delta_y != 0) {
            // it is a straight line
            ssd1306_framebuffer_put_pixel(fbp, x0, y0, color);
            while (delta_y--) {
                ssd1306_framebuffer_put_pixel(fbp, x0, y0, color);
                y0++;
            }
        } else {// single pixel
            ssd1306_framebuffer_put_pixel(fbp, x0, y0, color);
        }
        return 0;
    } else if (delta_y == 0) {
        if (delta_x != 0) {
            // it is a straight line
            ssd1306_framebuffer_put_pixel(fbp, x0, y0, color);
            while (delta_x--) {
                ssd1306_framebuffer_put_pixel(fbp, x0, y0, color);
                x0++;
            }
        } else {// single pixel
            ssd1306_framebuffer_put_pixel(fbp, x0, y0, color);
        }
        return 0;
    }
    // ok both delta_x and delta_y are not zero, and the line is not horizontal
    // or vertical
    // Bresenham's algorithm using http://www.phatcode.net/res/224/files/html/ch35/35-03.html
    // calculate the length of the line in each coordinate
    int dirxn = 0;
    bool is_octant_1 = false;
    if (delta_x > 0) {
        if (delta_x > delta_y) {
            dirxn = 1;
            is_octant_1 = false;
        } else {
            dirxn = 1;
            is_octant_1 = true;
        }
    } else {
        delta_x = abs(delta_x);
        if (delta_x > delta_y) {
            dirxn = -1;
            is_octant_1 = false;
        } else {
            dirxn = -1;
            is_octant_1 = true;
        }
    }
    if (is_octant_1) {
        int delta_x2 = delta_x * 2;
        int delta_x2_y2 = delta_x2 - (int)(delta_y * 2);
        int error_term = delta_x2 - (int)delta_y;
        ssd1306_framebuffer_put_pixel(fbp, x0, y0, color);
        while (delta_y--) {
            if (error_term >= 0) {
                x0 += dirxn;
                error_term += delta_x2_y2;
            } else {
                error_term += delta_x2;
            }
            y0++;
            ssd1306_framebuffer_put_pixel(fbp, x0, y0, color);
        }
    } else {
        int delta_y2 = delta_y * 2;
        int delta_y2_x2 = delta_y2 - (int)(delta_x * 2);
        int error_term = delta_y2 - (int)delta_x;
        ssd1306_framebuffer_put_pixel(fbp, x0, y0, color);
        while (delta_x--) {
            if (error_term >= 0) {
                y0++;
                error_term += delta_y2_x2;
            } else {
                error_term += delta_y2;
            }
            x0 += dirxn;
            ssd1306_framebuffer_put_pixel(fbp, x0, y0, color);
        }
    }
    return 0;
}

int ssd1306_framebuffer_draw_circle(ssd1306_framebuffer_t *fbp,
                int16_t xc, int16_t yc, uint16_t radius)
{
    SSD1306_FB_BAD_PTR_RETURN(fbp, -1);
    uint8_t w = fbp->width;
    uint8_t h = fbp->height;
    return -1;
}
