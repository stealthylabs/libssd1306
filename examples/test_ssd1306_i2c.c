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
    uint8_t *framebuf = NULL;
    size_t framebuf_len = 0;
    ssd1306_i2c_display_get_framebuffer(oled, &framebuf, &framebuf_len);

    ssd1306_i2c_framebuffer_draw_bricks(oled);
    ssd1306_i2c_framebuffer_hexdump(oled);
    ssd1306_i2c_framebuffer_bitdump(oled, 0, 0, ' ');
    ssd1306_i2c_display_update(oled);
    sleep(3);;
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_INVERTED, 0, 0);
    sleep(3);
    ssd1306_i2c_display_clear(oled);
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_NORMAL, 0, 0);
    ssd1306_i2c_framebuffer_draw_pixel(oled, 0, 0, false);
    ssd1306_i2c_framebuffer_draw_pixel(oled, oled->width - 1, 0, false);
    ssd1306_i2c_framebuffer_draw_pixel(oled, 0, oled->height - 1, false);
    ssd1306_i2c_framebuffer_draw_pixel(oled, oled->width - 1, oled->height - 1, false);
    ssd1306_i2c_framebuffer_draw_pixel(oled, 9, 10, false);
    ssd1306_i2c_framebuffer_bitdump(oled, 0, 0, ' ');
    ssd1306_i2c_display_update(oled);
    sleep(3);
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_OFF, 0, 0);
    ssd1306_i2c_close(oled);
    oled = NULL;
	return 0;
}
