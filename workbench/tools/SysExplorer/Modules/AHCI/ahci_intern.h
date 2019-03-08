#ifndef SYSEXPLORER_AHCI_INTERN_H
#define SYSEXPLORER_AHCI_INTERN_H

#include <exec/libraries.h>

#include <hidd/hidd.h>
#include <hidd/storage.h>
#include <hidd/ahci.h>

#include "sysexp_intern.h"
#include "sysexp_module.h"

struct SysexpAHCIBase
{
    struct Library              seab_Lib;
    struct SysexpBase           *seab_SysexpBase;
    struct SysexpModule         seab_Module;
    /**/
    struct MUI_CustomClass      *seab_AHCIBusWindowCLASS;
    struct MUI_CustomClass      *seab_AHCIUnitWindowCLASS;
    /**/
    struct MUI_CustomClass      *seab_DevicePageCLASS;
    struct MUI_CustomClass      *seab_GenericWindowCLASS;
};

#endif /* SYSEXPLORER_AHCI_INTERN_H */