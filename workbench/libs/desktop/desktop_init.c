/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <aros/libcall.h>
#include <libraries/desktop.h>
#include <libraries/mui.h>

#include <proto/exec.h>
#include <proto/muimaster.h>

#include "desktop_intern.h"
#include LC_LIBDEFS_FILE
#include "initstruct.h"
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

#include "desktop_intern_protos.h"

#include <stddef.h>

#define DEBUG 1
#include <aros/debug.h>

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const LIBFUNCTABLE[];
extern const struct inittable datatable;

extern struct DesktopBase *AROS_SLIB_ENTRY(init,Desktop)();
extern struct DesktopBase *AROS_SLIB_ENTRY(open,Desktop)();
extern BPTR AROS_SLIB_ENTRY(close,Desktop)();
extern BPTR AROS_SLIB_ENTRY(expunge,Desktop)();
extern int AROS_SLIB_ENTRY(null,Desktop)();
extern ULONG AROS_SLIB_ENTRY(add,Desktop)();
extern ULONG AROS_SLIB_ENTRY(asl,Desktop)();

extern const char end;

int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&end,
    RTF_AUTOINIT,
    1,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[]="desktop.library";

const char version[]="$VER: desktop.library 41.1 (29.08.02)\n\015";

const APTR inittabl[4]=
{
    (APTR)sizeof(struct DesktopBase),
    (APTR)LIBFUNCTABLE,
    (APTR)&datatable,
    &AROS_SLIB_ENTRY(init,Desktop)
};

struct inittable
{
    S_CPYO(1,1,B);
    S_CPYO(2,1,L);
    S_CPYO(3,1,B);
    S_CPYO(4,1,W);
    S_CPYO(5,1,W);
    S_CPYO(6,1,L);
    S_END (end);
};

#define O(n) offsetof(struct DesktopBase,n)

