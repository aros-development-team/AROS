/*
    Copyright � 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <string.h>

#ifndef __AROS__
#ifndef __MAXON__
#include <dos.h>
#endif
#endif

#include <intuition/classes.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/utility.h>
#ifdef __AROS__
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "support.h"
#include "muimaster_intern.h"

extern struct Library *MUIMasterBase;

/**************************************************************************
 check if region is entirely within given bounds
**************************************************************************/
int isRegionWithinBounds(struct Region *r, int left, int top, int width, int height)
{
	if ((left <= r->bounds.MinX) && (left + width  - 1 >= r->bounds.MaxX)
	 && (top  <= r->bounds.MinY) && (top  + height - 1 >= r->bounds.MaxY))
		return 1;

	return 0;
}


/**************************************************************************
 Converts a Rawkey to a vanillakey
**************************************************************************/
ULONG ConvertKey(struct IntuiMessage *imsg)
{
   struct InputEvent event;
   UBYTE code = 0;
   event.ie_NextEvent    = NULL;
   event.ie_Class        = IECLASS_RAWKEY;
   event.ie_SubClass     = 0;
   event.ie_Code         = imsg->Code;
   event.ie_Qualifier    = imsg->Qualifier;
   event.ie_EventAddress = (APTR *) *((ULONG *)imsg->IAddress);
   MapRawKey(&event, &code, 1, NULL);
   return code;
}

/**************************************************************************
...
**************************************************************************/
ULONG DoSuperNew(struct IClass *cl, Object * obj, ULONG tag1,...)
{
  return (DoSuperMethod(cl, obj, OM_NEW, (IPTR)&tag1, NULL));
}

/**************************************************************************
 Convient way to get an attribute of an object easily. If the object
 doesn't support the attribute this call returns an undefined value. So use
 this call only if the attribute is known to be known by the object. 
**************************************************************************/
IPTR xget(Object *obj, Tag attr)
{
  IPTR storage = 0;
  GetAttr(attr, obj, &storage);
  return storage;
}

/**************************************************************************
 Call the Setup Method of an given object, but before set the renderinfo
**************************************************************************/
ULONG DoSetupMethod(Object *obj, struct MUI_RenderInfo *info)
{
    /* MUI set the correct render info *before* it calls MUIM_Setup so please only use this function instead of DoMethodA() */
    muiRenderInfo(obj) = info;
    return DoMethod(obj, MUIM_Setup, (IPTR)info);
}

APTR AllocVecPooled (APTR pool, ULONG memsize)
{
    if (pool)
    {
	IPTR *mem;

	memsize += sizeof(IPTR);

	if ((mem = AllocPooled(pool, memsize)))
	{
	    *mem++ = memsize;
	}

	return mem;
    }
    else
    {
        return AllocVec(memsize, MEMF_PUBLIC | MEMF_CLEAR);
    }
}

void FreeVecPooled (APTR pool, APTR mem)
{
    if (mem)
    {
	if (pool)
	{
	    IPTR *imem = (IPTR *)mem;
	    IPTR size = *--imem;
 
	    FreePooled(pool, imem, size);
        }
	else
	{
             FreeVec(mem);
	}
    }
}

#ifndef __AROS__
char *StrDup(char *x)
{
    char *dup;
    if (!x) return NULL;
    dup = AllocVec(strlen(x) + 1, MEMF_PUBLIC);
    if (dup) CopyMem((x), dup, strlen(x) + 1);
    return dup;
}
#endif

void *Node_Next(APTR node)
{
    if(node == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Succ == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Succ->mln_Succ == NULL)
		return NULL;
    return ((struct MinNode*)node)->mln_Succ;
}

void *List_First(APTR list)
{
    if( !((struct MinList*)list)->mlh_Head) return NULL;
    if(((struct MinList*)list)->mlh_Head->mln_Succ == NULL) return NULL;
    return ((struct MinList*)list)->mlh_Head;
}

/**************************************************************************
 A temporary snprintf wrapper for SAS.
**************************************************************************/
#ifndef __AROS__

#include <stdarg.h>
#include <stdio.h>

int snprintf(char *buf, int size, const char *fmt, ...)
{
	int ret;
	va_list argptr;
	va_start(argptr,fmt);
	ret = vsprintf(buf,fmt,argptr);
	va_end(argptr);
	return ret;
}
#endif
