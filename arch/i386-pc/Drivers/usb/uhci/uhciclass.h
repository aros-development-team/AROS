#ifndef UHCICLASS_H
#define UHCICLASS_H

#include "hardware.h"

struct USBUHCI_staticdata {
	struct ExecBase *sysBase;
	struct Library *oopBase;
	struct Library *utilityBase;
	OOP_Object *uhcihidd;
	HIDDT_PCI_Device *card;
	OOP_Object *pcihidd;
	struct UHCIData data;
};

#if 0
#define XSD(cl) ((struct USBUHCI_staticdata *)cl->UserData)
#else
#define XSD(cl) (xsd)
#endif
#define UtilityBase ((struct Library *)XSD(cl)->utilityBase)
#define OOPBase ((struct Library *)XSD(cl)->oopBase)
#define SysBase (XSD(cl)->sysBase)

#endif

