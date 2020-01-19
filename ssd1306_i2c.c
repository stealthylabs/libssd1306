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
        uint8_t width, // OLED display width. valid values: 0 (default) or 128
        uint8_t height, // OLED display height. valid values: 0 (default) or 32 or 64
        FILE *logerr
    )
{
    ssd1306_i2c_t *oled = NULL;
    int rc = 0;
    FILE *err_fp = logerr == NULL ? stderr : logerr;
    do {
        if (!dev) {
            fprintf(err_fp, "ERROR: No device given.\n");
            rc = -1;
            break;
        }
        oled = calloc(1, sizeof(*oled));
        if (!oled) {
            fprintf(err_fp, "ERROR: Failed to allocate memory of size %zu bytes\n", sizeof(*oled));
            rc = -1;
            break;
        }
        oled->fd = -1; // force fd to be -1
        oled->err.err_fp = err_fp;
        oled->err.errstr = calloc(1, 256);
        if (!oled->err.errstr) {
            fprintf(err_fp, "ERROR: Failed to allocate memory of size 256 bytes\n");
            rc = -1;
            break;
        }
        oled->err.errstr_max_len = 256;
        oled->dev = strdup(dev);
        if (!oled->dev) {
            oled->err.errnum = errno;
            strerror_r(oled->err.errnum, oled->err.errstr, oled->err.errstr_max_len);
            fprintf(err_fp, "WARN: Failed to copy device name: %s. Ignoring potential memory error: %s\n", dev, oled->err.errstr);
        } else {
            oled->err.errnum = 0;
            memset(oled->err.errstr, 0, oled->err.errstr_max_len);
            strerror_r(oled->err.errnum, oled->err.errstr, oled->err.errstr_max_len);
        }
        if (addr == 0x3c || addr == 0) {
            oled->addr = 0x3c;
        } else if (addr == 0x3d) {
            oled->addr = 0x3d;
        } else {
            fprintf(err_fp, "WARN: I2C device addr cannot be 0x%x. Using 0x3c\n",
                    addr);
            oled->addr = 0x3c;
        }
        oled->width = 128;
        if (height == 32 || height == 0) {
            oled->height = 32;
        } else if (height == 64) {
            oled->height = 64;
        } else {
            fprintf(err_fp, "WARN: OLED screen height cannot be %d. Using %d\n",
                    height, 32);
            oled->height = 32;
        }
        oled->screen_buffer = calloc(1, (oled->width * oled->height + 1) * sizeof(uint8_t));
        if (!oled->screen_buffer) {
            oled->err.errnum = errno;
            strerror_r(oled->err.errnum, oled->err.errstr, oled->err.errstr_max_len);
            fprintf(err_fp, "ERROR: Out of memory allocating %zu bytes for screen buffer\n",
                    (oled->width * oled->height + 1) * sizeof(uint8_t));
            rc = -1;
            break;
        }
        oled->fd = open(dev, O_RDWR);
        if (oled->fd < 0) {
            oled->err.errnum = errno;
            strerror_r(oled->err.errnum, oled->err.errstr, oled->err.errstr_max_len);
            fprintf(err_fp, "ERROR: Failed to open %s in read/write mode: %s\n",
                    dev, oled->err.errstr);
            rc = -1;
            break;
        } else {
            fprintf(err_fp, "INFO: Opened %s at fd %d\n", dev, oled->fd);
            uint32_t addr = (uint32_t)oled->addr;
            if (ioctl(oled->fd, I2C_SLAVE, addr) < 0) {
                oled->err.errnum = errno;
                strerror_r(oled->err.errnum, oled->err.errstr, oled->err.errstr_max_len);
                fprintf(err_fp, "ERROR: Failed to set I2C_SLAVE for %s addr 0x%x: %s\n",
                        dev, addr, oled->err.errstr);
                rc = -1;
                break;
            } else {
                fprintf(err_fp, "INFO: I2C_SLAVE for %s addr 0x%x opened in RDWR mode\n",
                        dev, addr);
                rc = 0;
            }
        }
    } while (0);
    if (rc < 0) {
        ssd1306_i2c_close(oled);
        oled = NULL;
    }
    return oled;
}

