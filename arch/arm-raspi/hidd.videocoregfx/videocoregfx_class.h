#ifndef _VIDEOCOREGFX_CLASS_H
#define _VIDEOCOREGFX_CLASS_H
/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <exec/nodes.h>
#include <exec/types.h>

#include "videocoregfx_bitmap.h"

#define IID_Hidd_VideoCoreGfx  "hidd.gfx.videocore"
#define CLID_Hidd_VideoCoreGfx "hidd.gfx.videocore"

#define MAX_TAGS        64
#define ATTRBASES_NUM   7

struct VideoCoreGfx_staticdata {
        APTR                    vcsd_VCMBoxBase;
        unsigned int            *vcsd_VCMBoxMessage;

        struct SignalSemaphore  vcsd_GPUMemLock;
        struct MemHeaderExt     vcsd_GPUMemManage;

        OOP_Class               *vcsd_VideoCoreGfxClass;
	OOP_Object              *vcsd_VideoCoreGfxInstance;
	OOP_Class               *vcsd_VideoCoreGfxOnBMClass;
	OOP_Class               *vcsd_VideoCoreGfxOffBMClass;

        OOP_AttrBase	        vcsd_attrBases[ATTRBASES_NUM];

	struct MemHeader mh;

	OOP_Object *card;

	struct BitmapData *visible;
	VOID	(*activecallback)(APTR, OOP_Object *, BOOL);
	APTR	callbackdata;
	struct MouseData mouse;
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
#undef HiddBitMapAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddGfxAttrBase
#undef HiddAttrBase

/* These must stay in the same order as interfaces[] array in videocoregfx_init.c */
#define HiddVideoCoreGfxAttrBase         XSD(cl)->vcsd_attrBases[0]
#define HiddVideoCoreGfxBitMapAttrBase   XSD(cl)->vcsd_attrBases[1]
#define HiddBitMapAttrBase               XSD(cl)->vcsd_attrBases[2]
#define HiddPixFmtAttrBase               XSD(cl)->vcsd_attrBases[3]
#define HiddSyncAttrBase                 XSD(cl)->vcsd_attrBases[4]
#define HiddGfxAttrBase                  XSD(cl)->vcsd_attrBases[5]
#define HiddAttrBase                     XSD(cl)->vcsd_attrBases[6]

#define FNAME_SUPPORT(x) VideoCoreGfx__Support__ ## x

int FNAME_SUPPORT(InitMem)(void *, int, struct VideoCoreGfxBase *);
int FNAME_SUPPORT(SDTV_SyncGen)(struct List *);
int FNAME_SUPPORT(HDMI_SyncGen)(struct List *);

#endif /* _VIDEOCOREGFX_CLASS_H */
