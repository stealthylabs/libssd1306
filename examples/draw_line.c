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
        fbp = ssd1306_framebuffer_create(128, 32, errp);
        if (!fbp) {
            rc = -1;
            break;
        }
        fprintf(errp->err_fp, "DEBUG: draw horizontal line from (0,0) to (64,0)\n");
        ssd1306_framebuffer_draw_line(fbp, 0, 0, 64, 0, true);
        ssd1306_framebuffer_bitdump_nospace(fbp);
        ssd1306_framebuffer_clear(fbp);
        fprintf(errp->err_fp, "DEBUG: draw vertical line from (64,0) to (64,32)\n");
        ssd1306_framebuffer_draw_line(fbp, 64, 0, 64, 32, true);
        ssd1306_framebuffer_bitdump_nospace(fbp);
        ssd1306_framebuffer_clear(fbp);
        fprintf(errp->err_fp, "DEBUG: draw vertical line from (0,0) to (64,32)\n");
        ssd1306_framebuffer_draw_line(fbp, 0, 0, 64, 32, true);
        ssd1306_framebuffer_bitdump_nospace(fbp);
        ssd1306_framebuffer_clear(fbp);
    } while (0);
    if (fbp)
        ssd1306_framebuffer_destroy(fbp);
    if (errp)
        ssd1306_err_destroy(errp);
    return rc;
}
