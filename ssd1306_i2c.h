/*
 * COPYRIGHT: 2020. Stealthy Labs LLC.
 * DATE: 2020-01-15
 * SOFTWARE: libssd1306-i2c
 * LICENSE: Refer license file
 */
#ifndef __LIB_SSD1306_I2C_H__
#define __LIB_SSD1306_I2C_H__

#include <stdint.h>

typedef struct {
    int fd;
    int last_err;
    char *dev;      // device name. a copy is made.
    uint8_t addr;   // default 0x3c
    uint8_t height; // default 128
    uint8_t width;  // default 32
} ssd1306_i2c_t;

ssd1306_i2c_t *ssd1306_i2c_init(
        const char *dev, // name of the device such as /dev/i2c-1. cannot be NULL
        uint8_t daddr, // I2C address of the device. valid values: 0 (default) or 0x3c or 0x3d
        uint8_t height, // OLED display height. valid values: 0 (default) or 128
        uint8_t width // OLED display width. valid values: 0 (default), 32 or 64
    );

void ssd1306_i2c_free(ssd1306_i2c_t *); // free object

#endif /* __LIB_SSD1306_I2C_H__ */
