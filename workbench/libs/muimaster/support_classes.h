/*
    Copyright © 2002-2003, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUIMASTER_SUPPORT_CLASSES_H
#define _MUIMASTER_SUPPORT_CLASSES_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef INTUITION_CLASSUSR_H
#include <intuition/classusr.h>
#endif

#ifndef CLIB_MACROS_H
#include <clib/macros.h>
#endif

#ifdef __AROS__
#ifndef AROS_ASMCALL_H
#include <aros/asmcall.h>
#endif
#else
#ifndef _COMPILER_H
#include "compiler.h"
#endif
#endif

struct IClass *GetPublicClass(CONST_STRPTR className, struct Library *mb);
BOOL DestroyClasses(struct Library *MUIMasterBase);
struct IClass *CreateBuiltinClass(CONST_STRPTR className, struct Library *MUIMasterBase);

#ifdef __AROS__
AROS_UFP3(IPTR, metaDispatcher,
	AROS_UFPA(struct IClass  *, cl,  A0),
	AROS_UFPA(Object *, obj, A2),
	AROS_UFPA(Msg     , msg, A1));
#else
__asm ULONG metaDispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg);
#endif

#endif /* _MUIMASTER_SUPPORT_CLASSES_H */
