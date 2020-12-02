#ifndef SYSEXPLORER_PCI_INTERN_H
#define SYSEXPLORER_PCI_INTERN_H

#include <exec/libraries.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>

#include "sysexp_intern.h"
#include "sysexp_module.h"

struct SysexpPCIBase
{
    struct Library              sepb_Lib;
    struct SysexpBase           *sepb_SysexpBase;
    struct SysexpModule         sepb_Module;
    /**/
    struct MUI_CustomClass      *sepb_PCIWindowCLASS;
    struct MUI_CustomClass      *sepb_PCIDevWindowCLASS;
    /**/
    struct MUI_CustomClass      *sepb_DevicePageCLASS;
    struct MUI_CustomClass      *sepb_GenericWindowCLASS;
};

#endif /* SYSEXPLORER_PCI_INTERN_H */