/*
 * COPYRIGHT: 2020. Stealthy Labs LLC.
 * DATE: 2020-01-15
 * SOFTWARE: libssd1306-i2c
 * LICENSE: Refer license file
 */
#include <ssd1306_config.h>
#ifdef LIBSSD1306_HAVE_FEATURES_H
#include <features.h>
#endif
#ifdef LIBSSD1306_HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef LIBSSD1306_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef LIBSSD1306_HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef LIBSSD1306_HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef LIBSSD1306_HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef LIBSSD1306_HAVE_STRING_H
#include <string.h>
#endif
#ifdef LIBSSD1306_HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if LIBSSD1306_HAVE_LINUX_I2C_DEV_H
#include <linux/i2c-dev.h>
#else
#warning "You need to install libi2c-dev and i2c-tools"
// forcibly defining the ioctl number
#undef I2C_SLAVE
#define I2C_SLAVE 0x0703
#endif
#include <ssd1306_i2c.h>

#if LIBSSD1306_HAVE_DECL_STRERROR_R
// do nothing
#else
// rewrite it
#warning "strerror_r is reentrant. strerror is not, so removing usage of strerror_r"
#define strerror_r(A,B,C) do {} while (0)
#endif

// helpful macros
#ifndef SSD1306_I2C_GET_ERRFP
#define SSD1306_I2C_GET_ERRFP(P) ((P) != NULL && (P)->err != NULL && (P)->err->err_fp != NULL) ? (P)->err->err_fp : stderr
#endif // SSD1306_I2C_GET_ERRFP

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
        oled->err = ssd1306_err_create(err_fp);
        if (!oled->err) {
            rc = -1;
            break;
        }
        oled->dev = strdup(dev);
        if (!oled->dev) {
            oled->err->errnum = errno;
            strerror_r(oled->err->errnum, oled->err->errbuf, oled->err->errlen);
            fprintf(err_fp, "WARN: Failed to copy device name: %s. Ignoring potential memory error: %s\n", dev, oled->err->errbuf);
        } else {
            oled->err->errnum = 0;
            memset(oled->err->errbuf, 0, oled->err->errlen);
            strerror_r(oled->err->errnum, oled->err->errbuf, oled->err->errlen);
        }
        if (addr == 0x3c || addr == 0) {
            oled->addr = 0x3c;
        } else if (addr == 0x3d) {
            oled->addr = 0x3d;
        } else {
            fprintf(err_fp, "WARN: I2C device addr cannot be 0x%02x. Using 0x3c\n",
                    addr);
            oled->addr = 0x3c;
        }
        if (width == 128 || width == 0) {
            oled->width = 128;
        } else if (width == 96) {
            oled->width = 96;
        } else {
            fprintf(err_fp, "WARN: OLED screen width cannot be %d. has to be either 96 or 128. Using 128\n",
                    width);
            oled->width = 128;
        }
        if (height == 64 || height == 0) {
            oled->height = 64;
        } else if (height == 32) {
            oled->height = 32;
        } else if (height == 16) {
            oled->height = 16;
        } else {
            fprintf(err_fp, "WARN: OLED screen height cannot be %d. has to be either 16, 32 or 64. Using %d\n",
                    height, (oled->width == 96) ? 16 : 64);
            oled->height = (oled->width == 96) ? 16 : 64;
        }
        // this is width x height bits of GDDRAM
        oled->gddram_buffer_len = sizeof(uint8_t) * (oled->width * oled->height) / 8 + 1;
        oled->gddram_buffer = calloc(sizeof(uint8_t), oled->gddram_buffer_len);
        if (!oled->gddram_buffer) {
            oled->err->errnum = errno;
            strerror_r(oled->err->errnum, oled->err->errbuf, oled->err->errlen);
            fprintf(err_fp, "ERROR: Out of memory allocating %zu bytes for screen buffer\n",
                    oled->gddram_buffer_len);
            rc = -1;
            break;
        }
        oled->fd = open(dev, O_RDWR);
        if (oled->fd < 0) {
            oled->err->errnum = errno;
            strerror_r(oled->err->errnum, oled->err->errbuf, oled->err->errlen);
            fprintf(err_fp, "ERROR: Failed to open %s in read/write mode: %s\n",
                    dev, oled->err->errbuf);
            rc = -1;
            break;
        } else {
            fprintf(err_fp, "INFO: Opened %s at fd %d\n", dev, oled->fd);
            uint32_t addr = (uint32_t)oled->addr;
            if (ioctl(oled->fd, I2C_SLAVE, addr) < 0) {
                oled->err->errnum = errno;
                strerror_r(oled->err->errnum, oled->err->errbuf, oled->err->errlen);
                fprintf(err_fp, "ERROR: Failed to set I2C_SLAVE for %s addr 0x%02x: %s\n",
                        dev, addr, oled->err->errbuf);
                rc = -1;
                break;
            } else {
                fprintf(err_fp, "INFO: I2C_SLAVE for %s addr 0x%02x opened in RDWR mode\n",
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
        if (oled->gddram_buffer) {
            free(oled->gddram_buffer);
        }
        oled->gddram_buffer = NULL;
        if (oled->dev) {
            free(oled->dev);
        }
        oled->dev = NULL;
        ssd1306_err_destroy(oled->err);
        oled->err = NULL;
        memset(oled, 0, sizeof(*oled));
        free(oled);
        oled = NULL;
    }
}

static size_t ssd1306_i2c_internal_get_cmd_bytes(ssd1306_i2c_cmd_t cmd,
        uint8_t *data, size_t dlen, uint8_t *cmdbuf, size_t cmd_buf_max)
{
    size_t sz = 2; // default
    if (!cmdbuf || cmd_buf_max < 16 || (data != NULL && dlen == 0)) {
        return 0;//error
    }
    // fill it up
    for (size_t idx = 0; idx < cmd_buf_max; ++idx) {
        if (idx % 2 == 0) {
            cmdbuf[idx] = 0x80; // Co: 1 D/C#: 0 0b10000000
        } else {
            cmdbuf[idx] = 0xE3; // NOP by default
        }
    }
    switch (cmd) {
    case SSD1306_I2C_CMD_POWER_OFF: cmdbuf[1] = 0xAE; break;
    case SSD1306_I2C_CMD_POWER_ON: cmdbuf[1] = 0xAF; break;
    case SSD1306_I2C_CMD_MEM_ADDR_HORIZ:
        cmdbuf[1] = 0x20; // Set memory address
        cmdbuf[3] = 0x00; // horizontal
        sz = 4;
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
    case SSD1306_I2C_CMD_COLUMN_ADDR:
        cmdbuf[1] = 0x21; // set column address
        if (data && dlen >= 2) {
            cmdbuf[3] = data[0] & 0x7F;
            cmdbuf[5] = data[1] & 0x7F;
        } else {
            cmdbuf[3] = 0x00; // RESET
            cmdbuf[5] = 0x7F; // RESET
        }
        sz = 6;
        break;
    case SSD1306_I2C_CMD_PAGE_ADDR:
        cmdbuf[1] = 0x22; // set page address
        if (data && dlen >= 2) {
            cmdbuf[3] = data[0] & 0x07;
            cmdbuf[5] = data[1] & 0x07;
        } else {
            cmdbuf[3] = 0x00; // RESET
            cmdbuf[5] = 0x07; // RESET
        }
        sz = 6;
        break;
    case SSD1306_I2C_CMD_DISP_START_LINE:
        if (data && dlen > 0) {
            cmdbuf[1] = 0x40 | (data[0] & 0x3F); // set display start line bytes. 40-7F
        } else {
            cmdbuf[1] = 0x40; // set display start line bytes. 40-7F
        }
        break;
    case SSD1306_I2C_CMD_DISP_OFFSET:
        cmdbuf[1] = 0xD3;
        cmdbuf[3] = (data && dlen > 0) ? data[0] & 0x3F : 0x00; // 0x00-0x3F;
        sz = 4;
        break;
    case SSD1306_I2C_CMD_DISP_CLOCK_DIVFREQ:
        cmdbuf[1] = 0xD5;
        cmdbuf[3] = (data && dlen > 0) ? data[0] : 0x80;
        sz = 4;
        break;
    case SSD1306_I2C_CMD_DISP_CONTRAST:
        cmdbuf[1] = 0x81;
        cmdbuf[3] = (data && dlen > 0) ? data[0] : 0x7F;
        sz = 4;
        break;
    case SSD1306_I2C_CMD_DISP_NORMAL: cmdbuf[1] = 0xA6; break;
    case SSD1306_I2C_CMD_DISP_INVERTED: cmdbuf[1] = 0xA7; break;
    case SSD1306_I2C_CMD_DISP_DISABLE_ENTIRE_ON: cmdbuf[1] = 0xA4; break;
    case SSD1306_I2C_CMD_DISP_ENTIRE_ON: cmdbuf[1] = 0xA5; break;
    case SSD1306_I2C_CMD_SEG_REMAP:
        if (data && dlen > 0) {
            cmdbuf[1] = 0xA0 | (data[0] & 0x1);
        } else {
            cmdbuf[1] = 0xA0;
        }
        break;
    case SSD1306_I2C_CMD_MUX_RATIO:
        cmdbuf[1] = 0xA8;
        cmdbuf[3] = (data && dlen > 0) ? data[0] : 0xFF;
        sz = 4;
        break;
    case SSD1306_I2C_CMD_COM_SCAN_DIRXN_NORMAL: cmdbuf[1] = 0xC0; break;
    case SSD1306_I2C_CMD_COM_SCAN_DIRXN_INVERT: cmdbuf[1] = 0xC8; break;
    case SSD1306_I2C_CMD_COM_PIN_CFG:
        cmdbuf[1] = 0xDA;
        cmdbuf[3] = (data && dlen > 0) ? (data[0] & 0x32) : 0x02; // valid values: 0x02, 0x12, 0x22, 0x32
        sz = 4;
        break;
    case SSD1306_I2C_CMD_PRECHARGE_PERIOD:
        cmdbuf[1] = 0xD9;
        cmdbuf[3] = (data && dlen > 0) ? data[0] : 0x22; // 0 is invalid
        sz = 4;
        break;
    case SSD1306_I2C_CMD_VCOMH_DESELECT:
        cmdbuf[1] = 0xDB;
        cmdbuf[3] = (data && dlen > 0) ? (data[0] & 0x70) : 0x30; // 0b0AAA0000
        sz = 4;
        break;
    case SSD1306_I2C_CMD_ENABLE_CHARGE_PUMP:
        cmdbuf[1] = 0x8D;
        cmdbuf[3] = 0x14; // 0b00010100
        sz = 4;
        break;
    case SSD1306_I2C_CMD_DISABLE_CHARGE_PUMP:
        cmdbuf[1] = 0x8D;
        cmdbuf[3] = 0x10; // 0b00010000
        sz = 4;
        break;
    case SSD1306_I2C_CMD_SCROLL_DEACTIVATE:
        cmdbuf[1] = 0x2E;
        break;
    case SSD1306_I2C_CMD_SCROLL_LEFT_HORIZONTAL:
    case SSD1306_I2C_CMD_SCROLL_RIGHT_HORIZONTAL:
        // perform the scroll settings
        if (cmd == SSD1306_I2C_CMD_SCROLL_LEFT_HORIZONTAL) {
            cmdbuf[1] = 0x27;// deactivate existing settings
            cmdbuf[2] = 0x27; // 0b00100111 for left horizontal scroll
        } else {
            cmdbuf[1] = 0x26;// deactivate existing settings
            cmdbuf[2] = 0x26; // 0b00100110 for right horizontal scroll
        }
        cmdbuf[3] = 0x00; // dummy byte
        cmdbuf[4] = (data && dlen > 0) ? (data[0] & 0x07) : 0x00; // 3-bit Start page address. Default 0b000
        cmdbuf[5] = (data && dlen > 1) ? (data[1] & 0x07) : 0x00; // 3-bit scroll step in frame frequency
        cmdbuf[6] = (data && dlen > 2) ? (data[2] & 0x07) : 0x07; // 3-bit end page address. default is last page
        if (cmdbuf[6] < cmdbuf[4]) {
            // end page must be larger or equal
            // overwriting
            cmdbuf[6] = cmdbuf[4];
        }
        cmdbuf[7] = 0x00; // dummy byte
        cmdbuf[8] = 0XFF; // dummy byte
        cmdbuf[9] = 0x2F; // activate the scroll
        sz = 10;
        break;
    case SSD1306_I2C_CMD_SCROLL_VERTICAL_LEFT_HORIZONTAL:
    case SSD1306_I2C_CMD_SCROLL_VERTICAL_RIGHT_HORIZONTAL:
        if (cmd == SSD1306_I2C_CMD_SCROLL_VERTICAL_LEFT_HORIZONTAL) {
            cmdbuf[1] = 0x2A; // 0b00101010 for vertical and left horizontal scroll
            cmdbuf[2] = 0x2A; // 0b00101010 for vertical and left horizontal scroll
        } else {
            cmdbuf[1] = 0x29; // 0b00101001 for vertical and right horizontal scroll
            cmdbuf[2] = 0x29; // 0b00101001 for vertical and right horizontal scroll
        }
        cmdbuf[3] = 0x00; // dummy byte
        cmdbuf[4] = (data && dlen > 0) ? (data[0] & 0x07) : 0x00; // 3-bit Start page address. Default 0b000
        cmdbuf[5] = (data && dlen > 1) ? (data[1] & 0x07) : 0x00; // 3-bit scroll step in frame frequency
        cmdbuf[6] = (data && dlen > 2) ? (data[2] & 0x07) : 0x07; // 3-bit end page address. default is last page
        if (cmdbuf[6] < cmdbuf[4]) {
            // end page must be larger or equal
            // overwriting
            cmdbuf[6] = cmdbuf[4];
        }
        cmdbuf[7] = (data && dlen > 3) ? (data[3] & 0x3F) : 0x01; // 1-63 rows of vertical offset scrolling
        cmdbuf[8] = 0x2F; // activate the scroll
        sz = 9;
        break;
    case SSD1306_I2C_CMD_SCROLL_VERTICAL_AREA:
        cmdbuf[1] = 0xA3; //0b10100011
        cmdbuf[2] = 0xA3; //0b10100011
        cmdbuf[3] = (data && dlen > 0) ? (data[0] & 0x3F) : 0x00; // no. of rows in top fixed area. 0 is RESET
        cmdbuf[4] = (data && dlen > 1) ? (data[1] & 0x7F) : 0x40; // no. of rows in scroll area. 64 is RESET
        cmdbuf[5] = 0x2F; // activate the scroll
        sz = 6;
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
    uint8_t data;
    FILE *err_fp = SSD1306_I2C_GET_ERRFP(oled);
    if (!oled || oled->fd < 0) {
        fprintf(err_fp, "ERROR: Invalid ssd1306 I2C object\n");
        return -1;
    }
    do {
        // power off the display before doing anything
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_OFF, 0, 0);
        if (rc < 0) break;
        // force horizontal memory addressing
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_MEM_ADDR_HORIZ, 0, 0);
        if (rc < 0) break;
        // these instructions are from the software configuration section 15.2.3 in
        // the datasheet
        // Set MUX Ratio 0xA8, 0x3F
        data = oled->height - 1;
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_MUX_RATIO, &data, 1);
        if (rc < 0) break;
        // Set display offset 0xD3, 0x00
        data = 0x00;
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_OFFSET, &data, 1);
        if (rc < 0) break;
        // set display start line 0x40
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_START_LINE, 0, 0);
        if (rc < 0) break;
        // set segment remap 0xA0/0xA1
        data = 0x01;
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_SEG_REMAP, &data, 1);
        if (rc < 0) break;
        // set com output scan direction 0xC0/0xC8
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_COM_SCAN_DIRXN_INVERT, 0, 0);
        if (rc < 0) break;
        // set com pins hardware config 0xDA, 0x02
        data = (oled->height == 32) ? 0x02 : 0x12;
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_COM_PIN_CFG, &data, 1);
        if (rc < 0) break;
        // set contrast control 0x81, 0xFF
        data = 0xFF;
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_CONTRAST, &data, 1);
        if (rc < 0) break;
        // disable entire display on 0xA4
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_DISABLE_ENTIRE_ON, 0, 0);
        if (rc < 0) break;
        // set normal display 0xA6
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_NORMAL, 0, 0);
        if (rc < 0) break;
        // set osc frequency 0xD5, 0x80
        data = 0x80;//RESET 0b10000000
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_CLOCK_DIVFREQ, &data, 1);
        if (rc < 0) break;
        // set precharge period 0xD9, 0xF1
        data = 0xF1;
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_PRECHARGE_PERIOD, &data, 1);
        if (rc < 0) break;
        // set Vcomh Deselect 0xDB 0x30
        data = 0x30;
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_VCOMH_DESELECT, &data, 1);
        if (rc < 0) break;
        // enable charge pump regulator 0x8D, 0x14
        // charge pump has to be followed by a power on. section 15.2.1 in datasheet
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_ENABLE_CHARGE_PUMP, 0, 0);
        if (rc < 0) break;
        // power display on 0xAF
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_ON, 0, 0);
        if (rc < 0) break;
        // deactivate scrolling
        rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_SCROLL_DEACTIVATE, 0, 0);
        if (rc < 0) break;
        // clear the screen
        rc |= ssd1306_i2c_display_clear(oled);
        if (rc < 0) break;
    } while (0);
    return rc;
}

