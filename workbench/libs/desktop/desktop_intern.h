/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
   $Id$ 
 */

#ifndef DESKTOP_INTERN_H
#    define DESKTOP_INTERN_H

#    include <aros/libcall.h>
#    include <exec/execbase.h>
#    include <exec/libraries.h>
#    include <dos/dos.h>
#    include <intuition/intuitionbase.h>

// this is extremely temporary! This is part of the extensible
// context menus.  Because new menuitems can be added by anyone
// via a nice prefs program, we need to store what to do when
// a menu is chosen.  It will, shortly, somehow, be in a prefs file.. but
// until then, we hardcode them in a list here

struct DesktopOperation
{
    struct Node     do_Node;
    ULONG           do_Number;
    ULONG           do_Code;
    UBYTE          *do_Name;
    ULONG           do_MutualExclude;
    ULONG           do_Flags;
    struct List     do_SubItems;
    struct MUI_CustomClass *do_Impl;
};

#    define DOF_CHECKED        0x00000001
#    define DOF_CHECKABLE      0x00000002
#    define DOF_MUTUALEXCLUDE  0x00000004

// end temporary

struct DesktopBase
{
    struct Library  db_Library;

    BPTR            db_SegList;

    struct ExecBase *db_SysBase;
    struct Library *db_DOSBase;
    struct IntuitionBase *db_IntuitionBase;
    struct Library *db_GfxBase;
    struct Library *db_LayersBase;
    struct Library *db_UtilityBase;
    struct Library *db_MUIMasterBase;
    struct Library *db_IconBase;

    struct Library *db_InputBase;
    struct IORequest *db_InputIO;

    struct MUI_CustomClass *db_Observer;
    struct MUI_CustomClass *db_Presentation;
    struct MUI_CustomClass *db_AbstractIconContainer;
    struct MUI_CustomClass *db_IconContainer;
    struct MUI_CustomClass *db_IconContainerObserver;
    struct MUI_CustomClass *db_AbstractIcon;
    struct MUI_CustomClass *db_Icon;
    struct MUI_CustomClass *db_DiskIcon;
    struct MUI_CustomClass *db_DrawerIcon;
    struct MUI_CustomClass *db_TrashcanIcon;
    struct MUI_CustomClass *db_ToolIcon;
    struct MUI_CustomClass *db_ProjectIcon;
    struct MUI_CustomClass *db_IconObserver;
    struct MUI_CustomClass *db_ContainerIconObserver;
    struct MUI_CustomClass *db_DiskIconObserver;
    struct MUI_CustomClass *db_DrawerIconObserver;
    struct MUI_CustomClass *db_ToolIconObserver;
    struct MUI_CustomClass *db_ProjectIconObserver;
    struct MUI_CustomClass *db_TrashcanIconObserver;
    struct MUI_CustomClass *db_DesktopObserver;
    struct MUI_CustomClass *db_Desktop;

/*
   these will be moved into a new desktop context area 
 */
    Class   *db_DefaultWindow;
    struct TagItem *db_DefaultWindowArguments;

    struct SignalSemaphore db_BaseMutex;
    struct SignalSemaphore db_HandlerSafety;

    struct MsgPort *db_HandlerPort;

    BOOL            db_libsOpen;

// TEMPORARY!
    struct List     db_OperationList;
    struct MUI_CustomClass *db_Operation;

// END TEMPORARY!
};

extern struct DesktopBase *DesktopBase;

#    define init(dummybase, segList) \
AROS_LC2(LIBBASETYPEPTR, init, AROS_LHA(LIBBASETYPEPTR, BASENAME, D0), AROS_LHA(BPTR, segList, A0), struct ExecBase *, SysBase, 0, BASENAME)

#    define open(version) \
AROS_LC1(LIBBASETYPEPTR, open, AROS_LHA(ULONG, version, D0), LIBBASETYPEPTR, LIBBASE, 1, BASENAME)

#    define close() \
AROS_LC0(BPTR, close, LIBBASETYPEPTR, LIBBASE, 2, BASENAME)

#    define expunge() \
AROS_LC0(BPTR, expunge, LIBBASETYPEPTR, LIBBASE, 3, BASENAME)

#    define null() \
AROS_LC0(int, null, LIBBASETYPEPTR, LIBBASE, 4, BASENAME)

#    define add(a, b) \
AROS_LC2(ULONG, add, AROS_LHA(ULONG,a,D0), AROS_LHA(ULONG,b,D1), struct DesktopBase *,DesktopBase,5,Desktop)

#    define asl(a, b) \
AROS_LC2(ULONG, asl, AROS_LHA(ULONG,a,D0), AROS_LHA(ULONG,b,D1), struct DesktopBase *,DesktopBase,6,Desktop)

#    define SysBase DesktopBase->db_SysBase
#    define DOSBase DesktopBase->db_DOSBase
#    define GfxBase DesktopBase->db_GfxBase
#    define IntuitionBase DesktopBase->db_IntuitionBase
#    define LayersBase DesktopBase->db_LayersBase
#    define UtilityBase DesktopBase->db_UtilityBase
#    define MUIMasterBase DesktopBase->db_MUIMasterBase
#    define InputBase DesktopBase->db_InputBase
#    define IconBase DesktopBase->db_IconBase
#    define IconContainer DesktopBase->db_IconContainer
#    define IconContainerObserver DesktopBase->db_IconContainerObserver
#    define DiskIcon DesktopBase->db_DiskIcon
#    define DrawerIcon DesktopBase->db_DrawerIcon
#    define TrashcanIcon DesktopBase->db_TrashcanIcon
#    define ToolIcon DesktopBase->db_ToolIcon
#    define ProjectIcon DesktopBase->db_ProjectIcon
#    define IconObserver DesktopBase->db_IconObserver
#    define DiskIconObserver DesktopBase->db_DiskIconObserver
#    define DrawerIconObserver DesktopBase->db_DrawerIconObserver
#    define ToolIconObserver DesktopBase->db_ToolIconObserver
#    define ProjectIconObserver DesktopBase->db_ProjectIconObserver
#    define TrashcanIconObserver DesktopBase->db_TrashcanIconObserver
#    define DesktopObserver DesktopBase->db_DesktopObserver

#endif /* DESKTOP_INTERN_H */
