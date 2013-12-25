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
#include <kroki/likely.h>
#include <kroki/error.h>
#include <cuckoo_hash.h>
#include <pthread.h>
#include <X11/Xlib.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glxext.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

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


#define REDEF(ret, func, ...)                   \
  IMPORT(func);                                 \
                                                \
  ret                                           \
  func(PROTO(__VA_ARGS__))


#define UPCALL(target, func, ...)               \
  __builtin_choose_expr(                        \
    1,                                          \
    ((__typeof__(func(__VA_ARGS__))(*)())       \
     redef_##func.target##_fp)(__VA_ARGS__),    \
    (void) 0)

#define ACCL(func, ...)  UPCALL(accl, func, ##__VA_ARGS__)
#define DSPL(func, ...)  UPCALL(dspl, func, ##__VA_ARGS__)


#define UNUSED(v)                               \
  (void) (v)

#define PARENS(a)  (a)
#define COMMA(...)  , ##__VA_ARGS__

#define ACCL_DPY(ret, func, display, dpy, ...)                  \
  REDEF(ret, func, display, dpy, ##__VA_ARGS__)                 \
  {                                                             \
    UNUSED(dpy);                                                \
    return __builtin_choose_expr(                               \
      __builtin_types_compatible_p(display, Display *),         \
      ACCL(func, accl_dpy COMMA PARENS(ARGS(__VA_ARGS__))),     \
      (void) 0);                                                \
  }


#define DSPL_DPY(ret, func, ...)                \
  REDEF(ret, func, __VA_ARGS__)                 \
  {                                             \
    return __builtin_choose_expr(               \
      1,                                        \
      DSPL(func, ARGS(__VA_ARGS__)),            \
      (void) 0);                                \
  }


static void *dspl_libgl = NULL;
static Display *accl_dpy = NULL;
static int verbose = 0;
static enum { AUTO, RGB, BGRA } copy_method = AUTO;


#define COPY_CONFIDENCE_MASK_ZEROES  4
#define COPY_CONFIDENCE_MASK_DIVIDER  (1 << COPY_CONFIDENCE_MASK_ZEROES)


struct drw_info_key
{
  Display *dpy;
  GLXDrawable drw;
};

struct drw_info
{
  struct drw_info_key key;
  GLXFBConfig accl_config;
  GLXPbuffer accl_pbuffer;

  Drawable xdrw;
  GLsizei width;
  GLsizei height;
  GLuint accl_copy_pbuffers[2];
  GLuint dspl_texture;
  unsigned int frame_no;
  int copy_nsec;
  unsigned short copy_confidence_mask;
  bool copy_rgb;
};

struct ctx_info
{
  GLXContext accl_ctx;
  GLXFBConfig accl_config;
  GLXContext dspl_ctx;
};


static pthread_mutex_t ctx_infos_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct cuckoo_hash ctx_infos;


static
struct ctx_info *
ctx_info_create(GLXContext accl_ctx, GLXFBConfig accl_config,
                GLXContext dspl_ctx)
{
  struct ctx_info *res = MEM(malloc(sizeof(*res)));
  res->accl_ctx = accl_ctx;
  res->accl_config = accl_config;
  res->dspl_ctx = dspl_ctx;
  pthread_mutex_lock(&ctx_infos_mutex);
  CHECK(cuckoo_hash_insert(&ctx_infos,
                           &res->accl_ctx, sizeof(res->accl_ctx), res),
        != NULL, die, "%m");
  pthread_mutex_unlock(&ctx_infos_mutex);
  return res;
}


static
struct ctx_info *
ctx_info_lookup(GLXContext accl_ctx)
{
  pthread_mutex_lock(&ctx_infos_mutex);
  struct cuckoo_hash_item *it =
    cuckoo_hash_lookup(&ctx_infos, &accl_ctx, sizeof(accl_ctx));
  pthread_mutex_unlock(&ctx_infos_mutex);
  return (it ? it->value : NULL);
}


static
void
ctx_info_destroy(GLXContext accl_ctx)
{
  pthread_mutex_lock(&ctx_infos_mutex);
  struct cuckoo_hash_item *it =
    cuckoo_hash_lookup(&ctx_infos, &accl_ctx, sizeof(accl_ctx));
  cuckoo_hash_remove(&ctx_infos, it);
  pthread_mutex_unlock(&ctx_infos_mutex);
  free(it->value);
}


static pthread_mutex_t drw_infos_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct cuckoo_hash drw_infos;


static
struct drw_info *
drw_info_create(Display *dpy, GLXDrawable drw, GLXFBConfig config,
                Drawable xdrw)
{
  struct drw_info *res = MEM(malloc(sizeof(*res)));
  memset(&res->key, 0, sizeof(res->key));
  res->key.dpy = dpy;
  res->key.drw = drw;
  res->accl_config = config;
  res->accl_pbuffer = None;
  res->width = 0;
  res->height = 0;
  res->accl_copy_pbuffers[0] = None;
  res->accl_copy_pbuffers[1] = None;
  res->dspl_texture = None;
  res->frame_no = 0;
  res->copy_nsec = 0;
  res->copy_confidence_mask = 0;
  res->copy_rgb = (copy_method == RGB);
  res->xdrw = xdrw;
  pthread_mutex_lock(&drw_infos_mutex);
  CHECK(cuckoo_hash_insert(&drw_infos, &res->key, sizeof(res->key), res),
        != NULL, die, "%m");
  pthread_mutex_unlock(&drw_infos_mutex);
  return res;
}


static
struct drw_info *
drw_info_lookup(Display *dpy, GLXDrawable drw)
{
  struct drw_info_key key;
  memset(&key, 0, sizeof(key));
  key.dpy = dpy;
  key.drw = drw;
  pthread_mutex_lock(&drw_infos_mutex);
  struct cuckoo_hash_item *it =
    cuckoo_hash_lookup(&drw_infos, &key, sizeof(key));
  pthread_mutex_unlock(&drw_infos_mutex);
  return (it ? it->value : NULL);
}


static
void
drw_info_destroy(Display *dpy, GLXDrawable drw);


REDEF(void,
glXDestroyPbuffer, Display *, dpy, GLXPbuffer, pbuf)
{
  drw_info_destroy(dpy, pbuf);
  DSPL(glXDestroyPbuffer, dpy, pbuf);
}


IMPORT(glDeleteTextures);


static
void
drw_info_destroy(Display *dpy, GLXDrawable drw)
{
  struct drw_info_key key;
  memset(&key, 0, sizeof(key));
  key.dpy = dpy;
  key.drw = drw;
  pthread_mutex_lock(&drw_infos_mutex);
  struct cuckoo_hash_item *it =
    cuckoo_hash_lookup(&drw_infos, &key, sizeof(key));
  cuckoo_hash_remove(&drw_infos, it);
  pthread_mutex_unlock(&drw_infos_mutex);
  if (it)
    {
      struct drw_info *di = it->value;
      if (di->accl_pbuffer)
        ACCL(glXDestroyPbuffer, accl_dpy, di->accl_pbuffer);
      if (di->dspl_texture)
        {
          glDeleteBuffers(2, di->accl_copy_pbuffers);
          DSPL(glDeleteTextures, 1, &di->dspl_texture);
        }
      free(di);
    }
}


/*
  _kroki_glxoffload_get_proc_address() must be exported
  (i.e. non-static; see kroki-glxoffload-audit.c).
*/
fptr
_kroki_glxoffload_get_proc_address(const char *name)
{
  if (likely(name))
    {
      extern __attribute__((__visibility__("hidden")))
        struct redef_func __start__kroki_glxoffload,
                          __stop__kroki_glxoffload;

      for (const struct redef_func *af = &__start__kroki_glxoffload;
           af != &__stop__kroki_glxoffload; ++af)
        {
          if (strcmp(af->name, name) == 0)
            return af->addr_fp;
        }
    }

  return NULL;
}


REDEF(fptr,
glXGetProcAddress, const GLubyte *, procName)
{
  fptr res = _kroki_glxoffload_get_proc_address((const char *) procName);
  if (! res)
    res = ACCL(glXGetProcAddress, procName);
  return res;
}


REDEF(fptr,
glXGetProcAddressARB, const GLubyte *, procName)
{
  fptr res = _kroki_glxoffload_get_proc_address((const char *) procName);
  if (! res)
    res = ACCL(glXGetProcAddressARB, procName);
  return res;
}


static char *vendor = NULL;
static char *version = NULL;

static
const char *
get_client_string(char **pv, int name, const char *via);


REDEF(const char *,
glXGetClientString, Display *, dpy, int, name)
{
  UNUSED(dpy);

  switch (name)
    {
    case GLX_VENDOR:
      return get_client_string(&vendor, name, PACKAGE_NAME);

    case GLX_VERSION:
      return get_client_string(&version, name, PACKAGE_VERSION);

    case GLX_EXTENSIONS:
      return ACCL(glXGetClientString, accl_dpy, name);

    default:
      return NULL;
    }
}


#if (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 7))

#define __ATOMIC_ACQUIRE
#define __ATOMIC_RELEASE
#define __atomic_load_n(pv, mm)  *(volatile __typeof__(*(pv)) *) (pv)
#define __atomic_compare_exchange_n(pv, opv, nv, w, tmm, fmm)   \
  ({                                                            \
    __typeof__(*(opv)) _kroki_o = *(opv);                       \
    *(opv) = __sync_val_compare_and_swap(pv, _kroki_o, nv);     \
    (*(opv) == _kroki_o);                                       \
  })

#endif


static
const char *
get_client_string(char **pv, int name, const char *via)
{
  const char *v = __atomic_load_n(pv, __ATOMIC_ACQUIRE);
  if (unlikely(! v))
    {
      char *nv;
      SYS(asprintf(&nv, "%s (via %s)",
                   ACCL(glXGetClientString, accl_dpy, name), via));
      if (__atomic_compare_exchange_n(pv, &v, nv, 0,
                                      __ATOMIC_RELEASE, __ATOMIC_ACQUIRE))
        v = nv;
      else
        free(nv);
    }

  return v;
}


static
GLXFBConfig
get_dspl_config(Display *dpy, GLXFBConfig config);


REDEF(int,
glXGetFBConfigAttrib, Display *, dpy, GLXFBConfig, config,
      int, attribute, int *, value)
{
  switch (attribute)
    {
    case GLX_VISUAL_ID:
    case GLX_X_VISUAL_TYPE:
      {
        GLXFBConfig dspl_config = get_dspl_config(dpy, config);
        return DSPL(glXGetFBConfigAttrib, dpy, dspl_config, attribute, value);
      }

    default:
      {
        return ACCL(glXGetFBConfigAttrib, accl_dpy, config, attribute, value);
      }
    }
}


REDEF(GLXFBConfig *,
glXChooseFBConfig, Display *, dpy, int, screen,
      const int *, attrib_list, int *, nelements)
{
  UNUSED(dpy && screen);
  return ACCL(glXChooseFBConfig, accl_dpy, DefaultScreen(accl_dpy),
              attrib_list, nelements);
}


static
GLXFBConfig
get_dspl_config(Display *dpy, GLXFBConfig config)
{
  int attrib_list[] = {
    GLX_DOUBLEBUFFER, True,
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_ACCUM_RED_SIZE, 0,
    GLX_ACCUM_GREEN_SIZE, 0,
    GLX_ACCUM_BLUE_SIZE, 0,
    GLX_ACCUM_ALPHA_SIZE, 0,
    GLX_AUX_BUFFERS, 0,
    GLX_DEPTH_SIZE, 0,
    GLX_STENCIL_SIZE, 0,
    GLX_SAMPLE_BUFFERS, 0,
    GLX_SAMPLES, 0,

    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RED_SIZE, 0,
    GLX_GREEN_SIZE, 0,
    GLX_BLUE_SIZE, 0,
    GLX_ALPHA_SIZE, 0,
    GLX_LEVEL, 0,
    GLX_STEREO, False,
    None
  };

  for (int i = 11 * 2; attrib_list[i] != None; i += 2)
    {
      int err;
      CHECK(err = ACCL(glXGetFBConfigAttrib, accl_dpy, config,
                       attrib_list[i], &attrib_list[i + 1]),
            != 0, die, "failed with error 0x%x", err);
    }

  if (attrib_list[23] == GLX_PBUFFER_BIT)
    {
      for (int i = 0; i < 8; i += 2)
        {
          if (attrib_list[25 + i] > 8)
            attrib_list[25 + i] = 8;
        }
    }

  int nelements;
  GLXFBConfig *dspl_configs =
    MEM(DSPL(glXChooseFBConfig, dpy, DefaultScreen(dpy),
             attrib_list, &nelements));
  GLXFBConfig fbconfig = dspl_configs[0];
  XFree(dspl_configs);

  return fbconfig;
}


DSPL_DPY(int,
glXGetConfig, Display *, dpy, XVisualInfo *, vis, int, attrib, int *, value);


static
GLXFBConfig
get_accl_config(Display *dpy, XVisualInfo *vis)
{
  int attrib_list[] = {
    GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
    GLX_DOUBLEBUFFER, True,
    GLX_RENDER_TYPE, GLX_RGBA_BIT,

    GLX_RED_SIZE, 0,
    GLX_GREEN_SIZE, 0,
    GLX_BLUE_SIZE, 0,
    GLX_ALPHA_SIZE, 0,
    GLX_DEPTH_SIZE, 0,
    GLX_STENCIL_SIZE, 0,
    GLX_AUX_BUFFERS, 0,
    GLX_ACCUM_RED_SIZE, 0,
    GLX_ACCUM_GREEN_SIZE, 0,
    GLX_ACCUM_BLUE_SIZE, 0,
    GLX_ACCUM_ALPHA_SIZE, 0,
    GLX_SAMPLE_BUFFERS, 0,
    GLX_SAMPLES, 0,
    GLX_LEVEL, 0,
    GLX_STEREO, False,
    None
  };

  for (int i = 3 * 2; attrib_list[i] != None; i += 2)
    {
      int err;
      CHECK(err = DSPL(glXGetConfig, dpy, vis,
                       attrib_list[i], &attrib_list[i + 1]),
            != 0, die, "failed with error 0x%x", err);
    }

  int nelements;
  GLXFBConfig *configs =
    MEM(ACCL(glXChooseFBConfig, accl_dpy, DefaultScreen(accl_dpy),
             attrib_list, &nelements));
  GLXFBConfig fbconfig = configs[0];
  XFree(configs);

  return fbconfig;
}


REDEF(XVisualInfo *,
glXGetVisualFromFBConfig, Display *, dpy, GLXFBConfig, config)
{
  GLXFBConfig dspl_config = get_dspl_config(dpy, config);
  return DSPL(glXGetVisualFromFBConfig, dpy, dspl_config);
}


static
void
get_dimensions(Display *dpy, Drawable xdrw,
               GLsizei *pwidth, GLsizei *pheight)
{
  Window root;
  int x, y;
  unsigned int width, height, border_width, depth;
  XGetGeometry(dpy, xdrw, &root, &x, &y,
               &width, &height, &border_width, &depth);
  *pwidth = width;
  *pheight = height;
}


REDEF(GLXPbuffer,
glXCreatePbuffer, Display *, dpy, GLXFBConfig, config,
      const int *, attrib_list)
{
  /*
    Pbuffer on DSPL side is never used so we remember requested
    dimensions but create a Pbuffer of 1x1 size.
  */
  int i = 0;
  int wi = -1;
  int hi = -1;
  if (attrib_list)
    {
      for (; attrib_list[i] != None; i += 2)
        {
          if (attrib_list[i] == GLX_PBUFFER_WIDTH)
            wi = i + 1;
          else if (attrib_list[i] == GLX_PBUFFER_HEIGHT)
            hi = i + 1;
        }
    }
  int al[i + 1];
  if (attrib_list)
    {
      memcpy(al, attrib_list, sizeof(int) * i);
      if (wi != -1)
        al[wi] = 1;
      if (hi != -1)
        al[hi] = 1;
    }
  al[i] = None;

  GLXFBConfig dspl_config = get_dspl_config(dpy, config);
  GLXPbuffer res = DSPL(glXCreatePbuffer, dpy, dspl_config, al);
  if (res)
    {
      struct drw_info *di = drw_info_create(dpy, res, config, None);
      if (wi != -1)
        di->width = attrib_list[wi];
      if (hi != -1)
        di->height = attrib_list[hi];
    }
  return res;
}


static
struct drw_info *
lookup_with_pbuffer(Display *dpy, GLXDrawable draw, struct ctx_info *ci)
{
  struct drw_info *di = drw_info_lookup(dpy, draw);
  if (! di)
    {
      /*
        This is a drawable object other than GLXPbuffer, GLXPixmap and
        GLXWindow (plain Window or Pixmap) so we didn't intercept its
        creation and haven't assign drw_info to it.
      */
      di = drw_info_create(dpy, draw, ci->accl_config, draw);
    }
  if (! di->accl_pbuffer)
    {
      if (di->xdrw)
        get_dimensions(dpy, di->xdrw, &di->width, &di->height);
      int attrib_list[] = {
        GLX_PBUFFER_WIDTH, di->width,
        GLX_PBUFFER_HEIGHT, di->height,
        GLX_PRESERVED_CONTENTS, True,
        None
      };
      di->accl_pbuffer =
        ACCL(glXCreatePbuffer, accl_dpy, di->accl_config, attrib_list);
    }
  return di;
}


REDEF(void,
glXQueryDrawable, Display *, dpy, GLXDrawable, draw, int, attribute,
      unsigned int *, value)
{
  struct drw_info *di = lookup_with_pbuffer(dpy, draw, NULL);
  ACCL(glXQueryDrawable, accl_dpy, di->accl_pbuffer, attribute, value);
}


REDEF(int,
XDestroyWindow, Display *, dpy, Window, w)
{
  drw_info_destroy(dpy, w);
  return DSPL(XDestroyWindow, dpy, w);
}


REDEF(int,
XFreePixmap, Display *, dpy, Pixmap, pixmap)
{
  drw_info_destroy(dpy, pixmap);
  return DSPL(XFreePixmap, dpy, pixmap);
}


REDEF(Bool,
glXMakeContextCurrent, Display *, dpy, GLXDrawable, draw, GLXDrawable, read,
      GLXContext, ctx)
{
  GLXContext dspl_ctx;
  GLXDrawable accl_pbuffer;
  GLXDrawable read_accl_pbuffer;
  if (likely(draw && read && ctx))
    {
      struct ctx_info *ci = ctx_info_lookup(ctx);
      struct drw_info *di = lookup_with_pbuffer(dpy, draw, ci);
      struct drw_info *read_di = lookup_with_pbuffer(dpy, read, ci);
      dspl_ctx = ci->dspl_ctx;
      accl_pbuffer = di->accl_pbuffer;
      read_accl_pbuffer = read_di->accl_pbuffer;
    }
  else
    {
      dspl_ctx = NULL;
      accl_pbuffer = None;
      read_accl_pbuffer = None;
    }

  Bool res = ACCL(glXMakeContextCurrent, accl_dpy,
                  accl_pbuffer, read_accl_pbuffer, ctx);
  if (res)
    DSPL(glXMakeContextCurrent, dpy, draw, read, dspl_ctx);
  return res;
}


REDEF(Bool,
glXMakeCurrentReadSGI, Display *, dpy, GLXDrawable, draw, GLXDrawable, read,
      GLXContext, ctx)
{
  return glXMakeContextCurrent(dpy, draw, read, ctx);
}



REDEF(Bool,
glXMakeCurrent, Display *, dpy, GLXDrawable, draw, GLXContext, ctx)
{
  GLXContext dspl_ctx;
  GLXDrawable accl_pbuffer;
  if (likely(draw && ctx))
    {
      struct ctx_info *ci = ctx_info_lookup(ctx);
      struct drw_info *di = lookup_with_pbuffer(dpy, draw, ci);
      dspl_ctx = ci->dspl_ctx;
      accl_pbuffer = di->accl_pbuffer;
    }
  else
    {
      dspl_ctx = NULL;
      accl_pbuffer = None;
    }

  Bool res = ACCL(glXMakeCurrent, accl_dpy, accl_pbuffer, ctx);
  if (res)
    DSPL(glXMakeCurrent, dpy, draw, dspl_ctx);
  return res;
}


ACCL_DPY(Bool,
glXIsDirect, Display *, dpy, GLXContext, ctx);


REDEF(GLXContext,
glXCreateNewContext, Display *, dpy, GLXFBConfig, config,
      int, render_type, GLXContext, share_list,
      Bool, direct)
{
  UNUSED(render_type);
  GLXContext res = ACCL(glXCreateNewContext, accl_dpy, config,
                        GLX_RGBA_TYPE, share_list, direct);
  if (res)
    {
      GLXFBConfig dspl_config = get_dspl_config(dpy, config);
      GLXContext dspl_ctx = MEM(DSPL(glXCreateNewContext, dpy, dspl_config,
                                     GLX_RGBA_TYPE, NULL, True));
      if (DSPL(glXIsDirect, dpy, dspl_ctx) != True)
        error("connection to %s is not direct", getenv("DISPLAY"));
      ctx_info_create(res, config, dspl_ctx);
    }
  return res;
}


REDEF(GLXContext,
glXCreateContext, Display *, dpy, XVisualInfo *, vis,
      GLXContext, share_list, Bool, direct)
{
  GLXFBConfig accl_config = get_accl_config(dpy, vis);
  GLXContext res = ACCL(glXCreateNewContext, accl_dpy, accl_config,
                        GLX_RGBA_TYPE, share_list, direct);
  if (res)
    {
      GLXContext dspl_ctx = MEM(DSPL(glXCreateContext, dpy, vis, NULL, True));
      if (DSPL(glXIsDirect, dpy, dspl_ctx) != True)
        error("connection to %s is not direct", getenv("DISPLAY"));
      ctx_info_create(res, accl_config, dspl_ctx);
    }
  return res;
}


REDEF(GLXContext,
glXCreateContextAttribsARB, Display *, dpy, GLXFBConfig, config,
      GLXContext, share_list, Bool, direct, const int *, attrib_list)
{
  // Color index rendering is obsolete so let's pass attrib_list as-is.
  GLXFBConfig dspl_config = get_dspl_config(dpy, config);
  GLXContext dspl_ctx = DSPL(glXCreateContextAttribsARB, dpy,
                             dspl_config, NULL, True, attrib_list);
  if (! dspl_ctx)
    return NULL;
  if (DSPL(glXIsDirect, dpy, dspl_ctx) != True)
    error("connection to %s is not direct", getenv("DISPLAY"));
  GLXContext res = MEM(ACCL(glXCreateContextAttribsARB, accl_dpy, config,
                            share_list, direct, attrib_list));
  ctx_info_create(res, config, dspl_ctx);

  return res;
}


REDEF(void,
glXDestroyContext, Display *, dpy, GLXContext, ctx)
{
  struct ctx_info *ci = ctx_info_lookup(ctx);
  if (ci)
    {
      DSPL(glXDestroyContext, dpy, ci->dspl_ctx);
      ctx_info_destroy(ctx);
      ACCL(glXDestroyContext, accl_dpy, ctx);
    }
}


REDEF(Display *,
glXGetCurrentDisplay, void)
{
  return DSPL(glXGetCurrentDisplay);
}


REDEF(GLXDrawable,
glXGetCurrentDrawable, void)
{
  return DSPL(glXGetCurrentDrawable);
}


REDEF(GLXDrawable,
glXGetCurrentReadDrawable, void)
{
  return DSPL(glXGetCurrentReadDrawable);
}


REDEF(GLXDrawable,
glXGetCurrentReadDrawableSGI, void)
{
  return glXGetCurrentReadDrawable();
}


IMPORT(glGetError);
IMPORT(glViewport);
IMPORT(glDisable);
IMPORT(glDepthMask);
IMPORT(glPixelStorei);
IMPORT(glGenTextures);
IMPORT(glBindTexture);
IMPORT(glTexStorage2D);
IMPORT(glTexParameteri);
IMPORT(glVertexPointer);
IMPORT(glTexCoordPointer);
IMPORT(glEnableClientState);
IMPORT(glEnable);
IMPORT(glTexSubImage2D);
IMPORT(glDrawArrays);


static
void
process_frame(struct drw_info *di, bool read_rgb, bool draw_rgb)
{
  glBindBuffer(GL_PIXEL_PACK_BUFFER, di->accl_copy_pbuffers[di->frame_no % 2]);
  glReadPixels(0, 0, di->width, di->height,
               read_rgb ? GL_RGB : GL_BGRA,
               read_rgb ? GL_UNSIGNED_BYTE : GL_UNSIGNED_INT_8_8_8_8_REV,
               (GLvoid *) 0);

  ++di->frame_no;

  glBindBuffer(GL_PIXEL_PACK_BUFFER, di->accl_copy_pbuffers[di->frame_no % 2]);
  void *data = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
  DSPL(glTexSubImage2D, GL_TEXTURE_2D, 0, 0, 0, di->width, di->height,
       draw_rgb ? GL_RGB : GL_BGRA,
       draw_rgb ? GL_UNSIGNED_BYTE : GL_UNSIGNED_INT_8_8_8_8_REV, data);
  glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

  DSPL(glDrawArrays, GL_TRIANGLE_STRIP, 0, 4);
}


REDEF(void,
glXSwapBuffers, Display *, dpy, GLXDrawable, draw)
{
  struct drw_info *di = drw_info_lookup(dpy, draw);

  ACCL(glXSwapBuffers, accl_dpy, di->accl_pbuffer);

  /*
    If 'draw' is a (double buffered) Pbuffer then we have nothing to
    do because there's no point in updating Pbuffer on DSPL side.
  */
  if (!di->xdrw)
    return;

  GLenum accl_err = glGetError();
  GLenum dspl_err = DSPL(glGetError);

  GLint save_pixel_pack_buffer;
  glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &save_pixel_pack_buffer);

  GLsizei width;
  GLsizei height;
  if (di->frame_no % COPY_CONFIDENCE_MASK_DIVIDER == 0)
    get_dimensions(dpy, di->xdrw, &width, &height);
  if (di->frame_no % COPY_CONFIDENCE_MASK_DIVIDER == 0
      && unlikely(di->width != width || di->height != height))
    {
      di->width = width;
      di->height = height;

      ACCL(glXDestroyPbuffer, accl_dpy, di->accl_pbuffer);
      int attrib_list[] = {
        GLX_PBUFFER_WIDTH, di->width,
        GLX_PBUFFER_HEIGHT, di->height,
        GLX_PRESERVED_CONTENTS, True,
        None
      };
      di->accl_pbuffer =
        ACCL(glXCreatePbuffer, accl_dpy, di->accl_config, attrib_list);

      glDeleteBuffers(2, di->accl_copy_pbuffers);
      DSPL(glDeleteTextures, 1, &di->dspl_texture);
      di->accl_copy_pbuffers[0] = None;
      di->accl_copy_pbuffers[1] = None;
      di->dspl_texture = None;

      GLXContext ctx = glXGetCurrentContext();
      GLXDrawable read_drw = glXGetCurrentReadDrawable();
      struct drw_info *read_di = drw_info_lookup(dpy, read_drw);
      ACCL(glXMakeContextCurrent, accl_dpy,
           di->accl_pbuffer, read_di->accl_pbuffer, ctx);

      DSPL(glViewport, 0, 0, di->width, di->height);
    }

  if (unlikely(! di->dspl_texture))
    {
      GLsizei size = di->width * di->height * 4;

      glGenBuffers(2, di->accl_copy_pbuffers);
      glBindBuffer(GL_PIXEL_PACK_BUFFER, di->accl_copy_pbuffers[0]);
      glBufferData(GL_PIXEL_PACK_BUFFER, size, NULL, GL_STREAM_READ);
      glBindBuffer(GL_PIXEL_PACK_BUFFER, di->accl_copy_pbuffers[1]);
      glBufferData(GL_PIXEL_PACK_BUFFER, size, NULL, GL_STREAM_READ);

      DSPL(glDisable, GL_DEPTH_TEST);
      DSPL(glDepthMask, GL_FALSE);

      DSPL(glPixelStorei, GL_UNPACK_ALIGNMENT, 4);

      DSPL(glGenTextures, 1, &di->dspl_texture);
      DSPL(glBindTexture, GL_TEXTURE_2D, di->dspl_texture);
      DSPL(glTexStorage2D, GL_TEXTURE_2D, 1, GL_RGBA8, di->width, di->height);
      DSPL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      DSPL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      static const GLfloat vrect[] = {-1, -1,   1, -1,   -1,  1,   1,  1};
      static const GLfloat trect[] = { 0,  0,   1,  0,    0,  1,   1,  1};
      DSPL(glVertexPointer, 2, GL_FLOAT, 0, vrect);
      DSPL(glTexCoordPointer, 2, GL_FLOAT, 0, trect);
      DSPL(glEnableClientState, GL_VERTEX_ARRAY);
      DSPL(glEnableClientState, GL_TEXTURE_COORD_ARRAY);
      DSPL(glEnable, GL_TEXTURE_2D);

      glBindBuffer(GL_PIXEL_PACK_BUFFER, save_pixel_pack_buffer);

      GLenum err;
      CHECK(err = glGetError(),
            != accl_err, die, "failed with error 0x%x", err);
      CHECK(err = DSPL(glGetError),
            != dspl_err, die, "failed with error 0x%x", err);

      return;
    }

  GLint save_read_buffer;
  glGetIntegerv(GL_READ_BUFFER, &save_read_buffer);
  if ((GLuint) save_read_buffer != GL_FRONT)
    glReadBuffer(GL_FRONT);
  glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
  glPixelStorei(GL_PACK_ALIGNMENT, 4);
  glPixelStorei(GL_PACK_SWAP_BYTES, GL_FALSE);
  glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_PACK_SKIP_ROWS, 0);

  if (copy_method != AUTO || likely(di->frame_no & di->copy_confidence_mask))
    {
      process_frame(di, di->copy_rgb, di->copy_rgb);
    }
  else
    {
      /*
        Here we re-evaluate the performance of both copy methods.
        COPY_CONFIDENCE_MASK_DIVIDER is a power of two and is at least
        four.  We will get to this branch for a
        COPY_CONFIDENCE_MASK_DIVIDER consecutive frames because
        di->copy_confidence_mask has log(COPY_CONFIDENCE_MASK_DIVIDER)
        lower zero bits.  The algorithm consists of four phases:

        1. On the first pass we use current copy method for both
           reading of the current frame and drawing of the previous
           frame.  On this pass we store execution time multiplied by
           (COPY_CONFIDENCE_MASK_DIVIDER - 3) to di->copy_nsec.

        2. On the second pass we use other method for reading, while
           still current method for drawing because we draw the frame
           that we read on the previous pass.  We also swap current
           and other methods for the next phase.

        3. Third phase consists of (COPY_CONFIDENCE_MASK_DIVIDER - 3)
           passes.  On each pass we use current (previously other)
           method for both reading and drawing and each time subtract
           execution time from di->copy_nsec.

        4. On the last pass we evaluate the value of di->copy_nsec.
           Negative or zero means that other method wasn't any faster,
           so we increase confidence value (set higher bit in
           di->copy_confidence_mask which will result in twice less
           frequent re-evaluation) and restore copy method back to
           what it was before re-evaluation.

           On the other hand positive di->copy_nsec means that the
           other method was faster.  In this case we shift down the
           confidence vale if it is non-zero (thus scheduling next
           re-evaluation twice sooner) but again restore the original
           copy method.  Alternatively, when di->copy_confidence_mask
           has already dropped to zero (no confidence) we do not
           restore previous copy method and continue to use the other
           one (leaving it as current in di->copy_rgb flag).

        Note that we measure time on phase 1 only once, but several
        times on phase 3.  This is because the original copy method
        codepath is hot and we have confidence in it thus time
        measurement will be quite precise and we also can tolerate the
        error.  On the other hand the other copy method codepath is
        cold so we have to warm it up.

        Be aware that automatic copy method selection is not perfect:
        if CPU uses on-demand frequency scaling than a less efficient
        method may trigger higher CPU speed and thus will appear to
        perform better during re-evaluation, though frequency
        switching latencies and other CPU stalls will still result in
        a lower FPS rate on a long run.
      */
      bool read_rgb = di->copy_rgb;
      bool draw_rgb = di->copy_rgb;
      if (di->frame_no % COPY_CONFIDENCE_MASK_DIVIDER == 1)
        {
          read_rgb = ! read_rgb;
          di->copy_rgb = ! di->copy_rgb;
        }
      else if (di->frame_no % COPY_CONFIDENCE_MASK_DIVIDER
               == COPY_CONFIDENCE_MASK_DIVIDER - 1)
        {
          if (likely(di->copy_nsec <= 0))
            {
              /*
                Set re-evaluation rate to no less than 0.78% (1/2^(6+1)).
              */
              if (di->copy_confidence_mask
                  < (COPY_CONFIDENCE_MASK_DIVIDER << 6))
                {
                  di->copy_confidence_mask <<= 1;
                  di->copy_confidence_mask += COPY_CONFIDENCE_MASK_DIVIDER;
                }
              di->copy_rgb = ! di->copy_rgb;
              read_rgb = ! read_rgb;
            }
          else if (likely(di->copy_confidence_mask))
            {
              di->copy_confidence_mask -= COPY_CONFIDENCE_MASK_DIVIDER;
              di->copy_confidence_mask >>= 1;
              di->copy_rgb = ! di->copy_rgb;
              read_rgb = ! read_rgb;
            }
          else
            {
              if (verbose)
                {
                  warn(PACKAGE_NAME ": since frame %u (%ux%u)"
                       " using %s copy method", di->frame_no + 1,
                       (unsigned int) di->width, (unsigned int) di->height,
                       (di->copy_rgb ? "RGB" : "BGRA"));
                }
            }
        }

      struct timespec beg, end;
      SYS(clock_gettime(CLOCK_MONOTONIC, &beg));
      process_frame(di, read_rgb, draw_rgb);
      SYS(clock_gettime(CLOCK_MONOTONIC, &end));
      /*
        We can tolerate timing error anyway so let's assume that
        glTexSubImage2D() always takes less then a second (it should
        be that fast anyway).
      */
      int nsec = end.tv_nsec - beg.tv_nsec;
      if (nsec < 0)
        nsec = -nsec;

      /*
        Note that process_frame() has incremented di->frame_no.
      */
      if (di->frame_no % COPY_CONFIDENCE_MASK_DIVIDER == 1)
        di->copy_nsec = (COPY_CONFIDENCE_MASK_DIVIDER - 3) * nsec;
      else if (di->frame_no % COPY_CONFIDENCE_MASK_DIVIDER > 2)
        di->copy_nsec -= nsec;
    }

  DSPL(glXSwapBuffers, dpy, draw);

  glPopClientAttrib();
  if ((GLuint) save_read_buffer != GL_FRONT)
    glReadBuffer(save_read_buffer);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, save_pixel_pack_buffer);

  GLenum err;
  CHECK(err = glGetError(),
        != accl_err, die, "failed with error 0x%x", err);
  CHECK(err = DSPL(glGetError),
        != dspl_err, die, "failed with error 0x%x", err);
}


