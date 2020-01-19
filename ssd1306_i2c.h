/*
 * COPYRIGHT: 2020. Stealthy Labs LLC.
 * DATE: 2020-01-15
 * SOFTWARE: libssd1306-i2c
 * LICENSE: Refer license file
 */
#ifndef __LIB_SSD1306_I2C_H__
#define __LIB_SSD1306_I2C_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int errnum; // store the errno here
    char *errstr; // error string, allocated on the heap
    size_t errstr_max_len; // size of the error string buffer
    FILE *err_fp; // err file pointer for messages. default is stderr
} ssd1306_i2c_err_t;

typedef struct {
    int fd;
    char *dev;      // device name. a copy is made.
    uint8_t addr;   // default 0x3c
    uint8_t height; // default 128
    uint8_t width;  // default 32
    uint8_t *screen_buffer; // buffer for of size height x width + 1 ctrl byte for writing data on GDRAM
    bool is_power_on;  // check if power is on
    ssd1306_i2c_err_t err; // for re-entrant error handling
} ssd1306_i2c_t;

ssd1306_i2c_t *ssd1306_i2c_open( // open the device for read/write
        const char *dev, // name of the device such as /dev/i2c-1. cannot be NULL
        uint8_t daddr, // I2C address of the device. valid values: 0 (default) or 0x3c or 0x3d
        uint8_t width, // OLED display width. valid values: 0 (default) or 128
        uint8_t height, // OLED display height. valid values: 0 (default) or 32 or 64
        FILE *logerr // FILE* log ptr. use NULL or stderr for default
    );

void ssd1306_i2c_close(ssd1306_i2c_t *oled); // free object and close fd

typedef enum {
    SSD1306_I2C_CMD_NOP, // no operation
    // power
    SSD1306_I2C_CMD_POWER_OFF,
    SSD1306_I2C_CMD_POWER_ON,
    // addressing
    SSD1306_I2C_CMD_MEM_ADDR_HORIZ, // set horizontal addressing mode
    SSD1306_I2C_CMD_MEM_ADDR_VERT, // set vertical addressing mode
    SSD1306_I2C_CMD_MEM_ADDR_PAGE, // set page addressing mode (RESET)
    SSD1306_I2C_CMD_COLUMN_ADDR, // set column address. requires 2 data bytes
    SSD1306_I2C_CMD_PAGE_ADDR, // set page address. requires 2 data bytes
    // layout
    SSD1306_I2C_CMD_DISP_START_LINE, // set display start line. data 0x00-0x3F
    SSD1306_I2C_CMD_DISP_OFFSET, // set display offset. data 0x00-0x3F
    SSD1306_I2C_CMD_DISP_CLOCK_DIVFREQ, // set display clock divide/frequency.
                                    // data: refer datasheet
    SSD1306_I2C_CMD_DISP_CONTRAST, // set display contrast control. data: 1-255
    SSD1306_I2C_CMD_DISP_NORMAL, // set display normal
    SSD1306_I2C_CMD_DISP_INVERTED, // set display inverted
    SSD1306_I2C_CMD_DISP_DISABLE_ENTIRE_ON, // disable entire display on
    SSD1306_I2C_CMD_DISP_ENTIRE_ON, // set entire display on
    SSD1306_I2C_CMD_SEG_REMAP, // set segment remap. data: 0 for col 0 or 1 for col 127
    SSD1306_I2C_CMD_MUX_RATIO, // set multiplex ratio 0x1F or 0x3F
    SSD1306_I2C_CMD_COM_SCAN_DIRXN_NORMAL, // set COM pins scan direction normal
    SSD1306_I2C_CMD_COM_SCAN_DIRXN_INVERT, // set COM pins scan direction reverse
    SSD1306_I2C_CMD_COM_PIN_CFG, // set COM pin config. data: 0x02, 0x12, 0x22, 0x32
    SSD1306_I2C_CMD_PRECHARGE_PERIOD, // set precharge period. data: 0xF1 for reset. refer datasheet
    SSD1306_I2C_CMD_VCOMH_DESELECT, // set VCOMH deselect level
    SSD1306_I2C_CMD_ENABLE_CHARGE_PUMP, // enable charge pump regulator
    SSD1306_I2C_CMD_DISABLE_CHARGE_PUMP, // disable charge pump regulator
} ssd1306_i2c_cmd_t;

int ssd1306_i2c_run_cmd(ssd1306_i2c_t *oled, // the ssd1306_i2c_t object
        ssd1306_i2c_cmd_t cmd, // command to run on the display
        uint8_t *data, // optional command data, if any, otherwise use 0 or NULL
        size_t dlen // length of the data bytes. max is 6 per datasheet.
                    // anything more than 6 will be ignored.
    );

// initialize the display before use
int ssd1306_i2c_display_initialize(ssd1306_i2c_t *oled);
// update the display with the screen_buffer contents.
// this function can be called in an idle loop or on a timer or on-demand
int ssd1306_i2c_display_update(ssd1306_i2c_t *oled);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif /* __LIB_SSD1306_I2C_H__ */
