/*
 * COPYRIGHT: 2020. Stealthy Labs LLC.
 * DATE: 2020-01-15
 * SOFTWARE: libssd1306-i2c
 * LICENSE: Refer license file
 */
#include <features.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
//#include <i2c/smbus.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ssd1306_i2c.h>


ssd1306_i2c_t *ssd1306_i2c_init(
        const char *dev, // name of the device such as /dev/i2c-1. cannot be NULL
        uint8_t addr, // I2C address of the device. valid values: 0 (default) or 0x3c or 0x3d
        uint8_t height, // OLED display height. valid values: 0 (default) or 128
        uint8_t width // OLED display width. valid values: 0 (default), 32 or 64
    )
{
    ssd1306_i2c_t *ptr = NULL;
    do {
        if (!dev) {
            fprintf(stderr, "ERROR: No device given.\n");
            break;
        }
        ptr = calloc(1, sizeof(*ptr));
        if (!ptr) {
            fprintf(stderr, "ERROR: Failed to allocate memory of size %zu bytes\n", sizeof(*ptr));
            break;
        }
        ptr->dev = strdup(dev);
        if (!ptr->dev) {
            ptr->last_err = errno;
            fprintf(stderr, "WARN: Failed to copy device name: %s. Ignoring potential memory error: %s\n", dev, strerror(ptr->last_err));
        } else {
            ptr->last_err = 0;
        }
        if (addr == 0x3c || addr == 0) {
            ptr->addr = 0x3c;
        } else if (addr == 0x3d) {
            ptr->addr = 0x3d;
        } else {
            fprintf(stderr, "WARN: I2C device addr cannot be %x. Using 0x3c\n",
                    addr);
            ptr->addr = 0x3c;
        }
        ptr->height = 128;
        if (width == 32 || width == 0) {
            ptr->width = 32;
        } else if (width == 64) {
            ptr->width = 64;
        } else {
            fprintf(stderr, "WARN: OLED screen width cannot be %d. Using %d\n",
                    width, 32);
            ptr->width = 32;
        }
        ptr->fd = -1;
    } while (0);
    return ptr;
}

void ssd1306_i2c_free(ssd1306_i2c_t *ptr)
{
    if (ptr) {
        if (ptr->dev) {
            free(ptr->dev);
        }
        ptr->dev = NULL;
        memset(ptr, 0, sizeof(*ptr));
        free(ptr);
        ptr = NULL;
    }
}


