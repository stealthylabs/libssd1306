#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
//#include <i2c/smbus.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main ()
{
	const char *filename = "/dev/i2c-1";
	int file_i2c = -1;
	size_t length;
	unsigned char buffer[64] = { 0 };
	if ((file_i2c = open(filename, O_RDWR)) < 0) {
		fprintf(stderr, "ERROR: failed to open i2c bus");
		return -1;
	}
	printf("I2C device at %s opened with fd: %d\n", filename, file_i2c);
	int addr = 0x3C; // if 128x32 it is 0x3C or 0x00 (default). if 128x64 use 0x3D
	if (ioctl(file_i2c, I2C_SLAVE, addr) < 0) {
		fprintf(stderr, "ERROR: bus access failure");
		return -1;
	}
	length = 4;
	buffer[0] = 0xAF;
	buffer[1] = 0xA6;
	buffer[2] = 0xA7;
	buffer[3] = 0xA7;
	int rc = write(file_i2c, buffer, length);
	int err = errno;
	fprintf(stderr, "Write byte: %d Errno: %s\n", rc, strerror(err));

	close(file_i2c);
	return 0;
}
