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
    ssd1306_i2c_dev_t *devs = NULL;
    size_t num_devs = 0;
    int rc = ssd1306_i2c_search_addresses(&devs, &num_devs, stderr);
    if (rc < 0)
        return rc;
    if (!devs) {
        fprintf(stderr, "ERROR: Invalid return pointers\n");
        return -1;
    }
    if (devs && num_devs == 0) {
        fprintf(stderr, "INFO: No I2C devices found\n");
        free(devs);
        return 0;
    }
    fprintf(stderr, "INFO: %zu I2C devices found\n", num_devs);
    for (size_t idx = 0; idx < num_devs; ++idx) {
        fprintf(stderr, "INFO: Device: %s has %zu valid addresses",
                devs[idx].dev, devs[idx].num_addrs);
        if (devs[idx].num_addrs > 0) {
            fputs(": ", stderr);
        }
        for (size_t adx = 0; adx < devs[idx].num_addrs; ++adx) {
            fprintf(stderr, "0x%02x", devs[idx].addrs[adx]);
            if ((adx + 1) < devs[idx].num_addrs) {
                fputc(',', stderr);
            }
        }
        fputs("\n", stderr);
    }
    free(devs);
	return 0;
}
