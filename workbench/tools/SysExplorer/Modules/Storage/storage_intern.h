#ifndef SYSEXPLORER_STORAGE_INTERN_H
#define SYSEXPLORER_STORAGE_INTERN_H

#include <exec/libraries.h>

#include <hidd/hidd.h>
#include <hidd/storage.h>

#include "sysexp_intern.h"
#include "sysexp_module.h"

struct SysexpStorageBase
{
    struct Library              sesb_Lib;
    struct SysexpBase           *sesb_SysexpBase;
    struct SysexpModule         sesb_Module;
    /**/
    struct List                 sesb_HandlerList;
    /**/
    struct MUI_CustomClass      *sesb_DevicePageCLASS;
};

#endif /* SYSEXPLORER_STORAGE_INTERN_H */