REDEF(GLXFBConfig *,
glXGetFBConfigs, Display *, dpy, int, screen, int *, nelements)
{
  UNUSED(dpy && screen);
  return ACCL(glXGetFBConfigs, accl_dpy, DefaultScreen(accl_dpy), nelements);
}


enum extension_requires { NONE = 0x0, ACCL = 0x1, DSPL = 0x2 };

struct extension_info
{
  const char *name;
  enum extension_requires requires;
};

static const struct extension_info extension_supported[] = {
  /* glXCreateContextAttribsARB */
  { " GLX_ARB_create_context ", ACCL | DSPL },

  { " GLX_ARB_create_context_profile ", ACCL | DSPL },

  { " GLX_ARB_create_context_robustness ", ACCL | DSPL },

  { " GLX_ARB_fbconfig_float ", ACCL },

  { " GLX_ARB_framebuffer_sRGB ", ACCL },

  /* glXGetProcAddressARB */
  { " GLX_ARB_get_proc_address ", ACCL },

  { " GLX_ARB_multisample ", ACCL },

  { " GLX_EXT_buffer_age ", ACCL },

  { " GLX_EXT_create_context_es2_profile ", ACCL | DSPL },

  { " GLX_EXT_create_context_es_profile ", ACCL | DSPL },

  { " GLX_EXT_fbconfig_packed_float ", ACCL },

  { " GLX_EXT_framebuffer_sRGB ", ACCL },

#if 0
  /* glXFreeContextEXT, glXGetContextIDEXT, glXGetCurrentDisplayEXT,
     glXImportContextEXT, glXQueryContextInfoEXT */
  { " GLX_EXT_import_context ", ? },
#endif

  /* glXSwapIntervalEXT */
  { " GLX_EXT_swap_control ", DSPL },

  { " GLX_EXT_swap_control_tear ", DSPL },

#if 0
  /* glXBindTexImageEXT, glXReleaseTexImageEXT */
  { " GLX_EXT_texture_from_pixmap ", ? },
#endif

  { " GLX_EXT_visual_info ", DSPL },

  { " GLX_EXT_visual_rating ", DSPL },

  { " GLX_INTEL_swap_event ", DSPL },

#if 0
  /* glXGetAGPOffsetMESA */
  { " GLX_MESA_agp_offset ", ? },
#endif

  /* glXCopySubBufferMESA */
  { " GLX_MESA_copy_sub_buffer ", DSPL },

#if 0
  /* glXCreateGLXPixmapMESA */
  { " GLX_MESA_pixmap_colormap ", ? },
#endif

  /* glXReleaseBuffersMESA */
  { " GLX_MESA_release_buffers ", DSPL },

#if 0
  /* glXSet3DfxModeMESA */
  { " GLX_MESA_set_3dfx_mode ", ? },
#endif

  /* glXCopyImageSubDataNV */
  { " GLX_NV_copy_image ", ACCL },

  { " GLX_NV_float_buffer ", ACCL },

  { " GLX_NV_multisample_coverage ", ACCL },

#if 0
  /* glXBindVideoDeviceNV, glXEnumerateVideoDevicesNV */
  { " GLX_NV_present_video ", ? },
#endif

  /* glXBindSwapBarrierNV, glXJoinSwapGroupNV, glXQueryFrameCountNV,
     glXQueryMaxSwapGroupsNV, glXQuerySwapGroupNV, glXResetFrameCountNV */
  { " GLX_NV_swap_group ", DSPL },

#if 0
  /* glXBindVideoCaptureDeviceNV, glXEnumerateVideoCaptureDevicesNV,
     glXLockVideoCaptureDeviceNV, glXQueryVideoCaptureDeviceNV,
     glXReleaseVideoCaptureDeviceNV */
  { " GLX_NV_video_capture ", ? },
#endif

#if 0
  /* glXBindVideoImageNV, glXGetVideoDeviceNV, glXGetVideoInfoNV,
     glXReleaseVideoDeviceNV, glXReleaseVideoImageNV,
     glXSendPbufferToVideoNV */
  { " GLX_NV_video_output ", ? },
#endif

  { " GLX_OML_swap_method ", ACCL },

#if 0
  /* glXGetMscRateOML, glXGetSyncValuesOML, glXSwapBuffersMscOML,
     glXWaitForMscOML, glXWaitForSbcOML */
  { " GLX_OML_sync_control ", ? },
#endif

  { " GLX_SGIS_multisample ", ACCL },

#if 0
  { " GLX_SGIX_dmbuffer ", ? },
#endif

#if 0
  /* glXChooseFBConfigSGIX, glXCreateContextWithConfigSGIX,
     glXCreateGLXPixmapWithConfigSGIX, glXGetFBConfigAttribSGIX,
     glXGetFBConfigFromVisualSGIX, glXGetVisualFromFBConfigSGIX */
  { " GLX_SGIX_fbconfig ", ? },
#endif

#if 0
  /* glXQueryHyperpipeNetworkSGIX, glXHyperpipeConfigSGIX,
     glXQueryHyperpipeConfigSGIX, glXDestroyHyperpipeConfigSGIX,
     glXBindHyperpipeSGIX, glXQueryHyperpipeBestAttribSGIX,
     glXHyperpipeAttribSGIX, glXQueryHyperpipeAttribSGIX */
  { " GLX_SGIX_hyperpipe ", ? },
#endif

#if 0
  /* glXCreateGLXPbufferSGIX, glXDestroyGLXPbufferSGIX,
     glXGetSelectedEventSGIX, glXQueryGLXPbufferSGIX, glXSelectEventSGIX */
  { " GLX_SGIX_pbuffer ", ? },
#endif

  /* glXBindSwapBarrierSGIX, glXQueryMaxSwapBarriersSGIX */
  { " GLX_SGIX_swap_barrier ", DSPL },

  /* glXJoinSwapGroupSGIX */
  { " GLX_SGIX_swap_group ", DSPL },

#if 0
  /* glXBindChannelToWindowSGIX, glXChannelRectSGIX,
     glXChannelRectSyncSGIX, glXQueryChannelDeltasSGIX,
     glXQueryChannelRectSGIX */
  { " GLX_SGIX_video_resize ", ? },
#endif

#if 0
  { " GLX_SGIX_video_source ", ? },
#endif

  { " GLX_SGIX_visual_select_group ", ACCL },

  /* glXCushionSGI */
  { " GLX_SGI_cushion ", DSPL },

  /* glXGetCurrentReadDrawableSGI, glXMakeCurrentReadSGI */
  { " GLX_SGI_make_current_read ", NONE },

  /* glXSwapIntervalSGI */
  { " GLX_SGI_swap_control ", DSPL },

  /* glXGetVideoSyncSGI, glXWaitVideoSyncSGI */
  { " GLX_SGI_video_sync ", DSPL },

#if 0
  /* glXGetTransparentIndexSUN */
  { " GLX_SUN_get_transparent_index ", ? },
#endif
};


