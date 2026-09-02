#ifndef _LLVMPIPE_INTERN_H
#define _LLVMPIPE_INTERN_H

/*
    Copyright 2010-2021, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdint.h>
#include <stdbool.h>

#if defined(__has_include)
#if __has_include("frontend/sw_winsys.h")
#define AROS_LLVMPIPE_HAS_FRONTEND_SW_WINSYS 1
#endif
#endif

#if defined(AROS_LLVMPIPE_HAS_FRONTEND_SW_WINSYS)
#include "frontend/sw_winsys.h"
#else
#include "state_tracker/sw_winsys.h"
#endif

#include LC_LIBDEFS_FILE

#define CLID_Hidd_Gallium_Llvmpipe  "hidd.gallium.llvmpipe"

// The object instance data is used as our winsys wrapper
struct HiddGalliumLlvmpipeData
{
    struct sw_winsys llvmpipe_winsys;
    OOP_Object *llvmpipe_obj;
};

struct llvmpipestaticdata 
{
    OOP_Class       *galliumclass;
    OOP_AttrBase    hiddGalliumAB;
    struct Library  *CyberGfxBase;
    struct Library  *UtilityBase;
};

LIBBASETYPE 
{
    struct Library              LibNode;
    struct llvmpipestaticdata   sd;
};

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define BASE(lib) ((LIBBASETYPEPTR)(lib))

#define SD(cl) (&BASE(cl->UserData)->sd)

#ifdef __cplusplus
extern "C" {
#endif
void Llvmpipe_ForceLLVMPipeRTTI(void);
#ifdef __cplusplus
}
#endif

#endif
