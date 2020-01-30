/*
 * COPYRIGHT: 2020. Stealthy Labs LLC.
 * DATE: 2020-01-15
 * SOFTWARE: libssd1306-i2c
 * LICENSE: Refer license file
 */
#include <ssd1306_i2c_config.h>
#if HAVE_FEATURES_H
#include <features.h>
#endif
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ssd1306_i2c.h>

int ssd1306_i2c_framebuffer_draw_bricks(ssd1306_i2c_t *oled)
{
    if (!(oled != NULL && oled->gddram_buffer != NULL &&
        oled->gddram_buffer_len > 0)) {
        return -1;
    }
    uint8_t * const fb = &(oled->gddram_buffer[1]);
    size_t fblen = oled->gddram_buffer_len - 1;
    for (size_t i = 0; i < fblen; ++i) {
        if (i % 1) {
            fb[i] = 0xFF;
        }
        if (i % 3) {
            fb[i] = 0x7F;
        }
        if (i % 5) {
            fb[i] = 0x3F;
        }
    }
    return 0;
}
