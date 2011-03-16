/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Workbook headers
    Lang: english
*/

#ifndef WORKBOOK_H
#define WORKBOOK_H

#include <intuition/classes.h>

struct WorkbookBase {
    APTR wb_IntuitionBase;
    APTR wb_DOSBase;
    APTR wb_UtilityBase;
    APTR wb_GadToolsBase;
    APTR wb_IconBase;
    APTR wb_WorkbenchBase;
    APTR wb_GfxBase;

    Class  *wb_WBApp;
    Class  *wb_WBWindow;
    Class  *wb_WBVirtual;
    Class  *wb_WBIcon;
    Class  *wb_WBSet;

    Object *wb_App;
};

#define IntuitionBase wb->wb_IntuitionBase
#define DOSBase       wb->wb_DOSBase
#define UtilityBase   wb->wb_UtilityBase
#define GadToolsBase  wb->wb_GadToolsBase
#define IconBase      wb->wb_IconBase
#define WorkbenchBase wb->wb_WorkbenchBase
#define GfxBase       wb->wb_GfxBase

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

#endif /* WORKBOOK_H */
