/*
    Copyright (C) 2005 Neil Cafferkey
    Copyright (C) 2011 - 2013 The AROS Development Team.
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
   OOP_AttrBase pcidriver_attr_base;
   OOP_MethodID pcidevice_method_base;
   APTR kernelBase;
   struct MinList boards;
};

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase (base->pcidevice_attr_base)
#undef HiddPCIDeviceBase
#define HiddPCIDeviceBase (base->pcidevice_method_base)

struct PCIBoard
{
   struct MinNode node;
   struct Node *owner;
   OOP_Object *aros_board;
};


#endif
