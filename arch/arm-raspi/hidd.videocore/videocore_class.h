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
#include "videocore_hardware.h"
#include "videocore_bitmap.h"

#define IID_Hidd_VideoCore  "hidd.gfx.videocore"
#define CLID_Hidd_VideoCore "hidd.gfx.videocore"

#define MAX_TAGS        64

struct VideoCore_staticdata {
        APTR                    vcsd_VCMBoxBase;
        unsigned int            *vcsd_VCMBoxMessage;

        struct SignalSemaphore  vcsd_GPUMemLock;
        struct MemHeaderExt     vcsd_GPUMemManage;

	struct MemHeader mh;
	OOP_Class *videocoreclass;
	OOP_Class *videocoreonbmclass;
	OOP_Class *videocoreoffbmclass;
	OOP_Object *videocorehidd;
	OOP_Object *card;

	struct BitmapData *visible;
	VOID	(*activecallback)(APTR, OOP_Object *, BOOL);
	APTR	callbackdata;
	struct MouseData mouse;
	struct HWData data;
};

struct VideoCoreBase
{
    struct Library library;
    
    struct VideoCore_staticdata vsd;    
};

#define XSD(cl) (&((struct VideoCoreBase *)cl->UserData)->vsd)

int videocore_InitMem(void *, int, struct VideoCoreBase *);

#endif /* _VIDEOCORE_CLASS_H */
