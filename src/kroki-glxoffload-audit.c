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
#include <link.h>
#include <string.h>

#ifdef NEED_USCORE
#define USCORE  "_"
#else
#define USCORE  ""
#endif


#define UNUSED(v)                               \
  do                                            \
    (void) (v);                                 \
  while (0)


unsigned int
la_version(unsigned int version)
{
  if (version > LAV_CURRENT)
    version = LAV_CURRENT;

  return version;
}


unsigned int
la_objopen(struct link_map *map, Lmid_t lmid, uintptr_t *cookie)
{
  UNUSED(lmid && cookie);
  if (strstr(map->l_name, "/libkroki-glxoffload.so.0"))
    return LA_FLG_BINDTO | LA_FLG_BINDFROM;
  else
    return 0;
}


static void *(*get_proc_address)(const char *procName) = NULL;


static
uintptr_t
symbind(uintptr_t sym_st_value, unsigned int ndx,
        uintptr_t *refcook, uintptr_t *defcook,
        unsigned int *flags, const char *symname)
{
  UNUSED(ndx && refcook && defcook && flags);
  /*
    LD_AUDIT module is in a separate linker namespace and can't access
    application symbols directly.  Hence we intercept the lookup of
    '_kroki_glxoffload_get_proc_address' which we trigger in libGL.c
    constructor after initializing redirection table.
  */
  if (unlikely(! get_proc_address
               && strcmp(symname,
                         USCORE "_kroki_glxoffload_get_proc_address") == 0))
    {
      get_proc_address = (void *) sym_st_value;
    }

  if (unlikely(get_proc_address
               && strncmp(symname, USCORE "glX", sizeof(USCORE) - 1 + 3) == 0
               && ! get_proc_address(symname)))
    {
      warn("WARNING: %s doesn't handle %s",
           PACKAGE_NAME, symname + sizeof(USCORE) - 1);
    }

  return sym_st_value;

}


uintptr_t
la_symbind32(Elf32_Sym *sym, unsigned int ndx,
             uintptr_t *refcook, uintptr_t *defcook,
             unsigned int *flags, const char *symname)
{
  return symbind(sym->st_value, ndx, refcook, defcook, flags, symname);
}


uintptr_t
la_symbind64(Elf64_Sym *sym, unsigned int ndx,
             uintptr_t *refcook, uintptr_t *defcook,
             unsigned int *flags, const char *symname)
{
  return symbind(sym->st_value, ndx, refcook, defcook, flags, symname);
}
