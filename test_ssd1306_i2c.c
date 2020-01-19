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
    ssd1306_i2c_t *oled = ssd1306_i2c_open(filename, 0, 0, 0, NULL);
    if (!oled) {
        return -1;
    }
    ssd1306_i2c_display_initialize(oled);
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_ON, 0);
    sleep(5);
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_INVERTED, 0);
    sleep(5);
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_OFF, 0);
    ssd1306_i2c_close(oled);
    oled = NULL;
	return 0;
}
