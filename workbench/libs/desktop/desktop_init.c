/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
   $Id$ 
 */

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>
#include <libraries/desktop.h>
#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/muimaster.h>

#include "desktop_intern.h"
#include LC_LIBDEFS_FILE
#include "support.h"

#include "observer.h"
#include "presentation.h"
#include "iconcontainerclass.h"
#include "iconclass.h"
#include "diskiconclass.h"
#include "drawericonclass.h"
#include "tooliconclass.h"
#include "projecticonclass.h"
#include "trashcaniconclass.h"
#include "iconcontainerobserver.h"
#include "iconobserver.h"
#include "diskiconobserver.h"
#include "drawericonobserver.h"
#include "tooliconobserver.h"
#include "projecticonobserver.h"
#include "trashcaniconobserver.h"
#include "desktopobserver.h"
#include "operationclass.h"
#include "internaliconopsclass.h"
#include "internalwindowopsclass.h"
#include "internaldesktopopsclass.h"
#include "desktopclass.h"
#include "containericonobserver.h"
#include "abstracticon.h"
#include "abstracticoncontainer.h"

#include "desktop_intern_protos.h"

#define DEBUG 1
#include <aros/debug.h>

struct DesktopBase *DesktopBase;

AROS_SET_LIBFUNC(Init, LIBBASETYPE, desktopbase)
{
    AROS_SET_LIBFUNC_INIT
    
/*
   This function is single-threaded by exec by calling Forbid. If you break
   the Forbid() another task may enter this function at the same time. Take
   care. 
 */
    DesktopBase = desktopbase;

    InitSemaphore(&DesktopBase->db_BaseMutex);
    InitSemaphore(&DesktopBase->db_HandlerSafety);

    D(bug("*** Entering DesktopBase::init...\n"));

    DesktopBase->db_libsOpen = FALSE;
    DesktopBase->db_HandlerPort = NULL;
/*
   these will be moved into a new DesktopContext area 
 */
    DesktopBase->db_DefaultWindow = NULL;
    DesktopBase->db_DefaultWindowArguments = NULL;

// TEMPORARY! see note in DesktopOperation struct, in desktop_intern.h
    NEWLIST(&DesktopBase->db_OperationList);
// END TEMPORARY!

    D(bug("*** Exitiing DesktopBase::init...\n"));

/*
   You would return NULL here if the init failed. 
 */
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}


