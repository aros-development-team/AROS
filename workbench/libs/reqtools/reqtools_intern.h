/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/


#ifndef REQTOOLS_INTERN_H
#define REQTOOLS_INTERN_H

#include <libraries/reqtools.h>
#include <intuition/intuition.h>

struct IntReqToolsBase
{
    struct ReqToolsBase rt;

    struct Library *rt_SysBase;
    struct Library *rt_LocaleBase;
    struct Library *rt_LayersBase;

    struct IORequest rt_cdevio;   /* For communication with console.device */
};


struct rtWindowLock
{
    struct Requester     rtwl_Requester;
    LONG                 rtwl_Magic;
    struct rtWindowLock *rtwl_RequesterPtr;
    ULONG                rtwl_LockCount;
    BOOL                 rtwl_ReqInstalled;

    /* To save window parameters */
    APTR                 rtwl_Pointer;
    BYTE                 rtwl_PtrHeight;
    BYTE                 rtwl_PtrWidth;
    BYTE                 rtwl_XOffset;
    BYTE                 rtwl_YOffset;
    WORD                 rtwl_MinWidth;
    WORD                 rtwl_MaxWidth;
    WORD                 rtwl_MinHeight;
    WORD                 rtwl_MaxHeight;
};


/* Fix name clashes */
typedef  struct IntuitionBase  IntuiBase;


#define SysBase ((struct IntReqToolsBase *)RTBase)->rt_SysBase
#define UtilityBase ((struct IntReqToolsBase *)RTBase)->rt.rt_UtilityBase
#define IntuitionBase (((struct IntReqToolsBase *)RTBase)->rt.rt_IntuitionBase)

#define GPB(x) ((struct IntReqToolsBase *)x)

#define expunge() \
AROS_LC0(BPTR, expunge, struct IntReqToolsBase *, RTBase, 3, ReqTools)

#endif /* REQTOOLS_INTERN_H */
