/*
 * COPYRIGHT: 2020. Stealthy Labs LLC.
 * DATE: 2020-01-15
 * SOFTWARE: libssd1306-i2c
 * LICENSE: Refer license file
 */
#include <ssd1306_graphics.h>

int main()
{
    int rc = 0;
    ssd1306_err_t *errp = NULL;
    ssd1306_framebuffer_t *fbp = NULL;
    do { 
        errp = ssd1306_err_create(stderr);
        if (!errp) {
            rc = -1;
            break;
        }
        fprintf(errp->err_fp, "DEBUG: Using library version: %s\n", ssd1306_fb_version());
        fbp = ssd1306_framebuffer_create(128, 64, errp);
        if (!fbp) {
            rc = -1;
            break;
        }
        for (uint8_t i = 0; i < fbp->width; ++i) {
            ssd1306_framebuffer_put_pixel(fbp, i, i, true);
        }
        ssd1306_framebuffer_bitdump(fbp);
        ssd1306_framebuffer_clear(fbp);
        for (uint8_t i = 0; i < 16; ++i) {
            ssd1306_framebuffer_put_pixel(fbp, i, i, true);
        }
        for (uint8_t i = 0; i < 16; ++i) {
            ssd1306_framebuffer_put_pixel_rotation(fbp, i, i, true, 1 /* 90 degrees */);
        }
        for (uint8_t i = 0; i < 16; ++i) {
            ssd1306_framebuffer_put_pixel_rotation(fbp, i, i, true, 2 /* 180 degrees */);
        }
        for (uint8_t i = 0; i < 16; ++i) {
            ssd1306_framebuffer_put_pixel_rotation(fbp, i, i, true, 3 /* 270 degrees */);
        }
        ssd1306_framebuffer_bitdump(fbp);
        ssd1306_framebuffer_clear(fbp);
        const char *defstr = "ABCDeF";
        int8_t pixel = ssd1306_framebuffer_get_pixel(fbp, 0, 0);
        fprintf(errp->err_fp, "Pixel at [0,0] is %x. Expecting 1\n", pixel);
        ssd1306_framebuffer_box_t bbox;
        ssd1306_framebuffer_draw_text(fbp, defstr, 0, 32, 32, SSD1306_FONT_DEFAULT, 4, &bbox);
        ssd1306_framebuffer_bitdump(fbp);
        fprintf(errp->err_fp, "returned box: top: %d left: %d right: %d bottom: %d\n",
                bbox.top, bbox.left, bbox.right, bbox.bottom);
        ssd1306_framebuffer_clear(fbp);
        fprintf(errp->err_fp, "DEBUG: testing custom font and rotation of font\n");
        ssd1306_graphics_options_t opts[2];
        opts[0].type = SSD1306_OPT_FONT_FILE;
        opts[0].value.font_file = "/usr/share/fonts/truetype/msttcorefonts/Comic_Sans_MS.ttf";
        opts[1].type = SSD1306_OPT_ROTATE_FONT;
        opts[1].value.rotation_degrees = 30;
        ssd1306_framebuffer_draw_text_extra(fbp, defstr, 0, 32, 32, SSD1306_FONT_CUSTOM, 4, opts, 1, &bbox);
        ssd1306_framebuffer_bitdump(fbp);
        fprintf(errp->err_fp, "DEBUG: testing rotation of pixel\n");
        ssd1306_framebuffer_clear(fbp);
        opts[0].type = SSD1306_OPT_ROTATE_PIXEL;
        opts[0].value.rotation_degrees = 180;
        ssd1306_framebuffer_draw_text_extra(fbp, defstr, 0, 32, 32, SSD1306_FONT_DEFAULT, 4, opts, 1, &bbox);
        ssd1306_framebuffer_bitdump(fbp);
        fprintf(errp->err_fp, "DEBUG: testing rotation of both font and pixel\n");
        ssd1306_framebuffer_clear(fbp);
        opts[0].type = SSD1306_OPT_ROTATE_PIXEL;
        opts[0].value.rotation_degrees = 180;
        opts[1].type = SSD1306_OPT_ROTATE_FONT;
        opts[1].value.rotation_degrees = 30;
        ssd1306_framebuffer_draw_text_extra(fbp, defstr, 0, 32, 32, SSD1306_FONT_DEFAULT, 4, opts, 2, &bbox);
        ssd1306_framebuffer_bitdump(fbp);
        /* if library is compiled with libunistring */
#ifdef LIBSSD1306_HAVE_UNISTR_H
        const char *mycstr = "å,ä, ö";
        size_t myclen = strlen(mycstr);
        fprintf(errp->err_fp, "String: %s length: %zu\n", mycstr, myclen);
        fprintf(errp->err_fp, "DEBUG: Testing in utf32 mode\n");
        size_t mylen32 = 0;
        // we need to convert the C string to UTF-32 string.
        uint32_t *mystr32 = u8_to_u32((const uint8_t *)mycstr, myclen, NULL, &mylen32);
        ssd1306_framebuffer_clear(fbp);
        if (!(ssd1306_framebuffer_draw_text_utf32(fbp, mystr32, mylen32, 32, 32, SSD1306_FONT_DEFAULT, 4, NULL, 0, &bbox) < 0)) {
            ssd1306_framebuffer_bitdump(fbp);
        }
        free(mystr32);
        fprintf(errp->err_fp, "DEBUG: Testing in utf8 mode\n");
        // directly use the C-string as intended
        ssd1306_framebuffer_clear(fbp);
        if (!(ssd1306_framebuffer_draw_text_utf8(fbp, mycstr, myclen, 32, 32, SSD1306_FONT_DEFAULT, 4, NULL, 0, &bbox) < 0)) {
            ssd1306_framebuffer_bitdump(fbp);
        }
        // directly use the C-string as intended
        fprintf(errp->err_fp, "DEBUG: Testing in char mode\n");
        ssd1306_framebuffer_clear(fbp);
        if (!(ssd1306_framebuffer_draw_text_extra(fbp, "A Å", 0, 32, 32, SSD1306_FONT_DEFAULT, 4, NULL, 0, &bbox) < 0)) {
            ssd1306_framebuffer_bitdump(fbp);
        }
#endif /* LIBSSD1306_HAVE_UNISTR_H */
    } while (0);
    if (fbp)
        ssd1306_framebuffer_destroy(fbp);
    if (errp)
        ssd1306_err_destroy(errp);
    return rc;
}
