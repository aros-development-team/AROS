/*
    Copyright (C) 2013-2020, The AROS Development Team.
    $Id: main.c 54142 2017-03-16 01:57:58Z NicJA $
*/

#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <hidd/hidd.h>
#include <hidd/system.h>
#include <hidd/gfx.h>
#include <hidd/storage.h>
#include <hidd/pci.h>
#include <libraries/asl.h>
#include <mui/NListtree_mcc.h>
#include <mui/NListview_mcc.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/sysexp.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "locale.h"
#include "classes.h"

#include "enums.h"

extern void hwEnum(OOP_Object *obj, struct MUI_NListtree_TreeNode *tn);

CONST_STRPTR hwenumfunc_name = "HWEnum.Func";
CONST_STRPTR devicepageclass_name = "DevicePage.Class";
CONST_STRPTR genericwindowclass_name = "GenericWindow.Class";
CONST_STRPTR computerwindowclass_name = "ComputerWindow.Class";

BOOL ClassNameMatches(const char *classA, const char *classB)
{
    if ((!strncmp(classA, classB, strlen(classB))) &&
        (strlen(classA) == strlen(classB)))
    {
        return TRUE;
    }
    return FALSE;
}

/** Null function **/

AROS_LH0(void, Null,
         struct SysexpBase *, SysexpBase, 0, LIB)
{
    AROS_LIBFUNC_INIT

    return;

    AROS_LIBFUNC_EXIT
}

/** Library API functions **/

/*****************************************************************************
    NAME
        void RegisterModule(

    SYNOPSIS
        struct SysexpModule *Module, APTR ModBase)

    FUNCTION
        Register a SysExplorer module for startup, lookup and shutdown.

    INPUTS
        Module - Module descriptor, or NULL for no operation. sem_Node.ln_Name
            must point to a persistent NUL-terminated module identifier.
            sem_Node.ln_Pri sets startup and shutdown order. sem_Startup and
            sem_Shutdown may be NULL.
        ModBase - Open module-library base whose open reference will be closed
            by the host at shutdown.

    RESULT
        None.

    NOTES
        The descriptor is copied; the name and callback pointers are retained.
        The original descriptor may be reused after return, but its referenced
        storage and callback code must remain valid through host shutdown.
        Registration does not open the module or invoke either callback.

        Register once during module initialization, before the host traverses
        its module list. Higher priorities run first; equal priorities retain
        registration order. Shutdown uses the same order as startup, calls
        sem_Shutdown when present, then closes the registered module base.
        Callbacks receive the host SysexpBase.

        Use the host-supplied SysexpBase in task context. Serialize registry
        access; registration and lookup do not acquire locks. There is no
        unregister operation or automatic startup of a late registration.

    BUGS
        Allocation failure is not checked and can cause an invalid memory
        access. Duplicate registrations are accepted and can cause repeated
        callbacks and multiple closes of the same module.

    SEE ALSO
        GetBase(), RegisterBase()

*****************************************************************************/
AROS_LH2(void, RegisterModule,
         AROS_LHA(struct SysexpModule *, Module, A0),
         AROS_LHA(APTR, ModBase, A1),
         struct SysexpBase *, SysexpBase, 5, Sysexp)
{
    AROS_LIBFUNC_INIT

    D(bug("[sysexp.library] %s()\n", __func__));

    if (Module)
    {
        struct SysexpIntModule *IntModule = AllocMem(sizeof(struct SysexpIntModule), MEMF_ANY);
        CopyMem(Module, IntModule, sizeof(struct SysexpModule));
        IntModule->seim_ModuleBase = ModBase;
        Enqueue(&SysexpBase->sesb_Modules, &IntModule->seim_Module.sem_Node);
    }
    return;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        void RegisterBase(

    SYNOPSIS
        CONST_STRPTR BaseID, APTR Base)

    FUNCTION
        Associate an identifier with a pointer in the SysExplorer registry.

    INPUTS
        BaseID - Persistent, non-NULL, NUL-terminated identifier.
        Base - Pointer to publish. NULL is stored as a value.

    RESULT
        None.

    NOTES
        BaseID and Base are retained without copying or acquiring ownership.
        Keep their storage valid while the host can query or use the entry.
        The registry accepts library bases, class pointers and other values;
        the identifier's provider defines the result type.

