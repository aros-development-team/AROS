#ifndef  CLIPBOARD_INTERN_H
#define  CLIPBOARD_INTERN_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <dos/dosextens.h>
#include <devices/clipboard.h>
#ifndef __MORPHOS__
#include <dos/bptr.h>
#endif

struct ClipboardBase
{
    struct Device      cb_device;
    struct ExecBase   *cb_sysBase;
    struct Library    *cb_DosBase;
    struct Library    *cb_UtilityBase;
    BPTR               cb_seglist;
    
    struct SignalSemaphore  cb_SignalSemaphore;
    struct MinList          cb_UnitList;

    struct MinList          cb_HookList;    /* List of hooks called when data
					       on the clipboard has changed */

    STRPTR                  cb_ClipDir;     /* CLIPS: or Devs:Clipboards/ */
};


#define  CBUN_FILENAMELEN   28

struct PostRequest
{
    struct MinNode  pr_Link;
    struct Task    *pr_Waiter;
};

struct ClipboardUnit
{
    struct ClipboardUnitPartial cu_Head;	/* MUST be first! */

    LONG      cu_ReadID;
    LONG      cu_WriteID;
    LONG      cu_PostID;
    UWORD     cu_OpenCnt;
    BPTR      cu_clipFile;
    UBYTE     cu_clipFilename[CBUN_FILENAMELEN]; /* CLIPS:un or Devs:Clipboards/un
						    where un is the unit number. */
    ULONG     cu_clipSize;

    struct SatisfyMsg       cu_Satisfy;
    struct MsgPort         *cu_PostPort;    /* Port to post message to when a
					       POST needs to be satisfied */
    struct MinList          cu_PostRequesters;
    struct SignalSemaphore  cu_UnitLock;

};

#ifdef SysBase
#undef SysBase
#endif
#define SysBase CBBase->cb_sysBase

#ifdef DOSBase
#undef DOSBase
#endif
#define DOSBase CBBase->cb_DosBase

#ifdef UtilityBase
#undef UtilityBase
#endif
#define UtilityBase CBBase->cb_UtilityBase

#endif /* CLIPBOARD_INTERN_H */