int ssd1306_i2c_run_cmd(ssd1306_i2c_t *oled, ssd1306_i2c_cmd_t cmd, uint8_t *data, size_t dlen)
{
    FILE *err_fp = SSD1306_I2C_GET_ERRFP(oled);
    if (!oled || oled->fd < 0) {
        fprintf(err_fp, "ERROR: Invalid ssd1306 I2C object\n");
        return -1;
    }
    if (dlen > 0 && !data) {
        fprintf(err_fp, "WARN: data pointer is NULL but dlen is %zu. Ignoring\n", dlen);
        dlen = 0;
        data = NULL;
    }
    if (dlen > 6 && data != NULL) {
        fprintf(err_fp, "WARN: the maximum accepted data bytes for a command is 6. You gave %zu, adjusting to 6\n", dlen);
        dlen = 6;
    }
    uint8_t cmd_buf[16] = { 0 };
    const size_t cmd_buf_max = 16;
    size_t cmd_sz = ssd1306_i2c_internal_get_cmd_bytes(cmd, data, dlen,
                        cmd_buf, cmd_buf_max);
    if (cmd_sz == 0 || cmd_sz > cmd_buf_max) {
        fprintf(err_fp, "WARN: Unknown cmd given %d\n", cmd);
        return -1;
    }
    ssize_t nb = write(oled->fd, cmd_buf, cmd_sz);
    if (nb < 0) {
        oled->err->errnum = errno;
        strerror_r(oled->err->errnum, oled->err->errbuf, oled->err->errlen);
        fprintf(err_fp, "ERROR: Failed to write cmd ");
        for (size_t idx = 0; idx < cmd_sz; ++idx) {
            fprintf(err_fp, "%c0x%02x%c", (idx == 0) ? '[' : ' ',
                    cmd_buf[idx], (idx == (cmd_sz - 1)) ? ']' : ',');
        }
        fprintf(err_fp, " to device fd %d: %s\n",
                oled->fd, oled->err->errbuf);
        return -1;
    }
    fprintf(err_fp, "INFO: Wrote %zd bytes of cmd ", nb);
    for (size_t idx = 0; idx < cmd_sz; ++idx) {
        fprintf(err_fp, "%c0x%02x%c", (idx == 0) ? '[' : ' ',
                cmd_buf[idx], (idx == (cmd_sz - 1)) ? ']' : ',');
    }
    fprintf(err_fp, " to device fd %d\n", oled->fd);
    return 0;
}