static inline
void
bitmap_set(unsigned char *bitmap, size_t i)
{
  bitmap[i / 8] |= 1 << (i % 8);
}


static inline
int
bitmap_get(const unsigned char *bitmap, size_t i)
{
  return (bitmap[i / 8] >> (i % 8)) & 1;
}


static
void
set_unsupported(unsigned char *bitmap, const char *extensions,
                enum extension_requires set)
{
  size_t len = strlen(extensions);
  char *ext = MEM(malloc(1 + len + 1 + 1));
  ext[0] = ' ';
  strcpy(ext + 1, extensions);
  ext[len + 1] = ' ';
  ext[len + 2] = '\0';

  for (size_t i = 0;
       i < sizeof(extension_supported) / sizeof(*extension_supported);
       ++i)
    {
      if (! bitmap_get(bitmap, i)
          && (extension_supported[i].requires & set)
          && ! strstr(ext, extension_supported[i].name))
        bitmap_set(bitmap, i);
    }

  free(ext);
}


static char *extensions = NULL;


REDEF(const char *,
glXQueryExtensionsString, Display *, dpy, int, screen)
{
  const char *v = __atomic_load_n(&extensions, __ATOMIC_ACQUIRE);
  if (unlikely(! v))
    {
      size_t bits = sizeof(extension_supported) / sizeof(*extension_supported);
      unsigned char *bitmap = MEM(calloc(1, (bits + 7) / 8));
      set_unsupported(bitmap, ACCL(glXQueryExtensionsString,
                                   accl_dpy, DefaultScreen(accl_dpy)), ACCL);
      set_unsupported(bitmap, DSPL(glXQueryExtensionsString,
                                   dpy, screen), DSPL);

      char *nv = MEM(malloc(1));
      size_t len = 0;
      for (size_t i = 0;
           i < sizeof(extension_supported) / sizeof(*extension_supported);
           ++i)
        {
          if (! bitmap_get(bitmap, i))
            {
              size_t l = strlen(extension_supported[i].name + 1);
              nv = MEM(realloc(nv, len + l));
              memcpy(nv + len, extension_supported[i].name + 1, l);
              len += l;
            }
        }
      nv[len] = '\0';

      free(bitmap);

      if (__atomic_compare_exchange_n(&extensions, &v, nv, 0,
                                      __ATOMIC_RELEASE, __ATOMIC_ACQUIRE))
        v = nv;
      else
        free(nv);
    }
  return v;
}