void ssd1306_i2c_close(ssd1306_i2c_t *oled)
{
    if (oled) {
        if (oled->fd > 0) {
            close(oled->fd);
        }
        if (oled->screen_buffer) {
            free(oled->screen_buffer);
        }
        oled->screen_buffer = NULL;
        if (oled->dev) {
            free(oled->dev);
        }
        oled->dev = NULL;
        if (oled->err.errstr) {
            free(oled->err.errstr);
            oled->err.errstr = NULL;
        }
        memset(oled, 0, sizeof(*oled));
        free(oled);
        oled = NULL;
    }
}

static size_t ssd1306_i2c_internal_get_cmd_bytes(ssd1306_i2c_cmd_t cmd,
        uint8_t data, uint8_t cmdbuf[4])
{
    size_t sz = 2; // default
    cmdbuf[0] = 0x80; // Co: 1 D/C#: 0 0b10000000
    cmdbuf[2] = 0x80; // Co: 1 D/C#: 0 0b10000000
    cmdbuf[3] = 0xE3; // NOP by default
    switch (cmd) {
    case SSD1306_I2C_CMD_POWER_OFF: cmdbuf[1] = 0xAE; break;
    case SSD1306_I2C_CMD_POWER_ON: cmdbuf[1] = 0xAF; break;
    case SSD1306_I2C_CMD_MEM_ADDR_HORIZ:
        cmdbuf[1] = 0x20; // Set memory address
        cmdbuf[3] = 0x00; // horizontal
        break;
    case SSD1306_I2C_CMD_MEM_ADDR_VERT:
        cmdbuf[1] = 0x20; // Set memory address
        cmdbuf[3] = 0x01; // vertical
        sz = 4;
        break;
    case SSD1306_I2C_CMD_MEM_ADDR_PAGE:
        cmdbuf[1] = 0x20; // Set memory address
        cmdbuf[3] = 0x02; // page / reset
        sz = 4;
        break;
    case SSD1306_I2C_CMD_DISP_START_LINE:
        cmdbuf[1] = 0x40 | (data & 0x3F); // set display start line bytes. 40-7F
        break;
    case SSD1306_I2C_CMD_DISP_OFFSET:
        cmdbuf[1] = 0xD3;
        cmdbuf[3] = data & 0x3F; // 0x00-0x3F;
        break;
    case SSD1306_I2C_CMD_DISP_CLOCK_DIVFREQ:
        cmdbuf[1] = 0xD5;
        cmdbuf[3] = data;
        break;
    case SSD1306_I2C_CMD_DISP_CONTRAST:
        cmdbuf[1] = 0x81;
        cmdbuf[3] = data;
        break;
    case SSD1306_I2C_CMD_DISP_NORMAL: cmdbuf[1] = 0xA6; break;
    case SSD1306_I2C_CMD_DISP_INVERTED: cmdbuf[1] = 0xA7; break;
    case SSD1306_I2C_CMD_DISP_DISABLE_ENTIRE_ON: cmdbuf[1] = 0xA4; break;
    case SSD1306_I2C_CMD_DISP_ENTIRE_ON: cmdbuf[1] = 0xA5; break;
    case SSD1306_I2C_CMD_SEG_REMAP: cmdbuf[1] = 0xA0 | (data & 0x1); break;
    case SSD1306_I2C_CMD_MUX_RATIO:
        cmdbuf[1] = 0xA8;
        cmdbuf[3] = (data == 0) ? 0xFF : data; // all 1s is a RESET
        break;
    case SSD1306_I2C_CMD_COM_SCAN_DIRXN_NORMAL: cmdbuf[1] = 0xC0; break;
    case SSD1306_I2C_CMD_COM_SCAN_DIRXN_INVERT: cmdbuf[1] = 0xC8; break;
    case SSD1306_I2C_CMD_COM_PIN_CFG:
        cmdbuf[1] = 0xDA;
        cmdbuf[3] = (data & 0x32); // valid values: 0x02, 0x12, 0x22, 0x32
        break;
    case SSD1306_I2C_CMD_PRECHARGE_PERIOD:
        cmdbuf[1] = 0xD9;
        cmdbuf[3] = (data == 0) ? 0x22 : data; // 0 is invalid
        break;
    case SSD1306_I2C_CMD_VCOMH_DESELECT:
        cmdbuf[1] = 0xDB;
        cmdbuf[3] = (data & 0x70); // 0b0AAA0000
        break;
    case SSD1306_I2C_CMD_ENABLE_CHARGE_PUMP:
        cmdbuf[1] = 0x8D;
        cmdbuf[3] = 0x14; // 0b00010100
        break;
    case SSD1306_I2C_CMD_DISABLE_CHARGE_PUMP:
        cmdbuf[1] = 0x8D;
        cmdbuf[3] = 0x10; // 0b00010000
        break;
    case SSD1306_I2C_CMD_NOP: // fallthrough
    default:
        cmdbuf[1] = 0xE3; // NOP
        break;
    }
    return sz;
}

