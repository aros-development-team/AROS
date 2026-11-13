/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

#define USE_INLINE_STDARG
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#define __NOLIBBASE__
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/utility.h>
#include <proto/log.h>

#include <resources/log.h>
#include <workbench/startup.h>
#include <libraries/mui.h>

#include <stdlib.h>

#include "logview.h"

extern struct DosLibrary *DOSBase;
 
extern APTR     LogResBase;
extern Object   *o_listobj;

struct List     logList;
ULONG           logmask = LOGF_Flag_Type_Information|LOGF_Flag_Type_Verbose|LOGF_Flag_Type_Warn|LOGF_Flag_Type_Error|LOGF_Flag_Type_Crit;
char            *logfilt = NULL;

void FreeLogEntryList()
{
    struct Node *node;
    node = logList.lh_Head;
    while(node->ln_Succ)
    {
        Remove(node);
        FreeVec(node);
        node = logList.lh_Head;
    }
}

void CreateLogEntryList()
{
    struct LogListEntry *llenode;
    struct List *lst;
    APTR lognode;

    SET(o_listobj, MUIA_List_Quiet, TRUE);

    DoMethod(o_listobj, MUIM_List_Clear);

    FreeLogEntryList();
    logLockEntries(LLF_READ);
    lognode = 0;
    while((lognode = logNextEntry(&lognode)) && (lognode != (APTR)-1))
    {
        char *origStr;
        ULONG flags;
        struct TagItem entryAttribs[] = {
            { LOGMA_Flags,          (IPTR)&flags        },
            { LOGMA_Origin,         (IPTR)&origStr      },
            { TAG_DONE, 0 }
        };

        logGetEntryAttrs(lognode, entryAttribs);

        if((flags & logmask) != 0)
        {
            if (!logfilt || (MatchPatternNoCase(logfilt, origStr)))
            {
                if((llenode = AllocVec(sizeof(struct LogListEntry), MEMF_ANY)))
                {
                    llenode->entry = lognode;
                    AddTail(&logList, &llenode->node);
                    DoMethod(o_listobj, MUIM_List_InsertSingle, llenode, MUIV_List_Insert_Bottom);
                }
            }
        }
    }
    logUnlockEntries(LLF_READ);
    SET(o_listobj, MUIA_List_Quiet, FALSE);
    SET(o_listobj, MUIA_List_Active, MUIV_List_Active_Bottom);
}
