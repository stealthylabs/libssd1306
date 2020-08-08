/*
 * COPYRIGHT: 2020. Stealthy Labs LLC.
 * DATE: 2020-08-06
 * SOFTWARE: libssd1306-i2c
 * LICENSE: Refer license file
 */
#include <ssd1306_i2c.h>
#ifdef LIBSSD1306_HAVE_FEATURES_H
#include <features.h>
#endif
#ifdef LIBSSD1306_HAVE_FCNTL_H
    #include <fcntl.h>
#endif
#ifdef LIBSSD1306_HAVE_EV_H
    #include <ev.h>
#endif
#ifdef LIBSSD1306_TIME_WITH_SYS_TIME
    #include <time.h>
    #include <sys/time.h>
#else
    #include <time.h>
#endif

typedef struct {
    ssd1306_i2c_t *oled;
    ssd1306_framebuffer_t *fbp;
    int call_count;
} i2c_clock_t;

void onesec_timer_cb(EV_P_ ev_timer *w, int revents)
{
    struct tm _tm = { 0 };
    time_t _tsec = time(NULL);
#ifdef LIBSSD1306_HAVE_LOCALTIME_R
    localtime_r(&_tsec, &_tm);
#else /* not re-entrant */
    struct tm *_tm2 = localtime(&_tsec);
    memcpy(&_tm, _tm2, sizeof(_tm));
#endif
    char buf[16] = { 0 };
    snprintf(buf, sizeof(buf) - 1, "%02d:%02d:%02d", _tm.tm_hour, _tm.tm_min, _tm.tm_sec);
    printf("INFO: Time is %s\n", buf);
    if (w) {
        i2c_clock_t *i2c = (i2c_clock_t *)(w->data);
        /* write the clock to I2C device */
        ssd1306_framebuffer_clear(i2c->fbp);
        ssd1306_framebuffer_box_t bbox;
        ssd1306_framebuffer_draw_text(i2c->fbp, buf, 0, 32, 16, SSD1306_FONT_DEFAULT, 4, &bbox);
        if (ssd1306_i2c_display_update(i2c->oled, i2c->fbp) < 0) {
            fprintf(stderr, "ERROR: failed to update I2C display, exiting...\n");
            ev_break(EV_A_ EVBREAK_ALL);
        }
        if ((i2c->call_count--) <= 0) {
            ev_break(EV_A_ EVBREAK_ALL);
        }
    }
}

int main(int argc, char **argv)
{
    /* check if i2c device exists */
	const char *filename = (argc > 1) ? argv[1] : "/dev/i2c-1";
    size_t height = (argc > 2) ? strtoul(argv[2], NULL, 10) : 32;
    height = height & 0xFF;
    printf("INFO: Using I2C device %s. Assuming size 128x%zu\n", filename, height);
    ssd1306_i2c_t *oled = ssd1306_i2c_open(filename, 0x3c, 128, (uint8_t)height, NULL);
    if (!oled) {
        return -1;
    }
    /* initialize the I2C device */
    if (ssd1306_i2c_display_initialize(oled) < 0) {
        fprintf(stderr, "ERROR: Failed to initialize the display. Check if it is connected !\n");
        ssd1306_i2c_close(oled);
        return -1;
    }
    /* clear the display */
    ssd1306_i2c_display_clear(oled);
    /* create a framebuffer */
    ssd1306_framebuffer_t *fbp = ssd1306_framebuffer_create(oled->width, oled->height, oled->err);
    /* create an object to send to the callbacks */
    i2c_clock_t timer_data = {
        .oled = oled,
        .fbp = fbp,
        .call_count = 30 /* timeout after 30 seconds */
    };
    /* create the loop variable by using the default */
    struct ev_loop *loop = EV_DEFAULT;
    /* create the timer event object */
    ev_timer timer_watcher = { 0 };
    /* initialize the timer to update each second and invoke callback */
    ev_timer_init(&timer_watcher, onesec_timer_cb, 1., 1.);
    /* set the data pointer for the callback to use it */
    timer_watcher.data = &timer_data;
    /* start the timer */
    ev_timer_start(loop, &timer_watcher);
    ev_run(loop, 0);
    ssd1306_framebuffer_destroy(fbp);
    ssd1306_i2c_close(oled);
    return 0;
}
