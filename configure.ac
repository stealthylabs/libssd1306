########################################
##COPYRIGHT: 2020. Stealthy Labs LLC
##DATE: 2020-01-28
##LICENSE: Refer LICENSE file.
########################################
# initially generated using autoscan and then modified manually
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libssd1306], [0.2], [github@stealthylabs.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/ssd1306_i2c.c])
AM_CONFIG_HEADER(include/ssd1306_config.h)
AC_CONFIG_MACRO_DIRS([m4])

# Checks for programs.
## store user given CFLAGS
USER_CFLAGS=$CFLAGS
AC_PROG_CC
AC_PROG_INSTALL
LT_INIT([disable-static pic-only])
CFLAGS=$USER_CFLAGS

# shared library generation stuff
AC_ENABLE_SHARED
AC_DISABLE_STATIC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([ errno.h features.h fcntl.h inttypes.h stdlib.h string.h ])
AC_CHECK_HEADERS([sys/ioctl.h unistd.h stdio.h linux/i2c-dev.h ctype.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT64_T
AC_TYPE_INT64_T
AC_TYPE_UINT32_T
AC_TYPE_INT32_T
AC_TYPE_UINT16_T
AC_TYPE_INT16_T
AC_TYPE_UINT8_T
AC_TYPE_INT8_T
AC_TYPE_UINTPTR_T
AC_TYPE_INTPTR_T
AC_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([memset strdup memcpy calloc ioctl])

## debug test
AC_MSG_CHECKING([whether to build with debug information])
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug], [enable debug data generation (def=no)])],
              [debugit="$enableval"],
              [debugit=no])
AC_MSG_RESULT([$debugit])
AM_CFLAGS="-Wall -Wextra -Wno-uninitialized -fPIC -pedantic -posix -std=c11 -D_DEFAULT_SOURCE -D_REENTRANT"

if test x"$debugit" = x"yes"; then
    AC_DEFINE([DEBUG],[],[Debug Mode])
    dnl Remove all optimization flags from CFLAGS
    changequote({,})
    CFLAGS=`echo "$CFLAGS" | sed -e 's/-O[0-9s]*//g' | sed -e 's/-g[0-9]*//g'`
    changequote([,])
    echo "CFLAGS=$CFLAGS"

    AM_CFLAGS="$AM_CFLAGS -g -Werror -O0"
else
    AC_DEFINE([NDEBUG],[],[No Debug Mode])
    AM_CFLAGS="$AM_CFLAGS"
fi

AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_LDFLAGS])

PKG_CHECK_MODULES([FREETYPE2], [freetype2], [AC_DEFINE([HAVE_FREETYPE2], [1], [Use Freetype2])])
AC_SUBST([FREETYPE2_CFLAGS])
AC_SUBST([FREETYPE2_LIBS])

AC_ARG_WITH([libuv], AS_HELP_STRING([--with-libuv], [Build examples and applications that use libuv]))
AS_IF([test "x$with_libuv" = "xyes"], [
       #we have to force some constants because they do not get set
    PKG_CHECK_MODULES([LIBUV], [libuv], [
                       AC_DEFINE([USE_LIBUV], [1], [Use libuv])
                       AC_DEFINE([HAVE_UV_H], [1], [Define to 1 if uv.h is found])
                    ],
                    [AC_CHECK_HEADERS([uv.h], [
                       AC_DEFINE([USE_LIBUV], [1], [Use libuv])
                    ])]
    )
])
AC_SUBST([LIBUV_CFLAGS])
AC_SUBST([LIBUV_LIBS])

AC_ARG_WITH([libev], AS_HELP_STRING([--with-libev], [Build examples and applications that use libev]))
AS_IF([test "x$with_libev" = "xyes"], [
    PKG_CHECK_MODULES([LIBEV], [libev], [
                       AC_DEFINE([USE_LIBEV], [1], [Use libev])
                       AC_DEFINE([HAVE_EV_H], [1], [Have libev.h])
                      ],
                      [AC_CHECK_HEADERS([ev.h], [
                           AC_DEFINE([USE_LIBEV], [1], [Use libev])
                          ])]
                    )
])
AC_SUBST([LIBEV_CFLAGS])
AC_SUBST([LIBEV_LIBS])

AC_CONFIG_FILES([Makefile src/Makefile examples/Makefile])
AC_OUTPUT
