/*
    Copyright (C) 2013-2019, The AROS Development Team.
    $Id: main.c 54142 2017-03-16 01:57:58Z NicJA $
*/

#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <hidd/hidd.h>
#include <hidd/system.h>
#include <hidd/gfx.h>
#include <hidd/storage.h>
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
        if (newClass->enumFunc != hwEnum)
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

        if (add)
            Enqueue(&SysexpBase->sesb_ClassHandlers, &newClass->ch_Node);

        return TRUE;
    }
    return FALSE;

    AROS_LIBFUNC_EXIT
}

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

/** Library Initialization **/

void *SysexpLibrary_funcTable[] = {
  AROS_SLIB_ENTRY(Null, LIB, 0),	     /* Open() */
  AROS_SLIB_ENTRY(Null, LIB, 0),
  AROS_SLIB_ENTRY(Null, LIB, 0),	     /* Expunge() */
  AROS_SLIB_ENTRY(Null, LIB, 0),	     /* Reserved() */
  AROS_SLIB_ENTRY(RegisterModule, Sysexp, 5),
  AROS_SLIB_ENTRY(RegisterBase, Sysexp, 6),
  AROS_SLIB_ENTRY(GetBase, Sysexp, 7),
  AROS_SLIB_ENTRY(Null, LIB, 0),
  AROS_SLIB_ENTRY(RegisterClassHandler, Sysexp, 9),
  AROS_SLIB_ENTRY(FindClassHandler, Sysexp, 10),
  AROS_SLIB_ENTRY(FindObjectHandler, Sysexp, 11),
 
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

        RegisterBase(hwenumfunc_name, hwEnum);
        RegisterBase(devicepageclass_name, DevicePage_CLASS);
        RegisterBase(genericwindowclass_name, GenericWindow_CLASS);
        RegisterBase(computerwindowclass_name, ComputerWindow_CLASS);

        RegisterClassHandler(CLID_Hidd_Storage, 90, NULL, hwEnum, NULL);
        RegisterClassHandler(CLID_Hidd_Gfx, 60, NULL, hwEnum, NULL);
        RegisterClassHandler(CLID_Hidd_System, 30, NULL, hwEnum, NULL);
        RegisterClassHandler(CLID_HW_Root, 0, ComputerWindow_CLASS,  hwEnum, NULL);
        RegisterClassHandler(CLID_HW, -30, NULL, hwEnum, NULL);
        RegisterClassHandler(CLID_Hidd, -60, GenericWindow_CLASS, NULL, NULL);
    }
    *SysexpBasePtr = SysexpBase;
}