AROS_SET_LIBFUNC(Open, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    
    struct DesktopOperation *dob;
    struct List    *subList;

/*
   This function is single-threaded by exec by calling Forbid. If you break
   the Forbid() another task may enter this function at the same time. Take
   care. 
 */

    D(bug("*** Entered DesktopBase::open...\n"));

    ObtainSemaphore(&DesktopBase->db_BaseMutex);

    if (DesktopBase->db_libsOpen == FALSE)
    {
    // Any of these could potentially break the Forbid(),
    // so we have a semaphore
        DesktopBase->db_InputIO =
            AllocVec(sizeof(struct IORequest), MEMF_ANY);
        if (OpenDevice
            ("input.device", NULL,
             (struct IORequest *) DesktopBase->db_InputIO, NULL))
            return FALSE;
        DesktopBase->db_InputBase =
            (struct Library *) DesktopBase->db_InputIO->io_Device;

        DesktopBase->db_Presentation =
            MUI_CreateCustomClass(NULL, MUIC_Area, NULL,
                                  sizeof(struct PresentationClassData),
                                  presentationDispatcher);
        if (!DesktopBase->db_Presentation)
            return FALSE;

        DesktopBase->db_AbstractIconContainer =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Presentation,
                                  sizeof(struct AbstractIconContainerData),
                                  abstractIconContainerDispatcher);
        if (!DesktopBase->db_AbstractIconContainer)
            return FALSE;



        DesktopBase->db_IconContainer =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_AbstractIconContainer,
                                  sizeof(struct IconContainerClassData),
                                  iconContainerDispatcher);
        if (!DesktopBase->db_IconContainer)
            return FALSE;


        DesktopBase->db_Observer =
            MUI_CreateCustomClass(NULL, MUIC_Notify, NULL,
                                  sizeof(struct ObserverClassData),
                                  observerDispatcher);
        if (!DesktopBase->db_Observer)
            return FALSE;

        DesktopBase->db_IconObserver =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Observer,
                                  sizeof(struct IconObserverClassData),
                                  iconObserverDispatcher);
        if (!DesktopBase->db_IconObserver)
            return FALSE;

        DesktopBase->db_ContainerIconObserver =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_IconObserver,
                                  sizeof(struct
                                         ContainerIconObserverClassData),
                                  containerIconObserverDispatcher);
        if (!DesktopBase->db_ContainerIconObserver)
            return FALSE;

        DesktopBase->db_DiskIconObserver =
            MUI_CreateCustomClass(NULL, NULL,
                                  DesktopBase->db_ContainerIconObserver,
                                  sizeof(struct DiskIconObserverClassData),
                                  diskIconObserverDispatcher);
        if (!DesktopBase->db_DiskIconObserver)
            return FALSE;

        DesktopBase->db_DrawerIconObserver =
            MUI_CreateCustomClass(NULL, NULL,
                                  DesktopBase->db_ContainerIconObserver,
                                  sizeof(struct DrawerIconObserverClassData),
                                  drawerIconObserverDispatcher);
        if (!DesktopBase->db_DrawerIconObserver)
            return FALSE;

        DesktopBase->db_ToolIconObserver =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_IconObserver,
                                  sizeof(struct ToolIconObserverClassData),
                                  toolIconObserverDispatcher);
        if (!DesktopBase->db_ToolIconObserver)
            return FALSE;

        DesktopBase->db_ProjectIconObserver =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_IconObserver,
                                  sizeof(struct ProjectIconObserverClassData),
                                  projectIconObserverDispatcher);
        if (!DesktopBase->db_ProjectIconObserver)
            return FALSE;

        DesktopBase->db_TrashcanIconObserver =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_IconObserver,
                                  sizeof(struct
                                         TrashcanIconObserverClassData),
                                  trashcanIconObserverDispatcher);
        if (!DesktopBase->db_TrashcanIconObserver)
            return FALSE;

        DesktopBase->db_IconContainerObserver =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Observer,
                                  sizeof(struct
                                         IconContainerObserverClassData),
                                  iconContainerObserverDispatcher);
        if (!DesktopBase->db_IconContainerObserver)
            return FALSE;

        DesktopBase->db_DesktopObserver =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Observer,
                                  sizeof(struct DesktopObserverClassData),
                                  desktopObserverDispatcher);
        if (!DesktopBase->db_DesktopObserver)
            return FALSE;

        DesktopBase->db_AbstractIcon=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Presentation, sizeof(struct AbstractIconClassData), abstractIconDispatcher);
        if(!DesktopBase->db_AbstractIcon)
            return FALSE;

        DesktopBase->db_Icon =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_AbstractIcon,
                                  sizeof(struct IconClassData),
                                  iconDispatcher);
        if (!DesktopBase->db_Icon)
            return FALSE;

        DesktopBase->db_DiskIcon =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Icon,
                                  sizeof(struct DiskIconClassData),
                                  diskIconDispatcher);
        if (!DesktopBase->db_DiskIcon)
            return FALSE;

        DesktopBase->db_DrawerIcon =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Icon,
                                  sizeof(struct DrawerIconClassData),
                                  drawerIconDispatcher);
        if (!DesktopBase->db_DrawerIcon)
            return FALSE;

        DesktopBase->db_TrashcanIcon =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Icon,
                                  sizeof(struct TrashcanIconClassData),
                                  trashcanIconDispatcher);
        if (!DesktopBase->db_TrashcanIcon)
            return FALSE;

        DesktopBase->db_ToolIcon =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Icon,
                                  sizeof(struct ToolIconClassData),
                                  toolIconDispatcher);
        if (!DesktopBase->db_ToolIcon)
            return FALSE;

        DesktopBase->db_ProjectIcon =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Icon,
                                  sizeof(struct ProjectIconClassData),
                                  projectIconDispatcher);
        if (!DesktopBase->db_ProjectIcon)
            return FALSE;

        DesktopBase->db_Desktop =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_IconContainer,
                                  sizeof(struct DesktopClassData),
                                  desktopDispatcher);
        if (!DesktopBase->db_Desktop)
            return FALSE;

    // TEMPORARY! see note in DesktopOperation struct, in desktop_intern.h
        DesktopBase->db_Operation =
            MUI_CreateCustomClass(NULL, MUIC_Notify, NULL,
                                  sizeof(struct OperationClassData),
                                  operationDispatcher);
        if (!DesktopBase->db_Operation)
            return FALSE;

    // 1
        dob = AllocVec(sizeof(struct DesktopOperation), MEMF_ANY);
        dob->do_Code = (DOC_ICONOP | 1);
        dob->do_Name = "Open...";
        dob->do_MutualExclude = 0;
        dob->do_Flags = 0;
        dob->do_Number = 1;
        NEWLIST(&dob->do_SubItems);
        dob->do_Impl =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Operation,
                                  sizeof(struct InternalIconOpsClassData),
                                  internalIconOpsDispatcher);
        AddTail(&DesktopBase->db_OperationList, (struct Node *) dob);

    // 2
        dob = AllocVec(sizeof(struct DesktopOperation), MEMF_ANY);
        dob->do_Code = (DOC_WINDOWOP | 1);
        dob->do_Name = "Close";
        dob->do_MutualExclude = 0;
        dob->do_Flags = 0;
        dob->do_Number = 2;
        NEWLIST(&dob->do_SubItems);
        dob->do_Impl =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Operation,
                                  sizeof(struct InternalWindowOpsClassData),
                                  internalWindowOpsDispatcher);
        AddTail(&DesktopBase->db_OperationList, (struct Node *) dob);

    // 3
        dob = AllocVec(sizeof(struct DesktopOperation), MEMF_ANY);
        dob->do_Code = (DOC_WINDOWOP | 2);
        dob->do_Name = "View by";
        dob->do_MutualExclude = 0;
        dob->do_Flags = 0;
        dob->do_Number = 3;
        NEWLIST(&dob->do_SubItems);
        subList = &dob->do_SubItems;
        dob->do_Impl =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Operation,
                                  sizeof(struct InternalWindowOpsClassData),
                                  internalWindowOpsDispatcher);
        AddTail(&DesktopBase->db_OperationList, (struct Node *) dob);

    // 4
        dob = AllocVec(sizeof(struct DesktopOperation), MEMF_ANY);
        dob->do_Code = (DOC_WINDOWOP | 3);
        dob->do_Name = "Large icons";
        dob->do_MutualExclude = (1 << 5) | (1 << 6);
        dob->do_Flags = DOF_CHECKED | DOF_CHECKABLE | DOF_MUTUALEXCLUDE;
        dob->do_Number = 4;
        NEWLIST(&dob->do_SubItems);
        dob->do_Impl =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Operation,
                                  sizeof(struct InternalWindowOpsClassData),
                                  internalWindowOpsDispatcher);
        AddTail(subList, (struct Node *) dob);

    // 5
        dob = AllocVec(sizeof(struct DesktopOperation), MEMF_ANY);
        dob->do_Code = (DOC_WINDOWOP | 4);
        dob->do_Name = "Small icons";
        dob->do_MutualExclude = (1 << 4) | (1 << 6);
        dob->do_Flags = DOF_CHECKABLE | DOF_MUTUALEXCLUDE;
        dob->do_Number = 5;
        NEWLIST(&dob->do_SubItems);
        dob->do_Impl =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Operation,
                                  sizeof(struct InternalWindowOpsClassData),
                                  internalWindowOpsDispatcher);
        AddTail(subList, (struct Node *) dob);

    // 6
        dob = AllocVec(sizeof(struct DesktopOperation), MEMF_ANY);
        dob->do_Code = (DOC_WINDOWOP | 5);
        dob->do_Name = "Detail";
        dob->do_MutualExclude = (1 << 4) | (1 << 5);
        dob->do_Flags = DOF_CHECKABLE | DOF_MUTUALEXCLUDE;
        dob->do_Number = 6;
        NEWLIST(&dob->do_SubItems);
        dob->do_Impl =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Operation,
                                  sizeof(struct InternalWindowOpsClassData),
                                  internalWindowOpsDispatcher);
        AddTail(subList, (struct Node *) dob);

    // 7
        dob = AllocVec(sizeof(struct DesktopOperation), MEMF_ANY);
        dob->do_Code = (DOC_DESKTOPOP | 1);
        dob->do_Name = "Quit";
        dob->do_MutualExclude = 0;
        dob->do_Flags = 0;
        dob->do_Number = 7;
        NEWLIST(&dob->do_SubItems);
        dob->do_Impl =
            MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Operation,
                                  sizeof(struct InternalDesktopOpsClassData),
                                  internalDesktopOpsDispatcher);
        AddTail(&DesktopBase->db_OperationList, (struct Node *) dob);
    // END TEMPORARY!

        DesktopBase->db_libsOpen = TRUE;
    }

    if (!DesktopBase->db_HandlerPort)
        startDesktopHandler();

    handlerAddUser();

    D(bug("*** Exiting DesktopBase::open...\n"));

    ReleaseSemaphore(&DesktopBase->db_BaseMutex);

