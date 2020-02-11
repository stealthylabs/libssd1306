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
        ssd1306_framebuffer_bitdump(fbp, 0, 0, true);
    } while (0);
    if (fbp)
        ssd1306_framebuffer_destroy(fbp);
    if (errp)
        ssd1306_err_destroy(errp);
    return rc;
}
