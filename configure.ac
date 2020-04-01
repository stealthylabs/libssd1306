########################################
##COPYRIGHT: 2020. Stealthy Labs LLC
##DATE: 2020-01-28
##LICENSE: Refer LICENSE file.
########################################
# initially generated using autoscan and then modified manually
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libssd1306], [0.6], [github@stealthylabs.com])
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
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_HEADER_STDC
AC_CHECK_HEADERS([ errno.h features.h fcntl.h inttypes.h math.h ])
AC_CHECK_HEADERS([sys/ioctl.h unistd.h stdio.h linux/i2c-dev.h ctype.h sys/stat.h sys/types.h])

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

## pthread using m4/ax_pthread.m4
dnl check for threading support
AC_ARG_ENABLE([threading],
              [AS_HELP_STRING([--disable-threading], [disable threading support])],
              [enable_threading=no],
              [enable_threading=yes])
if test x"$enable_threading" = "xyes"; then
    AC_MSG_NOTICE([checking for pthreads support])
    AX_PTHREAD([
                AC_DEFINE([HAVE_PTHREAD], [1], [Undefine this to disable threading support])
                LIBS="$PTHREAD_LIBS $LIBS"
                CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
                CC="$PTHREAD_CC"
                ], [enable_threading=no])
fi
AC_MSG_NOTICE([checking if threading is enabled... ${enable_threading-no}])

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

AC_MSG_CHECKING([if freetype2 is installed])
PKG_CHECK_MODULES([FREETYPE2], [freetype2],
                    [
                     AC_DEFINE([HAVE_FREETYPE2], [1], [Use Freetype2])
                     ],
                    [
                     AC_MSG_ERROR([freetype2 not found, please install it first])
                     ]
                    )
AC_SUBST([FREETYPE2_CFLAGS])
AC_SUBST([FREETYPE2_LIBS])

AC_ARG_WITH([libuv],
              [AS_HELP_STRING([--without-libuv], [build examples without libuv support @<:@default=yes@:>@])],
              [with_libuv=no],
              [with_libuv=yes])
AS_IF([test "x$with_libuv" = "xyes"], [
    AC_MSG_CHECKING([if libuv is installed])
       #we have to force some constants because they do not get set
    PKG_CHECK_MODULES([LIBUV], [libuv >= 1.24],
                    [
                     use_libuv=true
                     AC_DEFINE([HAVE_UV_H], [1], [Have uv.h])
                    ],
                    [
                     AC_CHECK_HEADERS([uv.h], [use_libuv=true], [use_libuv=false])
                     AC_CHECK_LIB([uv], [uv_run], [use_libuv=true], [use_libuv=false])
                    ]
    )
])
AM_CONDITIONAL([HAVE_LIBUV], [test "x$use_libuv" = "xtrue"])
AC_SUBST([LIBUV_CFLAGS])
AC_SUBST([LIBUV_LIBS])

AC_ARG_WITH([libev],
              [AS_HELP_STRING([--without-libev], [build examples without libev support @<:@default=yes@:>@])],
              [with_libev=no],
              [with_libev=yes])
AS_IF([test "x$with_libev" = "xyes"], [
    AC_MSG_CHECKING([if libev is installed])
    PKG_CHECK_MODULES([LIBEV], [libev], 
                      [
                       use_libev=true
                       AC_DEFINE([HAVE_EV_H], [1], [Have libev.h])
                      ],
                      [
                       AC_CHECK_HEADERS([ev.h], [use_libev=true], [use_libev=false])
                       AC_CHECK_LIB([ev], [ev_run], [use_libev=true], [use_libev=false])
                      ]
    )
])
AM_CONDITIONAL([HAVE_LIBEV], [test "x$use_libev" = "xtrue"])
AC_SUBST([LIBEV_CFLAGS])
AC_SUBST([LIBEV_LIBS])

AC_CONFIG_FILES([Makefile src/Makefile examples/Makefile])
AC_OUTPUT
