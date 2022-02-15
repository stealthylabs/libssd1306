AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = $(ACLOCAL_FLAGS) 

lib_LTLIBRARIES=libssd1306_i2c.la
libssd1306_i2c_la_HEADERS=$(top_srcdir)/include/ssd1306_i2c.h \
						  $(top_srcdir)/include/ssd1306_graphics.h \
						  $(top_srcdir)/include/ssd1306_config.h
libssd1306_i2c_la_CFLAGS=$(FREETYPE2_CFLAGS)
libssd1306_i2c_ladir=$(includedir)
libssd1306_i2c_la_SOURCES=$(libssd1306_i2c_la_HEADERS) ssd1306_i2c.c graphics.c
libssd1306_i2c_la_LDFLAGS=-shared -version-info 0:3:0 -L$(top_builddir) -L$(builddir) $(FREETYPE2_LIBS)

if HAVE_UNISTRING
libssd1306_i2c_la_CFLAGS+=$(UNISTRING_CFLAGS)
libssd1306_i2c_la_LDFLAGS+=$(UNISTRING_LIBS)
endif
