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


#define  CBUN_FILENAMELEN   sizeof("DEVS:Clipboards/255")

typedef struct ClipboardUnit
{
    struct Node  cu_Node;                   /* These two fields MUST be  */
    ULONG        cu_UnitNum;                /* first and in this order in */
                                            /* the clipboard unit as they're
					       also present in struct
					       ClipboardUnitPartial so that
					       users can check the unitnum when
					       a hook is called. */

    LONG      cu_ReadID;
    LONG      cu_WriteID;
    LONG      cu_PostID;
    UWORD     cu_OpenCnt;
    BPTR      cu_clipFile;
    STRPTR    cu_clipFilename;              /* CLIPS:un or Devs:Clipboards/un
					       where un is the unit number. */
    ULONG     cu_clipSize;

    struct SatisfyMsg       cu_Satisfy;
    struct MsgPort         *cu_PostPort;    /* Port to post message to when a
					       POST needs to be satisfied */
    struct Task            *cu_Poster;      /* Task that made the last
					       CMD_POST. Used to avoid deadlock
					       for "stupid" users. */
    struct SignalSemaphore  cu_UnitLock;

} ClipboardUnit;


#define expunge() \
AROS_LC0(BPTR, expunge, struct ClipboardBase *, CBBase, 3, Clipboard)

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
