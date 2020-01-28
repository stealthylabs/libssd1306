AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES=libssd1306_i2c.la
libssd1306_i2c_la_HEADERS=ssd1306_i2c.h
libssd1306_i2c_ladir=$(includedir)
libssd1306_i2c_la_SOURCES=$(libssd1306_i2c_la_HEADERS) ssd1306_i2c.c
libssd1306_i2c_la_CFLAGS=-I$(top_srcdir) $(AM_CFLAGS)
libssd1306_i2c_la_LDFLAGS=-shared -version-info 0:1:0 -L$(top_builddir)

bin_PROGRAMS=test_ssd1306_i2c
test_ssd1306_i2c_SOURCES=test_ssd1306_i2c.c
test_ssd1306_i2c_CFLAGS=-I$(top_srcdir) $(AM_CFLAGS)
test_ssd1306_i2c_LDADD=libssd1306_i2c.la
#test_ssd1306_i2c_LDFLAGS=-Wl,-rpath,$(top_srcdir) -L$(top_builddir)

