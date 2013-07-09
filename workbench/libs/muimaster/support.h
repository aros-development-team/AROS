/*
    Copyright � 2002-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUIMASTER_SUPPORT_H
#define _MUIMASTER_SUPPORT_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef INTUITION_CLASSUSR_H
#   include <intuition/classusr.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef CLIB_MACROS_H
#   include <clib/macros.h>
#endif

#ifdef __AROS__
#   ifndef AROS_ASMCALL_H
#       include <aros/asmcall.h>
#   endif
#   include <aros/macros.h>
#   define IMSPEC_EXTERNAL_PREFIX "IMAGES:Zune/"
#   include "support_aros.h"
#else
#   include "support_amigaos.h"
#endif

struct MUI_RenderInfo;
struct IntuiMessage;
struct Region;
struct Rectangle;
struct Object;
struct Library;

#define mui_alloc(x) AllocVec(x,MEMF_CLEAR)
#define mui_alloc_struct(x) ((x *)AllocVec(sizeof(x),MEMF_CLEAR))
#define mui_free(x) FreeVec(x)


int isRegionWithinBounds(struct Region *r, int left, int top, int width,
    int height);
ULONG ConvertKey(struct IntuiMessage *imsg);

#define _between(a,x,b) ((x)>=(a) && (x)<=(b))
#define _isinobject(obj, x, y) (_between(_mleft(obj), (x), _mright(obj)) \
                          && _between(_mtop(obj), (y), _mbottom(obj)))

/* add mask in flags if tag is true, else sub mask */
#define _handle_bool_tag(flags, tag, mask) \
((tag != 0) ? ((flags) |= (mask)) : ((flags) &= ~(mask)))

#define CLAMP(x, low, high) \
    (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#ifndef __GNUC__
IPTR XGET(Object * obj, Tag attr);
#endif

IPTR DoSetupMethod(Object * obj, struct MUI_RenderInfo *info);
IPTR DoShowMethod(Object * obj);
IPTR DoHideMethod(Object * obj);

/* returns next node of this node */
void *Node_Next(APTR node);
/* returns first node of this list */
void *List_First(APTR list);

WORD SubtractRectFromRect(struct Rectangle *a, struct Rectangle *b,
    struct Rectangle *destrectarray);
ULONG IsObjectVisible(Object * child, struct Library *MUIMasterBase);

#endif /* _MUIMASTER_SUPPORT_H */
