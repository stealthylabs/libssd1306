# libssd1306

We have written blog posts documenting how to use this library here:
<https://stealthy.io/blog/tag/libssd1306/index.html>.

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
with libraries like `libev` or similar event-based libraries,  and control
the display using events and keeping the power consumption low on such devices.


## BUILD and TEST

### PRE-REQUISITES

Install Pre-requisties first.

```bash
$ sudo apt-get -y install libfreetype6-dev fonts-freefont-ttf ttf-bitstream-vera \
        autoconf automake libtool autotools-dev build-essential
### optional pre-requisites
$ sudo apt-get -y install libev-dev
```

You may install several other font libraries such as Microsoft fonts which we
support.

```bash
### optional Microsoft Fonts installer, if you want it
$ sudo apt-get -y install ttf-mscorefonts-installer
```

### COMPILE

```bash
$ ./autogen.sh
$ ./configure
$ make
```

If you want to install this library in a location other than `/usr/local` you
want to run `configure` with the `--prefix` option.

If you want to build in debug mode, you want to run `configure` with the
`--enable-debug` option.

The `configure` script auto-detects the presence of `libev` and
turns on compilation of examples that use `libev`. If you want to
disable this you may use the `--without-libev` option with
the `configure` script.

For example, here is how you would run the `configure` script with all the above
optional library options.

```bash
$ ./configure --enable-debug --prefix=$HOME/local
```

### TEST

We have provided some sample applications to test the framebuffer objects, the
I<sup>2</sup>C I/O, and to use `libev` along with this library to demonstrate
writing to the SSD1306 OLED screen using an event library.

```bash
###  if you want to run all the tests on the attached OLED device on a Raspberry Pi
$ make check

### if you want to check graphics framebuffer code without any device connected
$ ./examples/test_fb_graphics

### if you just want to run a single test on a connected SSD1306 I<sup>2</sup>C device  manually
$ ./examples/test_i2c_128x132

### if you want to run the clock display for 30 seconds
$ ./examples/test_libev_clock

```

### DEBUGGING

To load the example executables directly into the debugger like `gdb` or a
memory profiler like `valgrind`, you must use the `libtool` script created
during the build process. It will locate all the required libraries, set the
environment correctly and will run the program under the debugger. For example,

```bash
### check the libraries that are linked
$ ./libtool --mode=execute ldd -d -r ./examples/test_libev_clock

### running executable under gdb
$ ./libtool --mode=execute gdb --args ./examples/test_draw_line

### running executable under valgrind
$ ./libtool --mode=execute valgrind --tool=memcheck ./examples/test_fb_graphics
```

## COPYRIGHT

&copy; 2020. Stealthy Labs LLC. All Rights Reserved.
