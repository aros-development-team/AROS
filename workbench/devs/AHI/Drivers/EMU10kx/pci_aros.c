
#include <config.h>

#include <utility/hooks.h>
#include <exec/interrupts.h>
#include <asm/io.h>
#include <oop/oop.h>
#include <hidd/pci.h>
#include <hidd/irq.h>
#include <aros/asmcall.h>

#include <proto/oop.h>

#include "DriverData.h"
#include "pci_wrapper.h"

#include <aros/debug.h>
#define KPrintF kprintf

struct Library *OOPBase;

OOP_AttrBase __IHidd_PCIDev;
static OOP_Object *pciobj, *irqobj;

static OOP_MethodID mid_RB;
static OOP_MethodID mid_RW;
static OOP_MethodID mid_RL;
	
static OOP_MethodID mid_WB;
static OOP_MethodID mid_WW;
static OOP_MethodID mid_WL;

static HIDDT_IRQ_Handler inthandler;
static BOOL inthandler_added;

BOOL ahi_pci_init(struct DriverBase* AHIsubBase)
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

  KPrintF("== ahi_pci_init 1\n");
  OOPBase = OpenLibrary(AROSOOP_NAME, 0);
  if (OOPBase)
  {
    KPrintF("== ahi_pci_init 2\n");
    __IHidd_PCIDev = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    if (__IHidd_PCIDev)
    {
      KPrintF("== ahi_pci_init 3\n");

      pciobj = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
      irqobj = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
      if(pciobj && irqobj)
      {
	KPrintF("== ahi_pci_init 4\n");
	mid_RB = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);
	mid_RW = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord);
	mid_RL = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigLong);
	
	mid_WB = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);
	mid_WW = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigWord);
	mid_WL = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigLong);

	return TRUE;
      }
    }
  }
    
  return FALSE; 
}

void ahi_pci_exit(void)
{
  KPrintF("== ahi_pci_exit 1\n");
  if (irqobj) OOP_DisposeObject(irqobj);
  KPrintF("== ahi_pci_exit 2\n");
  if (pciobj) OOP_DisposeObject(pciobj);
  KPrintF("== ahi_pci_exit 3\n");
  if (__IHidd_PCIDev) OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
  KPrintF("== ahi_pci_exit 4\n");
  if (OOPBase) CloseLibrary(OOPBase);
  KPrintF("== ahi_pci_exit 5\n");
}

struct enum_data
{
    OOP_Object *prev_dev;
    OOP_Object *found_dev;
};

static AROS_UFH3(void, Enumerator,
		 AROS_UFHA(struct Hook *,    hook,   A0),
		 AROS_UFHA(OOP_Object *,     device, A2),
		 AROS_UFHA(APTR,             msg,    A1))
{
  AROS_USERFUNC_INIT

    struct enum_data *ed = (struct enum_data *)hook->h_Data;
    
  if ((ed->found_dev == 0) && (device != ed->prev_dev))
  {
    ed->found_dev = device;
  }

  AROS_USERFUNC_EXIT
    }

APTR ahi_pci_find_device(ULONG vendorid, ULONG deviceid, APTR dev)
{ 
  struct enum_data ed;
    
  struct Hook FindHook =
    {
      h_Entry:    (HOOKFUNC)Enumerator,
      h_Data:	    &ed,
    };

  struct TagItem Reqs[] =
    {
#if 0
      { tHidd_PCI_Class   	, 0x04 	    }, /* Multimedia */
      { tHidd_PCI_SubClass	, 0x01 	    }, /* Audio */
#endif
      { tHidd_PCI_VendorID	, vendorid  },
      { tHidd_PCI_ProductID	, deviceid  },
      { TAG_DONE  	    	, 0   	    },
    };

  struct pHidd_PCI_EnumDevices enummsg =
    {
      mID:		OOP_GetMethodID(CLID_Hidd_PCI, moHidd_PCI_EnumDevices),
      callback:	&FindHook,
      requirements:	(struct TagItem *)&Reqs,
    }, *msg = &enummsg;

  KPrintF("ahi_pci_find_device: prevdev = %lx\n", dev);
  ed.prev_dev = (OOP_Object *)dev;
  ed.found_dev = 0;
    
  OOP_DoMethod(pciobj, (OOP_Msg)msg);

  KPrintF("ahi_pci_find_device: found_dev = %lx\n", ed.found_dev);
    
  return (APTR)ed.found_dev;
}

