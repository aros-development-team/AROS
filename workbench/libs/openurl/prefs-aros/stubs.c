/*
**  OpenURL - MUI preferences for openurl.library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
**
**  Ported to OS4 by Alexandre Balaban <alexandre@balaban.name>
*/


#include "OpenURL.h"

/***********************************************************************/
#if !defined(__amigaos4__) && !defined(__AROS__)

APTR
NewObject(struct IClass *classPtr,UBYTE *classID,... )
{
    APTR    res;
    va_list va;

    va_start(va,classID);
    res = NewObjectA(classPtr,classID,(struct TagItem *)va->overflow_arg_area);
    va_end(va);

    return res;
}

/***********************************************************************/

APTR
MUI_NewObject(UBYTE *classID,... )
{
    APTR    res;
    va_list va;

    va_start(va,classID);
    res = MUI_NewObjectA(classID,(struct TagItem *)va->overflow_arg_area);
    va_end(va);

    return res;
}
#endif

/***********************************************************************/

#ifndef __AROS__

#if defined(__amigaos4__)
#include <stdarg.h>
#endif

ULONG VARARGS68K
DoSuperNew(struct IClass *cl,Object *obj,...)
{
    APTR    res;
    va_list va;

#if defined(__amigaos4__)
    va_startlinear(va,obj);
    res = (APTR)DoSuperMethod(cl,obj,OM_NEW,va_getlinearva(va,ULONG),NULL);
#else
    va_start(va,obj);
    res = (APTR)DoSuperMethod(cl,obj,OM_NEW,(ULONG)va->overflow_arg_area,NULL);
#endif
    va_end(va);

    return (ULONG)res;
}

#endif /* !__AROS__ */

/***********************************************************************/

