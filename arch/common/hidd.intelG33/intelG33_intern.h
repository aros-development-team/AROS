#ifndef INTELG33_H
#define INTELG33_H

#define DEBUG 1
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/types.h>

#include <hidd/graphics.h>
#include <hidd/irq.h>

#include "intelG33_regs.h"

#undef HiddPCIDeviceAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#undef HiddG33BitMapAttrBase
#undef __IHidd_PlanarBM
#define HiddPCIDeviceAttrBase (sd->pciAttrBase)
#define HiddBitMapAttrBase    (sd->bitMapAttrBase)
#define HiddPixFmtAttrBase    (sd->pixFmtAttrBase)
#define HiddGfxAttrBase       (sd->gfxAttrBase)
#define HiddSyncAttrBase      (sd->syncAttrBase)
#define HiddG33BitMapAttrBase (sd->G33BitMapAttrBase)
#define __IHidd_PlanarBM      (sd->planarAttrBase)

#define IID_Hidd_Gfx_IntelG33   "IntelG33"
#define CLID_Hidd_Gfx_IntelG33  "IntelG33"
#define IID_Hidd_G33BitMap      "IntelG33Bitmap"

struct IntelG33Chipset {

    struct        SignalSemaphore CSLock;

    UWORD	      VendorID;
    UWORD	      ProductID;

    APTR          MMADR;

    /*
      (MMADR) is used to access graphics control registers.
      (GMADR) is used to access graphics memory allocated via the graphics translation table.
      (GTTADR) is used to access the translation table.
      (GMCH) Graphics Control Register
      (BSM) Base of Stolen Memory
    */

};

struct staticdata {

    APTR          memPool;
    struct        MemHeader  mh;
    struct        MemHeader *G33Mem;

    OOP_Object   *pci;
    OOP_Object   *pciG33;
    OOP_Object   *pciDriver;

    IPTR          G33IntLine;
    HIDDT_IRQ_Handler *G33IRQ;

    OOP_AttrBase  pciAttrBase;
    OOP_AttrBase  bitMapAttrBase;
    OOP_AttrBase  pixFmtAttrBase;
    OOP_AttrBase  gfxAttrBase;
    OOP_AttrBase  syncAttrBase;
    OOP_AttrBase  planarAttrBase;
    OOP_AttrBase  G33BitMapAttrBase;

    OOP_Object   *IntelG33Object;
    OOP_Class    *IntelG33Class;
    OOP_Class    *OnBMClass;
    OOP_Class    *OffBMClass;
    OOP_Class    *PlanarBMClass;

    OOP_MethodID  mid_ReadLong;

    struct        IntelG33Chipset Chipset;
 
};

struct IntelG33Base {
    struct        Library LibNode;    
    struct        staticdata sd;
};

#define XSD(cl) (&((struct IntelG33Base *)cl->UserData)->sd)

#define LOCK_CS		     { ObtainSemaphore(&sd->Chipset.CSLock); }
#define UNLOCK_CS	     { ReleaseSemaphore(&sd->Chipset.CSLock); }


void GMBUS_Init(struct staticdata *sd);
UWORD GMBUS_GetStatus(struct staticdata *sd);

#endif
