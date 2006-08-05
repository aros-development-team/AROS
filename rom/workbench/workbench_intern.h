#ifndef __WORKBENCH_INTERN_H__
#define __WORKBENCH_INTERN_H__

/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Internal header file for workbench.library.
*/

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <utility/utility.h>
#include <intuition/intuition.h>

#include <workbench/icon.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <workbench/handler.h>

#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>

/*
    This is the WorkbenchBase structure. It is defined here because it is 
    completely private. Applications should treat it as a struct Library, and
    use the workbench.library functions to get information.
*/

struct WorkbenchBase
{
    struct Library          LibNode;

    struct MsgPort          wb_HandlerPort;            /* The handler's message port */
    struct MsgPort         *wb_WorkbenchPort;          /* The workbench application's message port */
    struct SignalSemaphore  wb_WorkbenchPortSemaphore; /* Arbitrates initializetion access to the port above */
    
    struct List             wb_AppWindows;
    struct List             wb_AppIcons;
    struct List             wb_AppMenuItems;

    BPTR                    wb_SearchPath;
    struct List             wb_HiddenDevices;           /* List of devices that Workbench will not show */
    ULONG                   wb_DefaultStackSize;
    ULONG                   wb_TypeRestartTime;

    struct SignalSemaphore  wb_InitializationSemaphore; /* Arbitrates library initialization */
    struct SignalSemaphore  wb_BaseSemaphore;           /* Arbitrates library base access */
    
    BOOL                    wb_Initialized;             /* Has the library been intialized in libOpen? */
};

#define LB(lb)          ((struct WorkbenchBase *) (lb))

#define LockWorkbench()   ObtainSemaphore(&(WorkbenchBase->wb_BaseSemaphore))
#define UnlockWorkbench() ReleaseSemaphore(&(WorkbenchBase->wb_BaseSemaphore))

/*
 * Defintion of internal structures.
 */

struct AppIcon
{
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

struct AppWindow
{
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


struct AppWindowDropZone
{
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

struct AppMenuItem
{
    struct Node     ami_Node;

    ULONG           ami_ID;
    ULONG           ami_UserData;

    STRPTR          ami_Text;
    STRPTR          ami_CommandKey;

    struct MsgPort *ami_MsgPort;
};

/* Internal WBHM structure, used to hold data which the user doesn't
   have to know about.  */
struct IntWBHandlerMessage
{
    struct WBHandlerMessage iwbhm_wbhm;
    union
    {
        struct
        {
	    struct IntuiMessage *imsg;
        } Hide;
    } iwbhm_Data;
};
#endif /* __WORKBENCH_INTERN_H__ */