REDEF(const char *,
glXQueryServerString, Display *, dpy, int, screen, int, name)
{
  UNUSED(dpy && screen);
  return ACCL(glXQueryServerString,
              accl_dpy, DefaultScreen(accl_dpy), name);
}


REDEF(GLXPixmap,
glXCreateGLXPixmap, Display *, dpy, XVisualInfo *, vis,
      Pixmap, pixmap)
{
  GLXPixmap res = DSPL(glXCreateGLXPixmap, dpy, vis, pixmap);
  if (res)
    drw_info_create(dpy, res, get_accl_config(dpy, vis), pixmap);
  return res;
}


REDEF(void,
glXDestroyGLXPixmap, Display *, dpy, GLXPixmap, pixmap)
{
  drw_info_destroy(dpy, pixmap);
  DSPL(glXDestroyGLXPixmap, dpy, pixmap);
}


REDEF(GLXPixmap,
glXCreatePixmap, Display *, dpy, GLXFBConfig, config,
      Pixmap, pixmap, const int *, attrib_list)
{
  GLXFBConfig dspl_config = get_dspl_config(dpy, config);
  GLXPixmap res = DSPL(glXCreatePixmap, dpy, dspl_config, pixmap, attrib_list);
  if (res)
    drw_info_create(dpy, res, config, pixmap);
  return res;
}