        New entries are appended. Registering an existing identifier does not
        replace it: GetBase() returns the first registered matching base.
        Generic-base entries take precedence over module names.

        Call with the host-supplied SysexpBase in task context. Serialize
        registration with lookup. No unregister operation is provided.

    BUGS
        Allocation failure is not checked and can cause an invalid memory access.

    SEE ALSO
        GetBase(), RegisterModule()

*****************************************************************************/
AROS_LH2(void, RegisterBase,
         AROS_LHA(CONST_STRPTR, BaseID, A0),
         AROS_LHA(APTR, Base, A1),
         struct SysexpBase *, SysexpBase, 6, Sysexp)
{
    AROS_LIBFUNC_INIT

    D(bug("[sysexp.library] %s()\n", __func__));

    struct SysexpIntBase *IntBase = AllocMem(sizeof(struct SysexpIntBase), MEMF_ANY);
    IntBase->seib_Node.ln_Name = (char *)BaseID;
    IntBase->seib_Base = Base;
    AddTail(&SysexpBase->sesb_GenericBases, &IntBase->seib_Node);

    return;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        APTR GetBase(

    SYNOPSIS
        CONST_STRPTR BaseID)

    FUNCTION
        Find a registered SysExplorer base or module by identifier.

    INPUTS
        BaseID - Non-NULL, NUL-terminated identifier; comparison is case-sensitive.

    RESULT
        Borrowed pointer from the first matching generic-base entry, otherwise
        from the first matching module, or NULL if neither matches. A stored
        NULL base is indistinguishable from absence and prevents module fallback.

    NOTES
        Generic bases are searched in registration order, then modules in
        priority order. Matching requires the complete identifier; there is
        no prefix matching or type validation.

        The call does not open a library or retain the result. Interpret it
        according to the identifier's provider and keep that provider alive.
        Do not close a module-library base obtained only through this lookup.

        Use the host-supplied SysexpBase and serialize access with registration.
        Lookup does not acquire a lock or invoke module callbacks.

    SEE ALSO
        RegisterBase(), RegisterModule()

*****************************************************************************/
AROS_LH1(APTR, GetBase,
         AROS_LHA(CONST_STRPTR, BaseID, A0),
         struct SysexpBase *, SysexpBase, 7, Sysexp)
{
    AROS_LIBFUNC_INIT
    struct SysexpIntModule *IntModule;
    struct SysexpIntBase *IntBase;

    D(bug("[sysexp.library] %s()\n", __func__));

