#ifndef _MUIMASTER_SUPPORT_H
#define _MUIMASTER_SUPPORT_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef INTUITION_CLASSUSR_H
#include "intuition/classusr.h"
#endif

#ifdef _AROS
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

int isRegionWithinBounds(struct Region *r, int left, int top, int width, int height);
struct IClass *GetPublicClass(CONST_STRPTR className, struct Library *mb);
BOOL DestroyClasses(struct Library *MUIMasterBase);
struct IClass *CreateBuiltinClass(CONST_STRPTR className, struct Library *MUIMasterBase);
ULONG ConvertKey(struct IntuiMessage *imsg);

#ifdef _AROS
AROS_UFP3(IPTR, metaDispatcher,
	AROS_UFPA(struct IClass  *, cl,  A0),
	AROS_UFPA(Object *, obj, A2),
	AROS_UFPA(Msg     , msg, A1));
#else
__asm ULONG metaDispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg);
#endif


ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...);

#define _between(a,x,b) ((x)>=(a) && (x)<=(b))
#define _isinobject(x,y) (_between(_mleft(obj),(x),_mright (obj)) \
                          && _between(_mtop(obj) ,(y),_mbottom(obj)))

/* add mask in flags if tag is true, else sub mask */
#define _handle_bool_tag(flags, tag, mask) \
((tag) ? ((flags) |= (mask)) : ((flags) &= ~(mask)))

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

ULONG DoSetupMethod(Object *obj, struct MUI_RenderInfo *info);

#endif
