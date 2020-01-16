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
	size_t length;
	unsigned char buffer[64] = { 0 };
    ssd1306_i2c_t *oled = ssd1306_i2c_open(filename, 0, 0, 0);
    if (!oled) {
        return -1;
    }
	length = 4;
	buffer[0] = 0xAF;
	buffer[1] = 0xA6;
	buffer[2] = 0xA7;
	buffer[3] = 0xA7;
	int rc = write(oled->fd, buffer, length);
	int err = errno;
	fprintf(stderr, "Write byte: %d Errno: %s\n", rc, strerror(err));
    ssd1306_i2c_close(oled);
    oled = NULL;
	return 0;
}
