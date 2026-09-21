/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Desc: BCM VideoCore4 Gfx Hidd initialisation code
*/

#define DEBUG 0
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <proto/mbox.h>
#include <proto/kernel.h>
#include <aros/kernel.h>
#include <proto/utility.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <graphics/driver.h>
#include <graphics/gfxbase.h>
#include <hidd/gfx.h>
#include <hidd/gallium.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <aros/symbolsets.h>

#include "vcgfx_hidd.h"
#include "vcgfx_hardware.h"

#include LC_LIBDEFS_FILE

#ifdef MBoxBase
#undef MBoxBase
#endif

#define MBoxBase      xsd->vcsd_MBoxBase

/* Set to 0 to hand the Pi 5 display back to fbgfx. */
#define VCGFX_BCM2712_ENABLE 1

IPTR            __arm_periiobase __attribute__((used)) = 0 ;
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
    IID_Hidd_Display,
    IID_Hidd_DMEnum,
    IID_Hidd
};

/* Real firmware (BCM2712 too) answers GETCLKMEASURED; QEMU leaves its
 * response length zero. ALLOCMEM is no test: Pi 5 firmware dropped it. */
static BOOL vc4_firmware_present(struct VideoCoreGfx_staticdata *xsd)
{
    volatile unsigned int *m = xsd->vcsd_MBoxMessage;

    m[0] = AROS_LONG2LE(8 * 4);
    m[1] = AROS_LONG2LE(VCTAG_REQ);
    m[2] = AROS_LONG2LE(VCTAG_GETCLKMEASURED);
    m[3] = AROS_LONG2LE(8);
    m[4] = AROS_LONG2LE(4);
    m[5] = AROS_LONG2LE(VCCLOCK_ARM);
    m[6] = 0;
    m[7] = 0;

    if (MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)m)
            == (volatile unsigned int *)-1)
        return FALSE;

    return ((AROS_LE2LONG(m[4]) & 0x7fffffff) >= 8)
        && (AROS_LE2LONG(m[6]) != 0);
}

