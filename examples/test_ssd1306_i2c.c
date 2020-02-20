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
#include <ssd1306_i2c.h>

int main ()
{
    fprintf(stderr, "DEBUG: Using library version: %s\n", ssd1306_i2c_version());
	const char *filename = "/dev/i2c-1";
    ssd1306_i2c_t *oled = ssd1306_i2c_open(filename, 0x3c, 128, 32, NULL);
    if (!oled) {
        return -1;
    }
    ssd1306_i2c_display_initialize(oled);
    sleep(3);
    ssd1306_framebuffer_t *fbp = ssd1306_framebuffer_create(oled->width, oled->height, oled->err);

    ssd1306_framebuffer_draw_bricks(fbp);
    ssd1306_framebuffer_hexdump(fbp);
    ssd1306_framebuffer_bitdump(fbp);
    ssd1306_i2c_display_update(oled, fbp);
    sleep(3);;
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_INVERTED, 0, 0);
    sleep(3);
    ssd1306_i2c_display_clear(oled);
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_NORMAL, 0, 0);
    ssd1306_framebuffer_clear(fbp);
    ssd1306_framebuffer_put_pixel(fbp, 0, 0, true);
    ssd1306_framebuffer_put_pixel(fbp, fbp->width - 1, 0, true);
    ssd1306_framebuffer_put_pixel(fbp, 0, fbp->height - 1, true);
    ssd1306_framebuffer_put_pixel(fbp, fbp->width - 1, fbp->height - 1, true);
    ssd1306_framebuffer_put_pixel(fbp, 9, 10, true);
    ssd1306_framebuffer_bitdump(fbp);
    ssd1306_i2c_display_update(oled, fbp);
    sleep(3);
    ssd1306_framebuffer_clear(fbp);
    ssd1306_framebuffer_put_pixel(fbp, 0, 0, false);
    ssd1306_framebuffer_put_pixel(fbp, fbp->width - 1, 0, false);
    ssd1306_framebuffer_put_pixel(fbp, 0, fbp->height - 1, false);
    ssd1306_framebuffer_put_pixel(fbp, fbp->width - 1, fbp->height - 1, false);
    ssd1306_framebuffer_put_pixel(fbp, 9, 10, false);
    ssd1306_framebuffer_bitdump(fbp);
    ssd1306_i2c_display_update(oled, fbp);
    sleep(3);
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_OFF, 0, 0);
    ssd1306_framebuffer_destroy(fbp);
    fbp = NULL;
    ssd1306_i2c_close(oled);
    oled = NULL;
	return 0;
}
