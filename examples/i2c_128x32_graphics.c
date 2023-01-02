/*
 * COPYRIGHT: 2020. Stealthy Labs LLC.
 * DATE: 2020-01-15
 * SOFTWARE: libssd1306-i2c
 * LICENSE: Refer license file
 */
#include <ssd1306_i2c.h>

int main ()
{
    fprintf(stderr, "DEBUG: Using library version: %s\n", ssd1306_i2c_version());
	const char *filename = "/dev/i2c-1";
    ssd1306_i2c_t *oled = ssd1306_i2c_open(filename, 0x3c, 128, 32, NULL);
    if (!oled) {
        return -1;
    }
    if (ssd1306_i2c_display_initialize(oled) < 0) {
        fprintf(stderr, "ERROR: Failed to initialize the display. Check if it is connected !\n");
        ssd1306_i2c_close(oled);
        return -1;
    }
    sleep(3);
    ssd1306_framebuffer_t *fbp = ssd1306_framebuffer_create(oled->width, oled->height, oled->err);

    ssd1306_i2c_display_clear(oled);
    for (uint8_t i = oled->height - 1; i < oled->height; ++i) {
        for (uint8_t j = 0; j < oled->width; ++j)
            ssd1306_framebuffer_put_pixel(fbp, j, i, true);
    }
    ssd1306_framebuffer_hexdump(fbp);
    ssd1306_framebuffer_bitdump(fbp);
    ssd1306_framebuffer_clear(fbp);
    ssd1306_framebuffer_box_t bbox;
#ifdef LIBSSD1306_HAVE_UNISTR_H
    ssd1306_framebuffer_draw_text(fbp, "åBCDeF", 0, 32, 16, SSD1306_FONT_DEFAULT, 4, &bbox);
#else
    ssd1306_framebuffer_draw_text(fbp, "ABCDeF", 0, 32, 16, SSD1306_FONT_DEFAULT, 4, &bbox);
#endif
    ssd1306_framebuffer_bitdump(fbp);
    ssd1306_i2c_display_update(oled, fbp);
    sleep(3);
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_INVERTED, 0, 0);
    sleep(3);
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_DISP_NORMAL, 0, 0);
    sleep(3);
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_SCROLL_DEACTIVATE, 0, 0);
    uint8_t scroll_data[3] = { 0x00 /* PAGE 0 */, 0x07 /* 2 frames */, 0x07 /* PAGE 7 */ };
    fprintf(stderr, "INFO: Starting left horizontal scroll test\n");
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_SCROLL_LEFT_HORIZONTAL, scroll_data, 3);
    fprintf(stderr, "INFO: Activating left horizontal scroll test and waiting 10 seconds...\n");
    sleep(10);
    fprintf(stderr, "INFO: Deactivating left horizontal scroll test\n");
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_SCROLL_DEACTIVATE, 0, 0);
    sleep(3);
    ssd1306_i2c_display_update(oled, fbp);
    fprintf(stderr, "INFO: Starting right horizontal scroll test\n");
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_SCROLL_RIGHT_HORIZONTAL, scroll_data, 3);
    fprintf(stderr, "INFO: Activating right horizontal scroll test and waiting 10 seconds...\n");
    sleep(10);
    fprintf(stderr, "INFO: Deactivating right horizontal scroll test\n");
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_SCROLL_DEACTIVATE, 0, 0);
    sleep(3);
    ssd1306_i2c_display_update(oled, fbp);
    fprintf(stderr, "INFO: Starting vertical left horizontal scroll test\n");
    uint8_t sarea[2] = { 0x00, 0x20 /* 32 rows */ };
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_SCROLL_DEACTIVATE, 0, 0);
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_SCROLL_VERTICAL_AREA, sarea, 2);
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_SCROLL_VERTICAL_LEFT_HORIZONTAL, scroll_data, 3);
    fprintf(stderr, "INFO: Activating vertical left horizontal scroll test and waiting 10 seconds...\n");
    sleep(10);
    fprintf(stderr, "INFO: Deactivating vertical left horizontal scroll test\n");
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_SCROLL_DEACTIVATE, 0, 0);
    sleep(3);
    ssd1306_i2c_display_update(oled, fbp);
    fprintf(stderr, "INFO: Starting vertical right horizontal scroll test\n");
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_SCROLL_VERTICAL_RIGHT_HORIZONTAL, scroll_data, 3);
    fprintf(stderr, "INFO: Activating vertical right horizontal scroll test and waiting 10 seconds...\n");
    sleep(10);
    fprintf(stderr, "INFO: Deactivating vertical right horizontal scroll test\n");
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_SCROLL_DEACTIVATE, 0, 0);
    sleep(3);
    ssd1306_i2c_display_update(oled, fbp);
    ssd1306_framebuffer_clear(fbp);
    ssd1306_framebuffer_put_pixel(fbp, 0, 0, true);
    ssd1306_framebuffer_put_pixel(fbp, fbp->width - 1, 0, true);
    ssd1306_framebuffer_put_pixel(fbp, 0, fbp->height - 1, true);
    ssd1306_framebuffer_put_pixel(fbp, fbp->width - 1, fbp->height - 1, true);
    ssd1306_framebuffer_put_pixel(fbp, 9, 10, true);
    ssd1306_framebuffer_bitdump(fbp);
    ssd1306_i2c_display_update(oled, fbp);
    sleep(3);
    ssd1306_framebuffer_clear(fbp);
    ssd1306_framebuffer_put_pixel(fbp, 0, 0, false);
    ssd1306_framebuffer_put_pixel(fbp, fbp->width - 1, 0, false);
    ssd1306_framebuffer_put_pixel(fbp, 0, fbp->height - 1, false);
    ssd1306_framebuffer_put_pixel(fbp, fbp->width - 1, fbp->height - 1, false);
    ssd1306_framebuffer_put_pixel(fbp, 9, 10, false);
    ssd1306_framebuffer_bitdump(fbp);
    ssd1306_i2c_display_update(oled, fbp);
    sleep(3);
    ssd1306_framebuffer_clear(fbp);
    ssd1306_framebuffer_draw_text(fbp, "A", 0, 0, 0, SSD1306_FONT_DEFAULT, 10, &bbox);
    ssd1306_framebuffer_bitdump(fbp);
    ssd1306_i2c_display_update(oled, fbp);
    sleep(3);
    ssd1306_graphics_options_t opts[3];
    opts[0].type = SSD1306_OPT_FONT_FILE;
    opts[0].value.font_file = "/usr/share/fonts/truetype/msttcorefonts/Comic_Sans_MS.ttf";
    opts[1].type = SSD1306_OPT_ROTATE_PIXEL;
    opts[1].value.rotation_degrees = 180;
    opts[2].type = SSD1306_OPT_ROTATE_FONT;
    opts[2].value.rotation_degrees = 30;
    ssd1306_framebuffer_clear(fbp);
    ssd1306_framebuffer_draw_text_extra(fbp, "a b c d e", 0, 32, 15, SSD1306_FONT_CUSTOM, 4, opts, 3, &bbox);
    ssd1306_framebuffer_bitdump(fbp);
    ssd1306_i2c_display_update(oled, fbp);
    sleep(3);
    ssd1306_framebuffer_clear(fbp);
    ssd1306_i2c_display_clear(oled);
    ssd1306_framebuffer_draw_line(fbp, 0, 0, 64, 32, true);
    ssd1306_framebuffer_draw_line(fbp, 64, 31, 128, 31, true);
    ssd1306_framebuffer_draw_line(fbp, 0, 0, 0, 32, true);
    ssd1306_framebuffer_bitdump(fbp);
    ssd1306_i2c_display_update(oled, fbp);
    sleep(3);
    ssd1306_i2c_run_cmd(oled, SSD1306_I2C_CMD_POWER_OFF, 0, 0);
    ssd1306_framebuffer_destroy(fbp);
    fbp = NULL;
    ssd1306_i2c_close(oled);
    oled = NULL;
	return 0;
}