REDEF(void,
glXDestroyPixmap, Display *, dpy, GLXPixmap, pixmap)
{
  drw_info_destroy(dpy, pixmap);
  DSPL(glXDestroyPixmap, dpy, pixmap);
}


REDEF(GLXWindow,
glXCreateWindow, Display *, dpy, GLXFBConfig, config,
      Window, win, const int *, attrib_list)
{
  GLXFBConfig dspl_config = get_dspl_config(dpy, config);
  GLXWindow res = DSPL(glXCreateWindow, dpy, dspl_config, win, attrib_list);
  if (res)
    drw_info_create(dpy, res, config, win);
  return res;
}


REDEF(void,
glXDestroyWindow, Display *, dpy, GLXWindow, win)
{
  drw_info_destroy(dpy, win);
  DSPL(glXDestroyWindow, dpy, win);
}


REDEF(void,
glXWaitGL, void)
{
  glFinish();
}


IMPORT(glXCopyImageSubDataNV);


ACCL_DPY(void,
glXCopyContext, Display *, dpy, GLXContext, src, GLXContext, dst,
      unsigned long, mask);


ACCL_DPY(Bool,
glXQueryExtension, Display *, dpy, int *, errorBase, int *, eventBase);


ACCL_DPY(Bool,
glXQueryVersion, Display *, dpy, int *, major, int *, minor);


