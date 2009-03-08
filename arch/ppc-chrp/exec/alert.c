/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display an alert.
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/libcall.h>
#include <exec/alerts.h>
#include <proto/exec.h>
#include <proto/kernel.h>

extern void *priv_KernelBase;

static inline void bug(const char *format, ...)
{
    void *KernelBase = priv_KernelBase;
    va_list args;
    va_start(args, format);
    KrnBug(format, args);
    va_end(args);
}

/*****************************************************************************

    NAME */

        AROS_LH1(void, Alert,

/*  SYNOPSIS */
        AROS_LHA(ULONG, alertNum, D7),

/*  LOCATION */
        struct ExecBase *, SysBase, 18, Exec)

/*  FUNCTION
        Alerts the user of a serious system problem.

    INPUTS
        alertNum - This is a number which contains information about
                the reason for the call.

    RESULT
        This routine may return, if the alert is not a dead-end one.

    NOTES
        You should not call this routine because it halts the machine,
        displays the message and then may reboot it.

    EXAMPLE
        // Dead-End alert: 680x0 Access To Odd Address
        Alert (0x80000003);

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
        26-08-95    digulla created after EXEC-Routine

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    static const char * CPUStrings[] =
    {
        "Hardware bus fault/access error",
        "Illegal address access (ie: odd)",
        "Illegal instruction",
        "Divide by zero",
        "Check instruction error",
        "TrapV instruction error",
        "Privilege violation error",
        "Trace error",
        "Line 1010 Emulator error",
        "Line 1111 Emulator error",
        "Stack frame format error",
        "Spurious interrupt error",
        "AutoVector Level 1 interrupt error",
        "AutoVector Level 2 interrupt error",
        "AutoVector Level 3 interrupt error",
        "AutoVector Level 4 interrupt error",
        "AutoVector Level 5 interrupt error",
        "AutoVector Level 6 interrupt error",
        "AutoVector Level 7 interrupt error",
    },
    * GenPurposeStrings[] =
    {
        "No memory",
        "Make library",
        "Open library",
        "Open device",
        "Open resource",
        "I/O error",
        "No signal",
        "Bad parameter",
        "Close library",
        "Close device",
        "Create process",
    },
    * AlertObjects[] =
    {
        "Exec",
        "Graphics",
        "Layers",
        "Intuition",

        "Math",
        "DOS",
        "RAM",
        "Icon",

        "Expansion",
        "Diskfont",
        "Utility",
        "Keymap",

        NULL,
        NULL,
        NULL,
        NULL,
    /* 0x10 */
        "Audio",
        "Console",
        "Gameport",
        "Keyboard",

        "Trackdisk",
        "Timer",
        NULL,
        NULL,

        NULL,
        NULL,
        NULL,
        NULL,

        NULL,
        NULL,
        NULL,
        NULL,
    /* 0x20 */
        "CIA",
        "Disk",
        "Misc",
        NULL,

        NULL,
        NULL,
        NULL,
        NULL,

        NULL,
        NULL,
        NULL,
        NULL,

        NULL,
        NULL,
        NULL,
        NULL,
    /* 0x30 */
        "Bootstrap",
        "Workbench",
        "Diskcopy",
        "Gadtools",

        "Unknown",
    },
    * ExecStrings[] =
    {
        "68000 exception vector checksum",
        "Execbase checksum",
        "Library checksum failure",
        NULL,

        "Corrupt memory list detected in FreeMem",
        "No memory for interrupt servers",
        "InitStruct() of an APTR source",
        "A semaphore is in an illegal state at ReleaseSemaphore",

        "Freeing memory already freed",
        "Illegal 68k exception taken",
        "Attempt to reuse active IORequest",
        "Sanity check on memory list failed",

        "IO attempted on closed IORequest",
        "Stack appears to extend out of range",
        "Memory header not located. (Usually an invalid address passed to FreeMem())",
        "An attempt was made to use the old message semaphores",

    };

    struct Task * task;

#   define GetSubSysId(a)       (((a) >> 24) & 0x7F)
#   define GetGenError(a)       (((a) >> 16) & 0xFF)
#   define GetSpecError(a)      ((a) & 0xFFFF)

    task = FindTask (NULL);

    /* since this is an emulation, we just show the bug in the console */
    bug ( "[exec] GURU Meditation %04lx %04lx\n[exec] "
        , alertNum >> 16
        , alertNum & 0xFFFF
    );

    if (alertNum & 0x80000000)
        bug ( "Deadend/" );
    else
        bug( "Recoverable/" );

    switch (GetSubSysId (alertNum))
    {
    case 0: /* CPU/OS/App */
        if (GetGenError (alertNum) == 0)
        {
            bug( "CPU/" );

            if (GetSpecError (alertNum) >= 2 && GetSpecError (alertNum) <= 0x1F)
                bug("%s"
                    , CPUStrings[GetSpecError (alertNum) - 2]
                );
            else
                bug("*unknown*");
        }
        else if (GetGenError (alertNum) <= 0x0B)
        {
            bug("%s/"
                , GenPurposeStrings[GetGenError (alertNum) - 1]
            );

            if (GetSpecError (alertNum) >= 0x8001
                && GetSpecError (alertNum) <= 0x8035)
            {
                bug("%s"
                    , AlertObjects[GetSpecError (alertNum) - 0x8001]
                );
            }
            else
                bug("*unknown*");
        }

        break;

    case 1: /* Exec */
        bug("Exec/");

        if (!GetGenError (alertNum)
            && GetSpecError (alertNum) >= 0x0001
            && GetSpecError (alertNum) <= 0x0010)
        {
            bug("%s"
                , ExecStrings[GetSpecError (alertNum) - 0x0001]
            );
        }
        else
        {
            bug("*unknown*");
        }

        break;

    case 2: /* Graphics */
        bug("Graphics/*unknown*");

        break;

    default:
        bug("*unknown*/*unknown*");
    }

    bug("\n[exec] Task: %p (%s)\n"
        , task
        , (task && task->tc_Node.ln_Name) ?
            task->tc_Node.ln_Name
            : "-- unknown task --"
    );

    if (alertNum & AT_DeadEnd)
    {
        /* Um, we have to do something here in order to prevent the
            computer from continuing... */
        ColdReboot();
    }
    AROS_LIBFUNC_EXIT
} /* Alert */

