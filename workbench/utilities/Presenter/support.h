/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef SUPPORT_H
#define SUPPORT_H

#include <exec/types.h>
#include "intuition/classusr.h"
#include <clib/macros.h>

#ifdef __AROS__
#ifndef AROS_ASMCALL_H
#include <aros/asmcall.h>
#endif
#else
#ifndef _COMPILER_H
#include "compiler.h"
#endif
#endif

#define mui_alloc(x) AllocVec(x,MEMF_CLEAR)
#define mui_alloc_struct(x) ((x *)AllocVec(sizeof(x),MEMF_CLEAR))
#define mui_free(x) FreeVec(x)

ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...);

#ifndef __SASC
struct MUI_RenderInfo;
#endif

ULONG DoSetupMethod(Object *obj, struct MUI_RenderInfo *info);

#endif /* SUPPORT_H */
