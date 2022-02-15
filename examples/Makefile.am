AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = $(ACLOCAL_FLAGS)

noinst_PROGRAMS=test_i2c_128x32 test_fb_graphics test_draw_line
TESTS=$(noinst_PROGRAMS)

SSD1306_LIB=$(top_builddir)/src/libssd1306_i2c.la

test_i2c_128x32_SOURCES=i2c_128x32_graphics.c
test_i2c_128x32_LDADD=$(SSD1306_LIB)

test_fb_graphics_SOURCES=fb_graphics.c
test_fb_graphics_CFLAGS=
test_fb_graphics_LDADD=$(SSD1306_LIB)

if HAVE_UNISTRING
test_fb_graphics_CFLAGS+=$(UNISTRING_CFLAGS)
test_fb_graphics_LDADD+=$(UNISTRING_LIBS)
endif

test_draw_line_SOURCES=draw_line.c
test_draw_line_LDADD=$(SSD1306_LIB)

if HAVE_LIBEV
noinst_PROGRAMS+=test_libev_clock
test_libev_clock_SOURCES=libev_clock.c
test_libev_clock_CFLAGS=$(LIBEV_CFLAGS)
test_libev_clock_LDADD=$(SSD1306_LIB) $(LIBEV_LIBS)
endif
