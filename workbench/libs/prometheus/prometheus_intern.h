/*
    Copyright (C) 2005 Neil Cafferkey
    Copyright (C) 2011 The AROS Development Team.
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
#include <aros/asmcall.h>
#include <aros/libcall.h>


#ifndef UPINT
typedef IPTR UPINT;
typedef SIPTR PINT;
#endif


#undef PCIBoard
typedef struct PCIBoard PCIBoard;


struct LibBase
{
   struct Library lib_header;
   OOP_Object *pci_hidd;
   OOP_AttrBase pcidevice_attr_base;
   struct MinList boards;
};


struct PCIBoard
{
   struct MinNode node;
   const VOID *owner;
   OOP_Object *aros_board;
   struct Interrupt *aros_irq;
};


#endif
