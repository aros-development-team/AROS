/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    nv_cubin without libelf. NVK only reads CUDA binaries for the
    NVX_binary_import extension, which it offers behind an experimental
    flag; without an ELF reader every module fails to load, and the
    extension stays off.
*/

#include "nv_cubin.h"

#include <stdlib.h>
#include <string.h>

bool
nv_cubin_module_init(struct nv_cubin_module *module, const void *data,
                     size_t size)
{
   (void)data;
   (void)size;
   memset(module, 0, sizeof(*module));
   return false;
}

void
nv_cubin_module_fini(struct nv_cubin_module *module)
{
   free(module->functions);
   memset(module, 0, sizeof(*module));
}

const struct nv_cubin_function *
nv_cubin_module_find_function(const struct nv_cubin_module *module,
                              const char *name)
{
   (void)module;
   (void)name;
   return NULL;
}

const struct nv_cubin_function_param_info *
nv_cubin_function_get_param_info(const struct nv_cubin_function *function,
                                 uint32_t index)
{
   (void)function;
   (void)index;
   return NULL;
}
