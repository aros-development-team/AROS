/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal header file for workbench library
    Lang: english
*/
#ifndef __WORKBENCH_INTERN_H__
#define __WORKBENCH_INTERN_H__

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif
#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif
#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif
#ifndef EXEC_MEMORY_H
#   include <exec/memory.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif
#ifndef DOS_DOSTAGS_H
#   include <dos/dostags.h>
#endif
#ifndef UTILITY_UTILITY_H
#   include <utility/utility.h>
#endif
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif

#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <aros/debug.h>

/*
    This is the WorkbenchBase structure. It is documented here because
    it is completely private. Applications should treat it as a struct
    Library, and use the workbench.library functions to get information.
*/

struct WorkbenchBase {
    struct Library          LibNode;
    BPTR                    wb_SegList;

    struct ExecBase        *wb_SysBase;
    struct Library         *wb_DOSBase;
    struct Library         *wb_UtilityBase;
    struct Library         *wb_IntuitionBase;

    struct MsgPort         *wb_HandlerPort;       /* The Workbench Handler's message port. */
    struct MsgPort         *wb_AppPort;           /* The Workbench App's message port. */

    struct List             wb_AppWindows;
    struct List             wb_AppIcons;
    struct List             wb_AppMenuItems;

    struct List             wb_HiddenDevices;     /* List of devices that Workbench will not show. */
    ULONG                   wb_DefaultStackSize;
    ULONG                   wb_TypeRestartTime;

    struct SignalSemaphore  wb_InitializationSemaphore; /* Semaphore for single-tasking library initializtion. */

    BOOL                    wb_LibsOpened;        /* Are the libraries opened? */
};

#define SysBase         (WorkbenchBase->wb_SysBase)
#define UtilityBase     ((struct UtilityBase *)(WorkbenchBase->wb_UtilityBase))
#define IntuitionBase   ((struct IntuitionBase *)(WorkbenchBase->wb_IntuitionBase))
#define DOSBase         ((struct DosLibrary *)(WorkbenchBase->wb_DOSBase))

/*
 * Defintion of internal structures.
 */

struct AppIcon {
    struct Node        ai_Node;

    ULONG              ai_ID;
    ULONG              ai_UserData;

    ULONG              ai_Flags;
    STRPTR             ai_Text;
    struct DiskObject *ai_DiskObject;

    struct Hook       *ai_RenderHook;
    struct MsgPort    *ai_MsgPort;
};

/* Valid values for ai_Flags. These correspond to the tag items
 * in <workbench/workbench.h>. */

#define WBAPPICONF_SupportsOpen            (1<<1)
#define WBAPPICONF_SupportsCopy            (1<<2)
#define WBAPPICONF_SupportsRename          (1<<3)
#define WBAPPICONF_SupportsInformation     (1<<4)
#define WBAPPICONF_SupportsSnapshot        (1<<5)
#define WBAPPICONF_SupportsUnSnapshot      (1<<6)
#define WBAPPICONF_SupportsLeaveOut        (1<<7)
#define WBAPPICONF_SupportsPutAway         (1<<8)
#define WBAPPICONF_SupportsDelete          (1<<9)
#define WBAPPICONF_SupportsFormatDisk      (1<<10)
#define WBAPPICONF_SupportsEmptyTrash      (1<<11)
#define WBAPPICONF_PropagatePosition       (1<<12)
#define WBAPPICONF_NotifySelectState       (1<<13)

struct AppWindow {
    struct Node     aw_Node;

    ULONG           aw_ID;
    ULONG           aw_UserData;

    struct Window  *aw_Window;
    struct List     aw_DropZones;   // List of AppWindowDropZones for this AppWindow.

    struct MsgPort *aw_MsgPort;
};


#define  AWDZFlag_fix        0	/* IBox value is actual coordinate */
#define  AWDZFlag_relLeft    1
#define  AWDZFlag_relRight   2
#define  AWDZFlag_relTop     3
#define  AWDZFlag_relBottom  4
#define  AWDZFlag_relWidth   5
#define  AWDZFlag_relHeight  6


struct AppWindowDropZone {
    struct Node       awdz_Node;

    ULONG             awdz_ID;
    ULONG             awdz_UserData;

    struct IBox       awdz_Box;

    /* These four tells how to compute the drop zone size and position */
    UWORD             awdz_leftSpecifier;
    UWORD             awdz_topSpecifier;
    UWORD             awdz_widthSpecifier;
    UWORD             awdz_heightSpecifier;
    struct Hook      *awdz_Hook;
};

struct AppMenuItem {
    struct Node     ami_Node;

    ULONG           ami_ID;
    ULONG           ami_UserData;

    STRPTR          ami_Text;
    STRPTR          ami_CommandKey;

    struct MsgPort *ami_MsgPort;
};

#endif /* __WORKBENCH_INTERN_H__  */