static int FNAME_SUPPORT(Init)(LIBBASETYPEPTR LIBBASE)
{
    struct VideoCoreGfx_staticdata *xsd = &LIBBASE->vsd;
    int retval = FALSE;

    KernelBase = OpenResource("kernel.resource");
    __arm_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    /* The mailbox is common to all VideoCores; the HVS is not. */
    xsd->vcsd_IsBCM2711 = (__arm_periiobase == BCM2711_PERIIOBASE);
    if (__arm_periiobase == BCM2712_PERIIOBASE)
        xsd->vcsd_HVSGen = VCGFX_HVS_HVS6;
    else if (xsd->vcsd_IsBCM2711)
        xsd->vcsd_HVSGen = VCGFX_HVS_HVS5;
    else
        xsd->vcsd_HVSGen = VCGFX_HVS_VC4;

    /* A Pi 5 booting to a black screen has no shell to turn this off from. */
    if ((xsd->vcsd_HVSGen == VCGFX_HVS_HVS6) && !VCGFX_BCM2712_ENABLE)
    {
        D(bug("[VideoCoreGfx] %s: BCM2712 disabled - leaving the display to fbgfx\n",
            __PRETTY_FUNCTION__));
        return FALSE;
    }

    D(bug("[VideoCoreGfx] %s: HVS generation %u\n", __PRETTY_FUNCTION__,
        xsd->vcsd_HVSGen));

    /* Unconditional output: report which driver owns the Pi 5 display. */
    if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS6)
    {
        xsd->vcsd_BootFB       = (ULONG)(IPTR)KrnGetSystemAttr(KATTR_FrameBuffer);
        xsd->vcsd_BootFBPitch  = (ULONG)KrnGetSystemAttr(KATTR_FrameBufferPitch);
        xsd->vcsd_BootFBWidth  = (ULONG)KrnGetSystemAttr(KATTR_FrameBufferWidth);
        xsd->vcsd_BootFBHeight = (ULONG)KrnGetSystemAttr(KATTR_FrameBufferHeight);

        bug("[VideoCoreGfx] BCM2712: mailbox paths only, HVS and DMA off;"
            " boot fb 0x%08x %ux%u pitch %u\n", xsd->vcsd_BootFB,
            xsd->vcsd_BootFBWidth, xsd->vcsd_BootFBHeight, xsd->vcsd_BootFBPitch);

        /* No bootstrap surface and no way to ask for one: leave it to fbgfx. */
        if (!xsd->vcsd_BootFB || !xsd->vcsd_BootFBPitch)
            return FALSE;
    }

    /* PV2 vsync IRQ handler; the source stays masked until the HVS
     * takeover arms it (vcgfx_hvs.c). */
    vc4_hvs_init(xsd);

    if (!FNAME_SUPPORT(GetAttrBases)(interfaces, xsd->vcsd_attrBases, ATTRBASES_NUM))
        goto failure;

    if (!(MBoxBase = OpenResource("mbox.resource")))
        goto failure;

    /* Own our cache lines; see <proto/mbox.h>. */
    if (!(xsd->vcsd_MBoxBuff = (IPTR)AllocVec(MBOX_MSG_ALIGN + (sizeof(IPTR) * 2 * MAX_TAGS), MEMF_CLEAR)))
        goto failure;

    xsd->vcsd_MBoxMessage =
        (unsigned int *)((xsd->vcsd_MBoxBuff + (MBOX_MSG_ALIGN - 1)) & ~(IPTR)(MBOX_MSG_ALIGN - 1));

    /* Init the mailbox lock before the first MBoxWrite/Read so every
     * transaction (even those before InitMem) can take it. */
    InitSemaphore(&xsd->vcsd_GPUMemLock);

    D(bug("[VideoCoreGfx] %s: VideoCore Mailbox resource @ 0x%p\n", __PRETTY_FUNCTION__, MBoxBase));
    D(bug("[VideoCoreGfx] %s: VideoCore message buffer @ 0x%p\n", __PRETTY_FUNCTION__, xsd->vcsd_MBoxMessage));

    /* Emulation has no display hardware; let fbgfx drive the boot framebuffer. */
    if (!vc4_firmware_present(xsd))
    {
        IPTR fb = (IPTR)KrnGetSystemAttr(KATTR_FrameBuffer);

        if (fb && (fb != (IPTR)-1))
        {
            D(bug("[VideoCoreGfx] %s: emulated - leaving the display to fbgfx\n",
                __PRETTY_FUNCTION__));
            goto failure;
        }
    }


    VC4_MBOX_LOCK(xsd);
    xsd->vcsd_MBoxMessage[0] = AROS_LE2LONG(8 * 4);
    xsd->vcsd_MBoxMessage[1] = AROS_LE2LONG(VCTAG_REQ);
    xsd->vcsd_MBoxMessage[2] = AROS_LE2LONG(VCTAG_GETVCRAM);
    xsd->vcsd_MBoxMessage[3] = AROS_LE2LONG(8);
    xsd->vcsd_MBoxMessage[4] = 0;

    xsd->vcsd_MBoxMessage[5] = 0;
    xsd->vcsd_MBoxMessage[6] = 0;

    xsd->vcsd_MBoxMessage[7] = 0; // terminate tag

    {
        BOOL  mbox_ok   = (MBoxCall((void*)VCMB_BASE, VCMB_PROPCHAN, xsd->vcsd_MBoxMessage)
                          != (volatile unsigned int *)-1);
        void *vc_base   = (void*)(IPTR)AROS_LE2LONG(xsd->vcsd_MBoxMessage[5]);
        ULONG vc_length = AROS_LE2LONG(xsd->vcsd_MBoxMessage[6]);
        VC4_MBOX_UNLOCK(xsd);
        if (mbox_ok && FNAME_SUPPORT(InitMem)(vc_base, vc_length, LIBBASE))
        {
            D(bug("[VideoCoreGfx] VideoCore GPU Found\n"));

            FNAME_HW(InitGfxHW)((APTR)xsd);
            FNAME_SUPPORT(InitCursor)(xsd);
            FNAME_SUPPORT(InitDMA)(xsd);

            if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41)) != NULL)
            {
                LIBBASE->vsd.vcsd_basebm = OOP_FindClass(CLID_Hidd_BitMap);

                /* The gallium hidd (vc4gallium or v3d, by SoC) lives on
                 * the FS, so it can't be opened from InitLib (no disk
                 * yet); it loads lazily from HIDD_Gfx_CreateObject,
                 * falling back to softpipe. */

                if (AddDisplayDriver(LIBBASE->vsd.vcsd_VideoCoreGfxClass, NULL, TAG_DONE) == DD_OK)
                {
                    D(bug("[VideoCoreGfx] Display Driver Registered\n"));
                    if (xsd->vcsd_HVSGen == VCGFX_HVS_HVS6)
                        bug("[VideoCoreGfx] BCM2712: display driver registered\n");

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
        D(bug("[VideoCoreGfx] No VideoCore GPU Found\n"));

        FreeVec((APTR)xsd->vcsd_MBoxBuff);

        FNAME_SUPPORT(FreeAttrBases)(interfaces, xsd->vcsd_attrBases, ATTRBASES_NUM);
    }

    return retval;
}

ADD2INITLIB(FNAME_SUPPORT(Init), 0)
