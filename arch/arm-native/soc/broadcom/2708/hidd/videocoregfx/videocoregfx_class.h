#ifndef _VIDEOCOREGFX_CLASS_H
#define _VIDEOCOREGFX_CLASS_H
/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <exec/nodes.h>
#include <exec/types.h>

#include "videocoregfx_hardware.h"

#define DEBUGMODEARRAY
//#define DEBUGPIXFMT
//#define DEBUGDISPLAY

//#define VC_FMT_32
#define VC_FMT_24
#define VC_FMT_16
//#define VC_FMT_15
//#define VC_FMT_8

#define IID_Hidd_VideoCoreGfx  "hidd.gfx.videocore"
#define CLID_Hidd_VideoCoreGfx "hidd.gfx.videocore"

#define MAX_TAGS        256
#define ATTRBASES_NUM   8

struct VideoCoreGfx_staticdata {
        APTR                    vcsd_VCMBoxBase;
        unsigned int            *vcsd_VCMBoxMessage;
        IPTR                    vcsd_VCMBoxBuff;

        struct SignalSemaphore  vcsd_GPUMemLock;
        struct MemHeaderExt     vcsd_GPUMemManage;

        OOP_Class               *vcsd_VideoCoreGfxClass;
	OOP_Object              *vcsd_VideoCoreGfxInstance;
	OOP_Class               *vcsd_VideoCoreGfxOnBMClass;
	OOP_Class               *vcsd_VideoCoreGfxOffBMClass;

        OOP_AttrBase	        vcsd_attrBases[ATTRBASES_NUM];

	APTR                    data;
};

struct VideoCoreGfxBase
{
    struct Library library;
    
    struct VideoCoreGfx_staticdata vsd;    
};

struct DisplayMode
{
    struct Node dm_Node;
    ULONG       dm_clock;
    ULONG       dm_hdisp;
    ULONG       dm_hstart;
    ULONG       dm_hend;
    ULONG       dm_htotal;
    ULONG       dm_vdisp;
    ULONG       dm_vstart;
    ULONG       dm_vend;
    ULONG       dm_vtotal;
    ULONG       dm_descr;
};

#define XSD(cl) (&((struct VideoCoreGfxBase *)cl->UserData)->vsd)

#undef HiddVideoCoreGfxAttrBase
#undef HiddVideoCoreGfxBitMapAttrBase
#undef HiddChunkyBMAttrBase
#undef HiddBitMapAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddGfxAttrBase
#undef HiddAttrBase

/* These must stay in the same order as interfaces[] array in videocoregfx_init.c */
#define HiddVideoCoreGfxAttrBase         XSD(cl)->vcsd_attrBases[0]
#define HiddVideoCoreGfxBitMapAttrBase   XSD(cl)->vcsd_attrBases[1]
#define HiddChunkyBMAttrBase             XSD(cl)->vcsd_attrBases[2]
#define HiddBitMapAttrBase               XSD(cl)->vcsd_attrBases[3]
#define HiddPixFmtAttrBase               XSD(cl)->vcsd_attrBases[4]
#define HiddSyncAttrBase                 XSD(cl)->vcsd_attrBases[5]
#define HiddGfxAttrBase                  XSD(cl)->vcsd_attrBases[6]
#define HiddAttrBase                     XSD(cl)->vcsd_attrBases[7]

#define FNAME_SUPPORT(x) VideoCoreGfx__Support__ ## x

int     FNAME_SUPPORT(InitMem)(void *, int, struct VideoCoreGfxBase *);
int     FNAME_SUPPORT(SDTV_SyncGen)(struct List *, OOP_Class *);
int     FNAME_SUPPORT(HDMI_SyncGen)(struct List *, OOP_Class *);
APTR    FNAME_SUPPORT(GenPixFmts)(OOP_Class *);

#endif /* _VIDEOCOREGFX_CLASS_H */
