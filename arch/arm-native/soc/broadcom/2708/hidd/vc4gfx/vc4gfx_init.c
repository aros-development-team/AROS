/*
    Copyright © 2013-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BCM VideoCore4 Gfx Hidd initialisation code
    Lang: english
*/

#define DEBUG 1
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <proto/mbox.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <graphics/driver.h>
#include <graphics/gfxbase.h>
#include <hidd/gfx.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <aros/symbolsets.h>

#include "vc4gfx_hidd.h"
#include "vc4gfx_hardware.h"

#include LC_LIBDEFS_FILE

#ifdef MBoxBase
#undef MBoxBase
#endif

#define MBoxBase      xsd->vcsd_MBoxBase

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
            bases[i] = (OOP_AttrBase)0;
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
    IID_Hidd_Gfx_VideoCore4,
    IID_Hidd_BitMap_VideoCore4,
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

    if (!(MBoxBase = OpenResource("mbox.resource")))
        goto failure;

    if (!(xsd->vcsd_MBoxBuff = (IPTR)AllocVec(16 + (sizeof(IPTR) * 2 * MAX_TAGS), MEMF_CLEAR)))
        goto failure;

    xsd->vcsd_MBoxMessage =
        (unsigned int *)((xsd->vcsd_MBoxBuff + 0xF) & ~0x0000000F);

    D(bug("[VideoCoreGfx] %s: VideoCore Mailbox resource @ 0x%p\n", __PRETTY_FUNCTION__, MBoxBase));
    D(bug("[VideoCoreGfx] %s: VideoCore message buffer @ 0x%p\n", __PRETTY_FUNCTION__, xsd->vcsd_MBoxMessage));


    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(8 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_GETVCRAM);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(8);
    xsd->vcsd_MBoxMessage[4] = 0;

    xsd->vcsd_MBoxMessage[5] = 0;
    xsd->vcsd_MBoxMessage[6] = 0;

    xsd->vcsd_MBoxMessage[7] = 0; // terminate tag

    MBoxWrite((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage);
    if (MBoxRead((void*)VCMB_BASE, VCMB_PROPCHAN) == xsd->vcsd_MBoxMessage)
    {
        if (FNAME_SUPPORT(InitMem)((void*)AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]), AROS_LE2LONG(xsd->vcsd_MBoxMessage[6]), LIBBASE))
        {
            bug("[VideoCoreGfx] VideoCore GPU Found\n");

            FNAME_HW(InitGfxHW)((APTR)xsd);

            if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41)) != NULL)
            {
                LIBBASE->vsd.vcsd_basebm = OOP_FindClass(CLID_Hidd_BitMap);
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

        FreeVec((APTR)xsd->vcsd_MBoxBuff);

        FNAME_SUPPORT(FreeAttrBases)(interfaces, xsd->vcsd_attrBases, ATTRBASES_NUM);
    }

    return retval;
}

ADD2INITLIB(FNAME_SUPPORT(Init), 0)
