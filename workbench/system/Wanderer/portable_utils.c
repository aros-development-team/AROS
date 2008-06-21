#include "portable_macros.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include <proto/graphics.h>
#include <proto/datatypes.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/locale.h>
#include <proto/layers.h>
#include <proto/diskfont.h>
#include <proto/asl.h>
#include <proto/keymap.h>
#include <proto/iffparse.h>
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <intuition/pointerclass.h>

#if defined(__AMIGA__) && !defined(__PPC__)
#define NO_INLINE_STDARG
#endif
#include <proto/intuition.h>
#include <proto/muimaster.h>

#ifndef __AROS__
#define DEBUG 1

#ifdef DEBUG
  #define D(x) if (DEBUG) x
  #ifdef __amigaos4__
  #define bug DebugPrintF
  #else
  #define bug kprintf
  #endif
#else
  #define  D(...)
#endif
#endif


#ifndef __MORPHOS__
Object * VARARGS68K DoSuperNew(struct IClass *cl, Object *obj, ...)
{
    Object *rc;
    VA_LIST args;

    VA_START(args, obj);

    rc = (Object *)DoSuperMethod(cl, obj, OM_NEW, VA_ARG(args, IPTR), NULL);

    VA_END(args);

    return rc;
}
#endif

Object *VARARGS68K DoSuperNewTags(struct IClass *cl, Object *obj, void *dummy, ...)
{
    Object *rc;
    VA_LIST argptr;
                                  
    VA_START(argptr, dummy);

    rc = (Object*)DoSuperMethod(cl,obj,OM_NEW,VA_ARG(argptr, IPTR),dummy);

    VA_END(argptr);

    return rc;
}

STRPTR StrDup (CONST_STRPTR str)
{
    STRPTR dup;
    ULONG  len;

    if (str == NULL) return NULL;

    len = strlen(str);
    dup = AllocVec(len + 1, MEMF_PUBLIC);
    if (dup != NULL) CopyMem(str, dup, len + 1);

    return dup;

} /* StrDup */


struct RastPort *CreateRastPort(void)
{
    struct RastPort *newrp = AllocMem(sizeof(*newrp), MEMF_PUBLIC);

    if (newrp)
    {
        InitRastPort(newrp);
    }

    return newrp;
}

struct RastPort *CloneRastPort(struct RastPort *rp)
{
    struct RastPort *newrp = NULL;

    if (rp)
    {
        newrp = AllocMem(sizeof(*newrp), MEMF_PUBLIC);
        if (newrp)
        {
            // *newrp = *rp;

            memcpy(newrp,rp,sizeof(struct RastPort));
        }
    }

    return newrp;
}

void FreeRastPort(struct RastPort *rp)
{
    FreeMem(rp, sizeof(*rp));
}


BOOL AndRectRect(struct Rectangle *rect1, struct Rectangle *rect2, struct Rectangle *intersect)
{                                                                                   
    if (!(((LONG)rect1 > 1024) && TypeOfMem((APTR)rect1)))
        D(bug("\x07, %ld: bad pointer: %s = $%lx\n", __LINE__, rect1, (APTR)(rect1)) );
   
    if (!(((LONG)(rect2) > 1024) && TypeOfMem((APTR)(rect2))))
        D(bug("\x07, %ld: bad pointer: %s = $%lx\n", __LINE__, rect2, (APTR)(rect2)) );

    if (!((((APTR)(intersect)) == NULL) ||  (((LONG)(intersect) > 1024) &&  TypeOfMem((APTR)(intersect)))))
        D(bug("\x07:%ld: bad pointer: %s = $%lx\n", __LINE__, intersect, (APTR)(intersect)));

    if (intersect)
        return _AndRectRect(rect1, rect2, intersect);
    else
        return overlap(*rect1, *rect2);

} /* AndRectRect */

#if defined(__AMIGA__) && !defined(__PPC__)
APTR AllocVecPooled(APTR  pool, ULONG size)
{
    
    IPTR *memory;
    
    if (pool == NULL) return NULL;
    
    size   += sizeof(IPTR);
    memory  = AllocPooled(pool, size);
    
    if (memory != NULL)
    {
        *memory++ = size;
    }

    return memory;
} /* AllocVecPooled() */


void FreeVecPooled(APTR pool, APTR memory)
{
    if (memory != NULL)
    {
        IPTR *real = (IPTR *) memory;
        IPTR size  = *--real;

        FreePooled(pool, real, size);
    }
} /* FreeVecPooled() */
#endif
