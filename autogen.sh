#!/bin/sh
libtoolize --copy
aclocal && autoheader && automake --add-missing && autoconf
