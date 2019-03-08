#ifndef SYSEXPLORER_INTERN_H
#define SYSEXPLORER_INTERN_H

#include <exec/libraries.h>
#include <libraries/mui.h>

#include "sysexp_module.h"

struct SysexpBase
{
    struct Library      sesb_Lib;
    struct List         sesb_GenericBases;
    struct List         sesb_Modules;
    /*
     * This lists contains handlers for known public classes.
     * It specifies information window class, as well as function
     * to enumerate children objects for the class.
     *
     * For proper operation CLIDs in this list should
     * be sorted from subclasses to superclasses.
     */
    struct List         sesb_ClassHandlers;

    Object              *sesb_Tree;

    ULONG               GlobalCount;
};

struct SysexpIntBase
{
    struct Node         seib_Node; 
    APTR                seib_Base;
};

struct SysexpIntModule
{
    struct SysexpModule seim_Module; 
    APTR                seim_ModuleBase;
};

#endif /* SYSEXPLORER_INTERN_H */