
#include <config.h>

#include <utility/hooks.h>
#include <exec/interrupts.h>

#include <proto/expansion.h>
#define __NOLIBBASE__
#include <proto/ahi_sub.h>
#include <proto/utility.h>

#include "DriverData.h"
#include "pci_wrapper.h"
#include "library.h"

struct ExpansionBase*       ExpansionBase = NULL;
struct ExpansionIFace*      IExpansion    = NULL;
struct PCIIFace*            IPCI          = NULL;

BOOL ahi_pci_init(struct DriverBase* AHIsubBase)
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

  ExpansionBase = (struct ExpansionBase*) OpenLibrary( "expansion.library", 1 );
  if( ExpansionBase == NULL )
  {
    Req( "Unable to open 'expansion.library' version 1.\n" );
    return FALSE;
  }
  if ((IExpansion = (struct ExpansionIFace *) GetInterface((struct Library *) ExpansionBase, "main", 1, NULL)) == NULL)
  {
    Req("Couldn't open IExpansion interface!\n");
    return FALSE;
  }

  if ((IPCI = (struct PCIIFace *) GetInterface((struct Library *) ExpansionBase, "pci", 1, NULL)) == NULL)
  {
    Req("Couldn't open IPCI interface!\n");
    return FALSE;
  }

  EMU10kxBase->flush_caches = TRUE;

  return TRUE; 
}

void ahi_pci_exit(void)
{
  DropInterface( (struct Interface *) IExpansion);
  DropInterface( (struct Interface *) IPCI);
  CloseLibrary( (struct Library*) ExpansionBase );        
}


APTR ahi_pci_find_device(ULONG vendorid, ULONG deviceid, APTR dev)
{
  if (dev) return NULL;
    
  return IPCI->FindDeviceTags( FDT_VendorID, vendorid,
			       FDT_DeviceID, deviceid,
			       TAG_DONE );
}

ULONG ahi_pci_inl(ULONG addr, APTR dev)
{
  return ((struct PCIDevice * )dev)->InLong(addr);
}

UWORD ahi_pci_inw(ULONG addr, APTR dev)
{
  return ((struct PCIDevice * )dev)->InWord(addr);
}

UBYTE ahi_pci_inb(ULONG addr, APTR dev)
{
  return ((struct PCIDevice * )dev)->InByte(addr);
}

void ahi_pci_outl(ULONG value, ULONG addr, APTR dev)
{  
  ((struct PCIDevice * )dev)->OutLong(addr, value);
}

void ahi_pci_outw(UWORD value, ULONG addr, APTR dev)
{  
  ((struct PCIDevice * )dev)->OutWord(addr, value);
}

void ahi_pci_outb(UBYTE value, ULONG addr, APTR dev)
{  
  ((struct PCIDevice * )dev)->OutByte(addr, value);
}

ULONG ahi_pci_read_config_long(UBYTE reg, APTR dev)
{
  return ((struct PCIDevice * )dev)->ReadConfigLong(reg);
}

UWORD ahi_pci_read_config_word(UBYTE reg, APTR dev)
{
  return ((struct PCIDevice * )dev)->ReadConfigWord(reg);
}

UBYTE ahi_pci_read_config_byte(UBYTE reg, APTR dev)
{
  return ((struct PCIDevice * )dev)->ReadConfigByte(reg);
}

void ahi_pci_write_config_long(UBYTE reg, ULONG val, APTR dev)
{
  ((struct PCIDevice * )dev)->WriteConfigLong(reg, val);
}

void ahi_pci_write_config_word(UBYTE reg, UWORD val, APTR dev)
{
  ((struct PCIDevice * )dev)->WriteConfigWord(reg, val);
}

void ahi_pci_write_config_byte(UBYTE reg, UBYTE val, APTR dev)
{
  ((struct PCIDevice * )dev)->WriteConfigByte(reg, val);
}

ULONG ahi_pci_get_irq(APTR dev)
{
  return ((struct PCIDevice * )dev)->MapInterrupt();
}

BOOL ahi_pci_add_intserver(struct Interrupt *i, APTR dev)
{
  AddIntServer(((struct PCIDevice * )dev)->MapInterrupt(), i);
    
  return TRUE;
}

void ahi_pci_rem_intserver(struct Interrupt *i, APTR dev)
{
  RemIntServer(((struct PCIDevice * )dev)->MapInterrupt(), i);
}

APTR ahi_pci_logic_to_physic_addr(APTR addr, APTR dev)
{
  return addr;
}

APTR ahi_pci_get_base_address(WORD which, APTR dev)
{
  return (APTR) ((struct PCIDevice * )dev)->GetResourceRange(which)->BaseAddress;
}

ULONG ahi_pci_get_base_size(WORD which, APTR dev)
{
  return ((struct PCIDevice * )dev)->GetResourceRange(which)->Size;
}
