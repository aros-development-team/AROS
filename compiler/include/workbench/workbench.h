#ifndef WORKBENCH_WORKBENCH_H
#define WORKBENCH_WORKBENCH_H

/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Workbench structures
    Lang: english
*/

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif
#ifndef EXEC_TASKS_H
#   include <exec/tasks.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif

#define WORKBENCH_NAME "workbench.library"

struct DrawerData
{
    struct NewWindow dd_NewWindow;
    LONG             dd_CurrentX;
    LONG             dd_CurrentY;
    ULONG            dd_Flags;
    UWORD            dd_ViewModes;
};
#define DRAWERDATAFILESIZE (sizeof (struct DrawerData))

struct DiskObject
{
    UWORD               do_Magic;
    UWORD               do_Version;
    struct Gadget       do_Gadget;
    UBYTE               do_Type;        /* see below */
    char              * do_DefaultTool;
    char             ** do_ToolTypes;
    LONG                do_CurrentX;
    LONG                do_CurrentY;
    struct DrawerData * do_DrawerData;
    char              * do_ToolWindow;
    LONG                do_StackSize;
};

#define WBDISK    1
#define WBDRAWER  2
#define WBTOOL    3
#define WBPROJECT 4
#define WBGARBAGE 5
#define WBDEVICE  6
#define WBKICK    7
#define WBAPPICON 8

#define WB_DISKVERSION  1
#define WB_DISKREVISION 1
#define WB_DISKREVISIONMASK 0xFF

#define WB_DISKMAGIC 0xE310

struct FreeList
{
    WORD        fl_NumFree;
    struct List fl_MemList;
};

/* Icons */
#define GFLG_GADGBACKFILL 0x0001
#define NO_ICON_POSITION  0x80000000

/* AppXXXX */

struct AppMessage
{
    struct Message am_Message;
    UWORD          am_Type;     /* see below */
    ULONG          am_UserData;
    ULONG          am_ID;
    LONG           am_NumArgs;
    struct WBArg * am_ArgList;
    UWORD          am_Version;  /* see below */
    UWORD          am_Class;
    WORD           am_MouseX;
    WORD           am_MouseY;
    ULONG          am_Seconds;
    ULONG          am_Micros;
    ULONG          am_Reserved[8];
};

#define AM_VERSION 1

#define AMTYPE_APPWINDOW   7
#define AMTYPE_APPICON     8
#define AMTYPE_APPMENUITEM 9

#endif /* WORKBENCH_WORKBENCH_H */