ULONG ahi_pci_inl(ULONG addr, APTR dev)
{
  return inl(addr);
}

UWORD ahi_pci_inw(ULONG addr, APTR dev)
{
  return inw(addr);
}

UBYTE ahi_pci_inb(ULONG addr, APTR dev)
{
  return inb(addr);
}

void ahi_pci_outl(ULONG value, ULONG addr, APTR dev)
{
  outl(value, addr);  
}

void ahi_pci_outw(UWORD value, ULONG addr, APTR dev)
{
  outw(value, addr);
}

void ahi_pci_outb(UBYTE value, ULONG addr, APTR dev)
{
  outb(value, addr);
}


ULONG ahi_pci_read_config_long(UBYTE reg, APTR dev)
{
  struct pHidd_PCIDevice_ReadConfigLong  msg;
    
  msg.mID = mid_RL;
  msg.reg = reg;
    
  return OOP_DoMethod((OOP_Object *)dev, (OOP_Msg)&msg);
}

UWORD ahi_pci_read_config_word(UBYTE reg, APTR dev)
{
  struct pHidd_PCIDevice_ReadConfigWord  msg;

  msg.mID = mid_RW;
  msg.reg = reg;
    
  return OOP_DoMethod((OOP_Object *)dev, (OOP_Msg)&msg);
}

UBYTE ahi_pci_read_config_byte(UBYTE reg, APTR dev)
{
  struct pHidd_PCIDevice_ReadConfigByte  msg;
    
  msg.mID = mid_RB;
  msg.reg = reg;
    
  return OOP_DoMethod(dev, (OOP_Msg)&msg);
}

void ahi_pci_write_config_long(UBYTE reg, ULONG val, APTR dev)
{
  struct pHidd_PCIDevice_WriteConfigLong msg;

  msg.mID = mid_WL;
  msg.reg = reg;
  msg.val = val;
    
  OOP_DoMethod((OOP_Object *)dev, (OOP_Msg)&msg);
}

void ahi_pci_write_config_word(UBYTE reg, UWORD val, APTR dev)
{
  struct pHidd_PCIDevice_WriteConfigWord msg;

  msg.mID = mid_WW;
  msg.reg = reg;
  msg.val = val;
    
  OOP_DoMethod((OOP_Object *)dev, (OOP_Msg)&msg);
}

void ahi_pci_write_config_byte(UBYTE reg, UBYTE val, APTR dev)
{
  struct pHidd_PCIDevice_WriteConfigByte msg;
    
  msg.mID = mid_WB;
  msg.reg = reg;
  msg.val = val;
    
  OOP_DoMethod((OOP_Object *)dev, (OOP_Msg)&msg);
}

ULONG ahi_pci_get_irq(APTR dev)
{
  IPTR val;
    
  OOP_GetAttr((OOP_Object *)dev, aHidd_PCIDevice_INTLine, &val);
  KPrintF("ahi_pci_get_irq: irq = %ld\n", val);
    
  return (ULONG)val;
}

static void interrupt_code(HIDDT_IRQ_Handler *irq, HIDDT_IRQ_HwInfo *hw)
{
  struct Interrupt *i = (struct Interrupt *)irq->h_Data;

  AROS_UFC3(void, i->is_Code,
	    AROS_UFCA(APTR, i->is_Data, A1),
	    AROS_UFCA(APTR, i->is_Code, A5),
	    AROS_UFCA(struct ExecBase *, SysBase, A6));	
}

