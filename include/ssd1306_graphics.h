/*
 * COPYRIGHT: 2020. Stealthy Labs LLC.
 * DATE: 2020-01-15
 * SOFTWARE: libssd1306-i2c
 * LICENSE: Refer license file
 */
#ifndef __LIB_SSD1306_GFX_H__
#define __LIB_SSD1306_GFX_H__

#include <inttypes.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SSD1306_ATOMIC_INCREMENT(A) __atomic_add_fetch((A), 1, __ATOMIC_SEQ_CST)
#define SSD1306_ATOMIC_DECREMENT(A) __atomic_sub_fetch((A), 1, __ATOMIC_SEQ_CST)
#define SSD1306_ATOMIC_ZERO(A) __atomic_clear((A), __ATOMIC_SEQ_CST)
#define SSD1306_ATOMIC_SET(A,B) __atomic_store_n((A),(B), __ATOMIC_SEQ_CST)
#define SSD1306_ATOMIC_IS_EQUAL(A,B) __atomic_compare_exchange_n((A),(B),*(A),1,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST)

const char *ssd1306_fb_version(void);

typedef struct {
    int errnum; // store the errno here
    char *errbuf; // error string buffer, allocated on the heap
    size_t errlen; // size of the error string buffer
    FILE *err_fp; // err file pointer for messages. default is stderr
    volatile int _ref; // reference counted
} ssd1306_err_t;

ssd1306_err_t *ssd1306_err_create(FILE *fp); // if fp is NULL, stderr is used
void ssd1306_err_destroy(ssd1306_err_t *err);
#define SSD1306_ERR_REF_INC(A) if ((A) != NULL) SSD1306_ATOMIC_INCREMENT(&((A)->_ref))

typedef struct ssd1306_font_ ssd1306_font_t;

typedef struct {
    uint8_t width; // width of the framebuffer
    uint8_t height; // height of the framebuffer
    uint8_t *buffer; // buffer pointer
    size_t len; // length of the buffer
    ssd1306_err_t *err; // pointer to an optional error object
    ssd1306_font_t *font; // pointer to an opaque font library implementation - default is freetype
} ssd1306_framebuffer_t;

// ssd1306 has 1:1 correspondence between pixel to bit
ssd1306_framebuffer_t *ssd1306_framebuffer_create(
                uint8_t width, // width of the screen in pixels
                uint8_t height, // height of the screen in pixels
                ssd1306_err_t *err // err object to be re-used. increments errstr_ref
            );

void ssd1306_framebuffer_destroy(ssd1306_framebuffer_t *fbp);

// clear the contents of the framebuffer
int ssd1306_framebuffer_clear(ssd1306_framebuffer_t *fbp);
// a debug function to dump the GDDRAM buffer to the FILE * err_fp in the
// fb->err object. This dumps the data in the widthxheight format so the
// developer can see how the bits in the RAM are being set 
// this is called framebuffer_hexdump because this is not the actual GDDRAM dump
// from the device, but the dump of the data stored in gddram_buffer pointer
int ssd1306_framebuffer_hexdump(const ssd1306_framebuffer_t *fbp);

// this dump function dumps the width x height as bits. so that you can really
// see deeply. set char zerobit to the character that represents the 0 bit. by
// default it is '.' if it is not printable or is the number 0.
// set char onebit to the character that represents the 1 bit. by default it is
// '|' if it not printable or is the number 0 or 1.
// set use_space to true if you want a space every byte, or false otherwise.
int ssd1306_framebuffer_bitdump_custom(const ssd1306_framebuffer_t *fbp,
                        char zerobit, char onebit, bool use_space);
#define ssd1306_framebuffer_bitdump(A) ssd1306_framebuffer_bitdump_custom((A), 0, 0, true)
// framebuffer or graphics functions that edit the framebuffer to perform
// drawing. the user must call the ssd1306_i2c_display_update() function every
// time they want to update the display on the screen.
// this is useful since the user may want to update the framebuffer several
// times to create a layered image (such as first draw bricks, then draw a
// person, then clip scenes that are unnecessary) before performing a display
// update.
int ssd1306_framebuffer_draw_bricks(ssd1306_framebuffer_t *fbp);

// the x,y coordinates are based on the screen widthxheight. color if true means
// color the pixel that was in that x,y location. false will clear the pixel.
// since the height and width are uint8_t the x,y are also the same. For a
// screen size of 128x64, the below diagram should display how to think about
// pixels and arrangement. Each pixel is a bit in the GDDRAM.
//  (0,0)   x ---->    (127,0)
//  y
//   |
//   V
//  (63,0)  x ---->    (127,63)
//
int ssd1306_framebuffer_put_pixel(ssd1306_framebuffer_t *fbp,
                uint8_t x, uint8_t y, bool color);
// returns the value at the pixel. is 0 if pixel is clear, is 1 if the pixel is
// colored and is -1 if the fbp pointer is bad or the pixel is not found
int8_t ssd1306_framebuffer_get_pixel(ssd1306_framebuffer_t *fbp,
                uint8_t x, uint8_t y);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif /* __LIB_SSD1306_GFX_H__ */
