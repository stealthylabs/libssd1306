/*
 * COPYRIGHT: 2020. Stealthy Labs LLC.
 * DATE: 2020-01-15
 * SOFTWARE: libssd1306-i2c
 * LICENSE: Refer license file
 */
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
        for (uint8_t i = 0; i < 128; ++i) {
            ssd1306_framebuffer_put_pixel(fbp, i, 0, true);
        }

        ssd1306_framebuffer_bitdump(fbp);
        int8_t pixel = ssd1306_framebuffer_get_pixel(fbp, 0, 0);
        fprintf(errp->err_fp, "Pixel at [0,0] is %x. Expecting 1\n", pixel);
        ssd1306_framebuffer_draw_text(fbp, "ABCDeF", 0, 32, 32, SSD1306_FONT_CUSTOM, 4);
        ssd1306_framebuffer_draw_text_extra(fbp, "ABCDeF", 0, 32, 32, SSD1306_FONT_DEFAULT, 4, NULL, 0);
        ssd1306_framebuffer_bitdump(fbp);
    } while (0);
    if (fbp)
        ssd1306_framebuffer_destroy(fbp);
    if (errp)
        ssd1306_err_destroy(errp);
    return rc;
}