int ssd1306_i2c_display_initialize(ssd1306_i2c_t *oled)
{
    int rc = 0;
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_OFF, 0);
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_MEM_ADDR_HORIZ, 0);
    // these instructions are from the software configuration section 15.2.3 in
    // the datasheet
    // Set MUX Ratio 0xA8, 0x3F
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_MUX_RATIO,
                        (oled->height == 32) ? 0x1F : 0x3F);
    // Set display offset 0xD3, 0x00
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_OFFSET, 0);
    // set display start line 0x40
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_START_LINE, 0);
    // set segment remap 0xA0/0xA1
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_SEG_REMAP, 1);
    // set com output scan direction 0xC0/0xC8
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_COM_SCAN_DIRXN_INVERT, 0);
    // set com pins hardware config 0xDA, 0x02
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_COM_PIN_CFG,
                        (oled->height == 32) ? 0x02 : 0x12);
    // set contrast control 0x81, 0x7F
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_CONTRAST, 0x7F);
    // disable entire display on 0xA4
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_DISABLE_ENTIRE_ON, 0);
    // set normal display 0xA6
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_NORMAL, 0);
    // set osc frequency 0xD5, 0x80
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_CLOCK_DIVFREQ, 0x80); //RESET 0b10000000
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_PRECHARGE_PERIOD, 0xF1);
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_VCOMH_DESELECT, 0x30);
    // enable charge pump regulator 0x8D, 0x14
    // charge pump has to be followed by a power on. section 15.2.1 in datasheet
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_ENABLE_CHARGE_PUMP, 0);
    // power display on 0xAF
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_ON, 0);
    return rc;
}

int ssd1306_i2c_run_cmd(ssd1306_i2c_t *oled, ssd1306_i2c_cmd_t cmd, uint8_t data)
{
    FILE *err_fp = (oled == NULL || (oled != NULL && oled->err.err_fp == NULL)) ?
                    stderr : oled->err.err_fp;
    if (!oled || oled->fd < 0) {
        fprintf(err_fp, "ERROR: Invalid ssd1306 I2C object\n");
        return -1;
    }
    uint8_t cmd_buf[4];
    size_t cmd_sz = ssd1306_i2c_internal_get_cmd_bytes(cmd, data, cmd_buf);
    if (cmd_sz == 0) {
        fprintf(err_fp, "WARN: Unknown cmd given %d\n", cmd);
        return -1;
    }
    ssize_t nb = write(oled->fd, cmd_buf, cmd_sz);
    if (nb < 0) {
        oled->err.errnum = errno;
        strerror_r(oled->err.errnum, oled->err.errstr, oled->err.errstr_max_len);
        fprintf(err_fp, "ERROR: Failed to write cmd [0x%x, 0x%x, 0x%x, 0x%x] to device fd %d: %s",
                cmd_buf[0], cmd_buf[1], cmd_buf[4], cmd_buf[3], oled->fd,
                oled->err.errstr);
        return -1;
    }
    fprintf(err_fp, "INFO: Wrote %zd bytes of cmd [0x%x, 0x%x, 0x%x, 0x%x] to device fd: %d\n",
            nb, cmd_buf[0], cmd_buf[1], cmd_buf[4], cmd_buf[3], oled->fd);
    return 0;
}

