#ifndef LIBRARIES_PROMETHEUS_H
#define LIBRARIES_PROMETHEUS_H

/*
    Copyright (C) 2005 Neil Cafferkey
    $Id$

    Desc: Definitions for prometheus.library
    Lang: english
*/

#include <exec/types.h>
#include <utility/tagitem.h>


/* Constants */
/* ========= */

#define PROMETHEUSNAME "prometheus.library"

#define PCIBoard VOID


/* NB: these tag values are different from those used on 68k as the original
   values were illegal */

#define PRM_Vendor         TAG_USER
#define PRM_Device         PRM_Vendor + 0x1
#define PRM_Revision       PRM_Vendor + 0x2
#define PRM_Class          PRM_Vendor + 0x3
#define PRM_SubClass       PRM_Vendor + 0x4
#define PRM_MemoryAddr0    PRM_Vendor + 0x10
#define PRM_MemoryAddr1    PRM_Vendor + 0x11
#define PRM_MemoryAddr2    PRM_Vendor + 0x12
#define PRM_MemoryAddr3    PRM_Vendor + 0x13
#define PRM_MemoryAddr4    PRM_Vendor + 0x14
#define PRM_MemoryAddr5    PRM_Vendor + 0x15
#define PRM_ROM_Address    PRM_Vendor + 0x16
#define PRM_MemorySize0    PRM_Vendor + 0x20
#define PRM_MemorySize1    PRM_Vendor + 0x21
#define PRM_MemorySize2    PRM_Vendor + 0x22
#define PRM_MemorySize3    PRM_Vendor + 0x23
#define PRM_MemorySize4    PRM_Vendor + 0x24
#define PRM_MemorySize5    PRM_Vendor + 0x25
#define PRM_ROM_Size       PRM_Vendor + 0x26

#define PRM_BoardOwner     PRM_Vendor + 0x5
#define PRM_SlotNumber     PRM_Vendor + 0x6
#define PRM_FunctionNumber PRM_Vendor + 0x7

#endif
