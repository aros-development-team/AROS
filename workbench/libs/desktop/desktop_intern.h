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
#    include <intuition/classes.h>

#    include <libcore/base.h>

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
    struct LibHeader db_LibHeader;

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

#    define add(a, b) \
AROS_LC2(ULONG, add, AROS_LHA(ULONG,a,D0), AROS_LHA(ULONG,b,D1), struct DesktopBase *,DesktopBase,5,Desktop)

#    define asl(a, b) \
AROS_LC2(ULONG, asl, AROS_LHA(ULONG,a,D0), AROS_LHA(ULONG,b,D1), struct DesktopBase *,DesktopBase,6,Desktop)

#    define SysBase ((struct LibHeader *)DesktopBase)->lh_SysBase
#    define DOSBase ((struct DesktopBase *)DesktopBase)->db_DOSBase
#    define GfxBase ((struct DesktopBase *)DesktopBase)->db_GfxBase
#    define IntuitionBase ((struct DesktopBase *)DesktopBase)->db_IntuitionBase
#    define LayersBase ((struct DesktopBase *)DesktopBase)->db_LayersBase
#    define UtilityBase ((struct DesktopBase *)DesktopBase)->db_UtilityBase
#    define MUIMasterBase ((struct DesktopBase *)DesktopBase)->db_MUIMasterBase
#    define InputBase ((struct DesktopBase *)DesktopBase)->db_InputBase
#    define IconBase ((struct DesktopBase *)DesktopBase)->db_IconBase
#    define IconContainer ((struct DesktopBase *)DesktopBase)->db_IconContainer
#    define IconContainerObserver ((struct DesktopBase *)DesktopBase)->db_IconContainerObserver
#    define DiskIcon ((struct DesktopBase *)DesktopBase)->db_DiskIcon
#    define DrawerIcon ((struct DesktopBase *)DesktopBase)->db_DrawerIcon
#    define TrashcanIcon ((struct DesktopBase *)DesktopBase)->db_TrashcanIcon
#    define ToolIcon ((struct DesktopBase *)DesktopBase)->db_ToolIcon
#    define ProjectIcon ((struct DesktopBase *)DesktopBase)->db_ProjectIcon
#    define IconObserver ((struct DesktopBase *)DesktopBase)->db_IconObserver
#    define DiskIconObserver ((struct DesktopBase *)DesktopBase)->db_DiskIconObserver
#    define DrawerIconObserver ((struct DesktopBase *)DesktopBase)->db_DrawerIconObserver
#    define ToolIconObserver ((struct DesktopBase *)DesktopBase)->db_ToolIconObserver
#    define ProjectIconObserver ((struct DesktopBase *)DesktopBase)->db_ProjectIconObserver
#    define TrashcanIconObserver ((struct DesktopBase *)DesktopBase)->db_TrashcanIconObserver
#    define DesktopObserver ((struct DesktopBase *)DesktopBase)->db_DesktopObserver

#endif /* DESKTOP_INTERN_H */