BOOL ahi_pci_add_intserver(struct Interrupt *i, APTR dev)
{
  struct pHidd_IRQ_AddHandler __msg__, *msg = &__msg__;
  IPTR val;
    
  KPrintF("ahi_pci_add_intserver\n");

  OOP_GetAttr((OOP_Object *)dev, aHidd_PCIDevice_INTLine, &val);
  KPrintF("ahi_pci_add_intserver: irq = %ld\n", val);

  inthandler.h_Node.ln_Pri = 0;
  inthandler.h_Node.ln_Name = "AHI SBLive Int";
  inthandler.h_Code = interrupt_code;
  inthandler.h_Data = i;

  msg->mID = OOP_GetMethodID(CLID_Hidd_IRQ, moHidd_IRQ_AddHandler);
  msg->handlerinfo = &inthandler;
  msg->id = val;

  OOP_DoMethod(irqobj, (OOP_Msg)msg);

  KPrintF("ahi_pci_add_intserver done\n");

  inthandler_added = TRUE;

  return TRUE;
}

void ahi_pci_rem_intserver(struct Interrupt *i, APTR dev)
{    
  KPrintF("ahi_pci_rem_intserver\n");
  if (inthandler_added)
  {
    struct pHidd_IRQ_RemHandler __msg__ =
      {
	mID:		    OOP_GetMethodID(CLID_Hidd_IRQ, moHidd_IRQ_RemHandler),
	handlerinfo:	    &inthandler,
      }, *msg = &__msg__;
	
    OOP_DoMethod(irqobj, (OOP_Msg)msg);
    	
    inthandler_added = FALSE;
  }    
  KPrintF("ahi_pci_rem_intserver\n");
}

APTR ahi_pci_logic_to_physic_addr(APTR addr, APTR dev)
{
  struct pHidd_PCIDriver_CPUtoPCI __msg__, *msg = &__msg__;
  IPTR      	    	    	    driverobj, retval;
   
  OOP_GetAttr((OOP_Object *)dev, aHidd_PCIDevice_Driver, &driverobj);

  msg->mID = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_CPUtoPCI);
  msg->address = addr;

  KPrintF("ahi_pci_logic_to_physic_addr(%lx)\n", msg->address);
    
  retval = OOP_DoMethod((OOP_Object *)driverobj, (OOP_Msg)msg);

  KPrintF("ahi_pci_logic_to_physic_addr(%lx) = %lx\n", msg->address, retval);

  return (APTR)retval;
}

APTR ahi_pci_get_base_address(WORD which, APTR dev)
{
  OOP_AttrID attr = 0;
  IPTR val = 0;
    
  switch(which)
  {
    case 0:
      attr = aHidd_PCIDevice_Base0;
      break;
	    
    case 1:
      attr = aHidd_PCIDevice_Base1;
      break;
	    
    case 2:
      attr = aHidd_PCIDevice_Base2;
      break;
	    
    case 3:
      attr = aHidd_PCIDevice_Base3;
      break;
	    
    case 4:
      attr = aHidd_PCIDevice_Base4;
      break;
	    
    case 5:
      attr = aHidd_PCIDevice_Base5;
      break;
	    
    default:
      return 0;
  }
    
  OOP_GetAttr((OOP_Object *)dev, attr, &val);

  KPrintF("ahi_pci_get_base_address. Result %lx\n", val);
    
  return (APTR)val;
}

ULONG ahi_pci_get_base_size(WORD which, APTR dev)
{
  OOP_AttrID attr = 0;
  IPTR val = 0;
    
  switch(which)
  {
    case 0:
      attr = aHidd_PCIDevice_Size0;
      break;
	    
    case 1:
      attr = aHidd_PCIDevice_Size1;
      break;
	    
    case 2:
      attr = aHidd_PCIDevice_Size2;
      break;
	    
    case 3:
      attr = aHidd_PCIDevice_Size3;
      break;
	    
    case 4:
      attr = aHidd_PCIDevice_Size4;
      break;
	    
    case 5:
      attr = aHidd_PCIDevice_Size5;
      break;
	    
    default:
      return 0;
  }
    
  OOP_GetAttr((OOP_Object *)dev, attr, &val);

  KPrintF("ahi_pci_get_base_size. Result %lx\n", val);
    
  return (ULONG)val;

}

