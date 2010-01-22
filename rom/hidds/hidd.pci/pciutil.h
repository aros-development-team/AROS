#ifndef PCIUTIL_H
#define PCIUTIL_H
/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI stuff for standalone i386 AROS
    Lang: english
*/

#include <exec/types.h>
#include <oop/oop.h>

#include "pci.h"

typedef struct _PCI_ClassCodes
{
        UBYTE Baseclass;
        UBYTE Subclass;
        UBYTE Prgif;
        STRPTR Basedesc;
        STRPTR Subdesc;
        STRPTR Prgifdesc;
} PCI_ClassCodes;

/*
  This list contains all classes defined in the
  PCI 2.2 standard
*/
static const PCI_ClassCodes PCI_ClassTable[] =
{
    { 0x00, 0x00, 0x00, "Legacy PCI", "Non-VGA", "" },
    { 0x00, 0x01, 0x00, "Legacy PCI", "VGA Compatible", "" },
    
    { 0x01, 0x00, 0x00, "Mass storage", "SCSI", "" },
    { 0x01, 0x01, 0x00, "Mass storage", "IDE", "" },
    { 0x01, 0x02, 0x00, "Mass storage", "Floppy", "" },
    { 0x01, 0x03, 0x00, "Mass storage", "IPI", "" },
    { 0x01, 0x04, 0x00, "Mass storage", "RAID", "" },
    { 0x01, 0x80, 0x00, "Mass storage", "Other", "" },

    { 0x02, 0x00, 0x00, "Network", "Ethernet", "" },
    { 0x02, 0x01, 0x00, "Network", "Token ring", "" },
    { 0x02, 0x02, 0x00, "Network", "FDDI", "" },
    { 0x02, 0x03, 0x00, "Network", "ATM", "" },
    { 0x02, 0x04, 0x00, "Network", "ISDN", "" },
    { 0x02, 0x80, 0x00, "Network", "Other", "" },

    { 0x03, 0x00, 0x00, "Video", "PC Compatible", "VGA" },
    { 0x03, 0x00, 0x01, "Video", "PC Compatible", "IBM8514" },
    { 0x03, 0x01, 0x00, "Video", "XGA", "" },
    { 0x03, 0x03, 0x00, "Video", "3D", "" },
    { 0x03, 0x80, 0x00, "Video", "Other", "" },
    
    { 0x04, 0x00, 0x00, "Multimedia", "Video", "" },
    { 0x04, 0x01, 0x00, "Multimedia", "Audio", "" },
    { 0x04, 0x02, 0x00, "Multimedia", "Telephony", "" },
    { 0x04, 0x80, 0x00, "Multimedia", "Other", "" },

    { 0x05, 0x00, 0x00, "Memory controller", "RAM", "" },
    { 0x05, 0x01, 0x00, "Memory controller", "Flash", "" },
    { 0x05, 0x80, 0x00, "Memory controller", "Other", "" },

    { 0x06, 0x00, 0x00, "Bridge", "Host-PCI", "" },
    { 0x06, 0x01, 0x00, "Bridge", "PCI-ISA", "" },
    { 0x06, 0x02, 0x00, "Bridge", "PCI-EISA", "" },
    { 0x06, 0x03, 0x00, "Bridge", "PCI-MCA", "" },
    { 0x06, 0x04, 0x00, "Bridge", "PCI-PCI", "Standard" },
    { 0x06, 0x04, 0x01, "Bridge", "PCI-PCI", "Subtractive" },
    { 0x06, 0x05, 0x00, "Bridge", "PCI-PCMCIA", "" },
    { 0x06, 0x06, 0x00, "Bridge", "PCI-NuBus", "" },
    { 0x06, 0x07, 0x00, "Bridge", "PCI-CardBus", "" },
    { 0x06, 0x08, 0x00, "Bridge", "PCI-RACEway", "" },
    { 0x06, 0x80, 0x00, "Bridge", "Other", "" },

    { 0x07, 0x00, 0x00, "Communication", "Serial", "XT compatible" },
    { 0x07, 0x00, 0x01, "Communication", "Serial", "16450 UART" },
    { 0x07, 0x00, 0x02, "Communication", "Serial", "16550 UART" },
    { 0x07, 0x00, 0x03, "Communication", "Serial", "16650 UART" },
    { 0x07, 0x00, 0x04, "Communication", "Serial", "16750 UART" },
    { 0x07, 0x00, 0x05, "Communication", "Serial", "16850 UART" },
    { 0x07, 0x00, 0x06, "Communication", "Serial", "16950 UART" },
    { 0x07, 0x01, 0x00, "Communication", "Parallel", "Standard" },
    { 0x07, 0x01, 0x01, "Communication", "Parallel", "Bi-directional" },
    { 0x07, 0x01, 0x02, "Communication", "Parallel", "ECP" },
    { 0x07, 0x01, 0x03, "Communication", "Parallel", "IEEE1284 host" },
    { 0x07, 0x01, 0x04, "Communication", "Parallel", "IEEE1284 device" },
    { 0x07, 0x02, 0x00, "Communication", "Multiport serial", "" },
    { 0x07, 0x03, 0x00, "Communication", "Modem", "Generic" },
    { 0x07, 0x03, 0x01, "Communication", "Modem", "Hayes 16450 UART" },
    { 0x07, 0x03, 0x02, "Communication", "Modem", "Hayes 16550 UART" },
    { 0x07, 0x03, 0x03, "Communication", "Modem", "Hayes 16650 UART" },
    { 0x07, 0x03, 0x04, "Communication", "Modem", "Hayes 16750 UART" },
    { 0x07, 0x80, 0x00, "Communication", "Other", "" },

    { 0x08, 0x00, 0x00, "System", "PIC", "Generic 8259" },
    { 0x08, 0x00, 0x01, "System", "PIC", "ISA" },
    { 0x08, 0x00, 0x02, "System", "PIC", "EISA" },
    { 0x08, 0x00, 0x03, "System", "PIC", "I/O APIC" },
    { 0x08, 0x00, 0x04, "System", "PIC", "I/O(x) APIC" },
    { 0x08, 0x01, 0x00, "System", "DMA", "Generic 8237" },
    { 0x08, 0x01, 0x01, "System", "DMA", "ISA" },
    { 0x08, 0x01, 0x02, "System", "DMA", "EISA" },
    { 0x08, 0x02, 0x00, "System", "Timer", "Generic 8254" },
    { 0x08, 0x02, 0x01, "System", "Timer", "ISA" },
    { 0x08, 0x02, 0x02, "System", "Timer", "EISA" },
    { 0x08, 0x03, 0x00, "System", "RTC", "Generic" },
    { 0x08, 0x03, 0x01, "System", "RTC", "ISA" },
    { 0x08, 0x04, 0x00, "System", "PCI Hotplug", "Generic" },
    { 0x08, 0x80, 0x00, "System", "Other", "" },

    { 0x09, 0x00, 0x00, "Input", "Keyboard", "" },
    { 0x09, 0x01, 0x00, "Input", "Digitizer", "" },
    { 0x09, 0x02, 0x00, "Input", "Mouse", "" },
    { 0x09, 0x03, 0x00, "Input", "Scanner", "" },
    { 0x09, 0x04, 0x00, "Input", "Gameport", "Generic" },
    { 0x09, 0x04, 0x01, "Input", "Gameport", "Legacy" },
    { 0x09, 0x80, 0x00, "Input", "Other", "" },

    { 0x0a, 0x00, 0x00, "Docking station", "Generic", "" },
    { 0x0a, 0x80, 0x00, "Docking station", "Other", "" },

    { 0x0b, 0x00, 0x00, "CPU", "386", "" },
    { 0x0b, 0x01, 0x00, "CPU", "486", "" },
    { 0x0b, 0x02, 0x00, "CPU", "Pentium", "" },
    { 0x0b, 0x10, 0x00, "CPU", "Alpha", "" },
    { 0x0b, 0x20, 0x00, "CPU", "PowerPC", "" },
    { 0x0b, 0x30, 0x00, "CPU", "MIPS", "" },
    { 0x0b, 0x40, 0x00, "CPU", "Co-processor", "" },
    
    { 0x0c, 0x00, 0x00, "Serial", "IEE1394", "FireWire" },
    { 0x0c, 0x00, 0x01, "Serial", "IEE1394", "OpenHCI" },
    { 0x0c, 0x01, 0x00, "Serial", "ACCESS.bus", "" },
    { 0x0c, 0x02, 0x00, "Serial", "SSA", "" },
    { 0x0c, 0x03, 0x00, "Serial", "USB", "UHCI" },
    { 0x0c, 0x03, 0x01, "Serial", "USB", "OHCI" },
    { 0x0c, 0x03, 0x02, "Serial", "USB", "Non-standard" },
    { 0x0c, 0x04, 0x00, "Serial", "Fibrechannel", "" },

    { 0x0d, 0x00, 0x00, "Wireless", "iRDA", "" },
    { 0x0d, 0x01, 0x00, "Wireless", "Consumer IR", "" },
    { 0x0d, 0x02, 0x00, "Wireless", "RF", "" },
    { 0x0d, 0x80, 0x00, "Wireless", "Other", "" },

    { 0x0e, 0x00, 0x00, "Intelligent I/O", "I2O", "" },

    { 0x0f, 0x00, 0x00, "Satellite", "TV", "" },
    { 0x0f, 0x01, 0x00, "Satellite", "Audio", "" },
    { 0x0f, 0x02, 0x00, "Satellite", "Voice", "" },
    { 0x0f, 0x03, 0x00, "Satellite", "Data", "" },

    { 0x10, 0x00, 0x00, "Crypto", "Network", "" },
    { 0x10, 0x10, 0x00, "Crypto", "Entertainment", "" },
    { 0x10, 0x80, 0x00, "Crypto", "Other", "" },

    { 0x11, 0x00, 0x00, "Data acquisition", "DPIO", "" },
    { 0x11, 0x80, 0x00, "Data acquisition", "Other", "" },
};

void getPCIClassDesc( UBYTE class, UBYTE sub, UBYTE prgif, STRPTR *cdesc, STRPTR *sdesc, STRPTR *pdesc );
ULONG sizePCIBaseReg( OOP_Object *driver, struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE func, UBYTE basenum );

/* Use this for walking the PCI_ClassTable */
#define PCI_CLASSTABLE_LEN (sizeof(PCI_ClassTable)/sizeof(PCI_ClassCodes))

#endif
