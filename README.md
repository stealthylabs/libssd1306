# libssd1306

## INTRODUCTION

This is a C library to be used for displaying on the SSD1306 OLED display such
as those sold by [Adafruit](https://www.adafruit.com/product/326), Sparkfun and
others, and whose data sheet can be found [here](https://www.olimex.com/Products/Modules/LCD/MOD-OLED-128x64/resources/SSD1306.pdf) and [here](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf).

If your goal is to use Arduino or CircuitPython, this library is **not** for
you.

This library is for the special use case where you have an application running
on the Raspberry Pi, Beaglebone Black or any other Linux single board computer
(SBC) with I<sup>2</sup>C pins available to connect this OLED screen to.

The library provides both the I<sup>2</sup>C user-land device driver and the graphics
primitives for the developer to be able to perform display tasks using a single
library.

Sometimes you want to integrate writing to this display into a larger code base which is
**not** written in Python but needs to run on such devices.

This is why this library is in C, so it can directly execute on the Raspberry Pi
or similar SBCs where it can be integrated into other applications easily.

Another reason for using this C library is to be able to integrate this display
with libraries like `libuv` or `libev` or similar event-based libraries,  and control
the display using events and keeping the power consumption low on such devices.


## BUILD and TEST

```bash
$ ./autogen.sh
$ ./configure
$ make
###  if you want to run all the tests on the attached OLED device on a Raspberry Pi
$ make check
### if you just want to run a single test manually
$ ./examples/test_ssd1306_i2c
```

If you want to install this library in a location other than `/usr/local` you
want to run `configure` with the `--prefix` option.

If you want to build in debug mode, you want to run `configure` with the
`--enable-debug` option.


## COPYRIGHT

&copy; 2020. Stealthy Labs LLC. All Rights Reserved.
