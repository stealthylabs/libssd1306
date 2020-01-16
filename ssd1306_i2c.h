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
    int errnum; // store the errno here
    char *errstr; // error string, allocated on the heap
    size_t errstr_max_len; // size of the error string buffer
} ssd1306_i2c_err_t;

typedef struct {
    int fd;
    char *dev;      // device name. a copy is made.
    uint8_t addr;   // default 0x3c
    uint8_t height; // default 128
    uint8_t width;  // default 32
    ssd1306_i2c_err_t err; // for re-entrant error handling
} ssd1306_i2c_t;

ssd1306_i2c_t *ssd1306_i2c_open( // open the device for read/write
        const char *dev, // name of the device such as /dev/i2c-1. cannot be NULL
        uint8_t daddr, // I2C address of the device. valid values: 0 (default) or 0x3c or 0x3d
        uint8_t height, // OLED display height. valid values: 0 (default) or 128
        uint8_t width // OLED display width. valid values: 0 (default), 32 or 64
    );

void ssd1306_i2c_close(ssd1306_i2c_t *); // free object and close fd

#endif /* __LIB_SSD1306_I2C_H__ */
