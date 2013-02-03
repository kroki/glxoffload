dnl Process this file with autoconf to produce a configure script.
dnl
dnl To bootstrap run './bootstrap.sh'.
dnl

m4_define([kroki_glxoffload_copyright],
          [patsubst([
Copyright (C) 2013 Tomash Brechko.  All rights reserved.

Kroki/glxoffload is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Kroki/glxoffload is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with kroki/glxoffload.  If not, see <http://www.gnu.org/licenses/>.
], [
], [\\n])])

AC_PREREQ(2.66)
m4_include([version.m4])
AC_INIT([Kroki/glxoffload], [VERSION_STRING], [tomash.brechko@gmail.com],
        [kroki-glxoffload], [https://github.com/kroki/glxoffload])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_AUX_DIR([auto])
AC_CONFIG_MACRO_DIR([m4])

AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([1.11.1 -Wall foreign no-define subdir-objects
                  dist-bzip2 no-dist-gzip color-tests])

AC_PREFIX_DEFAULT([/usr/local])

AC_CONFIG_SRCDIR([Makefile.common])

AM_PROG_AR

LT_INIT([disable-static])
AS_IF([test x"$enable_static" = x"yes"],
      [AC_MSG_ERROR([kroki-glxoffload can't be build statically])])

AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CC_C99

AC_CHECK_HEADER([dlfcn.h], [], [AC_MSG_ERROR([dlfcn.h is required])])
AC_CHECK_HEADER([kroki/likely.h], [],
                [AC_MSG_ERROR([kroki/likely.h is required])])
AC_CHECK_HEADER([kroki/error.h], [],
                [AC_MSG_ERROR([kroki/error.h is required])])
AC_CHECK_HEADER([cuckoo_hash.h], [],
                [AC_MSG_ERROR([cuckoo_hash.h is required])])
AC_CHECK_HEADER([link.h], [], [AC_MSG_ERROR([link.h is required])])
AC_CHECK_HEADER([X11/Xlib.h], [], [AC_MSG_ERROR([X11/Xlib.h is required])])
AC_CHECK_HEADER([GL/glx.h], [], [AC_MSG_ERROR([GL/glx.h is required])])
AC_CHECK_HEADER([GL/glxext.h], [], [AC_MSG_ERROR([GL/glxext.h is required])],
                [#include <GL/glx.h>])

save_LIBS=$LIBS

LIBADD_DLOPEN=
AC_SEARCH_LIBS([dlopen], [dl], [LIBADD_DLOPEN=$ac_cv_search_dlopen],
               [AC_MSG_ERROR([libkroki-glxoffload requires dlopen()])])
AC_SUBST([LIBADD_DLOPEN])

LT_FUNC_DLSYM_USCORE

LIBADD_CUCKOO_HASH=
AC_SEARCH_LIBS([cuckoo_hash_init], [cuckoo_hash],
               [LIBADD_CUCKOO_HASH=$ac_cv_search_cuckoo_hash_init],
               [AC_MSG_ERROR([libkroki-glxoffload requires libcuckoo_hash])])
AC_SUBST([LIBADD_CUCKOO_HASH])

LIBS=$save_LIBS

AC_SUBST([RPM_VERSION], [`echo VERSION_STRING | sed -e 's/-.*//'`])
AC_SUBST([RPM_RELEASE], [`echo VERSION_STRING | sed -e 's/[[^-]]*-\?//'`])
AC_DEFINE([PACKAGE_COPYRIGHT], ["kroki_glxoffload_copyright"],
          [Copyright string.])
AC_SUBST_FILE([README])
README=$srcdir/README

AC_CACHE_SAVE

AC_CONFIG_FILES([
kroki-glxoffload.spec
Makefile
src/Makefile
])
AC_CONFIG_FILES([bin/kroki-glxoffload], [chmod a+x bin/kroki-glxoffload])
AC_CONFIG_FILES([stamp-h], [echo timestamp > stamp-h])

AC_OUTPUT