ACCL_DPY(int,
glXQueryContext, Display *, dpy, GLXContext, ctx,
      int, attribute, int *, value);


ACCL_DPY(int,
glXQueryContextInfoEXT, Display *, dpy, GLXContext, ctx,
      int, attribute, int *, value);


DSPL_DPY(void,
glXGetSelectedEvent, Display *, dpy, GLXDrawable, draw,
         unsigned long *, event_mask);


DSPL_DPY(void,
glXSelectEvent, Display *, dpy, GLXDrawable, draw,
         unsigned long, event_mask);


DSPL_DPY(XVisualInfo *,
glXChooseVisual, Display *, dpy, int, screen, int *, attribList);


DSPL_DPY(void,
glXWaitX, void);


DSPL_DPY(void,
glXSwapIntervalEXT, Display *, dpy, GLXDrawable, drawable, int, interval);


DSPL_DPY(void,
glXCopySubBufferMESA, Display *, dpy, GLXDrawable, drawable,
         int, x, int, y, int, width, int, height);


DSPL_DPY(Bool,
glXReleaseBuffersMESA, Display *, dpy, GLXDrawable, draw);


DSPL_DPY(Bool,
glXJoinSwapGroupNV, Display *, dpy, GLXDrawable, drawable, GLuint, group);