int ssd1306_i2c_display_update(ssd1306_i2c_t *oled, const ssd1306_framebuffer_t *fbp)
{
    FILE *err_fp = SSD1306_I2C_GET_ERRFP(oled);
    if (!oled || oled->fd < 0 || !oled->gddram_buffer || oled->gddram_buffer_len == 0) {
        fprintf(err_fp, "ERROR: Invalid ssd1306 I2C object\n");
        return -1;
    }
    int rc = 0;
    uint8_t x[2] = { 0, oled->width - 1 };
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_COLUMN_ADDR, x, 2);
    x[0] = 0;
    x[1] = (oled->height / 8) - 1; // number of pages
    rc |= ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_PAGE_ADDR, x, 2);
    if (rc != 0) {
        fprintf(err_fp, "WARN: Unable to update display, exiting from earlier errors\n");
        return -1;
    }
    oled->gddram_buffer[0] = 0x40; // Co: 0 D/C#: 1 0b01000000
    // if the framebuffer pointer is provided, we copy it to GDDRAM, otherwise
    // we just display the GDDRAM
    if (fbp) {
        // invalid data in pointer
        if (!(fbp->buffer) || fbp->len == 0 || (fbp->len != (oled->gddram_buffer_len - 1))) {
            fprintf(err_fp, "ERROR: Invalid ssd1306 framebuffer object\n");
            return -1;
        }
        memcpy(&(oled->gddram_buffer[1]), fbp->buffer, fbp->len);
    }
    // the rest is framebuffer data for the GDDRAM as in section 8.1.5.2
    ssize_t nb = write(oled->fd, oled->gddram_buffer, oled->gddram_buffer_len);
    if (nb < 0) {
        oled->err->errnum = errno;
        strerror_r(oled->err->errnum, oled->err->errbuf, oled->err->errlen);
        fprintf(err_fp, "ERROR: Failed to write %zu bytes of screen buffer to device fd %d : %s\n",
                oled->gddram_buffer_len, oled->fd, oled->err->errbuf);
        return -1;
    }
    fprintf(err_fp, "INFO: Wrote %zd bytes of screen buffer to device fd %d\n", nb, oled->fd);
    return 0;
}

int ssd1306_i2c_display_clear(ssd1306_i2c_t *oled)
{
    if (oled != NULL && oled->gddram_buffer != NULL && oled->gddram_buffer_len > 0) {
        memset(oled->gddram_buffer, 0, sizeof(uint8_t) * oled->gddram_buffer_len);
        return ssd1306_i2c_display_update(oled, NULL);
    } else {
        FILE *err_fp = SSD1306_I2C_GET_ERRFP(oled);
        fprintf(err_fp, "ERROR: Invalid OLED object. Failed to clear display\n");
        return -1;
    }
}

const char *ssd1306_i2c_version(void)
{
    return LIBSSD1306_PACKAGE_VERSION;
}
