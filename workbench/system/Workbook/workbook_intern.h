/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Workbook headers
    Lang: english
*/

#ifndef WORKBOOK_H
#define WORKBOOK_H

#define WB_VERSION	1
#define WB_REVISION	0

#include <dos/bptr.h>
#include <intuition/classes.h>
#include <intuition/intuition.h>

struct WorkbookBase {
    APTR wb_IntuitionBase;
    APTR wb_DOSBase;
    APTR wb_UtilityBase;
    APTR wb_GadToolsBase;
    APTR wb_IconBase;
    APTR wb_WorkbenchBase;
    APTR wb_GfxBase;
    APTR wb_LayersBase;

    Class  *wb_WBApp;
    Class  *wb_WBWindow;
    Class  *wb_WBVirtual;
    Class  *wb_WBIcon;
    Class  *wb_WBSet;

    Object *wb_App;

    /* Create a new task that simply OpenWorkbenchObject()'s
     * it's argment.
     */
    BPTR wb_OpenerSegList;
};

/* FIXME: Remove these #define xxxBase hacks
   Do not use this in new code !
*/
#define IntuitionBase wb->wb_IntuitionBase
#define DOSBase       wb->wb_DOSBase
#define UtilityBase   wb->wb_UtilityBase
#define GadToolsBase  wb->wb_GadToolsBase
#define IconBase      wb->wb_IconBase
#define WorkbenchBase wb->wb_WorkbenchBase
#define GfxBase       wb->wb_GfxBase
#define LayersBase    wb->wb_LayersBase

extern struct ExecBase *SysBase;

#include <string.h>
#include <proto/exec.h>

static inline STRPTR StrDup(CONST_STRPTR str)
{
    STRPTR cp;
    int len;
   
    if (str == NULL)
    	return NULL;

    len = strlen(str) + 1;

    cp = AllocVec(len, MEMF_ANY);
    if (cp == NULL)
    	return NULL;

    CopyMem(str, cp, len);

    return cp;
}

struct Region *wbClipWindow(struct WorkbookBase *wb, struct Window *win);
void wbUnclipWindow(struct WorkbookBase *wb, struct Window *win, struct Region *clip);

#endif /* WORKBOOK_H */
