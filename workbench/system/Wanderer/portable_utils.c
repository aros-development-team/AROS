#include "portable_macros.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>
#include <proto/datatypes.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/locale.h>
#include <proto/layers.h>
#include <proto/diskfont.h>
#include <proto/asl.h>
#include <proto/keymap.h>
#include <proto/iffparse.h>


#ifdef __amigaos4__
Object *VARARGS68K DoSuperNewTags(struct IClass *cl, Object *obj, void *dummy, ...)
{
    va_list argptr;
    struct TagItem *tagList;
    va_startlinear(argptr, dummy);
    tagList = va_getlinearva(argptr, struct TagItem *);
    obj = (Object*)DoSuperMethod(cl,obj,OM_NEW,tagList,dummy);
    va_end(argptr);
    return obj;
}
#endif

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
