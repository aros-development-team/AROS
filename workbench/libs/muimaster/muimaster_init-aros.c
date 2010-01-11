/*
    Copyright © 2002-2010, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <proto/bootloader.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#include <clib/alib_protos.h>

#include "muimaster_intern.h"
#include "mui.h"

#include <aros/symbolsets.h>

#include <dos/dos.h>
#include <aros/bootloader.h>
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

struct Library *MUIMasterBase;

static struct TextAttr topaz8Attr =
    { "topaz.font", 8, FS_NORMAL, FPF_ROMFONT, };

/****************************************************************************************/

static int MUIMasterInit(LIBBASETYPEPTR lh)
{
    /* if poseidon is enabled, wait until ENV: exists
     * to avoid missing volume requester
     */
    APTR BootLoaderBase;
    if ((BootLoaderBase = OpenResource("bootloader.resource")))
    {
        /* TODO: create something like ExistsBootArg("enableusb") in bootloader.resource */
        struct List *list = GetBootInfo(BL_Args);
        BOOL enable = FALSE;
        if (list)
        {
            struct Node *node;
            ForeachNode(list, node)
            {
                if (stricmp(node->ln_Name, "enableusb") == 0)
                {
                    enable = TRUE;
                }
            }
        }

        if (enable)
        {
            struct DevProc* dp      = NULL;
            struct Process* me      = (struct Process *) FindTask(NULL);
            APTR            win     = me->pr_WindowPtr; /* backup old value */
            ULONG           cnt     = 0;
            const ULONG     max_cnt = 9;  /* 5s   */
            const ULONG     ticks   = 25; /* 0.5s */
            
            me->pr_WindowPtr = (APTR) -1; /* disable requester */
            while (TRUE)
            {
                dp = GetDeviceProc("ENV:", dp);
                if (dp)
                    break;
                else if (cnt > max_cnt)
                {
                    bug("No ENV: after %d second%s\n",
                        (ticks * cnt) / TICKS_PER_SECOND, cnt > 1 ? "s" : "");
                    break;
                }
                Delay(ticks);
                cnt++;
            }
            me->pr_WindowPtr = win; /* restore to old value */
            FreeDeviceProc(dp);
        }
    }
    
    MUIMasterBase = (struct Library *)lh;
    
    InitSemaphore(&MUIMB(lh)->ZuneSemaphore);
    
    NewList((struct List *)&MUIMB(lh)->BuiltinClasses);
    NewList((struct List *)&MUIMB(lh)->Applications);

    ((struct MUIMasterBase_intern *)MUIMasterBase)->topaz8font = OpenFont(&topaz8Attr);

    return TRUE;
}

static int MUIMasterExpunge(LIBBASETYPEPTR lh)
{
    MUIMasterBase = (struct Library *)lh;
    
    CloseFont(((struct MUIMasterBase_intern *)MUIMasterBase)->topaz8font);

    return TRUE;
}

ADD2INITLIB(MUIMasterInit, 0);
ADD2EXPUNGELIB(MUIMasterExpunge, 0);

/****************************************************************************************/
