/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VideoCore Hidd initialisation code
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <proto/vcmbox.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <graphics/driver.h>
#include <graphics/gfxbase.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <aros/symbolsets.h>

#include "videocoregfx_class.h"
#include "videocoregfx_hardware.h"

#include LC_LIBDEFS_FILE

#ifdef VCMBoxBase
#undef VCMBoxBase
#endif

#define VCMBoxBase      xsd->vcsd_VCMBoxBase

IPTR    	__arm_periiobase __attribute__((used)) = 0 ;
APTR KernelBase __attribute__((used)) = NULL;

static void FNAME_SUPPORT(FreeAttrBases)(const STRPTR *iftable, OOP_AttrBase *bases, ULONG num)
{
    ULONG i;
    
    for (i = 0; i < num; i++)
    {
	if (bases[i])
        {
	    OOP_ReleaseAttrBase(iftable[i]);
            bases[i] = NULL;
        }
    }
}

static BOOL FNAME_SUPPORT(GetAttrBases)(const STRPTR *iftable, OOP_AttrBase *bases, ULONG num)
{
    ULONG i;

    for (i = 0; i < num; i++)
    {
	bases[i] = OOP_ObtainAttrBase(iftable[i]);
	if (!bases[i])
	{
	    FNAME_SUPPORT(FreeAttrBases)(iftable, bases, i);
	    return FALSE;
	}
    }

    return TRUE;
}

static const STRPTR interfaces[] =
{
    IID_Hidd_VideoCoreGfx,
    IID_Hidd_VideoCoreGfxBitMap,
    IID_Hidd_ChunkyBM,
    IID_Hidd_BitMap,
    IID_Hidd_PixFmt,
    IID_Hidd_Sync,
    IID_Hidd_Gfx,
    IID_Hidd
};

static int FNAME_SUPPORT(Init)(LIBBASETYPEPTR LIBBASE)
{
    struct VideoCoreGfx_staticdata *xsd = &LIBBASE->vsd;
    int retval = FALSE;

    KernelBase = OpenResource("kernel.resource");
    __arm_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    if (!FNAME_SUPPORT(GetAttrBases)(interfaces, xsd->vcsd_attrBases, ATTRBASES_NUM))
        goto failure;

    if (!(VCMBoxBase = OpenResource("vcmbox.resource")))
        goto failure;

    if (!(xsd->vcsd_VCMBoxBuff = AllocVec(16 + (sizeof(IPTR) * 2 * MAX_TAGS), MEMF_CLEAR)))
        goto failure;

    xsd->vcsd_VCMBoxMessage =
        (unsigned int *)((xsd->vcsd_VCMBoxBuff + 0xF) & ~0x0000000F);

    D(bug("[VideoCoreGfx] %s: VideoCore Mailbox resource @ 0x%p\n", __PRETTY_FUNCTION__, VCMBoxBase));
    D(bug("[VideoCoreGfx] %s: VideoCore message buffer @ 0x%p\n", __PRETTY_FUNCTION__, xsd->vcsd_VCMBoxMessage));

    
    xsd->vcsd_VCMBoxMessage[0] = 8 * 4;
    xsd->vcsd_VCMBoxMessage[1] = VCTAG_REQ;
    xsd->vcsd_VCMBoxMessage[2] = VCTAG_GETVCRAM;
    xsd->vcsd_VCMBoxMessage[3] = 8;
    xsd->vcsd_VCMBoxMessage[4] = 0;

    xsd->vcsd_VCMBoxMessage[5] = 0;
    xsd->vcsd_VCMBoxMessage[6] = 0;

    xsd->vcsd_VCMBoxMessage[7] = 0; // terminate tag

    VCMBoxWrite(VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_VCMBoxMessage);
    if (VCMBoxRead(VCMB_BASE, VCMB_PROPCHAN) == xsd->vcsd_VCMBoxMessage)
    {
        if (FNAME_SUPPORT(InitMem)(xsd->vcsd_VCMBoxMessage[5], xsd->vcsd_VCMBoxMessage[6], LIBBASE))
        {
            bug("[VideoCoreGfx] VideoCore GPU Found\n");

            FNAME_HW(InitGfxHW)((APTR)xsd);

            if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41)) != NULL)
            {
                if (AddDisplayDriver(LIBBASE->vsd.vcsd_VideoCoreGfxClass, NULL, DDRV_BootMode, TRUE, TAG_DONE) == DD_OK)
                {
                    bug("[VideoCoreGfx] BootMode Display Driver Registered\n");

                    LIBBASE->library.lib_OpenCnt++;
                    retval = TRUE;
                }
                CloseLibrary(&GfxBase->LibNode);
            }
        }
    }

failure:
    if (!(retval))
    {
        bug("[VideoCoreGfx] No VideoCore GPU Found\n");

        FreeVec((APTR)xsd->vcsd_VCMBoxBuff);

        FNAME_SUPPORT(FreeAttrBases)(interfaces, xsd->vcsd_attrBases, ATTRBASES_NUM);
    }

    return retval;
}

ADD2INITLIB(FNAME_SUPPORT(Init), 0)
