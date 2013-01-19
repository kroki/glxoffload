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


#define ARG_COUNT_SHIFT(t7, v7, t6, v6, t5, v5,         \
                        t4, v4, t3, v3, t2, v2, t1, v1, \
                        t0, n, ...)  n
#define ARG_COUNT(...)                                          \
  ARG_COUNT_SHIFT(__VA_ARGS__,                                  \
                  ?, 7, ?, 6, ?, 5, ?, 4, ?, 3, ?, 2, ?, 1, 0)

#define CONCAT(a, b)  a##b
#define EVAL_CONCAT(a, b)  CONCAT(a, b)

#define PROTO(...)  EVAL_CONCAT(PROTO, ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)
#define ARGS(...)  EVAL_CONCAT(ARGS, ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)

#define PROTO0(void)  void
#define PROTO1(t1, v1)  t1 v1
#define PROTO2(t1, v1, t2, v2)  t1 v1, t2 v2
#define PROTO3(t1, v1, t2, v2, t3, v3)  t1 v1, t2 v2, t3 v3
#define PROTO4(t1, v1, t2, v2, t3, v3, t4, v4)  t1 v1, t2 v2, t3 v3, t4 v4
#define PROTO5(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5)  \
  t1 v1, t2 v2, t3 v3, t4 v4, t5 v5
#define PROTO6(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6)     \
  t1 v1, t2 v2, t3 v3, t4 v4, t5 v5, t6 v6
#define PROTO7(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7)  \
  t1 v1, t2 v2, t3 v3, t4 v4, t5 v5, t6 v6, t7 v7

#define ARGS0(void)
#define ARGS1(t1, v1)  v1
#define ARGS2(t1, v1, t2, v2)  v1, v2
#define ARGS3(t1, v1, t2, v2, t3, v3)  v1, v2, v3
#define ARGS4(t1, v1, t2, v2, t3, v3, t4, v4)  v1, v2, v3, v4
#define ARGS5(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5)   \
  v1, v2, v3, v4, v5
#define ARGS6(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6)   \
  v1, v2, v3, v4, v5, v6
#define ARGS7(t1, v1, t2, v2, t3, v3, t4, v4, t5, v5, t6, v6, t7, v7)   \
  v1, v2, v3, v4, v5, v6, v7


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