DSPL_DPY(Bool,
glXBindSwapBarrierNV, Display *, dpy, GLuint, group, GLuint, barrier);


DSPL_DPY(Bool,
glXQuerySwapGroupNV, Display *, dpy, GLXDrawable,
         drawable, GLuint *, group, GLuint *, barrier);

DSPL_DPY(Bool,
glXQueryMaxSwapGroupsNV, Display *, dpy, int, screen,
         GLuint *, maxGroups, GLuint *, maxBarriers);


DSPL_DPY(Bool,
glXQueryFrameCountNV, Display *, dpy, int, screen, GLuint *, count);


DSPL_DPY(Bool,
glXResetFrameCountNV, Display *, dpy, int, screen);


DSPL_DPY(void,
BindSwapBarrierSGIX, Display *, dpy, GLXDrawable, drawable, int, barrier);


DSPL_DPY(Bool,
QueryMaxSwapBarriersSGIX, Display *, dpy, int, screen, int *, max);


DSPL_DPY(void,
JoinSwapGroupSGIX, Display *, dpy, GLXDrawable, drawable, GLXDrawable, member);


DSPL_DPY(void,
glXCushionSGI, Display *, dpy, Window, window, float, cushion);


DSPL_DPY(int,
glXSwapIntervalSGI, int, interval);


DSPL_DPY(int,
glXGetVideoSyncSGI, uint *, count);


DSPL_DPY(int,
glXWaitVideoSyncSGI, int, divisor, int, remainder, unsigned int *, count);


