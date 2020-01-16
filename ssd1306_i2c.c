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


ssd1306_i2c_t *ssd1306_i2c_open(
        const char *dev, // name of the device such as /dev/i2c-1. cannot be NULL
        uint8_t addr, // I2C address of the device. valid values: 0 (default) or 0x3c or 0x3d
        uint8_t height, // OLED display height. valid values: 0 (default) or 128
        uint8_t width // OLED display width. valid values: 0 (default), 32 or 64
    )
{
    ssd1306_i2c_t *ptr = NULL;
    int rc = 0;
    do {
        if (!dev) {
            fprintf(stderr, "ERROR: No device given.\n");
            rc = -1;
            break;
        }
        ptr = calloc(1, sizeof(*ptr));
        if (!ptr) {
            fprintf(stderr, "ERROR: Failed to allocate memory of size %zu bytes\n", sizeof(*ptr));
            rc = -1;
            break;
        }
        ptr->err.errstr = calloc(1, 256);
        if (!ptr->err.errstr) {
            fprintf(stderr, "ERROR: Failed to allocate memory of size 256 bytes\n");
            rc = -1;
            break;
        }
        ptr->err.errstr_max_len = 256;
        ptr->dev = strdup(dev);
        if (!ptr->dev) {
            ptr->err.errnum = errno;
            strerror_r(ptr->err.errnum, ptr->err.errstr, ptr->err.errstr_max_len);
            fprintf(stderr, "WARN: Failed to copy device name: %s. Ignoring potential memory error: %s\n", dev, ptr->err.errstr);
        } else {
            ptr->err.errnum = 0;
            memset(ptr->err.errstr, 0, ptr->err.errstr_max_len);
            strerror_r(ptr->err.errnum, ptr->err.errstr, ptr->err.errstr_max_len);
        }
        if (addr == 0x3c || addr == 0) {
            ptr->addr = 0x3c;
        } else if (addr == 0x3d) {
            ptr->addr = 0x3d;
        } else {
            fprintf(stderr, "WARN: I2C device addr cannot be 0x%x. Using 0x3c\n",
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
        ptr->fd = open(dev, O_RDWR);
        if (ptr->fd < 0) {
            ptr->err.errnum = errno;
            strerror_r(ptr->err.errnum, ptr->err.errstr, ptr->err.errstr_max_len);
            fprintf(stderr, "ERROR: Failed to open %s in read/write mode: %s\n",
                    dev, ptr->err.errstr);
            rc = -1;
            break;
        } else {
            fprintf(stderr, "INFO: Opened %s at fd %d\n", dev, ptr->fd);
            uint32_t addr = (uint32_t)ptr->addr;
            if (ioctl(ptr->fd, I2C_SLAVE, addr) < 0) {
                ptr->err.errnum = errno;
                strerror_r(ptr->err.errnum, ptr->err.errstr, ptr->err.errstr_max_len);
                fprintf(stderr, "ERROR: Failed to set I2C_SLAVE for %s addr 0x%x: %s\n",
                        dev, addr, ptr->err.errstr);
                rc = -1;
                break;
            } else {
                fprintf(stderr, "INFO: I2C_SLAVE for %s addr 0x%x opened in RDWR mode\n",
                        dev, addr);
                rc = 0;
            }
        }
    } while (0);
    if (rc < 0) {
        ssd1306_i2c_close(ptr);
        ptr = NULL;
    }
    return ptr;
}

void ssd1306_i2c_close(ssd1306_i2c_t *ptr)
{
    if (ptr) {
        if (ptr->fd > 0) {
            close(ptr->fd);
        }
        if (ptr->dev) {
            free(ptr->dev);
        }
        ptr->dev = NULL;
        if (ptr->err.errstr) {
            free(ptr->err.errstr);
            ptr->err.errstr = NULL;
        }
        memset(ptr, 0, sizeof(*ptr));
        free(ptr);
        ptr = NULL;
    }
}


