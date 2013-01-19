/*
  Copyright (C) 2013 Tomash Brechko.  All rights reserved.

  This file is part of kroki/glxoffload.

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
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <kroki/error.h>
#include <X11/Xlib.h>
#include <dlfcn.h>
#include <stdlib.h>

#ifdef NEED_USCORE
#define USCORE  "_"
#else
#define USCORE
#endif


typedef void (*fptr)();

struct redef_func
{
  fptr accl_fp;
  fptr dspl_fp;
  fptr addr_fp;
  const char *const name;
};


#define IMPORT(func)                                            \
  static __attribute__((__section__("_kroki_glxoffload")))      \
  struct redef_func redef_##func = {                            \
    .name = #func,                                              \
  };                                                            \
                                                                \
  static __attribute__((__weakref__(#func)))                    \
  void                                                          \
  weakref_##func();                                             \
                                                                \
  static __attribute__((__constructor__(1001)))                 \
  void                                                          \
  init1_##func(void)                                            \
  {                                                             \
    redef_##func.accl_fp = dlsym(RTLD_NEXT, USCORE #func);      \
    redef_##func.dspl_fp = dlsym(dspl_libgl, USCORE #func);     \
    redef_##func.addr_fp = weakref_##func;                      \
  }


static void *dspl_libgl = NULL;
static Display *accl_dpy = NULL;


static __attribute__((__constructor__(1000)))
void
init0(void)
{
  /*
    RTLD_DEEPBIND in dlopen() makes original KROKI_GLXOFFLOAD_LIBGL
    see symbols of its own dependencies rather than our overrides.
  */
  dspl_libgl = CHECK(dlopen(getenv("KROKI_GLXOFFLOAD_DSPL_LIBGL"),
                            RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND),
                     == NULL, die, "%s", dlerror());

  accl_dpy = MEM(XOpenDisplay(getenv("KROKI_GLXOFFLOAD_DPY")),
                 " for %s", getenv("KROKI_GLXOFFLOAD_DPY"));
}


static __attribute__((__destructor__))
void
fini(void)
{
  XCloseDisplay(accl_dpy);

  dlclose(dspl_libgl);
}
