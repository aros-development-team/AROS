/*
    Copyright © 2005-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Prometheus initialisation code.
    Lang: English.
*/

#include <exec/types.h>
#include <exec/resident.h>
#include <utility/utility.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/alib.h>

#include LC_LIBDEFS_FILE
#include "prometheus_intern.h"
#include <aros/symbolsets.h>

#undef HiddPCIDriverAttrBase
#define HiddPCIDriverAttrBase (base->pcidriver_attr_base)

/* Private prototypes */

AROS_UFP3(static VOID, EnumHook, AROS_UFPA(struct Hook *, hook, A0),
   AROS_UFPA(OOP_Object *, aros_board, A2), AROS_UFPA(APTR, message, A1));
static int DeleteLibrary(LIBBASETYPE *base);

static int LibInit(LIBBASETYPEPTR base)
{
    BOOL success = TRUE;

    base->kernelBase = OpenResource("kernel.resource");
    if (!base->kernelBase)
        return FALSE;

    NewList((APTR)&base->boards);

    /* Open HIDDs */
    base->pci_hidd = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    base->pcidevice_attr_base = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    base->pcidriver_attr_base = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    base->pcidevice_method_base = OOP_GetMethodID(IID_Hidd_PCIDevice, 0);
    if (base->pci_hidd == NULL || base->pcidevice_attr_base == 0 ||
        base->pcidriver_attr_base == 0)
         success = FALSE;

   /* Make a list of all boards in the system */
   if(success)
   {
      struct Hook hook;

      hook.h_Entry    = (APTR)EnumHook;
      hook.h_Data     = base;
      hook.h_SubEntry = (APTR)TRUE; /* (Ab)use this as success indicator */

      HIDD_PCI_EnumDevices(base->pci_hidd, &hook, NULL);

      success = (IPTR)hook.h_SubEntry;
   }

   if(!success)
      DeleteLibrary(base);

   return success;
}



AROS_UFH3(static VOID, EnumHook, AROS_UFHA(struct Hook *, hook, A0),
   AROS_UFHA(OOP_Object *, aros_board, A2), AROS_UFHA(APTR, message, A1))
{
   AROS_USERFUNC_INIT

   LIBBASETYPE *base = hook->h_Data;
   struct PCIBoard *board;
   OOP_Object *driver;
   IPTR direct;

   /*
    * We have no map/unmap calls, so we can work only with "direct" buses.
    * An alternative would be to map regions on first access, but
    * this could be a significant resource hog (like the same region being
    * mapped several times by several different APIs).
    */   
   OOP_GetAttr(aros_board, aHidd_PCIDevice_Driver, (IPTR *)&driver);
   OOP_GetAttr(driver, aHidd_PCIDriver_DirectBus, &direct);
   if (!direct)
        return;

   /* Add board to our list */
   board = AllocMem(sizeof(struct PCIBoard), MEMF_PUBLIC | MEMF_CLEAR);
   if (board != NULL)
   {
      /*
       * FIXME: Here we should parse device's memory regions and map them all
       * using MapPCI method. Without this drivers will not work on systems
       * with complex memory protection, as well as on Linux-hosted.
       * However, there's another problem with this. What if the device is already
       * owned by somebody else, and its region is already mapped ? How can we
       * know ?
       * Actually this is significant design problem. Alternately we can add some
       * attribute to the driver telling that "we have no permanent mapping".
       * In this case prometheus.library simply will not work. However it is better
       * than crashing.
       */
      board->aros_board = aros_board;
      AddTail((APTR)&base->boards, (APTR)board);
   }
   else
      hook->h_SubEntry = NULL;

   AROS_USERFUNC_EXIT
}



static int DeleteLibrary(LIBBASETYPE *base)
{
   struct PCIBoard *board;

   /* Free the list of boards */

   while((board = (APTR)RemTail((struct List *)&base->boards)) != NULL)
      FreeMem(board, sizeof(struct PCIBoard));

   /* Close HIDDs */

   if(base->pcidevice_attr_base != 0)
      OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
   if(base->pcidriver_attr_base != 0)
      OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
   if(base->pci_hidd != NULL)
      OOP_DisposeObject(base->pci_hidd);

   return TRUE;
}



ADD2INITLIB(LibInit, 0);
ADD2EXPUNGELIB(DeleteLibrary, 0);
