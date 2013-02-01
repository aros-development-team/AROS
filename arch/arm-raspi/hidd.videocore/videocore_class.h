#ifndef _VIDEOCORE_CLASS_H
#define _VIDEOCORE_CLASS_H
/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/memheaderext.h>
#include <exec/nodes.h>
#include <exec/types.h>

#include "videocore_bitmap.h"

#define IID_Hidd_VideoCore  "hidd.gfx.videocore"
#define CLID_Hidd_VideoCore "hidd.gfx.videocore"

#define MAX_TAGS        64
#define ATTRBASES_NUM   7

struct VideoCore_staticdata {
        APTR                    vcsd_VCMBoxBase;
        unsigned int            *vcsd_VCMBoxMessage;

        struct SignalSemaphore  vcsd_GPUMemLock;
        struct MemHeaderExt     vcsd_GPUMemManage;

        OOP_Class               *vcsd_VideoCoreGfxClass;
        OOP_AttrBase	        vcsd_attrBases[ATTRBASES_NUM];

	struct MemHeader mh;
	OOP_Class *videocoreonbmclass;
	OOP_Class *videocoreoffbmclass;
	OOP_Object *videocorehidd;
	OOP_Object *card;

	struct BitmapData *visible;
	VOID	(*activecallback)(APTR, OOP_Object *, BOOL);
	APTR	callbackdata;
	struct MouseData mouse;
	APTR                    data;
};

struct VideoCoreBase
{
    struct Library library;
    
    struct VideoCore_staticdata vsd;    
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

#define XSD(cl) (&((struct VideoCoreBase *)cl->UserData)->vsd)

#undef HiddBitMapAttrBase
#undef HiddVideoCoreBitMapAttrBase
#undef HiddVideoCoreAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddGfxAttrBase
#undef HiddAttrBase

/* These must stay in the same order as interfaces[] array in videocore_init.c */
#define HiddBitMapAttrBase               XSD(cl)->vcsd_attrBases[0]
#define HiddVideoCoreBitMapAttrBase      XSD(cl)->vcsd_attrBases[1]
#define HiddVideoCoreAttrBase            XSD(cl)->vcsd_attrBases[2]
#define HiddPixFmtAttrBase               XSD(cl)->vcsd_attrBases[3]
#define HiddSyncAttrBase                 XSD(cl)->vcsd_attrBases[4]
#define HiddGfxAttrBase                  XSD(cl)->vcsd_attrBases[5]
#define HiddAttrBase                     XSD(cl)->vcsd_attrBases[6]

int videocore_InitMem(void *, int, struct VideoCoreBase *);
int VideoCore_SDTV_SyncGen(struct List *);
int VideoCore_HDMI_SyncGen(struct List *);

#endif /* _VIDEOCORE_CLASS_H */
