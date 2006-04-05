#ifndef UHCICLASS_H
#define UHCICLASS_H

#include "hardware.h"

struct USBUHCI_staticdata {
    OOP_Object *uhcihidd;
    HIDDT_PCI_Device *card;
    OOP_Object *pcihidd;
    struct UHCIData data;
};

struct USBUHCIBase
{
    struct Library library;
    struct ExecBase *sysBase;
    BPTR	SegList;
    
    struct USBHCI_staticdata usd;
};

#define XSD(cl) (&((struct USBUHCIBase *)cl->UserData)->usd)

#endif

