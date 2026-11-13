/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#define __NOLIBBASE__
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/bootloader.h>

#include <proto/log.h>

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <aros/bootloader.h>

#include <resources/log.h>

#include <string.h>

#include "seriallogger_intern.h"

#include LC_LIBDEFS_FILE

#define SERLOG_LINEBUFSIZE  1024

#define LogResBase  LIBBASE->selrb_LogResBase
#define DOSBase LIBBASE->selrb_DosBase

BOOL GM_UNIQUENAME(OpenDOS)(LIBBASETYPEPTR LIBBASE)
{
    if(DOSBase)
        return TRUE;
    else if((DOSBase = OpenLibrary("dos.library", 39)))
        return TRUE;
    return FALSE;
}

AROS_UFH2(void, GM_UNIQUENAME(PutChar),
                   AROS_UFHA(char, ch, D0),
                   AROS_UFHA(void *, unused, A3))
{
    AROS_USERFUNC_INIT

    (void)unused;  // suppress unused parameter warning
    RawPutChar(ch);

    AROS_USERFUNC_EXIT
}

static void seriallogger_HandleEntry(LIBBASETYPEPTR LIBBASE, APTR logentry)
{
    char outBuf[SERLOG_LINEBUFSIZE];
    struct DateStamp *ds = NULL;
    char *origStr = NULL;
    char *compStr = NULL;
    char *subStr = NULL;
    char *tagStr = NULL;
    char *logEntryStr = NULL;
    ULONG flags;

    struct TagItem entryAttribs[] = {
        { LOGMA_Flags,          (IPTR)&flags        },
        { LOGMA_DateStamp,      (IPTR)&ds           },
        { LOGMA_Origin,         (IPTR)&origStr    },
        { LOGMA_Component,      (IPTR)&compStr      },
        { LOGMA_SubComponent,   (IPTR)&subStr       },
        { LOGMA_LogTag,         (IPTR)&tagStr       },
        { LOGMA_Entry,          (IPTR)&logEntryStr    },
        { TAG_DONE, 0 }
    };

    logGetEntryAttrs(logentry, entryAttribs);

    if ((flags & LOGM_Flag_TypeMask) == LOGF_Flag_Type_Debug)
    {
        char dsstrchs[LEN_DATSTRING], *dsStr;
        if (GM_UNIQUENAME(OpenDOS)(LIBBASE))
        {
            struct DateTime dt;
            dsStr = dsstrchs;
            dt.dat_Stamp.ds_Days = ds->ds_Days;
            dt.dat_Stamp.ds_Minute = ds->ds_Minute;
            dt.dat_Stamp.ds_Tick = ds->ds_Tick;
            dt.dat_Format = FORMAT_DEF;
            dt.dat_Flags = 0;
            dt.dat_StrDay = NULL;
            dt.dat_StrDate = NULL;
            dt.dat_StrTime = dsStr;
            DateToStr(&dt);
        }
        else
        {
            // TODO: Format something from the data we have.
            dsStr="        ";
        }
        logRawDoFmtCB(outBuf, SERLOG_LINEBUFSIZE, (VOID_FUNC) GM_UNIQUENAME(PutChar), NULL, "%s  - %s - [%s] %s: %s\n", dsStr, origStr, compStr, tagStr, logEntryStr);
    }
}

static void seriallogger_Task(LIBBASETYPEPTR LIBBASE)
{
    APTR serialListener = NULL;

    logAddEntry(LOGF_Flag_Type_Information, LIBBASE->selrb_Provider, "", __func__, 0,
                        "AROS Serial Debug Logger v%u.%u\nSerial debug started", MAJOR_VERSION, MINOR_VERSION);
    {
        APTR lognode;
        logLockEntries(LLF_READ);

        // Register our listener
        LIBBASE->selrb_Port->mp_Node.ln_Pri = 127;
        if ((serialListener = logAddListener(LIBBASE->selrb_Port, LIBBASE->selrb_Mask)) != NULL)
        {
            // Dump all the pending entries..
            lognode = LOGEntry_First;
            while((lognode = logNextEntry(&lognode)) && (lognode != (APTR)LOGEntry_Last))
            {
                seriallogger_HandleEntry(LIBBASE, lognode);
            }
        }
        logUnlockEntries(LLF_READ); 
    }

    if (serialListener)
    {
        BOOL running = TRUE;

        // Start listening for new events to output..
        while (running)
        {
            struct LogResBroadcastMsg *lrbMsg = NULL;
            ULONG signals = Wait((1 << LIBBASE->selrb_Port->mp_SigBit) | SIGBREAKF_CTRL_C);
            if (signals & SIGBREAKF_CTRL_C)
            {
                running = FALSE;
                continue;
            }
            else if (signals & (1 << LIBBASE->selrb_Port->mp_SigBit))
                while ((lrbMsg = (struct LogResBroadcastMsg *)GetMsg(LIBBASE->selrb_Port)) != NULL)
                {
                    seriallogger_HandleEntry(LIBBASE, lrbMsg->lrbm_Target);
                    logDeleteBroadcast(serialListener, lrbMsg);
                }
        }
        logRemListener(serialListener);
    }
    logFinalise(LIBBASE->selrb_Provider);
}

static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR LIBBASE)
{
    APTR    BootLoaderBase = OpenResource("bootloader.resource");

    LIBBASE->selrb_Mask = 0;
    if ((LogResBase = OpenResource("log.resource")) != NULL)
    {
        if (BootLoaderBase != NULL)
        {
            struct List *list;
            struct Node *node;

            list = (struct List *)GetBootInfo(BL_Args);
            if (list)
            {
                ForeachNode(list, node)
                {
                    if (strncmp("debug=", node->ln_Name, 6) == 0)
                    {
                        const char *CmdLine = &node->ln_Name[6];

                        if (strstr(CmdLine, "serial"))
                        {
                            LIBBASE->selrb_Mask |= (LOGF_Flag_Type_Debug | EHMF_ADDENTRY);
                        }
                    }
                }
            }
        }

        if (LIBBASE->selrb_Mask != 0)
        {
            LIBBASE->selrb_Provider = logInitialise(LIBBASE);

            LIBBASE->selrb_Task = NewCreateTask(TASKTAG_NAME, "Serial Debug Logger",
                        TASKTAG_PC, seriallogger_Task,
                        TASKTAG_TASKMSGPORT, &LIBBASE->selrb_Port,
                        TASKTAG_PRI, 21,
                        TASKTAG_ARG1, LIBBASE,
                        TAG_END);
            return TRUE;
        }
    }
    return FALSE;
}

ADD2INITLIB(GM_UNIQUENAME(libInit), 0)
