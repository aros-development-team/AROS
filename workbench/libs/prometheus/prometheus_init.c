/*
    Copyright (C)©2005 Neil Cafferkey.
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


struct HookContext
{
   LIBBASETYPE *library;
   BOOL success;
};


/* Private prototypes */

AROS_UFP3(static VOID, EnumHook, AROS_UFPA(struct Hook *, hook, A0),
   AROS_UFPA(OOP_Object *, aros_board, A2), AROS_UFPA(APTR, message, A1));
static VOID DeleteLibrary(LIBBASETYPE *base);


/* Constants */

static const TEXT oop_name[] = AROSOOP_NAME;
static const TEXT utility_name[] = UTILITYNAME;


AROS_SET_LIBFUNC(LibInit, LIBBASETYPE, base)
{
   AROS_SET_LIBFUNC_INIT

   BOOL success = TRUE;
   struct Hook *hook;
   struct HookContext *hook_context;

   /* Open HIDDs */

   NewList((APTR)&base->boards);

    base->pci_hidd = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    base->irq_hidd = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
    base->pcidevice_attr_base = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    if(base->pci_hidd == NULL || base->irq_hidd == NULL
       || base->pcidevice_attr_base == 0)
         success = FALSE;

   /* Make a list of all boards in the system */

   hook = AllocMem(sizeof(struct Hook), MEMF_PUBLIC | MEMF_CLEAR);
   hook_context = AllocMem(sizeof(struct HookContext),
      MEMF_PUBLIC | MEMF_CLEAR);
   if(hook == NULL || hook_context == NULL)
      success = FALSE;

   if(success)
   {
      hook->h_Entry = (APTR)EnumHook;
      hook->h_Data = hook_context;
      hook_context->library = base;
      hook_context->success = TRUE;

      HIDD_PCI_EnumDevices(base->pci_hidd, hook, NULL);

      success = hook_context->success;
   }

   if(hook != NULL)
      FreeMem(hook, sizeof(struct Hook));
   if(hook_context != NULL)
      FreeMem(hook_context, sizeof(struct HookContext));

   if(!success)
      DeleteLibrary(base);

   return success;

   AROS_SET_LIBFUNC_EXIT
}



AROS_UFH3(static VOID, EnumHook, AROS_UFHA(struct Hook *, hook, A0),
   AROS_UFHA(OOP_Object *, aros_board, A2), AROS_UFHA(APTR, message, A1))
{
   AROS_USERFUNC_INIT

   LIBBASETYPE *base;
   struct PCIBoard *board;
   struct HookContext *context;
   OOP_AttrBase HiddPCIDeviceAttrBase;

   /* Add board to our list */

   context = hook->h_Data;
   base = context->library;
   HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

   if(HiddPCIDeviceAttrBase != 0)
   {
      board = AllocMem(sizeof(struct PCIBoard), MEMF_PUBLIC | MEMF_CLEAR);
      if(board != NULL)
      {
         board->aros_board = aros_board;
         AddTail((APTR)&base->boards, (APTR)board);
      }
      else
         context->success = FALSE;
      OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
   }

   return;

   AROS_USERFUNC_EXIT
}



AROS_SET_LIBFUNC(LibExpunge, LIBBASETYPE, LIBBASE)
{
   AROS_SET_LIBFUNC_INIT

   DeleteLibrary(LIBBASE);

   return TRUE;

   AROS_SET_LIBFUNC_EXIT
}



static VOID DeleteLibrary(LIBBASETYPE *base)
{
   struct PCIBoard *board;

   /* Free the list of boards */

   while((board = (APTR)RemTail((struct List *)&base->boards)) != NULL)
      FreeMem(board, sizeof(struct PCIBoard));

   /* Close HIDDs */

   if(base->pcidevice_attr_base != 0)
      OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
   if(base->irq_hidd != NULL)
      OOP_DisposeObject(base->irq_hidd);
   if(base->pci_hidd != NULL)
      OOP_DisposeObject(base->pci_hidd);

   return;
}



ADD2INITLIB(LibInit, 0);
ADD2EXPUNGELIB(LibExpunge, 0);



