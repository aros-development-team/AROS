/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PCI_WRAPPER_H
#define PCI_WRAPPER_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#include "DriverData.h"

#undef PCI_COMMAND
#undef PCI_COMMAND_IO
#undef PCI_COMMAND_MEMORY
#undef PCI_COMMAND_MASTER
#undef PCI_BASE_ADDRESS_IO_MASK
#undef PCI_REVISION_ID
#undef PCI_SUBSYSTEM_ID
#undef PCI_DEVICE_ID
#undef PCI_SUBSYSTEM_VENDOR_ID


#define PCI_COMMAND 	    4
#define PCI_COMMAND_IO      1
#define PCI_COMMAND_MEMORY  2
#define PCI_COMMAND_MASTER  4

#define PCI_BASE_ADDRESS_0 0x10
#define PCI_BASE_ADDRESS_1 0x14
#define PCI_BASE_ADDRESS_2 0x18

#define PCI_BASE_ADDRESS_IO_MASK (~0x3UL)

#define PCI_REVISION_ID     	    8
#define PCI_SUBSYSTEM_ID    	    0x2e
#define PCI_DEVICE_ID	    	    2
#define PCI_SUBSYSTEM_VENDOR_ID     0x2c

BOOL ahi_pci_init(struct DriverBase* AHIsubBase);
void ahi_pci_exit(void);

APTR ahi_pci_find_device(ULONG vendorid, ULONG deviceid, APTR dev);

ULONG pci_inl(ULONG addr, struct CardData *card);
UWORD pci_inw(ULONG addr, struct CardData *card);
UBYTE pci_inb(ULONG addr, struct CardData *card);

void pci_outl(ULONG value, ULONG addr, struct CardData *card);
void pci_outw(UWORD value, ULONG addr, struct CardData *card);
void pci_outb(UBYTE value, ULONG addr, struct CardData *card);

void outb_setbits(UBYTE value, ULONG addr, struct CardData *card);
void outw_setbits(UWORD value, ULONG addr, struct CardData *card);
void outl_setbits(ULONG value, ULONG addr, struct CardData *card);
void outb_clearbits(UBYTE value, ULONG addr, struct CardData *card);
void outw_clearbits(UWORD value, ULONG addr, struct CardData *card);
void outl_clearbits(ULONG value, ULONG addr, struct CardData *card);

ULONG inl_config(UBYTE reg, APTR dev);
UWORD inw_config(UBYTE reg, APTR dev);
UBYTE inb_config(UBYTE reg, APTR dev);

void outl_config(UBYTE reg, ULONG val, APTR dev);
void outw_config(UBYTE reg, UWORD val, APTR dev);
void outb_config(UBYTE reg, UBYTE val, APTR dev);

ULONG ahi_pci_get_irq(APTR dev);

BOOL ahi_pci_add_intserver(struct Interrupt *i, APTR dev);
void ahi_pci_rem_intserver(struct Interrupt *i, APTR dev);

APTR ahi_pci_logic_to_physic_addr(APTR addr, APTR dev);

APTR ahi_pci_get_base_address(WORD which, APTR dev);
ULONG ahi_pci_get_base_size(WORD which, APTR dev);
ULONG ahi_pci_get_type(WORD which, APTR dev);
ULONG ahi_pci_mem_map(APTR addr, APTR dev);

#ifdef __AMIGAOS4__
#define ALLOCVEC AllocVec
#define FREEVEC FreeVec
#define DEBUGPRINTF IExec->DebugPrintF
#define CAUSE Cause
#define CALLHOOK CallHookPkt
#define INITSEMAPHORE InitSemaphore
#define OBTAINSEMAPHORE ObtainSemaphore
#define RELEASESEMAPHORE ReleaseSemaphore
#define CREATEPORT(a, b) CreatePort(a, b)
#define CREATEIOREQUEST CreateIORequest
#define OPENDEVICE OpenDevice
#define DOIO DoIO
#define DELETEIOREQUEST DeleteIORequest
#define CLOSEDEVICE IExec->CloseDevice
#define DELETEPORT IExec->DeletePort
#define GETTAGDATA IUtility->GetTagData
#define IRQTYPE INTERRUPT_NODE_TYPE
#define DELAY IDOS->Delay
#define bug DebugPrintF
#else
#include <aros/debug.h>
#define ALLOCVEC AllocVec
#define FREEVEC FreeVec
#define DEBUGPRINTF kprintf
#define CAUSE Cause
#define CALLHOOK CallHookA
#define INITSEMAPHORE InitSemaphore
#define OBTAINSEMAPHORE ObtainSemaphore
#define RELEASESEMAPHORE ReleaseSemaphore
#define CREATEPORT(a, b) CreateMsgPort()
#define CREATEIOREQUEST CreateIORequest
#define OPENDEVICE OpenDevice
#define DOIO DoIO
#define DELETEIOREQUEST DeleteIORequest
#define CLOSEDEVICE CloseDevice
#define DELETEPORT DeleteMsgPort
#define GETTAGDATA GetTagData
#define IRQTYPE  INTERRUPT_NODE_TYPE
#define DELAY Delay
#endif

#endif
