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
    for (size_t i = 0; i < framebuf_len; ++i) {
        if (i % 1) {
            framebuf[i] = 0xFF;
        }
        if (i % 3) {
            framebuf[i] = 0x7F;
        }
        if (i % 5) {
            framebuf[i] = 0x3F;
        }
    }
    ssd1306_i2c_display_update(oled);
    sleep(3);
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_INVERTED, 0, 0);
    ssd1306_i2c_display_update(oled);
    sleep(3);
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_OFF, 0, 0);
    ssd1306_i2c_close(oled);
    oled = NULL;
	return 0;
}