/*
   You would return NULL if the open failed. 
 */
    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}


AROS_SET_LIBFUNC(Close, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    
    /*
       This function is single-threaded by exec by calling Forbid. If you
       break the Forbid() another task may enter this function at the same
       time. Take care. 
     */
        D(bug("*** Entering DesktopBase::close...\n"));

    handlerSubUser();

    D(bug("*** Exiting DesktopBase::close...\n"));

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(Expunge, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    
    struct DesktopOperation *dob;

/*
   This function is single-threaded by exec by calling Forbid. Never break
   the Forbid() or strange things might happen. 
 */
    D(bug("*** Entering DesktopBase::expunge...\n"));

// TEMPORARY!
    dob = (struct DesktopOperation *) DesktopBase->db_OperationList.lh_Head;
    while (dob->do_Node.ln_Succ)
    {
        if (dob->do_Impl)
            MUI_DeleteCustomClass(dob->do_Impl);
        dob = (struct DesktopOperation *) dob->do_Node.ln_Succ;
    }
    FreeVec(dob);

    if (DesktopBase->db_Operation)
        MUI_DeleteCustomClass(DesktopBase->db_Operation);
// END TEMPORARY

    if (DesktopBase->db_DiskIconObserver)
        MUI_DeleteCustomClass(DesktopBase->db_DiskIconObserver);
    if (DesktopBase->db_DrawerIconObserver)
        MUI_DeleteCustomClass(DesktopBase->db_DrawerIconObserver);
    if (DesktopBase->db_ToolIconObserver)
        MUI_DeleteCustomClass(DesktopBase->db_ToolIconObserver);
    if (DesktopBase->db_Desktop)
        MUI_DeleteCustomClass(DesktopBase->db_Desktop);
    if (DesktopBase->db_ProjectIconObserver)
        MUI_DeleteCustomClass(DesktopBase->db_ProjectIconObserver);
    if (DesktopBase->db_TrashcanIconObserver)
        MUI_DeleteCustomClass(DesktopBase->db_TrashcanIconObserver);
    if (DesktopBase->db_IconObserver)
        MUI_DeleteCustomClass(DesktopBase->db_IconObserver);
    if (DesktopBase->db_DesktopObserver)
        MUI_DeleteCustomClass(DesktopBase->db_DesktopObserver);
    if (DesktopBase->db_IconContainerObserver)
        MUI_DeleteCustomClass(DesktopBase->db_IconContainerObserver);
    if (DesktopBase->db_ContainerIconObserver)
        MUI_DeleteCustomClass(DesktopBase->db_ContainerIconObserver);
    if (DesktopBase->db_Observer)
        MUI_DeleteCustomClass(DesktopBase->db_Observer);
    if (DesktopBase->db_ToolIcon)
        MUI_DeleteCustomClass(DesktopBase->db_ToolIcon);
    if (DesktopBase->db_DiskIcon)
        MUI_DeleteCustomClass(DesktopBase->db_DiskIcon);
    if (DesktopBase->db_DrawerIcon)
        MUI_DeleteCustomClass(DesktopBase->db_DrawerIcon);
    if (DesktopBase->db_TrashcanIcon)
        MUI_DeleteCustomClass(DesktopBase->db_TrashcanIcon);
    if (DesktopBase->db_ProjectIcon)
        MUI_DeleteCustomClass(DesktopBase->db_ProjectIcon);
    if (DesktopBase->db_Icon)
        MUI_DeleteCustomClass(DesktopBase->db_Icon);
    if (DesktopBase->db_IconContainer)
        MUI_DeleteCustomClass(DesktopBase->db_IconContainer);
    if (DesktopBase->db_Presentation)
        MUI_DeleteCustomClass(DesktopBase->db_Presentation);

    if (DesktopBase->db_InputBase)
        CloseDevice(&DesktopBase->db_InputIO);

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(Init, 0);
ADD2OPENLIB(Open, 0);
ADD2CLOSELIB(Close, 0);
ADD2EXPUNGELIB(Expunge, 0);