const struct inittable datatable=
{
    { { I_CPYO(1,B,O(db_Library.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(db_Library.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(db_Library.lib_Flags       )), { LIBF_SUMUSED|LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(db_Library.lib_Version     )), { 1 } } },
    { { I_CPYO(1,W,O(db_Library.lib_Revision    )), { 0 } } },
    { { I_CPYO(1,L,O(db_Library.lib_IdString    )), { (IPTR)&version[6] } } },
    I_END ()
};

#undef O

struct DesktopBase *DesktopBase;

#undef SysBase

AROS_LH2(struct DesktopBase *, init,
 AROS_LHA(struct DesktopBase *, desktopbase, D0),
 AROS_LHA(BPTR,               segList,   A0),
       struct ExecBase *, SysBase, 0, BASENAME)
{
    AROS_LIBFUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

    DesktopBase=desktopbase;

    InitSemaphore(&DesktopBase->db_BaseMutex);
    InitSemaphore(&DesktopBase->db_HandlerSafety);

    D(bug("*** Entering DesktopBase::init...\n"));

    DesktopBase->db_libsOpen=FALSE;

    /* Store arguments */
    DesktopBase->db_SysBase=SysBase;
    DesktopBase->db_SegList=segList;
    DesktopBase->db_HandlerPort=NULL;
    /* these will be moved into a new DesktopContext area */
    DesktopBase->db_DefaultWindow=NULL;
    DesktopBase->db_DefaultWindowArguments=NULL;

    // TEMPORARY!  see note in DesktopOperation struct, in desktop_intern.h
    NewList(&DesktopBase->db_OperationList);
    // END TEMPORARY!

    D(bug("*** Exitiing DesktopBase::init...\n"));

    /* You would return NULL here if the init failed. */
    return DesktopBase;
    AROS_LIBFUNC_EXIT
}

/* Use this from now on */
#ifdef SysBase
#undef SysBase
#endif
#define SysBase DesktopBase->db_SysBase

AROS_LH1(struct DesktopBase *, open,
 AROS_LHA(ULONG, version, D0),
       struct DesktopBase *, desktopbase, 1, BASENAME)
{
    AROS_LIBFUNC_INIT

    struct DesktopOperation *dob;
    struct List *subList;
    /*
    This function is single-threaded by exec by calling Forbid.
    If you break the Forbid() another task may enter this function
    at the same time. Take care.
    */

    D(bug("*** Entered DesktopBase::open...\n"));

    ObtainSemaphore(&DesktopBase->db_BaseMutex);

    /* Keep compiler happy */
    version=0;

    /* I have one more opener. */
    DesktopBase->db_Library.lib_OpenCnt++;
    DesktopBase->db_Library.lib_Flags&=~LIBF_DELEXP;

    if(DesktopBase->db_libsOpen==FALSE)
    {
        // Any of these could potentially break the Forbid(),
        // so we have a semaphore
        DesktopBase->db_DOSBase=OpenLibrary("dos.library", 0);
        if(!DesktopBase->db_DOSBase)
            return NULL;

        DesktopBase->db_GfxBase=OpenLibrary("graphics.library", 0);
        if(!DesktopBase->db_GfxBase)
            return NULL;

        DesktopBase->db_IntuitionBase=OpenLibrary("intuition.library", 0);
        if(!DesktopBase->db_IntuitionBase)
            return NULL;

        DesktopBase->db_LayersBase=OpenLibrary("layers.library", 0);
        if(!DesktopBase->db_LayersBase)
            return NULL;

        DesktopBase->db_UtilityBase=OpenLibrary("utility.library", 0);
        if(!DesktopBase->db_UtilityBase)
            return NULL;

        DesktopBase->db_IconBase=OpenLibrary("icon.library", 0);
        if(!DesktopBase->db_IconBase)
            return NULL;

        DesktopBase->db_MUIMasterBase=OpenLibrary("muimaster.library", 0);
        if(!DesktopBase->db_MUIMasterBase)
            return NULL;

        DesktopBase->db_InputIO=AllocVec(sizeof(struct IORequest), MEMF_ANY);
        if(OpenDevice("input.device", NULL, (struct IORequest*)DesktopBase->db_InputIO, NULL))
            return NULL;
        DesktopBase->db_InputBase=(struct Library*)DesktopBase->db_InputIO->io_Device;

        DesktopBase->db_Presentation=MUI_CreateCustomClass(NULL, MUIC_Area, NULL, sizeof(struct PresentationClassData), presentationDispatcher);
        if(!DesktopBase->db_Presentation)
            return NULL;

        DesktopBase->db_IconContainer=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Presentation, sizeof(struct IconContainerClassData), iconContainerDispatcher);
        if(!DesktopBase->db_IconContainer)
            return NULL;

        DesktopBase->db_Observer=MUI_CreateCustomClass(NULL, MUIC_Notify, NULL, sizeof(struct ObserverClassData), observerDispatcher);
        if(!DesktopBase->db_Observer)
            return NULL;

        DesktopBase->db_IconObserver=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Observer, sizeof(struct IconObserverClassData), iconObserverDispatcher);
        if(!DesktopBase->db_IconObserver)
            return NULL;

        DesktopBase->db_ContainerIconObserver=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_IconObserver, sizeof(struct ContainerIconObserverClassData), containerIconObserverDispatcher);
        if(!DesktopBase->db_ContainerIconObserver)
            return NULL;

        DesktopBase->db_DiskIconObserver=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_ContainerIconObserver, sizeof(struct DiskIconObserverClassData), diskIconObserverDispatcher);
        if(!DesktopBase->db_DiskIconObserver)
            return NULL;

        DesktopBase->db_DrawerIconObserver=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_ContainerIconObserver, sizeof(struct DrawerIconObserverClassData), drawerIconObserverDispatcher);
        if(!DesktopBase->db_DrawerIconObserver)
            return NULL;

        DesktopBase->db_ToolIconObserver=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_IconObserver, sizeof(struct ToolIconObserverClassData), toolIconObserverDispatcher);
        if(!DesktopBase->db_ToolIconObserver)
            return NULL;

        DesktopBase->db_ProjectIconObserver=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_IconObserver, sizeof(struct ProjectIconObserverClassData), projectIconObserverDispatcher);
        if(!DesktopBase->db_ProjectIconObserver)
            return NULL;

        DesktopBase->db_TrashcanIconObserver=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_IconObserver, sizeof(struct TrashcanIconObserverClassData), trashcanIconObserverDispatcher);
        if(!DesktopBase->db_TrashcanIconObserver)
            return NULL;

        DesktopBase->db_IconContainerObserver=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Observer, sizeof(struct IconContainerObserverClassData), iconContainerObserverDispatcher);
        if(!DesktopBase->db_IconContainerObserver)
            return NULL;

        DesktopBase->db_DesktopObserver=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Observer, sizeof(struct DesktopObserverClassData), desktopObserverDispatcher);
        if(!DesktopBase->db_DesktopObserver)
            return NULL;

        DesktopBase->db_Icon=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Presentation, sizeof(struct IconClassData), iconDispatcher);
        if(!DesktopBase->db_Icon)
            return NULL;

        DesktopBase->db_DiskIcon=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Icon, sizeof(struct DiskIconClassData), diskIconDispatcher);
        if(!DesktopBase->db_DiskIcon)
            return NULL;

        DesktopBase->db_DrawerIcon=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Icon, sizeof(struct DrawerIconClassData), drawerIconDispatcher);
        if(!DesktopBase->db_DrawerIcon)
            return NULL;

        DesktopBase->db_TrashcanIcon=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Icon, sizeof(struct TrashcanIconClassData), trashcanIconDispatcher);
        if(!DesktopBase->db_TrashcanIcon)
            return NULL;

        DesktopBase->db_ToolIcon=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Icon, sizeof(struct ToolIconClassData), toolIconDispatcher);
        if(!DesktopBase->db_ToolIcon)
            return NULL;

        DesktopBase->db_ProjectIcon=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Icon, sizeof(struct ProjectIconClassData), projectIconDispatcher);
        if(!DesktopBase->db_ProjectIcon)
            return NULL;

        DesktopBase->db_Desktop=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_IconContainer, sizeof(struct DesktopClassData), desktopDispatcher);
        if(!DesktopBase->db_Desktop)
            return NULL;

        // TEMPORARY!  see note in DesktopOperation struct, in desktop_intern.h
        DesktopBase->db_Operation=MUI_CreateCustomClass(NULL, MUIC_Notify, NULL, sizeof(struct OperationClassData), operationDispatcher);
        if(!DesktopBase->db_Operation)
            return NULL;

        //1
        dob=AllocVec(sizeof(struct DesktopOperation), MEMF_ANY);
        dob->do_Code=(DOC_ICONOP | 1);
        dob->do_Name="Open...";
        dob->do_MutualExclude=0;
        dob->do_Flags=0;
        dob->do_Number=1;
        NewList(&dob->do_SubItems);
        dob->do_Impl=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Operation, sizeof(struct InternalIconOpsClassData), internalIconOpsDispatcher);
        AddTail(&DesktopBase->db_OperationList, (struct Node*)dob);

        //2
        dob=AllocVec(sizeof(struct DesktopOperation), MEMF_ANY);
        dob->do_Code=(DOC_WINDOWOP | 1);
        dob->do_Name="Close";
        dob->do_MutualExclude=0;
        dob->do_Flags=0;
        dob->do_Number=2;
        NewList(&dob->do_SubItems);
        dob->do_Impl=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Operation, sizeof(struct InternalWindowOpsClassData), internalWindowOpsDispatcher);
        AddTail(&DesktopBase->db_OperationList, (struct Node*)dob);

        //3
        dob=AllocVec(sizeof(struct DesktopOperation), MEMF_ANY);
        dob->do_Code=(DOC_WINDOWOP | 2);
        dob->do_Name="View by";
        dob->do_MutualExclude=0;
        dob->do_Flags=0;
        dob->do_Number=3;
        NewList(&dob->do_SubItems);
        subList=&dob->do_SubItems;
        dob->do_Impl=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Operation, sizeof(struct InternalWindowOpsClassData), internalWindowOpsDispatcher);
        AddTail(&DesktopBase->db_OperationList, (struct Node*)dob);

        //4
        dob=AllocVec(sizeof(struct DesktopOperation), MEMF_ANY);
        dob->do_Code=(DOC_WINDOWOP | 3);
        dob->do_Name="Large icons";
        dob->do_MutualExclude=(1 << 5) | (1 << 6);
        dob->do_Flags=DOF_CHECKED | DOF_CHECKABLE | DOF_MUTUALEXCLUDE;
        dob->do_Number=4;
        NewList(&dob->do_SubItems);
        dob->do_Impl=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Operation, sizeof(struct InternalWindowOpsClassData), internalWindowOpsDispatcher);
        AddTail(subList, (struct Node*)dob);

        //5
        dob=AllocVec(sizeof(struct DesktopOperation), MEMF_ANY);
        dob->do_Code=(DOC_WINDOWOP | 4);
        dob->do_Name="Small icons";
        dob->do_MutualExclude=(1 << 4) | (1 << 6);
        dob->do_Flags=DOF_CHECKABLE | DOF_MUTUALEXCLUDE;
        dob->do_Number=5;
        NewList(&dob->do_SubItems);
        dob->do_Impl=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Operation, sizeof(struct InternalWindowOpsClassData), internalWindowOpsDispatcher);
        AddTail(subList, (struct Node*)dob);

        //6
        dob=AllocVec(sizeof(struct DesktopOperation), MEMF_ANY);
        dob->do_Code=(DOC_WINDOWOP | 5);
        dob->do_Name="Detail";
        dob->do_MutualExclude=(1 << 4) | (1 << 5);
        dob->do_Flags=DOF_CHECKABLE | DOF_MUTUALEXCLUDE;
        dob->do_Number=6;
        NewList(&dob->do_SubItems);
        dob->do_Impl=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Operation, sizeof(struct InternalWindowOpsClassData), internalWindowOpsDispatcher);
        AddTail(subList, (struct Node*)dob);

        //7
        dob=AllocVec(sizeof(struct DesktopOperation), MEMF_ANY);
        dob->do_Code=(DOC_DESKTOPOP | 1);
        dob->do_Name="Quit";
        dob->do_MutualExclude=0;
        dob->do_Flags=0;
        dob->do_Number=7;
        NewList(&dob->do_SubItems);
        dob->do_Impl=MUI_CreateCustomClass(NULL, NULL, DesktopBase->db_Operation, sizeof(struct InternalDesktopOpsClassData), internalDesktopOpsDispatcher);
        AddTail(&DesktopBase->db_OperationList, (struct Node*)dob);
        // END TEMPORARY!

        DesktopBase->db_libsOpen=TRUE;
    }

    if(!DesktopBase->db_HandlerPort)
        startDesktopHandler();

    handlerAddUser();

    D(bug("*** Exiting DesktopBase::open...\n"));

    ReleaseSemaphore(&DesktopBase->db_BaseMutex);

    /* You would return NULL if the open failed. */
    return DesktopBase;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, close, struct DesktopBase *, DesktopBase, 2, Desktop)
{
    AROS_LIBFUNC_INIT
    /*
    This function is single-threaded by exec by calling Forbid.
    If you break the Forbid() another task may enter this function
    at the same time. Take care.
    */

    D(bug("*** Entering DesktopBase::close...\n"));

    handlerSubUser();

    /* I have one fewer opener. */
    if(!--DesktopBase->db_Library.lib_OpenCnt)
    {
        /* Delayed expunge pending? */
        if(DesktopBase->db_Library.lib_Flags&LIBF_DELEXP)
            /* Then expunge the library */
            /* At this point the handler should have exited
               on its own, and completed the final CloseLibrary() */
            return expunge();
    }

    D(bug("*** Exiting DesktopBase::close...\n"));

    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, expunge, struct DesktopBase *, DesktopBase, 3, BASENAME)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    struct DesktopOperation *dob;
    /*
    This function is single-threaded by exec by calling Forbid.
    Never break the Forbid() or strange things might happen.
    */
    D(bug("*** Entering DesktopBase::expunge...\n"));

    /* Test for openers. */
    if(DesktopBase->db_Library.lib_OpenCnt)
    {
        /* Set the delayed expunge flag and return. */
        DesktopBase->db_Library.lib_Flags|=LIBF_DELEXP;
        return 0;
    }

    // TEMPORARY!
    dob=(struct DesktopOperation*)DesktopBase->db_OperationList.lh_Head;
    while(dob->do_Node.ln_Succ)
    {
        if(dob->do_Impl)
            MUI_DeleteCustomClass(dob->do_Impl);
        dob=(struct DesktopOperation*)dob->do_Node.ln_Succ;
    }
    FreeVec(dob);

    if(DesktopBase->db_Operation)
        MUI_DeleteCustomClass(DesktopBase->db_Operation);
    // END TEMPORARY

    if(DesktopBase->db_DiskIconObserver)
        MUI_DeleteCustomClass(DesktopBase->db_DiskIconObserver);
    if(DesktopBase->db_DrawerIconObserver)
        MUI_DeleteCustomClass(DesktopBase->db_DrawerIconObserver);
    if(DesktopBase->db_ToolIconObserver)
        MUI_DeleteCustomClass(DesktopBase->db_ToolIconObserver);
    if(DesktopBase->db_Desktop)
        MUI_DeleteCustomClass(DesktopBase->db_Desktop);
    if(DesktopBase->db_ProjectIconObserver)
        MUI_DeleteCustomClass(DesktopBase->db_ProjectIconObserver);
    if(DesktopBase->db_TrashcanIconObserver)
        MUI_DeleteCustomClass(DesktopBase->db_TrashcanIconObserver);
    if(DesktopBase->db_IconObserver)
        MUI_DeleteCustomClass(DesktopBase->db_IconObserver);
    if(DesktopBase->db_DesktopObserver)
        MUI_DeleteCustomClass(DesktopBase->db_DesktopObserver);
    if(DesktopBase->db_IconContainerObserver)
        MUI_DeleteCustomClass(DesktopBase->db_IconContainerObserver);
    if(DesktopBase->db_ContainerIconObserver)
        MUI_DeleteCustomClass(DesktopBase->db_ContainerIconObserver);
    if(DesktopBase->db_Observer)
        MUI_DeleteCustomClass(DesktopBase->db_Observer);
    if(DesktopBase->db_ToolIcon)
        MUI_DeleteCustomClass(DesktopBase->db_ToolIcon);
    if(DesktopBase->db_DiskIcon)
        MUI_DeleteCustomClass(DesktopBase->db_DiskIcon);
    if(DesktopBase->db_DrawerIcon)
        MUI_DeleteCustomClass(DesktopBase->db_DrawerIcon);
    if(DesktopBase->db_TrashcanIcon)
        MUI_DeleteCustomClass(DesktopBase->db_TrashcanIcon);
    if(DesktopBase->db_ProjectIcon)
        MUI_DeleteCustomClass(DesktopBase->db_ProjectIcon);
    if(DesktopBase->db_Icon)
        MUI_DeleteCustomClass(DesktopBase->db_Icon);
    if(DesktopBase->db_IconContainer)
        MUI_DeleteCustomClass(DesktopBase->db_IconContainer);
    if(DesktopBase->db_Presentation)
        MUI_DeleteCustomClass(DesktopBase->db_Presentation);

    if(DesktopBase->db_InputBase)
        CloseDevice(&DesktopBase->db_InputIO);

    if(DesktopBase->db_MUIMasterBase)
        CloseLibrary(DesktopBase->db_MUIMasterBase);
    if(DesktopBase->db_IconBase)
        CloseLibrary(DesktopBase->db_IconBase);
    if(DesktopBase->db_UtilityBase)
        CloseLibrary(DesktopBase->db_UtilityBase);
    if(DesktopBase->db_LayersBase)
        CloseLibrary(DesktopBase->db_LayersBase);
    if(DesktopBase->db_IntuitionBase)
        CloseLibrary(DesktopBase->db_IntuitionBase);
    if(DesktopBase->db_GfxBase)
        CloseLibrary(DesktopBase->db_GfxBase);
    if(DesktopBase->db_DOSBase)
        CloseLibrary(DesktopBase->db_DOSBase);

    /* Get rid of the library. Remove it from the list. */
    Remove(&DesktopBase->db_Library.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=DesktopBase->db_SegList;

    /* Free the memory. */
    FreeMem((char *)DesktopBase-DesktopBase->db_Library.lib_NegSize,
        DesktopBase->db_Library.lib_NegSize+DesktopBase->db_Library.lib_PosSize);

    D(bug("*** Exiting DesktopBase::expunge...\n"));

    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(int, null, struct DesktopBase *, DesktopBase, 4, Desktop)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

AROS_LH2I(ULONG, add,
    AROS_LHA(ULONG,a,D0),
    AROS_LHA(ULONG,b,D1),
    struct DesktopBase *,DesktopBase,5,Desktop)
{
    AROS_LIBFUNC_INIT
    return a+b;
    AROS_LIBFUNC_EXIT
}

AROS_LH2I(ULONG, asl,
    AROS_LHA(ULONG,a,D0),
    AROS_LHA(ULONG,b,D1),
    struct DesktopBase *,DesktopBase,6,Desktop)
{
    AROS_LIBFUNC_INIT
    return a<<b;
    AROS_LIBFUNC_EXIT
}

const char end=0;
