/*
 * AROS Mesa compatibility: <dlfcn.h> stand-in.
 *
 * AROS has no dynamic loader interface. Mesa's Vulkan runtime includes
 * "dlfcn.h" unconditionally on non-Windows hosts, but only calls into it
 * for the RenderDoc hook (HAVE_RENDERDOC_INTEGRATION), which is not built.
 * Found through the aros_compat -iquote path of the Mesa linklibs.
 */
#ifndef _AROS_MESA_DLFCN_H
#define _AROS_MESA_DLFCN_H

#include <stddef.h>

#define RTLD_LAZY   0x0001
#define RTLD_NOW    0x0002
#define RTLD_LOCAL  0x0000
#define RTLD_GLOBAL 0x0100
#define RTLD_NOLOAD 0x0004

static inline void *dlopen(const char *filename, int flags)
{
    (void)filename;
    (void)flags;
    return NULL;
}

static inline void *dlsym(void *handle, const char *symbol)
{
    (void)handle;
    (void)symbol;
    return NULL;
}

static inline int dlclose(void *handle)
{
    (void)handle;
    return 0;
}

static inline char *dlerror(void)
{
    return (char *)"dynamic loading is not available";
}

#endif /* _AROS_MESA_DLFCN_H */
