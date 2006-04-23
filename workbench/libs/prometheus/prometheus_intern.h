/*
    Copyright (C) 2005 Neil Cafferkey
    $Id$
*/

#ifndef PROMETHEUS_INTERN_H
#define PROMETHEUS_INTERN_H


#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <libraries/prometheus.h>
#include <oop/oop.h>
#include <hidd/pci.h>
#include <hidd/irq.h>
#include <aros/asmcall.h>
#include <aros/libcall.h>
#include <libcore/base.h>


#ifndef UPINT
typedef ULONG UPINT;
typedef LONG PINT;
#endif


#undef PCIBoard
typedef struct PCIBoard PCIBoard;


struct LibBase
{
   struct LibHeader lib_header;
   APTR seg_list;
   struct ExecBase *sys_base;
   OOP_Object *pci_hidd;
   OOP_Object *irq_hidd;
   OOP_AttrBase pcidevice_attr_base;
   struct MinList boards;
};


struct PCIBoard
{
   struct MinNode node;
   const VOID *owner;
   OOP_Object *aros_board;
   HIDDT_IRQ_Handler *aros_irq;
};


#endif