    ForeachNode(&SysexpBase->sesb_GenericBases, IntBase)
    {
        if (!strcmp(IntBase->seib_Node.ln_Name, BaseID))
            return IntBase->seib_Base;
    }
    ForeachNode(&SysexpBase->sesb_Modules, IntModule)
    {
        if (!strcmp(IntModule->seim_Module.sem_Node.ln_Name, BaseID))
            return IntModule->seim_ModuleBase;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        BOOL RegisterClassHandler(

    SYNOPSIS
        CONST_STRPTR classid, BYTE pri, struct MUI_CustomClass *customwinclass, CLASS_ENUMFUNC enumfunc, CLASS_VALIDFUNC validfunc)

    FUNCTION
        Register or replace a SysExplorer handler for an OOP class.

    INPUTS
        classid - Persistent, non-NULL, NUL-terminated class identifier.
        pri - Signed priority. Higher-priority handlers are considered first.
        customwinclass - Borrowed MUI custom class for the property window,
            or NULL for no property window.
        enumfunc - CLASS_ENUMFUNC callback, or NULL for no child enumeration.
        validfunc - CLASS_VALIDFUNC callback, or NULL to accept the object.

    RESULT
        TRUE if the handler was inserted or replaced; FALSE on allocation
        failure or if an existing handler has a non-NULL enumerator other
        than the host's HWEnum.Func callback. A NULL existing enumerator may
        be replaced.

    NOTES
        The class identifier, custom class and callbacks are retained. Keep
        them valid while the host uses the handler. No callback is invoked
        by registration. There is no unregister operation.

        A replacement changes all fields and reorders the entry by priority.
        Equal priorities retain insertion order; a replacement goes after
        existing entries of the same priority. Register specific subclasses
        before broader classes by assigning them higher priorities.

        During host enumeration, validfunc(obj, flags) returns TRUE to accept
        an object or FALSE to omit it and its descendants. flags initially
        contains TNF_LIST|TNF_OPEN when enumfunc is present, otherwise zero;
        the callback may adjust these tree flags. Do not retain flags.

        For a visible accepted object, enumfunc(obj, parent) is called with
        its new tree node when TNF_LIST remains set. For a skipped class, the
        host calls the enumerator with the existing parent instead. Callback
        objects and tree nodes are borrowed; do not dispose them.

        Call in the host task during setup, before enumeration. No registry
        lock is taken; serialize registration, lookup and callback use.
        Registration does not refresh existing tree nodes or property windows.

    SEE ALSO
        FindClassHandler(), FindObjectHandler(), RegisterSkipClass()

*****************************************************************************/
AROS_LH5(BOOL, RegisterClassHandler,
         AROS_LHA(CONST_STRPTR, classid, A0),
         AROS_LHA(BYTE, pri, D0),
         AROS_LHA(struct MUI_CustomClass *, customwinclass, A1),
         AROS_LHA(CLASS_ENUMFUNC, enumfunc, A2),
         AROS_LHA(CLASS_VALIDFUNC, validfunc, A3),
         struct SysexpBase *, SysexpBase, 9, Sysexp)
{
    AROS_LIBFUNC_INIT

    struct ClassHandlerNode *newClass;
    BOOL add = TRUE;
    if ((newClass = FindClassHandler(classid, &SysexpBase->sesb_ClassHandlers)))
    {
        if ((newClass->enumFunc) && (newClass->enumFunc != hwEnum))
            return FALSE;

        D(bug("[sysexp.library] %s: Updating '%s'..\n", __func__, classid));
        add = FALSE;
    }

    if (add)
    {
        D(bug("[sysexp.library] %s: Registering '%s'..\n", __func__, classid));
        newClass = AllocMem(sizeof(struct ClassHandlerNode), MEMF_CLEAR);
    }

    if (newClass)
    {
        newClass->ch_Node.ln_Name = (char *)classid;
        newClass->ch_Node.ln_Pri = pri;
        newClass->muiClass = customwinclass;
        newClass->enumFunc = enumfunc;
        newClass->validFunc = validfunc;

        if (!add)
            Remove(&newClass->ch_Node);
        Enqueue(&SysexpBase->sesb_ClassHandlers, &newClass->ch_Node);

        return TRUE;
    }
    return FALSE;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        struct ClassHandlerNode * FindClassHandler(

    SYNOPSIS
        CONST_STRPTR classid, struct List *_handlers)

    FUNCTION
        Find the first handler whose class identifier matches exactly.

    INPUTS
        classid - Non-NULL, NUL-terminated class identifier.
        _handlers - Initialized list of ClassHandlerNode entries with valid
            names. NULL does not select a default list.

    RESULT
        Borrowed pointer to the first exact, case-sensitive match, or NULL.

    NOTES
        The list is searched in its existing order. No superclass lookup or
        validation callback is performed. The result is not copied or retained;
        do not unlink or free an entry owned by the registry.

        Keep the list and its entries valid and serialize access with changes.
        No lock is acquired. A later registration may update the returned node.

    SEE ALSO
        FindObjectHandler(), RegisterClassHandler()

*****************************************************************************/
AROS_LH2(struct ClassHandlerNode *, FindClassHandler,
         AROS_LHA(CONST_STRPTR, classid, A0),
         AROS_LHA(struct List *, _handlers, A1),
         struct SysexpBase *, SysexpBase, 10, Sysexp)
{
    AROS_LIBFUNC_INIT

    struct ClassHandlerNode *curHandler;
 
    ForeachNode(_handlers, curHandler)
    {
        if (ClassNameMatches(classid, curHandler->ch_Node.ln_Name))
        {
            D(bug("[sysexp.library] %s: Returning class '%s'\n", __func__, curHandler->ch_Node.ln_Name));
            return curHandler;
        }
    }
    return NULL;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        struct ClassHandlerNode * FindObjectHandler(

    SYNOPSIS
        OOP_Object *obj, struct List *_handlers)

    FUNCTION
        Find the first handler matching an object class or one of its superclasses.

    INPUTS
        obj - Live, non-NULL OOP object with a valid class chain.
        _handlers - Initialized list of ClassHandlerNode entries with valid
            class identifiers. NULL does not select a default list.

    RESULT
        Borrowed matching handler, or NULL if no list entry matches the class chain.

    NOTES
        List order takes precedence over inheritance distance. Each handler
        is compared with the object's class and then its superclasses before
        the next handler is considered. A broad handler earlier in the list
        can therefore win over a later handler for the exact class.

        Class names must match completely and case-sensitively. This lookup
        does not call validFunc, create a window or enumerate children.

        Keep the object, class chain, list and entries valid during lookup.
        The result is not copied or retained. Serialize lookup with registry
        changes; no lock is acquired. Do not free a registry-owned handler.

    SEE ALSO
        FindClassHandler(), RegisterClassHandler()

*****************************************************************************/
AROS_LH2(struct ClassHandlerNode *, FindObjectHandler,
         AROS_LHA(OOP_Object *, obj, A0),
         AROS_LHA(struct List *, _handlers, A1),
         struct SysexpBase *, SysexpBase, 11, Sysexp)
{
    AROS_LIBFUNC_INIT

    struct ClassHandlerNode *curHandler;
 
    D(bug("[sysexp.library] %s: Finding Handler for Object @ 0x%p\n", __func__, obj));

    ForeachNode(_handlers, curHandler)
    {
        OOP_Class *cl;
        D(bug("[sysexp.library] %s:    Checking match with '%s'\n", __func__, curHandler->ch_Node.ln_Name));

        for (cl = OOP_OCLASS(obj); cl ; cl = cl->superclass)
        {
            D(bug("[sysexp.library] %s:      Object Class -> '%s'\n", __func__, cl->ClassNode.ln_Name));
            if (ClassNameMatches(cl->ClassNode.ln_Name, curHandler->ch_Node.ln_Name))
            {
                D(bug("[sysexp.library] %s: Returning obj class '%s'\n", __func__, curHandler->ch_Node.ln_Name));
                return curHandler;
            }
        }
    }
    return NULL;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        void RegisterSkipClass(

    SYNOPSIS
        CONST_STRPTR ClassID)

    FUNCTION
        Hide a class node during SysExplorer hardware-tree enumeration.

    INPUTS
        ClassID - Non-NULL, NUL-terminated class identifier to copy.

    RESULT
        None.

    NOTES
        An exact, case-sensitive duplicate has no effect. The identifier is
        copied and may be released after return. No unregister operation is
        provided.

        The host tests an object's concrete class, not its superclasses. When
        an accepted object is skipped, its enumerator can still add children
        to the existing parent. The storage module has its own enumeration
        path and does not apply this skip list.

        Register during host setup, before enumeration. Existing tree nodes
        are not removed. Serialize access with SkipClass(); no lock is taken.

    BUGS
        Allocation failure is not checked and can cause an invalid memory access.

    SEE ALSO
        SkipClass(), RegisterClassHandler()

*****************************************************************************/
AROS_LH1(void, RegisterSkipClass,
         AROS_LHA(CONST_STRPTR, ClassID, A0),
         struct SysexpBase *, SysexpBase, 15, Sysexp)
{
    AROS_LIBFUNC_INIT

    D(bug("[sysexp.library] %s('%s')\n", __func__, ClassID));

    if (!SkipClass(ClassID))
    {
        struct Node *skipNode = AllocVec(sizeof(struct Node) + strlen(ClassID) + 1, MEMF_CLEAR);
        skipNode->ln_Name = (APTR)((IPTR)skipNode + sizeof(struct Node));
        CopyMem(ClassID, skipNode->ln_Name, strlen(ClassID));
        AddTail(&SysexpBase->sesb_ClassIgnore, skipNode);
    }

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************
    NAME
        BOOL SkipClass(

    SYNOPSIS
        CONST_STRPTR ClassID)

    FUNCTION
        Test whether an exact class identifier is in the SysExplorer skip list.

    INPUTS
        ClassID - Non-NULL, NUL-terminated class identifier.

    RESULT
        TRUE for an exact, case-sensitive match; FALSE otherwise.

    NOTES
        This call only queries the list. It does not traverse superclasses,
        modify the tree or suppress child enumeration.

        Use the host-supplied SysexpBase. Serialize lookup with registration;
        no lock is acquired. The input string is not retained.

    SEE ALSO
        RegisterSkipClass()

*****************************************************************************/
AROS_LH1(BOOL, SkipClass,
         AROS_LHA(CONST_STRPTR, ClassID, A0),
         struct SysexpBase *, SysexpBase, 16, Sysexp)
{
    AROS_LIBFUNC_INIT
    struct Node *skipNode;

    D(bug("[sysexp.library] %s()\n", __func__));

    ForeachNode(&SysexpBase->sesb_ClassIgnore, skipNode)
    {
        if (!strcmp(skipNode->ln_Name, ClassID))
            return TRUE;
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
}

/** Library Initialization **/

void *SysexpLibrary_funcTable[] = {
  AROS_SLIB_ENTRY(Null, LIB, 0),             /* Open() */
  AROS_SLIB_ENTRY(Null, LIB, 0),
  AROS_SLIB_ENTRY(Null, LIB, 0),             /* Expunge() */
  AROS_SLIB_ENTRY(Null, LIB, 0),             /* Reserved() */
  AROS_SLIB_ENTRY(RegisterModule, Sysexp, 5),
  AROS_SLIB_ENTRY(RegisterBase, Sysexp, 6),
  AROS_SLIB_ENTRY(GetBase, Sysexp, 7),
  AROS_SLIB_ENTRY(Null, LIB, 0),
  AROS_SLIB_ENTRY(RegisterClassHandler, Sysexp, 9),
  AROS_SLIB_ENTRY(FindClassHandler, Sysexp, 10),
  AROS_SLIB_ENTRY(FindObjectHandler, Sysexp, 11),
  AROS_SLIB_ENTRY(Null, LIB, 0),
  AROS_SLIB_ENTRY(Null, LIB, 0),
  AROS_SLIB_ENTRY(Null, LIB, 0),
  AROS_SLIB_ENTRY(RegisterSkipClass, Sysexp, 15),
  AROS_SLIB_ENTRY(SkipClass, Sysexp, 16),
  (void *)-1
};

void sysexp_initlib(struct SysexpBase **SysexpBasePtr)
{
    struct SysexpBase *SysexpBase = NULL;

    D(bug("[SysExplorer] %s()\n", __func__));
    SysexpBase = (struct SysexpBase *)MakeLibrary(SysexpLibrary_funcTable,
                                NULL,
                                 NULL,
                                 sizeof(struct SysexpBase),
                                 BNULL);

    D(bug("[SysExplorer] %s: SysexpBase @ %p\n", __func__, SysexpBase));

    if (SysexpBase)
    {
        NEWLIST(&SysexpBase->sesb_GenericBases);
        NEWLIST(&SysexpBase->sesb_Modules);
        NEWLIST(&SysexpBase->sesb_ClassHandlers);
        NEWLIST(&SysexpBase->sesb_ClassIgnore);

        RegisterBase(hwenumfunc_name, hwEnum);
        RegisterBase(devicepageclass_name, DevicePage_CLASS);
        RegisterBase(genericwindowclass_name, GenericWindow_CLASS);
        RegisterBase(computerwindowclass_name, ComputerWindow_CLASS);

        /* We dont want the PCI base object to show in the tree */
        RegisterSkipClass(CLID_Hidd_PCI);

        RegisterClassHandler(CLID_Hidd_Storage, 90, NULL, hwEnum, NULL);
        RegisterClassHandler(CLID_Hidd_StorageController, 90, GenericWindow_CLASS, NULL, NULL);
        RegisterClassHandler(CLID_HW_Gfx, 60, NULL,  hwEnum, NULL);
        RegisterClassHandler(CLID_Hidd_Gfx, 60, GenericWindow_CLASS, hwEnum, NULL);
        RegisterClassHandler(CLID_Hidd_System, 30, NULL, hwEnum, NULL);
        RegisterClassHandler(CLID_HW_Root, 0, ComputerWindow_CLASS,  hwEnum, NULL);
        RegisterClassHandler(CLID_HW, -30, NULL, hwEnum, NULL);
        RegisterClassHandler(CLID_Hidd, -60, GenericWindow_CLASS, NULL, NULL);
    }
    *SysexpBasePtr = SysexpBase;
}
