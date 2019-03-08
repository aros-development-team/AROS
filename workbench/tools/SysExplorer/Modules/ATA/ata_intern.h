#ifndef SYSEXPLORER_ATA_INTERN_H
#define SYSEXPLORER_ATA_INTERN_H

#include <exec/libraries.h>

#include <hidd/hidd.h>
#include <hidd/storage.h>
#include <hidd/ata.h>

#include "sysexp_intern.h"
#include "sysexp_module.h"

struct SysexpATABase
{
    struct Library              seab_Lib;
    struct SysexpBase           *seab_SysexpBase;
    struct SysexpModule         seab_Module;
    /**/
    struct MUI_CustomClass      *seab_ATABusWindowCLASS;
    struct MUI_CustomClass      *seab_ATAUnitWindowCLASS;
    /**/
    struct MUI_CustomClass      *seab_DevicePageCLASS;
    struct MUI_CustomClass      *seab_GenericWindowCLASS;
};

#endif /* SYSEXPLORER_ATA_INTERN_H */