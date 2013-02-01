/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VideoCore Hidd initialisation code
    Lang: english
*/

#define DEBUG 1
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

#include "videocore_class.h"
#include "videocore_hardware.h"

#include LC_LIBDEFS_FILE

#ifdef VCMBoxBase
#undef VCMBoxBase
#endif

#define VCMBoxBase      xsd->vcsd_VCMBoxBase

/*
 * The following two functions are candidates for inclusion into oop.library.
 */
static void FreeAttrBases(const STRPTR *iftable, OOP_AttrBase *bases, ULONG num)
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

static BOOL GetAttrBases(const STRPTR *iftable, OOP_AttrBase *bases, ULONG num)
{
    ULONG i;

    for (i = 0; i < num; i++)
    {
	bases[i] = OOP_ObtainAttrBase(iftable[i]);
	if (!bases[i])
	{
	    FreeAttrBases(iftable, bases, i);
	    return FALSE;
	}
    }

    return TRUE;
}

static const STRPTR interfaces[] =
{
    IID_Hidd_BitMap,
    IID_Hidd_VideoCoreBitMap,
    IID_Hidd_VideoCore,
    IID_Hidd_PixFmt,
    IID_Hidd_Sync,
    IID_Hidd_Gfx,
    IID_Hidd
};

static int VideoCore_Init(LIBBASETYPEPTR LIBBASE)
{
    struct VideoCore_staticdata *xsd = &LIBBASE->vsd;
    int retval = FALSE;
    
    if (!GetAttrBases(interfaces, xsd->vcsd_attrBases, ATTRBASES_NUM))
        goto failure;

    if (!(VCMBoxBase = OpenResource("vcmbox.resource")))
        goto failure;

    if (!(xsd->vcsd_VCMBoxMessage = AllocVec(sizeof(IPTR) * 2 * MAX_TAGS, MEMF_CLEAR)))
        goto failure;

    D(bug("[VideoCore] Init: VideoCore Mailbox resource @ 0x%p\n", VCMBoxBase));
    D(bug("[VideoCore] Init: VideoCore message buffer @ 0x%p\n", xsd->vcsd_VCMBoxMessage));

    
    xsd->vcsd_VCMBoxMessage[0] = 8 * 4;
    xsd->vcsd_VCMBoxMessage[1] = VCTAG_REQ;
    xsd->vcsd_VCMBoxMessage[2] = VCTAG_GETVCRAM;
    xsd->vcsd_VCMBoxMessage[3] = 8;
    xsd->vcsd_VCMBoxMessage[4] = 0;

    xsd->vcsd_VCMBoxMessage[5] = 0;
    xsd->vcsd_VCMBoxMessage[6] = 0;

    xsd->vcsd_VCMBoxMessage[7] = 0; // terminate tag

    VCMBoxWrite(VCMB_BASE, VCMB_FBCHAN, xsd->vcsd_VCMBoxMessage);
    if (VCMBoxRead(VCMB_BASE, VCMB_FBCHAN) == xsd->vcsd_VCMBoxMessage)
    {
        if (videocore_InitMem(xsd->vcsd_VCMBoxMessage[5], xsd->vcsd_VCMBoxMessage[6], LIBBASE))
        {
            D(bug("[VideoCore] Init: VideoCore GPU Found\n"));

            initVideoCoreGfxHW((APTR)xsd);

            if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41)) != NULL)
            {
                if (AddDisplayDriver(LIBBASE->vsd.vcsd_VideoCoreGfxClass, NULL, DDRV_BootMode, TRUE, TAG_DONE) == DD_OK)
                {
                    D(bug("[VideoCore] Init: Display Driver registered\n"));

                    /* We use ourselves, and no one else does */
                    LIBBASE->library.lib_OpenCnt = 1;
                    retval = TRUE;
                }
                CloseLibrary(&GfxBase->LibNode);
            }
        }
    }

failure:
    if (!(retval))
    {
        D(bug("[VideoCore] Init: No VideoCore GPU Found\n"));

        FreeVec(xsd->vcsd_VCMBoxMessage);

        FreeAttrBases(interfaces, xsd->vcsd_attrBases, ATTRBASES_NUM);
    }

    return retval;
}

ADD2INITLIB(VideoCore_Init, 0)
