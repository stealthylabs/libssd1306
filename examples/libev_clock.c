/*
 * COPYRIGHT: 2020. Stealthy Labs LLC.
 * DATE: 2020-08-06
 * SOFTWARE: libssd1306-i2c
 * LICENSE: Refer license file
 */
#include <ssd1306_i2c.h>
#ifdef LIBSSD1306_HAVE_FCNTL_H
    #include <fcntl.h>
#endif
#ifdef LIBSSD1306_HAVE_EV_H
    #include <ev.h>
#endif

int main(int argc, char **argv)
{
    int rc = 0;
    struct ev_loop *loop = EV_DEFAULT;
    return rc;
}
