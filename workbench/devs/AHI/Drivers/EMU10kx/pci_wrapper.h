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

#define PCI_BASE_ADDRESS_IO_MASK (~0x3UL)

#define PCI_REVISION_ID     	    8
#define PCI_SUBSYSTEM_ID    	    0x2e
#define PCI_DEVICE_ID	    	    2
#define PCI_SUBSYSTEM_VENDOR_ID     0x2c

BOOL ahi_pci_init(struct DriverBase* AHIsubBase);
void ahi_pci_exit(void);

APTR ahi_pci_find_device(ULONG vendorid, ULONG deviceid, APTR dev);

ULONG ahi_pci_inl(ULONG addr, APTR dev);
UWORD ahi_pci_inw(ULONG addr, APTR dev);
UBYTE ahi_pci_inb(ULONG addr, APTR dev);

void ahi_pci_outl(ULONG value, ULONG addr, APTR dev);
void ahi_pci_outw(UWORD value, ULONG addr, APTR dev);
void ahi_pci_outb(UBYTE value, ULONG addr, APTR dev);

ULONG ahi_pci_read_config_long(UBYTE reg, APTR dev);
UWORD ahi_pci_read_config_word(UBYTE reg, APTR dev);
UBYTE ahi_pci_read_config_byte(UBYTE reg, APTR dev);

void ahi_pci_write_config_long(UBYTE reg, ULONG val, APTR dev);
void ahi_pci_write_config_word(UBYTE reg, UWORD val, APTR dev);
void ahi_pci_write_config_byte(UBYTE reg, UBYTE val, APTR dev);

ULONG ahi_pci_get_irq(APTR dev);

BOOL ahi_pci_add_intserver(struct Interrupt *i, APTR dev);
void ahi_pci_rem_intserver(struct Interrupt *i, APTR dev);

APTR ahi_pci_logic_to_physic_addr(APTR addr, APTR dev);

APTR ahi_pci_get_base_address(WORD which, APTR dev);
ULONG ahi_pci_get_base_size(WORD which, APTR dev);

#endif