struct fnt_info_key
{
  Display *dpy;
  Font dspl_font;
};


struct fnt_info
{
  struct fnt_info_key key;
  char *name;
  Font accl_font;
  unsigned long use_count;
};

static pthread_mutex_t fnt_infos_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct cuckoo_hash fnt_infos;


static
struct fnt_info *
fnt_info_create(Display *dpy, Font dspl_font, const char *name)
{
  struct fnt_info *res = MEM(malloc(sizeof(*res)));
  memset(&res->key, 0, sizeof(res->key));
  res->key.dpy = dpy;
  res->key.dspl_font = dspl_font;
  res->name = MEM(strdup(name));
  res->accl_font = None;
  res->use_count = 1;
  pthread_mutex_lock(&fnt_infos_mutex);
  struct cuckoo_hash_item *it =
    CHECK(cuckoo_hash_insert(&fnt_infos, &res->key, sizeof(res->key), res),
          == CUCKOO_HASH_FAILED, die, "%m");
  if (it)
    {
      free(res);
      res = it->value;
      ++res->use_count;
    }
  pthread_mutex_unlock(&fnt_infos_mutex);
  return res;
}


static
struct fnt_info *
fnt_info_lookup(Display *dpy, Font dspl_font)
{
  struct fnt_info_key key;
  memset(&key, 0, sizeof(key));
  key.dpy = dpy;
  key.dspl_font = dspl_font;
  pthread_mutex_lock(&fnt_infos_mutex);
  struct cuckoo_hash_item *it =
    cuckoo_hash_lookup(&fnt_infos, &key, sizeof(key));
  pthread_mutex_unlock(&fnt_infos_mutex);
  return (it ? it->value : NULL);
}


static
void
fnt_info_destroy(Display *dpy, Font dspl_font);


REDEF(int,
XFreeFont, Display *, dpy, XFontStruct *, font_struct)
{
  fnt_info_destroy(dpy, font_struct->fid);
  return DSPL(XFreeFont, dpy, font_struct);
}


REDEF(int,
XUnloadFont, Display *, dpy, Font, font)
{
  fnt_info_destroy(dpy, font);
  return DSPL(XUnloadFont, dpy, font);
}


static
void
fnt_info_destroy(Display *dpy, Font dspl_font)
{
  struct fnt_info_key key;
  memset(&key, 0, sizeof(key));
  key.dpy = dpy;
  key.dspl_font = dspl_font;
  pthread_mutex_lock(&fnt_infos_mutex);
  struct cuckoo_hash_item *it =
    cuckoo_hash_lookup(&fnt_infos, &key, sizeof(key));
  if (it)
    {
      struct fnt_info *fi = it->value;
      if (--fi->use_count == 0)
        cuckoo_hash_remove(&fnt_infos, it);
      else
        it = NULL;
    }
  pthread_mutex_unlock(&fnt_infos_mutex);
  if (it)
    {
      struct fnt_info *fi = it->value;
      if (fi->accl_font)
        ACCL(XUnloadFont, accl_dpy, fi->accl_font);
      free(fi);
    }
}


REDEF(Font,
XLoadFont, Display *, dpy, const char *, name)
{
  Font res = DSPL(XLoadFont, dpy, name);
  if (res)
    fnt_info_create(dpy, res, name);
  return res;
}


REDEF(XFontStruct *,
XLoadQueryFont, Display *, dpy, const char *, name)
{
  XFontStruct *res = DSPL(XLoadQueryFont, dpy, name);
  if (res)
    fnt_info_create(dpy, res->fid, name);
  return res;
}


REDEF(void,
glXUseXFont, Font, font, int, first, int, count, int, listBase)
{
  struct fnt_info *fi = fnt_info_lookup(glXGetCurrentDisplay(), font);
  if (! fi->accl_font)
    fi->accl_font = ACCL(XLoadFont, accl_dpy, fi->name);
  ACCL(glXUseXFont, fi->accl_font, first, count, listBase);
}


static
void
process_options(void)
{
  char *options = CHECK(getenv("KROKI_GLXOFFLOAD_OPTIONS"),
                        == NULL, die, "not set");
  char key;
  char val[22];
  int consumed;
  while (sscanf(options, " %c=%21s%n", &key, val, &consumed) >= 2)
    {
      switch (key)
        {
        case 'c':
          {
            if (strcmp(val, "RGB") == 0)
              copy_method = RGB;
            else if (strcmp(val, "BGRA") == 0)
              copy_method = BGRA;
            else if (strcmp(val, "AUTO") == 0)
              copy_method = AUTO;
            else
              die("unknown copy method c=%s", val);
          }
          break;

        case 'd':
          {
            char *end;
            long res = strtol(val, &end, 10);
            if (*end != '\0' || res < 0 || res > 1)
              die("wrong verbose level d=%s", val);
            verbose = res;
          }
          break;

        default:
          die("unknown key=value '%c=%s'", key, val);
        }

      options += consumed;
    }
}


static __attribute__((__constructor__(1000)))
void
init0(void)
{
  CHECK(cuckoo_hash_init(&ctx_infos, 2), == false, die, "%m");
  CHECK(cuckoo_hash_init(&drw_infos, 2), == false, die, "%m");
  CHECK(cuckoo_hash_init(&fnt_infos, 2), == false, die, "%m");

  /*
    RTLD_DEEPBIND in dlopen() makes original KROKI_GLXOFFLOAD_DSPL_LIBGL
    see symbols of its own dependencies rather than our overrides.
  */
  dspl_libgl = CHECK(dlopen(getenv("KROKI_GLXOFFLOAD_DSPL_LIBGL"),
                            RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND),
                     == NULL, die, "%s", dlerror());

  accl_dpy = MEM(XOpenDisplay(getenv("KROKI_GLXOFFLOAD_DPY")),
                 " for %s", getenv("KROKI_GLXOFFLOAD_DPY"));

  process_options();
}


static __attribute__((__constructor__(1002)))
void
init2(void)
{
  /*
    Some functions may only be accessible by glXGetProcAddress().
    Let's fill addresses of those.
  */
  extern __attribute__((__visibility__("hidden")))
    struct redef_func __start__kroki_glxoffload,
                      __stop__kroki_glxoffload;
  for (struct redef_func *af = &__start__kroki_glxoffload;
       af != &__stop__kroki_glxoffload; ++af)
    {
      if (! af->accl_fp)
        {
          af->accl_fp = ACCL(glXGetProcAddress, (GLubyte *) af->name);
          if (! af->addr_fp)
            af->addr_fp = af->accl_fp;
        }
      if (! af->dspl_fp)
        af->dspl_fp = DSPL(glXGetProcAddress, (GLubyte *) af->name);
    }

  /*
    Trigger '_kroki_glxoffload_get_proc_address' symbol lookup after
    initializing redirection table (see kroki-glxoffload-audit.c).
  */
  _kroki_glxoffload_get_proc_address(NULL);
}


static __attribute__((__destructor__))
void
fini(void)
{
  XCloseDisplay(accl_dpy);

  dlclose(dspl_libgl);

  cuckoo_hash_destroy(&fnt_infos);
  cuckoo_hash_destroy(&drw_infos);
  cuckoo_hash_destroy(&ctx_infos);

  free(extensions);
  free(version);
  free(vendor);
}
