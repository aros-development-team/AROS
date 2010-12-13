
#include <config.h>

#include <utility/hooks.h>
#include <exec/interrupts.h>

#include <libraries/openpci.h>
#include <proto/openpci.h>
#include <clib/alib_protos.h>

#include "library.h"
#include "DriverData.h"
#include "pci_wrapper.h"

struct Library *OpenPciBase;

BOOL ahi_pci_init(struct DriverBase* AHIsubBase)
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

  OpenPciBase = OpenLibrary( "openpci.library", 1 );

  if( OpenPciBase == NULL )
  {
    Req( "Unable to open 'openpci.library' version 1.\n" );
    return FALSE;
  }

  EMU10kxBase->flush_caches = pci_bus() & ( GrexA1200Bus | GrexA4000Bus );

  return TRUE;
}

void ahi_pci_exit(void)
{
  CloseLibrary(OpenPciBase);
}

APTR ahi_pci_find_device(ULONG vendorid, ULONG deviceid, APTR dev)
{
  return pci_find_device( vendorid, deviceid, dev );
}

ULONG ahi_pci_inl(ULONG addr, APTR dev)
{
  return SWAPLONG(pci_inl(addr));
}

UWORD ahi_pci_inw(ULONG addr, APTR dev)
{
  return SWAPWORD(pci_inw(addr));
}

UBYTE ahi_pci_inb(ULONG addr, APTR dev)
{
  return pci_inb(addr);
}

void ahi_pci_outl(ULONG value, ULONG addr, APTR dev)
{
  pci_outl(SWAPLONG(value), addr);
}

void ahi_pci_outw(UWORD value, ULONG addr, APTR dev)
{
  pci_outw(SWAPWORD(value), addr);
}

void ahi_pci_outb(UBYTE value, ULONG addr, APTR dev)
{
  pci_outb(value, addr);
}

ULONG ahi_pci_read_config_long(UBYTE reg, APTR dev)
{
  return pci_read_config_long(reg, dev);
}

UWORD ahi_pci_read_config_word(UBYTE reg, APTR dev)
{
  return pci_read_config_word(reg, dev);
}
 
UBYTE ahi_pci_read_config_byte(UBYTE reg, APTR dev)
{
  return pci_read_config_byte(reg, dev);
}

void ahi_pci_write_config_long(UBYTE reg, ULONG val, APTR dev)
{
  pci_write_config_long(reg, val, dev);
}

void ahi_pci_write_config_word(UBYTE reg, UWORD val, APTR dev)
{
  pci_write_config_word(reg, val, dev);
}

void ahi_pci_write_config_byte(UBYTE reg, UBYTE val, APTR dev)
{
  pci_write_config_byte(reg, val, dev);
}

ULONG ahi_pci_get_irq(APTR dev)
{
  return ((struct pci_dev *)dev)->irq;
}

BOOL ahi_pci_add_intserver(struct Interrupt *i, APTR dev)
{
  return pci_add_intserver(i, dev);
}

void ahi_pci_rem_intserver(struct Interrupt *i, APTR dev)
{
  pci_rem_intserver(i, dev);
}

APTR ahi_pci_logic_to_physic_addr(APTR addr, APTR dev)
{
  return pci_logic_to_physic_addr(addr, dev);
}

APTR ahi_pci_get_base_address(WORD which, APTR dev)
{
  return (APTR) ((struct pci_dev *)dev)->base_address[which];
}

ULONG ahi_pci_get_base_size(WORD which, APTR dev)
{
  return ((struct pci_dev *)dev)->base_size[which];
}
