/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: nvidia.hidd initialization
    Lang: English
*/

 /***************************************************************************\
|*                                                                           *|
|*       Copyright 2003 NVIDIA, Corporation.  All rights reserved.           *|
|*                                                                           *|
|*     NOTICE TO USER:   The source code  is copyrighted under  U.S. and     *|
|*     international laws.  Users and possessors of this source code are     *|
|*     hereby granted a nonexclusive,  royalty-free copyright license to     *|
|*     use this code in individual and commercial software.                  *|
|*                                                                           *|
|*     Any use of this source code must include,  in the user documenta-     *|
|*     tion and  internal comments to the code,  notices to the end user     *|
|*     as follows:                                                           *|
|*                                                                           *|
|*       Copyright 2003 NVIDIA, Corporation.  All rights reserved.           *|
|*                                                                           *|
|*     NVIDIA, CORPORATION MAKES NO REPRESENTATION ABOUT THE SUITABILITY     *|
|*     OF  THIS SOURCE  CODE  FOR ANY PURPOSE.  IT IS  PROVIDED  "AS IS"     *|
|*     WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  NVIDIA, CORPOR-     *|
|*     ATION DISCLAIMS ALL WARRANTIES  WITH REGARD  TO THIS SOURCE CODE,     *|
|*     INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGE-     *|
|*     MENT,  AND FITNESS  FOR A PARTICULAR PURPOSE.   IN NO EVENT SHALL     *|
|*     NVIDIA, CORPORATION  BE LIABLE FOR ANY SPECIAL,  INDIRECT,  INCI-     *|
|*     DENTAL, OR CONSEQUENTIAL DAMAGES,  OR ANY DAMAGES  WHATSOEVER RE-     *|
|*     SULTING FROM LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION     *|
|*     OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  ARISING OUT OF     *|
|*     OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOURCE CODE.     *|
|*                                                                           *|
|*     U.S. Government  End  Users.   This source code  is a "commercial     *|
|*     item,"  as that  term is  defined at  48 C.F.R. 2.101 (OCT 1995),     *|
|*     consisting  of "commercial  computer  software"  and  "commercial     *|
|*     computer  software  documentation,"  as such  terms  are  used in     *|
|*     48 C.F.R. 12.212 (SEPT 1995)  and is provided to the U.S. Govern-     *|
|*     ment only as  a commercial end item.   Consistent with  48 C.F.R.     *|
|*     12.212 and  48 C.F.R. 227.7202-1 through  227.7202-4 (JUNE 1995),     *|
|*     all U.S. Government End Users  acquire the source code  with only     *|
|*     those rights set forth herein.                                        *|
|*                                                                           *|
 \***************************************************************************/


#include <aros/libcall.h>
#include <aros/asmcall.h>

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/semaphores.h>

#include <utility/utility.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>

#include <oop/oop.h>

#include <hidd/pci.h>
#include <hidd/graphics.h>

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include "nv.h"
#include "nv_local.h"
#include "nv_dma.h"

#define SysBase			(sd->sysbase)
#define OOPBase			(sd->oopbase)
#define UtilityBase		(sd->utilitybase)

static inline __attribute__((always_inline))
    ULONG pciReadLong(struct staticdata *sd,
	UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    struct pHidd_PCIDriver_ReadConfigLong __msg = {
	sd->mid_ReadLong,
	bus, dev, sub, reg
    }, *msg = &__msg;
    
    return (ULONG)OOP_DoMethod(sd->pcidriver, (OOP_Msg)msg);
}

void nv4GetConfig(struct staticdata *sd)
{
    struct Card *pNv = &sd->Card;

    if (pNv->PFB[0x0000/4] & 0x00000100) {
        pNv->RamAmountKBytes = ((pNv->PFB[0x0000/4] >> 12) & 0x0F) * 1024 * 2
                              + 1024 * 2;
    } else {
        switch (pNv->PFB[0x0000/4] & 0x00000003) {
        case 0:
            pNv->RamAmountKBytes = 1024 * 32;
            break;
        case 1:
            pNv->RamAmountKBytes = 1024 * 4;
            break;
        case 2:
            pNv->RamAmountKBytes = 1024 * 8;
            break;
        case 3:
        default:
            pNv->RamAmountKBytes = 1024 * 16;
            break;
        }
    }
    pNv->CrystalFreqKHz = (pNv->PEXTDEV[0x0000/4] & 0x00000040) ? 14318 : 13500;
    pNv->CURSOR         = &(pNv->PRAMIN[0x1E00]);
    pNv->MinVClockFreqKHz = 12000;
    pNv->MaxVClockFreqKHz = 350000;
}

void nv10GetConfig (struct staticdata *sd)
{
    struct Card *pNv = &sd->Card;
    ULONG implementation = pNv->Chipset & 0x0ff0;

#if AROS_BIG_ENDIAN
    /* turn on big endian register access */
    if(!(pNv->PMC[0x0004/4] & 0x01000001)) {
       pNv->PMC[0x0004/4] = 0x01000001;
//       mem_barrier();
    }
#endif

    if(implementation == 0x01a0) {
        int amt = pciReadLong(sd, 0, 0, 1, 0x7C);
        pNv->RamAmountKBytes = (((amt >> 6) & 31) + 1) * 1024;
    } else if(implementation == 0x01f0) {
        int amt = pciReadLong(sd, 0, 0, 1, 0x84);
        pNv->RamAmountKBytes = (((amt >> 4) & 127) + 1) * 1024;
    } else {
        pNv->RamAmountKBytes = (pNv->PFB[0x020C/4] & 0xFFF00000) >> 10;
    }

    pNv->CrystalFreqKHz = (pNv->PEXTDEV[0x0000/4] & (1 << 6)) ? 14318 : 13500;
    
    if((implementation == 0x0170) ||
       (implementation == 0x0180) ||
       (implementation == 0x01F0) ||
       (implementation >= 0x0250))
    {
       if(pNv->PEXTDEV[0x0000/4] & (1 << 22))
           pNv->CrystalFreqKHz = 27000;
    }

    pNv->CursorStart      = (pNv->RamAmountKBytes - 96) * 1024;
    pNv->CURSOR           = NULL;  /* can't set this here */
    pNv->MinVClockFreqKHz = 12000;
    pNv->MaxVClockFreqKHz = pNv->twoStagePLL ? 400000 : 350000;
}



static inline void CRTC_out(struct staticdata *sd, UBYTE index, UBYTE val)
{
    VGA_WR08(sd->Card.PCIO, 0x3d4, index);
    VGA_WR08(sd->Card.PCIO, 0x3d5, val);
}

static inline UBYTE CRTC_in(struct staticdata *sd, UBYTE index)
{
    VGA_WR08(sd->Card.PCIO, 0x3d4, index);
    return VGA_RD08(sd->Card.PCIO, 0x3d5);
}

static inline void GRA_out(struct staticdata *sd, UBYTE index, UBYTE val)
{
    VGA_WR08(sd->Card.PVIO, 0x3ce, index);
    VGA_WR08(sd->Card.PVIO, 0x3cf, val);
}

static inline UBYTE GRA_in(struct staticdata *sd, UBYTE index)
{
    VGA_WR08(sd->Card.PVIO, 0x3ce, index);
    return VGA_RD08(sd->Card.PVIO, 0x3cf);
}

static inline void SEQ_out(struct staticdata *sd, UBYTE index, UBYTE val)
{
    VGA_WR08(sd->Card.PVIO, 0x3c4, index);
    VGA_WR08(sd->Card.PVIO, 0x3c5, val);
}

static inline UBYTE SEQ_in(struct staticdata *sd, UBYTE index)
{
    VGA_WR08(sd->Card.PVIO, 0x3c4, index);
    return VGA_RD08(sd->Card.PVIO, 0x3c5);
}

static inline void ATTR_out(struct staticdata *sd, UBYTE index, UBYTE val)
{
    UBYTE tmp;
    
    tmp = VGA_RD08(sd->Card.PCIO, 0x3da);
    if (sd->Card.paletteEnabled)
	index &= ~0x20;
    else
	index |= 0x20;
	
    VGA_WR08(sd->Card.PCIO, 0x3c0, index);
    VGA_WR08(sd->Card.PCIO, 0x3c0, val);
}

static inline UBYTE ATTR_in(struct staticdata *sd, UBYTE index)
{
    UBYTE tmp;
    
    tmp = VGA_RD08(sd->Card.PCIO, 0x3da);
    if (sd->Card.paletteEnabled)
	index &= ~0x20;
    else
	index |= 0x20;
    VGA_WR08(sd->Card.PCIO, 0x3c0, index);

    return VGA_RD08(sd->Card.PCIO, 0x3c1);
}

static inline void MISC_out(struct staticdata *sd, UBYTE val)
{
    VGA_WR08(sd->Card.PVIO, 0x3c2, val);
}

static inline UBYTE MISC_in(struct staticdata *sd)
{
    return VGA_RD08(sd->Card.PVIO, 0x3cc);
}

void NVLockUnlock(struct staticdata *sd, UBYTE Lock)
{
    UBYTE cr11;

    ObtainSemaphore(&sd->HWLock);
    VGA_WR08(sd->Card.PCIO, 0x3D4, 0x1F);
    VGA_WR08(sd->Card.PCIO, 0x3D5, Lock ? 0x99 : 0x57);

    VGA_WR08(sd->Card.PCIO, 0x3D4, 0x11);
    cr11 = VGA_RD08(sd->Card.PCIO, 0x3D5);
    if(Lock) cr11 |= 0x80;
    else cr11 &= ~0x80;
    VGA_WR08(sd->Card.PCIO, 0x3D5, cr11);
    ReleaseSemaphore(&sd->HWLock);
}

int NVShowHideCursor (struct staticdata *sd, UBYTE ShowHide)
{
    int current = sd->Card.CurrentState->cursor1;

    ObtainSemaphore(&sd->HWLock);
    sd->Card.CurrentState->cursor1 = (sd->Card.CurrentState->cursor1 & 0xFE) |
                                 (ShowHide & 0x01);
    VGA_WR08(sd->Card.PCIO, 0x3D4, 0x31);
    VGA_WR08(sd->Card.PCIO, 0x3D5, sd->Card.CurrentState->cursor1);

    ReleaseSemaphore(&sd->HWLock);
    return (current & 0x01);
}

/****************************************************************************\
*                                                                            *
* The video arbitration routines calculate some "magic" numbers.  Fixes      *
* the snow seen when accessing the framebuffer without it.                   *
* It just works (I hope).                                                    *
*                                                                            *
\****************************************************************************/

typedef struct {
  int graphics_lwm;
  int video_lwm;
  int graphics_burst_size;
  int video_burst_size;
  int valid;
} nv4_fifo_info;

typedef struct {
  int pclk_khz;
  int mclk_khz;
  int nvclk_khz;
  char mem_page_miss;
  char mem_latency;
  int memory_width;
  char enable_video;
  char gr_during_vid;
  char pix_bpp;
  char mem_aligned;
  char enable_mp;
} nv4_sim_state;

typedef struct {
  int graphics_lwm;
  int video_lwm;
  int graphics_burst_size;
  int video_burst_size;
  int valid;
} nv10_fifo_info;

typedef struct {
  int pclk_khz;
  int mclk_khz;
  int nvclk_khz;
  char mem_page_miss;
  char mem_latency;
  int memory_type;
  int memory_width;
  char enable_video;
  char gr_during_vid;
  char pix_bpp;
  char mem_aligned;
  char enable_mp;
} nv10_sim_state;

static void nvGetClocks(NVPtr pNv, ULONG *MClk, ULONG *NVClk)
{
    unsigned int pll, N, M, MB, NB, P;

    if(pNv->Architecture >= NV_ARCH_40) {
       pll = pNv->PMC[0x4020/4];
       P = (pll >> 16) & 0x03;
       pll = pNv->PMC[0x4024/4];
       M = pll & 0xFF;
       N = (pll >> 8) & 0xFF;
       MB = (pll >> 16) & 0xFF;
       NB = (pll >> 24) & 0xFF;
       *MClk = ((N * NB * pNv->CrystalFreqKHz) / (M * MB)) >> P;

       pll = pNv->PMC[0x4000/4];
       P = (pll >> 16) & 0x03;  
       pll = pNv->PMC[0x4004/4];
       M = pll & 0xFF;
       N = (pll >> 8) & 0xFF;
       MB = (pll >> 16) & 0xFF;
       NB = (pll >> 24) & 0xFF;

       *NVClk = ((N * NB * pNv->CrystalFreqKHz) / (M * MB)) >> P;
    } else
    if(pNv->twoStagePLL) {
       pll = pNv->PRAMDAC0[0x0504/4];
       M = pll & 0xFF; 
       N = (pll >> 8) & 0xFF; 
       P = (pll >> 16) & 0x0F;
       pll = pNv->PRAMDAC0[0x0574/4];
       if(pll & 0x80000000) {
           MB = pll & 0xFF; 
           NB = (pll >> 8) & 0xFF;
       } else {
           MB = 1;
           NB = 1;
       }
       *MClk = ((N * NB * pNv->CrystalFreqKHz) / (M * MB)) >> P;

       pll = pNv->PRAMDAC0[0x0500/4];
       M = pll & 0xFF; 
       N = (pll >> 8) & 0xFF; 
       P = (pll >> 16) & 0x0F;
       pll = pNv->PRAMDAC0[0x0570/4];
       if(pll & 0x80000000) {
           MB = pll & 0xFF;
           NB = (pll >> 8) & 0xFF;
       } else {
           MB = 1;
           NB = 1;
       }
       *NVClk = ((N * NB * pNv->CrystalFreqKHz) / (M * MB)) >> P;
    } else 
    if(((pNv->Chipset & 0x0ff0) == 0x0300) ||
       ((pNv->Chipset & 0x0ff0) == 0x0330))
    {
       pll = pNv->PRAMDAC0[0x0504/4];
       M = pll & 0x0F; 
       N = (pll >> 8) & 0xFF;
       P = (pll >> 16) & 0x07;
       if(pll & 0x00000080) {
           MB = (pll >> 4) & 0x07;     
           NB = (pll >> 19) & 0x1f;
       } else {
           MB = 1;
           NB = 1;
       }
       *MClk = ((N * NB * pNv->CrystalFreqKHz) / (M * MB)) >> P;

       pll = pNv->PRAMDAC0[0x0500/4];
       M = pll & 0x0F;
       N = (pll >> 8) & 0xFF;
       P = (pll >> 16) & 0x07;
       if(pll & 0x00000080) {
           MB = (pll >> 4) & 0x07;
           NB = (pll >> 19) & 0x1f;
       } else {
           MB = 1;
           NB = 1;
       }
       *NVClk = ((N * NB * pNv->CrystalFreqKHz) / (M * MB)) >> P;
    } else {
       pll = pNv->PRAMDAC0[0x0504/4];
       M = pll & 0xFF; 
       N = (pll >> 8) & 0xFF; 
       P = (pll >> 16) & 0x0F;
       *MClk = (N * pNv->CrystalFreqKHz / M) >> P;

       pll = pNv->PRAMDAC0[0x0500/4];
       M = pll & 0xFF; 
       N = (pll >> 8) & 0xFF; 
       P = (pll >> 16) & 0x0F;
       *NVClk = (N * pNv->CrystalFreqKHz / M) >> P;
    }
}

static void nv4CalcArbitration (
    nv4_fifo_info *fifo,
    nv4_sim_state *arb
)
{
    int data, pagemiss, cas,width, video_enable, bpp;
    int nvclks, mclks, pclks, vpagemiss, crtpagemiss, vbs;
    int found, mclk_extra, mclk_loop, cbs, m1, p1;
    int mclk_freq, pclk_freq, nvclk_freq, mp_enable;
    int us_m, us_n, us_p, video_drain_rate, crtc_drain_rate;
    int vpm_us, us_video, vlwm, video_fill_us, cpm_us, us_crt,clwm;

    fifo->valid = 1;
    pclk_freq = arb->pclk_khz;
    mclk_freq = arb->mclk_khz;
    nvclk_freq = arb->nvclk_khz;
    pagemiss = arb->mem_page_miss;
    cas = arb->mem_latency;
    width = arb->memory_width >> 6;
    video_enable = arb->enable_video;
    bpp = arb->pix_bpp;
    mp_enable = arb->enable_mp;
    clwm = 0;
    vlwm = 0;
    cbs = 128;
    pclks = 2;
    nvclks = 2;
    nvclks += 2;
    nvclks += 1;
    mclks = 5;
    mclks += 3;
    mclks += 1;
    mclks += cas;
    mclks += 1;
    mclks += 1;
    mclks += 1;
    mclks += 1;
    mclk_extra = 3;
    nvclks += 2;
    nvclks += 1;
    nvclks += 1;
    nvclks += 1;
    if (mp_enable)
        mclks+=4;
    nvclks += 0;
    pclks += 0;
    found = 0;
    vbs = 0;
    while (found != 1)
    {
        fifo->valid = 1;
        found = 1;
        mclk_loop = mclks+mclk_extra;
        us_m = mclk_loop *1000*1000 / mclk_freq;
        us_n = nvclks*1000*1000 / nvclk_freq;
        us_p = nvclks*1000*1000 / pclk_freq;
        if (video_enable)
        {
            video_drain_rate = pclk_freq * 2;
            crtc_drain_rate = pclk_freq * bpp/8;
            vpagemiss = 2;
            vpagemiss += 1;
            crtpagemiss = 2;
            vpm_us = (vpagemiss * pagemiss)*1000*1000/mclk_freq;
            if (nvclk_freq * 2 > mclk_freq * width)
                video_fill_us = cbs*1000*1000 / 16 / nvclk_freq ;
            else
                video_fill_us = cbs*1000*1000 / (8 * width) / mclk_freq;
            us_video = vpm_us + us_m + us_n + us_p + video_fill_us;
            vlwm = us_video * video_drain_rate/(1000*1000);
            vlwm++;
            vbs = 128;
            if (vlwm > 128) vbs = 64;
            if (vlwm > (256-64)) vbs = 32;
            if (nvclk_freq * 2 > mclk_freq * width)
                video_fill_us = vbs *1000*1000/ 16 / nvclk_freq ;
            else
                video_fill_us = vbs*1000*1000 / (8 * width) / mclk_freq;
            cpm_us = crtpagemiss  * pagemiss *1000*1000/ mclk_freq;
            us_crt =
            us_video
            +video_fill_us
            +cpm_us
            +us_m + us_n +us_p
            ;
            clwm = us_crt * crtc_drain_rate/(1000*1000);
            clwm++;
        }
        else
        {
            crtc_drain_rate = pclk_freq * bpp/8;
            crtpagemiss = 2;
            crtpagemiss += 1;
            cpm_us = crtpagemiss  * pagemiss *1000*1000/ mclk_freq;
            us_crt =  cpm_us + us_m + us_n + us_p ;
            clwm = us_crt * crtc_drain_rate/(1000*1000);
            clwm++;
        }
        m1 = clwm + cbs - 512;
        p1 = m1 * pclk_freq / mclk_freq;
        p1 = p1 * bpp / 8;
        if ((p1 < m1) && (m1 > 0))
        {
            fifo->valid = 0;
            found = 0;
            if (mclk_extra ==0)   found = 1;
            mclk_extra--;
        }
        else if (video_enable)
        {
            if ((clwm > 511) || (vlwm > 255))
            {
                fifo->valid = 0;
                found = 0;
                if (mclk_extra ==0)   found = 1;
                mclk_extra--;
            }
        }
        else
        {
            if (clwm > 519)
            {
                fifo->valid = 0;
                found = 0;
                if (mclk_extra ==0)   found = 1;
                mclk_extra--;
            }
        }
        if (clwm < 384) clwm = 384;
        if (vlwm < 128) vlwm = 128;
        data = (int)(clwm);
        fifo->graphics_lwm = data;
        fifo->graphics_burst_size = 128;
        data = (int)((vlwm+15));
        fifo->video_lwm = data;
        fifo->video_burst_size = vbs;
    }
}

static void nv4UpdateArbitrationSettings (
    ULONG      VClk, 
    ULONG      pixelDepth, 
    ULONG      *burst,
    ULONG      *lwm,
    NVPtr        pNv
)
{
    nv4_fifo_info fifo_data;
    nv4_sim_state sim_data;
    ULONG MClk, NVClk, cfg1;

    nvGetClocks(pNv, &MClk, &NVClk);

    cfg1 = pNv->PFB[0x00000204/4];
    sim_data.pix_bpp        = (char)pixelDepth;
    sim_data.enable_video   = 0;
    sim_data.enable_mp      = 0;
    sim_data.memory_width   = (pNv->PEXTDEV[0x0000/4] & 0x10) ? 128 : 64;
    sim_data.mem_latency    = (char)cfg1 & 0x0F;
    sim_data.mem_aligned    = 1;
    sim_data.mem_page_miss  = (char)(((cfg1 >> 4) &0x0F) + ((cfg1 >> 31) & 0x01));
    sim_data.gr_during_vid  = 0;
    sim_data.pclk_khz       = VClk;
    sim_data.mclk_khz       = MClk;
    sim_data.nvclk_khz      = NVClk;
    nv4CalcArbitration(&fifo_data, &sim_data);
    if (fifo_data.valid)
    {
        int  b = fifo_data.graphics_burst_size >> 4;
        *burst = 0;
        while (b >>= 1) (*burst)++;
        *lwm   = fifo_data.graphics_lwm >> 3;
    }
}

static void nv10CalcArbitration (
    nv10_fifo_info *fifo,
    nv10_sim_state *arb
)
{
    int data, pagemiss, width, video_enable, bpp;
    int nvclks, mclks, pclks, vpagemiss, crtpagemiss;
    int nvclk_fill;
    int found, mclk_extra, mclk_loop, cbs, m1;
    int mclk_freq, pclk_freq, nvclk_freq, mp_enable;
    int us_m, us_m_min, us_n, us_p, crtc_drain_rate;
    int vus_m;
    int vpm_us, us_video, cpm_us, us_crt,clwm;
    int clwm_rnd_down;
    int m2us, us_pipe_min, p1clk, p2;
    int min_mclk_extra;
    int us_min_mclk_extra;

    fifo->valid = 1;
    pclk_freq = arb->pclk_khz; /* freq in KHz */
    mclk_freq = arb->mclk_khz;
    nvclk_freq = arb->nvclk_khz;
    pagemiss = arb->mem_page_miss;
    width = arb->memory_width/64;
    video_enable = arb->enable_video;
    bpp = arb->pix_bpp;
    mp_enable = arb->enable_mp;
    clwm = 0;

    cbs = 512;

    pclks = 4; /* lwm detect. */

    nvclks = 3; /* lwm -> sync. */
    nvclks += 2; /* fbi bus cycles (1 req + 1 busy) */

    mclks  = 1;   /* 2 edge sync.  may be very close to edge so just put one. */

    mclks += 1;   /* arb_hp_req */
    mclks += 5;   /* ap_hp_req   tiling pipeline */

    mclks += 2;    /* tc_req     latency fifo */
    mclks += 2;    /* fb_cas_n_  memory request to fbio block */
    mclks += 7;    /* sm_d_rdv   data returned from fbio block */

    /* fb.rd.d.Put_gc   need to accumulate 256 bits for read */
    if (arb->memory_type == 0)
      if (arb->memory_width == 64) /* 64 bit bus */
        mclks += 4;
      else
        mclks += 2;
    else
      if (arb->memory_width == 64) /* 64 bit bus */
        mclks += 2;
      else
        mclks += 1;

    if ((!video_enable) && (arb->memory_width == 128))
    {  
      mclk_extra = (bpp == 32) ? 31 : 42; /* Margin of error */
      min_mclk_extra = 17;
    }
    else
    {
      mclk_extra = (bpp == 32) ? 8 : 4; /* Margin of error */
      /* mclk_extra = 4; */ /* Margin of error */
      min_mclk_extra = 18;
    }

    nvclks += 1; /* 2 edge sync.  may be very close to edge so just put one. */
    nvclks += 1; /* fbi_d_rdv_n */
    nvclks += 1; /* Fbi_d_rdata */
    nvclks += 1; /* crtfifo load */

    if(mp_enable)
      mclks+=4; /* Mp can get in with a burst of 8. */
    /* Extra clocks determined by heuristics */

    nvclks += 0;
    pclks += 0;
    found = 0;
    while(found != 1) {
      fifo->valid = 1;
      found = 1;
      mclk_loop = mclks+mclk_extra;
      us_m = mclk_loop *1000*1000 / mclk_freq; /* Mclk latency in us */
      us_m_min = mclks * 1000*1000 / mclk_freq; /* Minimum Mclk latency in us */
      us_min_mclk_extra = min_mclk_extra *1000*1000 / mclk_freq;
      us_n = nvclks*1000*1000 / nvclk_freq;/* nvclk latency in us */
      us_p = pclks*1000*1000 / pclk_freq;/* nvclk latency in us */
      us_pipe_min = us_m_min + us_n + us_p;

      vus_m = mclk_loop *1000*1000 / mclk_freq; /* Mclk latency in us */

      if(video_enable) {
        crtc_drain_rate = pclk_freq * bpp/8; /* MB/s */

        vpagemiss = 1; /* self generating page miss */
        vpagemiss += 1; /* One higher priority before */

        crtpagemiss = 2; /* self generating page miss */
        if(mp_enable)
            crtpagemiss += 1; /* if MA0 conflict */

        vpm_us = (vpagemiss * pagemiss)*1000*1000/mclk_freq;

        us_video = vpm_us + vus_m; /* Video has separate read return path */

        cpm_us = crtpagemiss  * pagemiss *1000*1000/ mclk_freq;
        us_crt =
          us_video  /* Wait for video */
          +cpm_us /* CRT Page miss */
          +us_m + us_n +us_p /* other latency */
          ;

        clwm = us_crt * crtc_drain_rate/(1000*1000);
        clwm++; /* fixed point <= float_point - 1.  Fixes that */
      } else {
        crtc_drain_rate = pclk_freq * bpp/8; /* bpp * pclk/8 */

        crtpagemiss = 1; /* self generating page miss */
        crtpagemiss += 1; /* MA0 page miss */
        if(mp_enable)
            crtpagemiss += 1; /* if MA0 conflict */
        cpm_us = crtpagemiss  * pagemiss *1000*1000/ mclk_freq;
        us_crt =  cpm_us + us_m + us_n + us_p ;
        clwm = us_crt * crtc_drain_rate/(1000*1000);
        clwm++; /* fixed point <= float_point - 1.  Fixes that */

  /*
          //
          // Another concern, only for high pclks so don't do this
          // with video:
          // What happens if the latency to fetch the cbs is so large that
          // fifo empties.  In that case we need to have an alternate clwm value
          // based off the total burst fetch
          //
          us_crt = (cbs * 1000 * 1000)/ (8*width)/mclk_freq ;
          us_crt = us_crt + us_m + us_n + us_p + (4 * 1000 * 1000)/mclk_freq;
          clwm_mt = us_crt * crtc_drain_rate/(1000*1000);
          clwm_mt ++;
          if(clwm_mt > clwm)
              clwm = clwm_mt;
  */
          /* Finally, a heuristic check when width == 64 bits */
          if(width == 1){
              nvclk_fill = nvclk_freq * 8;
              if(crtc_drain_rate * 100 >= nvclk_fill * 102)
                      clwm = 0xfff; /*Large number to fail */

              else if(crtc_drain_rate * 100  >= nvclk_fill * 98) {
                  clwm = 1024;
                  cbs = 512;
              }
          }
      }


      /*
        Overfill check:

        */

      clwm_rnd_down = ((int)clwm/8)*8;
      if (clwm_rnd_down < clwm)
          clwm += 8;

      m1 = clwm + cbs -  1024; /* Amount of overfill */
      m2us = us_pipe_min + us_min_mclk_extra;

      /* pclk cycles to drain */
      p1clk = m2us * pclk_freq/(1000*1000); 
      p2 = p1clk * bpp / 8; /* bytes drained. */

      if((p2 < m1) && (m1 > 0)) {
          fifo->valid = 0;
          found = 0;
          if(min_mclk_extra == 0)   {
            if(cbs <= 32) {
              found = 1; /* Can't adjust anymore! */
            } else {
              cbs = cbs/2;  /* reduce the burst size */
            }
          } else {
            min_mclk_extra--;
          }
      } else {
        if (clwm > 1023){ /* Have some margin */
          fifo->valid = 0;
          found = 0;
          if(min_mclk_extra == 0)   
              found = 1; /* Can't adjust anymore! */
          else 
              min_mclk_extra--;
        }
      }

      if(clwm < (1024-cbs+8)) clwm = 1024-cbs+8;
      data = (int)(clwm);
      /*  printf("CRT LWM: %f bytes, prog: 0x%x, bs: 256\n", clwm, data ); */
      fifo->graphics_lwm = data;   fifo->graphics_burst_size = cbs;

      fifo->video_lwm = 1024;  fifo->video_burst_size = 512;
    }
}

static void nv10UpdateArbitrationSettings (
    ULONG      VClk, 
    ULONG      pixelDepth, 
    ULONG     *burst,
    ULONG     *lwm,
    NVPtr        pNv
)
{
    nv10_fifo_info fifo_data;
    nv10_sim_state sim_data;
    ULONG MClk, NVClk, cfg1;

    nvGetClocks(pNv, &MClk, &NVClk);

    cfg1 = pNv->PFB[0x0204/4];
    sim_data.pix_bpp        = (char)pixelDepth;
    sim_data.enable_video   = 1;
    sim_data.enable_mp      = 0;
    sim_data.memory_type    = (pNv->PFB[0x0200/4] & 0x01) ? 1 : 0;
    sim_data.memory_width   = (pNv->PEXTDEV[0x0000/4] & 0x10) ? 128 : 64;
    sim_data.mem_latency    = (char)cfg1 & 0x0F;
    sim_data.mem_aligned    = 1;
    sim_data.mem_page_miss  = (char)(((cfg1>>4) &0x0F) + ((cfg1>>31) & 0x01));
    sim_data.gr_during_vid  = 0;
    sim_data.pclk_khz       = VClk;
    sim_data.mclk_khz       = MClk;
    sim_data.nvclk_khz      = NVClk;

    nv10CalcArbitration(&fifo_data, &sim_data);
    if (fifo_data.valid) {
        int  b = fifo_data.graphics_burst_size >> 4;
        *burst = 0;
        while (b >>= 1) (*burst)++;
        *lwm   = fifo_data.graphics_lwm >> 3;
    }
}

static void nForceUpdateArbitrationSettings (
    ULONG      VClk,
    ULONG      pixelDepth,
    ULONG     *burst,
    ULONG     *lwm,
    struct staticdata *sd,
    NVPtr        pNv
)
{
    nv10_fifo_info fifo_data;
    nv10_sim_state sim_data;
    unsigned int M, N, P, pll, MClk, NVClk, memctrl;

    if((pNv->Chipset & 0x0FF0) == 0x01A0) {
       unsigned int uMClkPostDiv;

       uMClkPostDiv = (pciReadLong(sd, 0, 0, 3, 0x6C) >> 8) & 0xf;
       if(!uMClkPostDiv) uMClkPostDiv = 4; 
       MClk = 400000 / uMClkPostDiv;
    } else {
       MClk = pciReadLong(sd, 0, 0, 5, 0x4C) / 1000;
    }

    pll = pNv->PRAMDAC0[0x0500/4];
    M = (pll >> 0)  & 0xFF; N = (pll >> 8)  & 0xFF; P = (pll >> 16) & 0x0F;
    NVClk  = (N * pNv->CrystalFreqKHz / M) >> P;
    sim_data.pix_bpp        = (char)pixelDepth;
    sim_data.enable_video   = 0;
    sim_data.enable_mp      = 0;
    sim_data.memory_type    = (pciReadLong(sd, 0, 0, 1, 0x7C) >> 12) & 1;
    sim_data.memory_width   = 64;

    memctrl = pciReadLong(sd, 0, 0, 3, 0x00) >> 16;

    if((memctrl == 0x1A9) || (memctrl == 0x1AB) || (memctrl == 0x1ED)) {
        int dimm[3];

        dimm[0] = (pciReadLong(sd, 0, 0, 2, 0x40) >> 8) & 0x4F;
        dimm[1] = (pciReadLong(sd, 0, 0, 2, 0x44) >> 8) & 0x4F;
        dimm[2] = (pciReadLong(sd, 0, 0, 2, 0x48) >> 8) & 0x4F;

        if((dimm[0] + dimm[1]) != dimm[2]) {
             bug("WARNING: "
              "your nForce DIMMs are not arranged in optimal banks!\n");
        } 
    }

    sim_data.mem_latency    = 3;
    sim_data.mem_aligned    = 1;
    sim_data.mem_page_miss  = 10;
    sim_data.gr_during_vid  = 0;
    sim_data.pclk_khz       = VClk;
    sim_data.mclk_khz       = MClk;
    sim_data.nvclk_khz      = NVClk;
    nv10CalcArbitration(&fifo_data, &sim_data);
    if (fifo_data.valid)
    {
        int  b = fifo_data.graphics_burst_size >> 4;
        *burst = 0;
        while (b >>= 1) (*burst)++;
        *lwm   = fifo_data.graphics_lwm >> 3;
    }
}

/****************************************************************************\
*                                                                            *
*                          RIVA Mode State Routines                          *
*                                                                            *
\****************************************************************************/

/*
 * Calculate the Video Clock parameters for the PLL.
 */
static void CalcVClock (
    ULONG        clockIn,
    ULONG        *clockOut,
    ULONG        *pllOut,
    NVPtr        pNv
)
{
    unsigned lowM, highM;
    unsigned DeltaNew, DeltaOld;
    unsigned VClk, Freq;
    unsigned M, N, P;
    
    DeltaOld = 0xFFFFFFFF;

    VClk = (unsigned)clockIn;
    
    if (pNv->CrystalFreqKHz == 13500) {
        lowM  = 7;
        highM = 13;
    } else {
        lowM  = 8;
        highM = 14;
    }

    for (P = 0; P <= 4; P++) {
        Freq = VClk << P;
        if ((Freq >= 128000) && (Freq <= 350000)) {
            for (M = lowM; M <= highM; M++) {
                N = ((VClk << P) * M) / pNv->CrystalFreqKHz;
                if(N <= 255) {
                    Freq = ((pNv->CrystalFreqKHz * N) / M) >> P;
                    if (Freq > VClk)
                        DeltaNew = Freq - VClk;
                    else
                        DeltaNew = VClk - Freq;
                    if (DeltaNew < DeltaOld) {
                        *pllOut   = (P << 16) | (N << 8) | M;
                        *clockOut = Freq;
                        DeltaOld  = DeltaNew;
                    }
                }
            }
        }
    }
}

static void CalcVClock2Stage (
    ULONG        clockIn,
    ULONG        *clockOut,
    ULONG        *pllOut,
    ULONG        *pllBOut,
    NVPtr        pNv
)
{
    unsigned DeltaNew, DeltaOld;
    unsigned VClk, Freq;
    unsigned M, N, P;

    DeltaOld = 0xFFFFFFFF;

    *pllBOut = 0x80000401;  /* fixed at x4 for now */

    VClk = (unsigned)clockIn;

    for (P = 0; P <= 6; P++) {
        Freq = VClk << P;
        if ((Freq >= 400000) && (Freq <= 1000000)) {
            for (M = 1; M <= 13; M++) {
                N = ((VClk << P) * M) / (pNv->CrystalFreqKHz << 2);
                if((N >= 5) && (N <= 255)) {
                    Freq = (((pNv->CrystalFreqKHz << 2) * N) / M) >> P;
                    if (Freq > VClk)
                        DeltaNew = Freq - VClk;
                    else
                        DeltaNew = VClk - Freq;
                    if (DeltaNew < DeltaOld) {
                        *pllOut   = (P << 16) | (N << 8) | M;
                        *clockOut = Freq;
                        DeltaOld  = DeltaNew;
                    }
                }
            }
        }
    }
}


/*
 * Calculate extended mode parameters (SVGA) and save in a 
 * mode state structure.
 */
void NVCalcStateExt (
    struct staticdata *sd,
    NVPtr pNv,
    RIVA_HW_STATE *state,
    int            bpp,
    int            width,
    int            hDisplaySize,
    int            height,
    int            dotClock,
    int		   flags 
)
{
    ULONG pixelDepth, VClk;
    /*
     * Save mode parameters.
     */
    state->bpp    = bpp;    /* this is not bitsPerPixel, it's 8,15,16,32 */
    state->width  = width;
    state->height = height;
    /*
     * Extended RIVA registers.
     */
    pixelDepth = (bpp + 1)/8;

    if(pNv->twoStagePLL)
        CalcVClock2Stage(dotClock, &VClk, &state->pll, &state->pllB, pNv);
    else
        CalcVClock(dotClock, &VClk, &state->pll, pNv);

    switch (pNv->Architecture)
    {
        case NV_ARCH_04:
            nv4UpdateArbitrationSettings(VClk, 
                                         pixelDepth * 8, 
                                        &(state->arbitration0),
                                        &(state->arbitration1),
                                         pNv);
            state->cursor0  = 0x00;
            state->cursor1  = 0xbC;
	    if (flags & V_DBLSCAN)
		state->cursor1 |= 2;
            state->cursor2  = 0x00000000;
            state->pllsel   = 0x10000700;
            state->config   = 0x00001114;
            state->general  = bpp == 16 ? 0x00101100 : 0x00100100;
            state->repaint1 = hDisplaySize < 1280 ? 0x04 : 0x00;
            break;
        case NV_ARCH_10:
        case NV_ARCH_20:
        case NV_ARCH_30:
        default:
            if(((pNv->Chipset & 0xffff) == 0x01A0) ||
               ((pNv->Chipset & 0xffff) == 0x01f0))
            {
                nForceUpdateArbitrationSettings(VClk,
                                          pixelDepth * 8,
                                         &(state->arbitration0),
                                         &(state->arbitration1),
					  sd,
                                          pNv);
            } else {
                nv10UpdateArbitrationSettings(VClk, 
                                          pixelDepth * 8, 
                                         &(state->arbitration0),
                                         &(state->arbitration1),
                                          pNv);
            }
            state->cursor0  = 0x80 | (pNv->CursorStart >> 17);
            state->cursor1  = (pNv->CursorStart >> 11) << 2;
	    state->cursor2  = pNv->CursorStart >> 24;
	    if (flags & V_DBLSCAN) 
		state->cursor1 |= 2;
            state->pllsel   = 0x10000700;
            state->config   = pNv->PFB[0x00000200/4];
            state->general  = bpp == 16 ? 0x00101100 : 0x00100100;
            state->repaint1 = hDisplaySize < 1280 ? 0x04 : 0x00;
            break;
    }
    if(bpp != 8) /* DirectColor */
	state->general |= 0x00000030;

    state->repaint0 = (((width / 8) * pixelDepth) & 0x700) >> 3;
    state->pixel    = (pixelDepth > 2) ? 3 : pixelDepth;
}

/*
** stegerg: Status NVLoadStateExt(): should "match" xfree nv NVLoadStateExt()
** in xc/programs/Xserver/hw/xfree86/drivers/nv/nv_hw.c version:
** 1.8 2004/03/20 01:52:16 mvojkovi
** 
** Exception: some waitvsyncpossible stuff commented out
*/
     
void NVLoadStateExt (
    NVPtr pNv,
    RIVA_HW_STATE *state
)
{
    int i;

    pNv->PMC[0x0140/4] = 0x00000000;
    pNv->PMC[0x0200/4] = 0xFFFF00FF;
    pNv->PMC[0x0200/4] = 0xFFFFFFFF;

    pNv->PTIMER[0x0200] = 0x00000008;
    pNv->PTIMER[0x0210] = 0x00000003;
    pNv->PTIMER[0x0140] = 0x00000000;
    pNv->PTIMER[0x0100] = 0xFFFFFFFF;

    if(pNv->Architecture == NV_ARCH_04) {
        pNv->PFB[0x0200/4] = state->config;
    } else {
        pNv->PFB[0x0240/4] = 0;
        pNv->PFB[0x0244/4] = pNv->FrameBufferSize - 1;
        pNv->PFB[0x0250/4] = 0;
        pNv->PFB[0x0254/4] = pNv->FrameBufferSize - 1;
        pNv->PFB[0x0260/4] = 0;
        pNv->PFB[0x0264/4] = pNv->FrameBufferSize - 1;
        pNv->PFB[0x0270/4] = 0;
        pNv->PFB[0x0274/4] = pNv->FrameBufferSize - 1;
        pNv->PFB[0x0280/4] = 0;
        pNv->PFB[0x0284/4] = pNv->FrameBufferSize - 1;
        pNv->PFB[0x0290/4] = 0;
        pNv->PFB[0x0294/4] = pNv->FrameBufferSize - 1;
        pNv->PFB[0x02A0/4] = 0;
        pNv->PFB[0x02A4/4] = pNv->FrameBufferSize - 1;
        pNv->PFB[0x02B0/4] = 0;
        pNv->PFB[0x02B4/4] = pNv->FrameBufferSize - 1;
    }
 
    if(pNv->Architecture >= NV_ARCH_40) {
       pNv->PRAMIN[0x0000] = 0x80000010;
       pNv->PRAMIN[0x0001] = 0x00101202;
       pNv->PRAMIN[0x0002] = 0x80000011;
       pNv->PRAMIN[0x0003] = 0x00101204;
       pNv->PRAMIN[0x0004] = 0x80000012;
       pNv->PRAMIN[0x0005] = 0x00101206;
       pNv->PRAMIN[0x0006] = 0x80000013;
       pNv->PRAMIN[0x0007] = 0x00101208;
       pNv->PRAMIN[0x0008] = 0x80000014;
       pNv->PRAMIN[0x0009] = 0x0010120A;
       pNv->PRAMIN[0x000A] = 0x80000015;
       pNv->PRAMIN[0x000B] = 0x0010120C;
       pNv->PRAMIN[0x000C] = 0x80000016;
       pNv->PRAMIN[0x000D] = 0x0010120E;
       pNv->PRAMIN[0x000E] = 0x80000017;
       pNv->PRAMIN[0x000F] = 0x00101210;
       pNv->PRAMIN[0x0800] = 0x00003000;
       pNv->PRAMIN[0x0801] = pNv->FrameBufferSize - 1;
       pNv->PRAMIN[0x0802] = 0x00000002;
       pNv->PRAMIN[0x0808] = 0x02080062;
       pNv->PRAMIN[0x0809] = 0x00000000;
       pNv->PRAMIN[0x080A] = 0x00001200;
       pNv->PRAMIN[0x080B] = 0x00001200;
       pNv->PRAMIN[0x080C] = 0x00000000;
       pNv->PRAMIN[0x080D] = 0x00000000;
       pNv->PRAMIN[0x0810] = 0x02080043;
       pNv->PRAMIN[0x0811] = 0x00000000;
       pNv->PRAMIN[0x0812] = 0x00000000;
       pNv->PRAMIN[0x0813] = 0x00000000;
       pNv->PRAMIN[0x0814] = 0x00000000;
       pNv->PRAMIN[0x0815] = 0x00000000;
       pNv->PRAMIN[0x0818] = 0x02080044;
       pNv->PRAMIN[0x0819] = 0x02000000;
       pNv->PRAMIN[0x081A] = 0x00000000;
       pNv->PRAMIN[0x081B] = 0x00000000;
       pNv->PRAMIN[0x081C] = 0x00000000;
       pNv->PRAMIN[0x081D] = 0x00000000;
       pNv->PRAMIN[0x0820] = 0x02080019;
       pNv->PRAMIN[0x0821] = 0x00000000;
       pNv->PRAMIN[0x0822] = 0x00000000;
       pNv->PRAMIN[0x0823] = 0x00000000;
       pNv->PRAMIN[0x0824] = 0x00000000;
       pNv->PRAMIN[0x0825] = 0x00000000;
       pNv->PRAMIN[0x0828] = 0x020A005C;
       pNv->PRAMIN[0x0829] = 0x00000000;
       pNv->PRAMIN[0x082A] = 0x00000000;
       pNv->PRAMIN[0x082B] = 0x00000000;
       pNv->PRAMIN[0x082C] = 0x00000000;
       pNv->PRAMIN[0x082D] = 0x00000000;
       pNv->PRAMIN[0x0830] = 0x0208009F;
       pNv->PRAMIN[0x0831] = 0x00000000;
       pNv->PRAMIN[0x0832] = 0x00001200;
       pNv->PRAMIN[0x0833] = 0x00001200;
       pNv->PRAMIN[0x0834] = 0x00000000;
       pNv->PRAMIN[0x0835] = 0x00000000;
       pNv->PRAMIN[0x0838] = 0x0208004A;
       pNv->PRAMIN[0x0839] = 0x02000000;
       pNv->PRAMIN[0x083A] = 0x00000000;
       pNv->PRAMIN[0x083B] = 0x00000000;
       pNv->PRAMIN[0x083C] = 0x00000000;
       pNv->PRAMIN[0x083D] = 0x00000000;
       pNv->PRAMIN[0x0840] = 0x02080077;
       pNv->PRAMIN[0x0841] = 0x00000000;
       pNv->PRAMIN[0x0842] = 0x00001200;
       pNv->PRAMIN[0x0843] = 0x00001200;
       pNv->PRAMIN[0x0844] = 0x00000000;
       pNv->PRAMIN[0x0845] = 0x00000000;
       pNv->PRAMIN[0x084C] = 0x00003002;
       pNv->PRAMIN[0x084D] = 0x00007FFF;
       pNv->PRAMIN[0x084E] = pNv->FbUsableSize | 0x00000002;

#if AROS_BIG_ENDIAN
       pNv->PRAMIN[0x080A] |= 0x01000000;
       pNv->PRAMIN[0x0812] |= 0x01000000;
       pNv->PRAMIN[0x081A] |= 0x01000000;
       pNv->PRAMIN[0x0822] |= 0x01000000;
       pNv->PRAMIN[0x082A] |= 0x01000000;
       pNv->PRAMIN[0x0832] |= 0x01000000;
       pNv->PRAMIN[0x083A] |= 0x01000000;
       pNv->PRAMIN[0x0842] |= 0x01000000;  
       pNv->PRAMIN[0x0819] = 0x01000000;
       pNv->PRAMIN[0x0839] = 0x01000000;
#endif
    } else {

	pNv->PRAMIN[0x0000] = 0x80000010;
	pNv->PRAMIN[0x0001] = 0x80011201;  
	pNv->PRAMIN[0x0002] = 0x80000011;
	pNv->PRAMIN[0x0003] = 0x80011202; 
	pNv->PRAMIN[0x0004] = 0x80000012;
	pNv->PRAMIN[0x0005] = 0x80011203;
	pNv->PRAMIN[0x0006] = 0x80000013;
	pNv->PRAMIN[0x0007] = 0x80011204;
	pNv->PRAMIN[0x0008] = 0x80000014;
	pNv->PRAMIN[0x0009] = 0x80011205;
	pNv->PRAMIN[0x000A] = 0x80000015;
	pNv->PRAMIN[0x000B] = 0x80011206;
	pNv->PRAMIN[0x000C] = 0x80000016;
	pNv->PRAMIN[0x000D] = 0x80011207;
	pNv->PRAMIN[0x000E] = 0x80000017;
	pNv->PRAMIN[0x000F] = 0x80011208;
	pNv->PRAMIN[0x0800] = 0x00003000;
	pNv->PRAMIN[0x0801] = pNv->FrameBufferSize - 1;
	pNv->PRAMIN[0x0802] = 0x00000002;
	pNv->PRAMIN[0x0803] = 0x00000002;
	if(pNv->Architecture >= NV_ARCH_10)
	   pNv->PRAMIN[0x0804] = 0x01008062;
	else
	   pNv->PRAMIN[0x0804] = 0x01008042;
	pNv->PRAMIN[0x0805] = 0x00000000;
	pNv->PRAMIN[0x0806] = 0x12001200;
	pNv->PRAMIN[0x0807] = 0x00000000;
	pNv->PRAMIN[0x0808] = 0x01008043;
	pNv->PRAMIN[0x0809] = 0x00000000;
	pNv->PRAMIN[0x080A] = 0x00000000;
	pNv->PRAMIN[0x080B] = 0x00000000;
	pNv->PRAMIN[0x080C] = 0x01008044;
	pNv->PRAMIN[0x080D] = 0x00000002;
	pNv->PRAMIN[0x080E] = 0x00000000;
	pNv->PRAMIN[0x080F] = 0x00000000;
	pNv->PRAMIN[0x0810] = 0x01008019;
	pNv->PRAMIN[0x0811] = 0x00000000;
	pNv->PRAMIN[0x0812] = 0x00000000;
	pNv->PRAMIN[0x0813] = 0x00000000;
	pNv->PRAMIN[0x0814] = 0x0100A05C;
	pNv->PRAMIN[0x0815] = 0x00000000;
	pNv->PRAMIN[0x0816] = 0x00000000;
	pNv->PRAMIN[0x0817] = 0x00000000;
// aros: commented out for now
//        if(pNv->WaitVSyncPossible)
//            pNv->PRAMIN[0x0818] = 0x0100809F;
//        else
	    pNv->PRAMIN[0x0818] = 0x0100805F;
	pNv->PRAMIN[0x0819] = 0x00000000;
	pNv->PRAMIN[0x081A] = 0x12001200;
	pNv->PRAMIN[0x081B] = 0x00000000;
	pNv->PRAMIN[0x081C] = 0x0100804A;
	pNv->PRAMIN[0x081D] = 0x00000002;
	pNv->PRAMIN[0x081E] = 0x00000000;
	pNv->PRAMIN[0x081F] = 0x00000000;
	pNv->PRAMIN[0x0820] = 0x01018077;
	pNv->PRAMIN[0x0821] = 0x00000000;
	pNv->PRAMIN[0x0822] = 0x12001200;
	pNv->PRAMIN[0x0823] = 0x00000000;
	pNv->PRAMIN[0x0824] = 0x00003002;
	pNv->PRAMIN[0x0825] = 0x00007FFF;
	pNv->PRAMIN[0x0826] = pNv->FbUsableSize | 0x00000002;
	pNv->PRAMIN[0x0827] = 0x00000002;

#if AROS_BIG_ENDIAN
	pNv->PRAMIN[0x0804] |= 0x00080000;
	pNv->PRAMIN[0x0808] |= 0x00080000;
	pNv->PRAMIN[0x080C] |= 0x00080000;
	pNv->PRAMIN[0x0810] |= 0x00080000;
	pNv->PRAMIN[0x0814] |= 0x00080000;
	pNv->PRAMIN[0x0818] |= 0x00080000;
	pNv->PRAMIN[0x081C] |= 0x00080000;
	pNv->PRAMIN[0x0820] |= 0x00080000;

	pNv->PRAMIN[0x080D] = 0x00000001;
	pNv->PRAMIN[0x081D] = 0x00000001;
#endif
    }
    
    if(pNv->Architecture < NV_ARCH_10) {
       if((pNv->Chipset & 0x0fff) == 0x0020) {
           pNv->PRAMIN[0x0824] |= 0x00020000;
           pNv->PRAMIN[0x0826] += (IPTR)pNv->FbAddress;
       }
       pNv->PGRAPH[0x0080/4] = 0x000001FF;
       pNv->PGRAPH[0x0080/4] = 0x1230C000;
       pNv->PGRAPH[0x0084/4] = 0x72111101;
       pNv->PGRAPH[0x0088/4] = 0x11D5F071;
       pNv->PGRAPH[0x008C/4] = 0x0004FF31; /* stegerg: strange but that's so in xfree nv driver sources */
       pNv->PGRAPH[0x008C/4] = 0x4004FF31; /* stegerg: strange but that's so in xfree nv driver sources */

       pNv->PGRAPH[0x0140/4] = 0x00000000;
       pNv->PGRAPH[0x0100/4] = 0xFFFFFFFF;
       pNv->PGRAPH[0x0170/4] = 0x10010100;
       pNv->PGRAPH[0x0710/4] = 0xFFFFFFFF;
       pNv->PGRAPH[0x0720/4] = 0x00000001;

       pNv->PGRAPH[0x0810/4] = 0x00000000;
       pNv->PGRAPH[0x0608/4] = 0xFFFFFFFF;
    } else {
       pNv->PGRAPH[0x0080/4] = 0xFFFFFFFF;
       pNv->PGRAPH[0x0080/4] = 0x00000000;

       pNv->PGRAPH[0x0140/4] = 0x00000000;
       pNv->PGRAPH[0x0100/4] = 0xFFFFFFFF;
       pNv->PGRAPH[0x0144/4] = 0x10010100;
       pNv->PGRAPH[0x0714/4] = 0xFFFFFFFF;
       pNv->PGRAPH[0x0720/4] = 0x00000001;
       pNv->PGRAPH[0x0710/4] &= 0x0007ff00;
       pNv->PGRAPH[0x0710/4] |= 0x00020100;

       if(pNv->Architecture == NV_ARCH_10) {
           pNv->PGRAPH[0x0084/4] = 0x00118700;
           pNv->PGRAPH[0x0088/4] = 0x24E00810;
           pNv->PGRAPH[0x008C/4] = 0x55DE0030;

           for(i = 0; i < 32; i++)
             pNv->PGRAPH[(0x0B00/4) + i] = pNv->PFB[(0x0240/4) + i];

           pNv->PGRAPH[0x640/4] = 0;
           pNv->PGRAPH[0x644/4] = 0;
           pNv->PGRAPH[0x684/4] = pNv->FrameBufferSize - 1;
           pNv->PGRAPH[0x688/4] = pNv->FrameBufferSize - 1;

           pNv->PGRAPH[0x0810/4] = 0x00000000;
           pNv->PGRAPH[0x0608/4] = 0xFFFFFFFF;
       } else {
         if(pNv->Architecture >= NV_ARCH_40) {
              pNv->PGRAPH[0x0084/4] = 0x401287c0;
              pNv->PGRAPH[0x008C/4] = 0x60de8051; 
              pNv->PGRAPH[0x0090/4] = 0x00008000;
              pNv->PGRAPH[0x0610/4] = 0x00be3c5f;
              pNv->PGRAPH[0x09b0/4] = 0x83280fff;
              pNv->PGRAPH[0x09b4/4] = 0x000000a0;
              pNv->PGRAPH[0x09b8/4] = 0x0078e366;
              pNv->PGRAPH[0x09bc/4] = 0x0000014c;
              pNv->PGRAPH[0x0b38/4] = 0x2ffff800;
              pNv->PGRAPH[0x0b3c/4] = 0x00006000;
              pNv->PGRAPH[0x032C/4] = 0x01000000; 
              pNv->PGRAPH[0x0220/4] = 0x00001200;
           } else
           if(pNv->Architecture == NV_ARCH_30) {
              pNv->PGRAPH[0x0084/4] = 0x40108700;
              pNv->PGRAPH[0x0890/4] = 0x00140000;
              pNv->PGRAPH[0x008C/4] = 0xf00e0431;
              pNv->PGRAPH[0x0090/4] = 0x00008000;
              pNv->PGRAPH[0x0610/4] = 0xf04b1f36;
              pNv->PGRAPH[0x0B80/4] = 0x1002d888;
              pNv->PGRAPH[0x0B88/4] = 0x62ff007f;
           } else {
              pNv->PGRAPH[0x0084/4] = 0x00118700;
              pNv->PGRAPH[0x008C/4] = 0xF20E0431;
              pNv->PGRAPH[0x0090/4] = 0x00000000;
              pNv->PGRAPH[0x009C/4] = 0x00000040;

              if((pNv->Chipset & 0x0ff0) >= 0x0250) {
                 pNv->PGRAPH[0x0890/4] = 0x00080000;
                 pNv->PGRAPH[0x0610/4] = 0x304B1FB6; 
                 pNv->PGRAPH[0x0B80/4] = 0x18B82880; 
                 pNv->PGRAPH[0x0B84/4] = 0x44000000; 
                 pNv->PGRAPH[0x0098/4] = 0x40000080; 
                 pNv->PGRAPH[0x0B88/4] = 0x000000ff; 
              } else {
                 pNv->PGRAPH[0x0880/4] = 0x00080000;
                 pNv->PGRAPH[0x0094/4] = 0x00000005;
                 pNv->PGRAPH[0x0B80/4] = 0x45CAA208; 
                 pNv->PGRAPH[0x0B84/4] = 0x24000000;
                 pNv->PGRAPH[0x0098/4] = 0x00000040;
                 pNv->PGRAPH[0x0750/4] = 0x00E00038;
                 pNv->PGRAPH[0x0754/4] = 0x00000030;
                 pNv->PGRAPH[0x0750/4] = 0x00E10038;
                 pNv->PGRAPH[0x0754/4] = 0x00000030;
              }
           }

           for(i = 0; i < 32; i++)
             pNv->PGRAPH[(0x0900/4) + i] = pNv->PFB[(0x0240/4) + i];

           pNv->PGRAPH[0x09A4/4] = pNv->PFB[0x0200/4];
           pNv->PGRAPH[0x09A8/4] = pNv->PFB[0x0204/4];

           if(pNv->Architecture >= NV_ARCH_40) {
              pNv->PGRAPH[0x69A4/4] = pNv->PFB[0x0200/4];
              pNv->PGRAPH[0x69A8/4] = pNv->PFB[0x0204/4];
           } else {
              pNv->PGRAPH[0x0750/4] = 0x00EA0000;
              pNv->PGRAPH[0x0754/4] = pNv->PFB[0x0200/4];
              pNv->PGRAPH[0x0750/4] = 0x00EA0004;
              pNv->PGRAPH[0x0754/4] = pNv->PFB[0x0204/4];
    	   }
	   
           pNv->PGRAPH[0x0820/4] = 0;
           pNv->PGRAPH[0x0824/4] = 0;
           pNv->PGRAPH[0x0864/4] = pNv->FrameBufferSize - 1;
           pNv->PGRAPH[0x0868/4] = pNv->FrameBufferSize - 1;

           pNv->PGRAPH[0x0B20/4] = 0x00000000;
           pNv->PGRAPH[0x0B04/4] = 0xFFFFFFFF;
       }
    }
    pNv->PGRAPH[0x053C/4] = 0;
    pNv->PGRAPH[0x0540/4] = 0;
    pNv->PGRAPH[0x0544/4] = 0x00007FFF;
    pNv->PGRAPH[0x0548/4] = 0x00007FFF;

    pNv->PFIFO[0x0140] = 0x00000000;
    pNv->PFIFO[0x0141] = 0x00000001;
    pNv->PFIFO[0x0480] = 0x00000000;
    pNv->PFIFO[0x0494] = 0x00000000;
    if(pNv->Architecture >= NV_ARCH_40)
       pNv->PFIFO[0x0481] = 0x00010000;
    else
       pNv->PFIFO[0x0481] = 0x00000100;
    pNv->PFIFO[0x0490] = 0x00000000;
    pNv->PFIFO[0x0491] = 0x00000000;
    if(pNv->Architecture >= NV_ARCH_40)
       pNv->PFIFO[0x048B] = 0x00001213;
    else
       pNv->PFIFO[0x048B] = 0x00001209;
    pNv->PFIFO[0x0400] = 0x00000000;
    pNv->PFIFO[0x0414] = 0x00000000;
    pNv->PFIFO[0x0084] = 0x03000100;
    pNv->PFIFO[0x0085] = 0x00000110;
    pNv->PFIFO[0x0086] = 0x00000112;
    pNv->PFIFO[0x0143] = 0x0000FFFF;
    pNv->PFIFO[0x0496] = 0x0000FFFF;
    pNv->PFIFO[0x0050] = 0x00000000;
    pNv->PFIFO[0x0040] = 0xFFFFFFFF;
    pNv->PFIFO[0x0415] = 0x00000001;
    pNv->PFIFO[0x048C] = 0x00000000;
    pNv->PFIFO[0x04A0] = 0x00000000;
#if AROS_BIG_ENDIAN
    pNv->PFIFO[0x0489] = 0x800F0078;
#else
    pNv->PFIFO[0x0489] = 0x000F0078;
#endif
    pNv->PFIFO[0x0488] = 0x00000001;
    pNv->PFIFO[0x0480] = 0x00000001;
    pNv->PFIFO[0x0494] = 0x00000001;
    pNv->PFIFO[0x0495] = 0x00000001;
    pNv->PFIFO[0x0140] = 0x00000001;

    if(pNv->Architecture >= NV_ARCH_10) {
        if(pNv->twoHeads) {
           pNv->PCRTC0[0x0860/4] = state->head;
           pNv->PCRTC0[0x2860/4] = state->head2;
        }
        pNv->PRAMDAC[0x0404/4] |= (1 << 25);
    
        pNv->PMC[0x8704/4] = 1;
        pNv->PMC[0x8140/4] = 0;
        pNv->PMC[0x8920/4] = 0;
        pNv->PMC[0x8924/4] = 0;
        pNv->PMC[0x8908/4] = pNv->FrameBufferSize - 1;
        pNv->PMC[0x890C/4] = pNv->FrameBufferSize - 1;
        pNv->PMC[0x1588/4] = 0;

        pNv->PCRTC[0x0810/4] = state->cursorConfig;
        pNv->PCRTC[0x0830/4] = state->displayV - 3;
        pNv->PCRTC[0x0834/4] = state->displayV - 1;
    
        if(pNv->FlatPanel) {
           if((pNv->Chipset & 0x0ff0) == 0x0110) {
               pNv->PRAMDAC[0x0528/4] = state->dither;
           } else 
           if(pNv->twoHeads) {
               pNv->PRAMDAC[0x083C/4] = state->dither;
           }
    
           VGA_WR08(pNv->PCIO, 0x03D4, 0x53);
           VGA_WR08(pNv->PCIO, 0x03D5, state->timingH);
           VGA_WR08(pNv->PCIO, 0x03D4, 0x54);
           VGA_WR08(pNv->PCIO, 0x03D5, state->timingV);
           VGA_WR08(pNv->PCIO, 0x03D4, 0x21);
           VGA_WR08(pNv->PCIO, 0x03D5, 0xfa);
        }

        VGA_WR08(pNv->PCIO, 0x03D4, 0x41);
        VGA_WR08(pNv->PCIO, 0x03D5, state->extra);
    }

    VGA_WR08(pNv->PCIO, 0x03D4, 0x19);
    VGA_WR08(pNv->PCIO, 0x03D5, state->repaint0);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x1A);
    VGA_WR08(pNv->PCIO, 0x03D5, state->repaint1);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x25);
    VGA_WR08(pNv->PCIO, 0x03D5, state->screen);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x28);
    VGA_WR08(pNv->PCIO, 0x03D5, state->pixel);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x2D);
    VGA_WR08(pNv->PCIO, 0x03D5, state->horiz);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x1B);
    VGA_WR08(pNv->PCIO, 0x03D5, state->arbitration0);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x20);
    VGA_WR08(pNv->PCIO, 0x03D5, state->arbitration1);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x30);
    VGA_WR08(pNv->PCIO, 0x03D5, state->cursor0);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x31);
    VGA_WR08(pNv->PCIO, 0x03D5, state->cursor1);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x2F);
    VGA_WR08(pNv->PCIO, 0x03D5, state->cursor2);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x39);
    VGA_WR08(pNv->PCIO, 0x03D5, state->interlace);

    if(!pNv->FlatPanel) {
       pNv->PRAMDAC0[0x050C/4] = state->pllsel;
       pNv->PRAMDAC0[0x0508/4] = state->vpll;
       if(pNv->twoHeads)
          pNv->PRAMDAC0[0x0520/4] = state->vpll2;
       if(pNv->twoStagePLL) {
          pNv->PRAMDAC0[0x0578/4] = state->vpllB;
          pNv->PRAMDAC0[0x057C/4] = state->vpll2B;
       }
    } else {
       pNv->PRAMDAC[0x0848/4] = state->scale;
    }
    pNv->PRAMDAC[0x0600/4] = state->general;

    pNv->PCRTC[0x0140/4] = 0;
    pNv->PCRTC[0x0100/4] = 1;

    pNv->CurrentState = state;
}

/*
** stegerg: Status NVUnloadStateExt(): should "match" xfree nv NVUnloadStateExt()
** in xc/programs/Xserver/hw/xfree86/drivers/nv/nv_hw.c version:
** 1.8 2004/03/20 01:52:16 mvojkovi
*/

void NVUnloadStateExt
(
    NVPtr pNv,
    RIVA_HW_STATE *state
)
{
    VGA_WR08(pNv->PCIO, 0x03D4, 0x19);
    state->repaint0     = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x1A);
    state->repaint1     = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x25);
    state->screen       = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x28);
    state->pixel        = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x2D);
    state->horiz        = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x1B);
    state->arbitration0 = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x20);
    state->arbitration1 = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x30);
    state->cursor0      = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x31);
    state->cursor1      = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x2F);
    state->cursor2      = VGA_RD08(pNv->PCIO, 0x03D5);
    VGA_WR08(pNv->PCIO, 0x03D4, 0x39);
    state->interlace    = VGA_RD08(pNv->PCIO, 0x03D5);
    state->vpll         = pNv->PRAMDAC0[0x0508/4];
    if(pNv->twoHeads)
       state->vpll2     = pNv->PRAMDAC0[0x0520/4];
    if(pNv->twoStagePLL) {
        state->vpllB    = pNv->PRAMDAC0[0x0578/4];
        state->vpll2B   = pNv->PRAMDAC0[0x057C/4];
    }
    state->pllsel       = pNv->PRAMDAC0[0x050C/4];
    state->general      = pNv->PRAMDAC[0x0600/4];
    state->scale        = pNv->PRAMDAC[0x0848/4];
    state->config       = pNv->PFB[0x0200/4];

    if(pNv->Architecture >= NV_ARCH_10) {
        if(pNv->twoHeads) {
           state->head     = pNv->PCRTC0[0x0860/4];
           state->head2    = pNv->PCRTC0[0x2860/4];
           VGA_WR08(pNv->PCIO, 0x03D4, 0x44);
           state->crtcOwner = VGA_RD08(pNv->PCIO, 0x03D5);
        }
        VGA_WR08(pNv->PCIO, 0x03D4, 0x41);
        state->extra = VGA_RD08(pNv->PCIO, 0x03D5);
        state->cursorConfig = pNv->PCRTC[0x0810/4];

        if((pNv->Chipset & 0x0ff0) == 0x0110) {
           state->dither = pNv->PRAMDAC[0x0528/4];
        } else 
        if(pNv->twoHeads) {
            state->dither = pNv->PRAMDAC[0x083C/4];
        }

        if(pNv->FlatPanel) {
           VGA_WR08(pNv->PCIO, 0x03D4, 0x53);
           state->timingH = VGA_RD08(pNv->PCIO, 0x03D5);
           VGA_WR08(pNv->PCIO, 0x03D4, 0x54);
           state->timingV = VGA_RD08(pNv->PCIO, 0x03D5);
        }
    }
}

void NVSetStartAddress (
    NVPtr   pNv,
    ULONG start
)
{
    pNv->PCRTC[0x800/4] = start;
}

void NVLoadDAC(
    NVPtr   pNv
)
{
    int i;
    
    VGA_WR08(pNv->PDIO, 0x3c6, 0xff);
    VGA_WR08(pNv->PDIO, 0x3c8, 0);
    for (i=0; i < 768; i++)
	VGA_WR08(pNv->PDIO, 0x3c9, pNv->CurrentState->Regs.dac[i]);
}

static void InitBaseRegs(struct staticdata *sd, struct CardState *card, Sync *mode)
{
    /* Determine sync polarity */
    if (mode->VDisplay < 400)
	card->Regs.misc = 0xa3;
    else if (mode->VDisplay < 480)
	card->Regs.misc = 0x63;
    else if (mode->VDisplay < 768)
	card->Regs.misc = 0xe3;
    else
	card->Regs.misc = 0x23;

    card->Regs.seq[0] = 0x03;
    card->Regs.seq[1] = 0x01;
    card->Regs.seq[2] = 0x0f;
    card->Regs.seq[3] = 0x00;
    card->Regs.seq[4] = 0x0e;

    card->Regs.crtc[8] = 0;
    card->Regs.crtc[10] = 0;  //0x20;
    card->Regs.crtc[14] = 0;
    card->Regs.crtc[17] = (mode->VSyncEnd & 0x0f) | 0x20;
    card->Regs.crtc[18] = (mode->VDisplay - 1) & 0xff;
    card->Regs.crtc[19] = card->width >> 4;
    card->Regs.crtc[20] = 0;
    card->Regs.crtc[21] = (mode->VSyncStart - 1) & 0xff;
    card->Regs.crtc[22] = (mode->VSyncEnd - 1) & 0xff;
    card->Regs.crtc[23] = 0xc3;	// 0xc3
    card->Regs.crtc[24] = 0xff;
    card->Regs.crtc[40] = 0x40;

    card->Regs.gra[0] = 0x00;
    card->Regs.gra[1] = 0x00;
    card->Regs.gra[2] = 0x00;
    card->Regs.gra[3] = 0x00;
    card->Regs.gra[4] = 0x00;
    card->Regs.gra[5] = 0x40;
    card->Regs.gra[6] = 0x05;
    card->Regs.gra[7] = 0x0f;
    card->Regs.gra[8] = 0xff;

    card->Regs.attr[0] = 0x00;
    card->Regs.attr[1] = 0x01;
    card->Regs.attr[2] = 0x02;
    card->Regs.attr[3] = 0x03;
    card->Regs.attr[4] = 0x04;
    card->Regs.attr[5] = 0x05;
    card->Regs.attr[6] = 0x06;
    card->Regs.attr[7] = 0x07;
    card->Regs.attr[8] = 0x08;
    card->Regs.attr[9] = 0x09;
    card->Regs.attr[10] = 0x0a;
    card->Regs.attr[11] = 0x0b;
    card->Regs.attr[12] = 0x0c;
    card->Regs.attr[13] = 0x0d;
    card->Regs.attr[14] = 0x0e;
    card->Regs.attr[15] = 0x0f;
    card->Regs.attr[16] = 0x41;
    card->Regs.attr[17] = 0x01;
    card->Regs.attr[18] = 0x0f;
    card->Regs.attr[19] = 0x00;
    card->Regs.attr[20] = 0x00;
}

#define BITMASK(t,b) (((unsigned)(1U << (((t)-(b)+1)))-1)  << (b))
#define MASKEXPAND(mask) BITMASK(1?mask,0?mask)
#define SetBF(mask,value) ((value) << (0?mask))
#define GetBF(var,mask) (((unsigned)((var) & MASKEXPAND(mask))) >> (0?mask) )
#define SetBitField(value,from,to) SetBF(to, GetBF(value,from))
#define SetBit(n) (1<<(n))
#define Set8Bits(value) ((value)&0xff)

void InitMode(struct staticdata *sd, struct CardState *state,
    ULONG width, ULONG height, UBYTE bpp, ULONG pixelc, ULONG base,
    ULONG HDisplay, ULONG VDisplay, 
    ULONG HSyncStart, ULONG HSyncEnd, ULONG HTotal,
    ULONG VSyncStart, ULONG VSyncEnd, ULONG VTotal)
{
    D(bug("[NVidia] Init %dx%dx%d @%x mode\n", width, height, bpp, base));

    ULONG   HBlankStart, HBlankEnd, VBlankStart, VBlankEnd, OrgHDisplay = HDisplay;
    
    Sync mode = {
	pixelc, 0,
	HDisplay, HSyncStart, HSyncEnd, HTotal,
	VDisplay, VSyncStart, VSyncEnd, VTotal
    };

    InitBaseRegs(sd, state, &mode);

    HDisplay	= (HDisplay >> 3) - 1;
    HSyncStart	= (HSyncStart >> 3) - 1;
    HSyncEnd	= (HSyncEnd >> 3) - 1;
    HTotal	= (HTotal >> 3) - 5;
    HBlankStart	= HDisplay;
    HBlankEnd	= HTotal + 4;

    VDisplay	-= 1;
    VSyncStart	-= 1;
    VSyncEnd	-= 1;
    VTotal	-= 2;
    VBlankStart	= VDisplay;
    VBlankEnd	= VTotal + 1;

    if (bpp <= 8)
        state->bitsPerPixel = 8;
    else if (bpp <= 16)
	state->bitsPerPixel = 16;
    else
	state->bitsPerPixel = 32;

    if (sd->Card.FlatPanel)
    {
	VSyncStart = VTotal - 3;
	VSyncEnd = VTotal - 2;
	VBlankStart = VSyncStart;
	HSyncStart = HTotal - 5;
	HSyncEnd = HTotal - 2;
	HBlankEnd = HTotal + 4;
    }

    state->Regs.crtc[0x00] = Set8Bits(HTotal);
    state->Regs.crtc[0x01] = Set8Bits(HDisplay);
    state->Regs.crtc[0x02] = Set8Bits(HBlankStart);
    state->Regs.crtc[0x03] = SetBitField(HBlankEnd, 4:0, 4:0) | SetBit(7);
    state->Regs.crtc[0x04] = Set8Bits(HSyncStart);
    state->Regs.crtc[0x05] = SetBitField(HBlankEnd, 5:5, 7:7) |
			     SetBitField(HSyncEnd, 4:0, 4:0);
    state->Regs.crtc[0x06] = SetBitField(VTotal, 7:0, 7:0);
    state->Regs.crtc[0x07] = SetBitField(VTotal, 8:8, 0:0) |
			     SetBitField(VDisplay, 8:8, 1:1) |
			     SetBitField(VSyncStart, 8:8, 2:2) |
			     SetBitField(VBlankStart, 8:8, 3:3) |
			     SetBit(4) |
			     SetBitField(VTotal, 9:9, 5:5) |
			     SetBitField(VDisplay, 9:9, 6:6) |
			     SetBitField(VSyncStart, 9:9, 7:7);
    state->Regs.crtc[0x09] = SetBitField(VBlankStart, 9:9, 5:5) | SetBit(6); // V_DOUBLESCAN?
    state->Regs.crtc[0x10] = Set8Bits(VSyncStart);
    state->Regs.crtc[0x11] = SetBitField(VSyncEnd, 3:0, 3:0) | SetBit(5);
    state->Regs.crtc[0x12] = Set8Bits(VDisplay);
    state->Regs.crtc[0x13] = ((width / 8) * (state->bitsPerPixel / 8));
    state->Regs.crtc[0x15] = Set8Bits(VBlankStart);
    state->Regs.crtc[0x16] = Set8Bits(VBlankEnd);

    state->Regs.attr[0x10] = 0x01;
    
    state->screen =
	SetBitField(HBlankEnd, 6:6, 4:4) |
	SetBitField(VBlankStart, 10:10, 3:3) |
	SetBitField(VSyncStart, 10:10, 2:2) |
	SetBitField(VDisplay, 10:10, 1:1) |
	SetBitField(VTotal, 10:10, 0:0);

    state->horiz = 
	SetBitField(HTotal, 8:8, 0:0) |
	SetBitField(HDisplay, 8:8, 1:1) |
	SetBitField(HBlankStart, 8:8, 2:2) |
	SetBitField(HSyncStart, 8:8, 3:3);

    state->extra =
	SetBitField(VTotal, 11:11, 0:0) |
	SetBitField(VDisplay, 11:11, 2:2) |
	SetBitField(VSyncStart, 11:11, 4:4) |
	SetBitField(VBlankStart, 11:11, 6:6);

    state->interlace = 0xff;

    if (bpp >= 24) bpp = 32;
		    
    if (sd->Card.Architecture >= NV_ARCH_10)
	sd->Card.CURSOR = (ULONG*)(sd->Card.FrameBuffer + sd->Card.CursorStart);

//    NVLockUnlock(sd, 0);
    NVCalcStateExt(sd, &sd->Card, state, bpp, width, OrgHDisplay, height, pixelc, 0);

    state->scale = sd->Card.PRAMDAC[0x0848/4] & 0xfff000ff; 
    if (sd->Card.FlatPanel)
    {
	state->pixel |= (1 << 7);
	if (!sd->Card.fpScaler 	|| (sd->Card.fpWidth <= mode.HDisplay)
				|| (sd->Card.fpHeight <= mode.VDisplay))
	{
	    state->scale |= (1 << 8);
	}
    }
    
    
    state->cursorConfig = 0x00000100;
    if (sd->Card.alphaCursor)
    {
        state->cursorConfig |= 0x04011000;
        state->general |= (1 << 29);

        if((sd->Card.Chipset & 0x0ff0) == 0x0110) {
            state->dither = sd->Card.PRAMDAC[0x0528/4] & ~0x00010000;
            if(0) //sd->Card.FPDither)
               state->dither |= 0x00010000;
            else
               state->cursorConfig |= (1 << 28);
        } else 
        if((sd->Card.Chipset & 0x0ff0) >= 0x0170) {
           state->dither = sd->Card.PRAMDAC[0x083C/4] & ~1;
           state->cursorConfig |= (1 << 28);
           if(0) //pNv->FPDither)
              state->dither |= 1;
        } else {
           state->cursorConfig |= (1 << 28);
        }
    } else
       state->cursorConfig |= 0x02000000;


    // Init DAC
    if (bpp >1)
    {
	int i;
	for (i=0; i <256; i++)
	{
	    state->Regs.dac[i*3+0] = i;
	    state->Regs.dac[i*3+1] = i;
	    state->Regs.dac[i*3+2] = i;
	}
    }

    state->offset = base;
    state->vpll = state->pll;
    state->vpll2 = state->pll;
    state->vpllB = state->pllB;
    state->vpll2B = state->pllB;

    if(sd->Card.CRTCnumber) {
       state->head  = sd->Card.PCRTC0[0x00000860/4] & ~0x00001000;
       state->head2 = sd->Card.PCRTC0[0x00002860/4] | 0x00001000;
       state->crtcOwner = 3;
       state->pllsel |= 0x20000800;
       state->vpll = sd->Card.PRAMDAC0[0x0508/4];
       if(sd->Card.twoStagePLL)
          state->vpllB = sd->Card.PRAMDAC0[0x0578/4];
    } else
    if(sd->Card.twoHeads) {
       state->head  =  sd->Card.PCRTC0[0x00000860/4] | 0x00001000;
       state->head2 =  sd->Card.PCRTC0[0x00002860/4] & ~0x00001000;
       state->crtcOwner = 0;
       state->vpll2 = sd->Card.PRAMDAC0[0x0520/4];
       if(sd->Card.twoStagePLL)
          state->vpll2B = sd->Card.PRAMDAC0[0x057C/4];
    }
    
    state->timingH = 0;
    state->timingV = 0;
    state->displayV = VDisplay;
}

void acc_reset(struct staticdata *);

void LoadState(struct staticdata *sd, struct CardState *state)
{
    int i;

    ObtainSemaphore(&sd->HWLock);
    
//    CRTC_out(sd, 0x11, 0x00);

    NVLockUnlock(sd, 0);
    NVLoadStateExt(&sd->Card, state);

    MISC_out(sd, sd->Card.CurrentState->Regs.misc);
    
    for (i=0; i < 0x05; i++)
    {
	SEQ_out(sd, i, sd->Card.CurrentState->Regs.seq[i]);
    }

    CRTC_out(sd, 17, sd->Card.CurrentState->Regs.crtc[17] & ~0x80);
    for (i=0; i < 0x41; i++)
    {
	switch (i)
	{
	    case 0x19:
	    case 0x20 ... 0x40:
		break;
	    default:
		CRTC_out(sd, i, sd->Card.CurrentState->Regs.crtc[i]);
		break;
	}
    }

    for (i=0; i < 0x15; i++)
	ATTR_out(sd, i, sd->Card.CurrentState->Regs.attr[i]);

    for (i=0; i < 0x09; i++)
	GRA_out(sd, i, sd->Card.CurrentState->Regs.gra[i]);

    NVSetStartAddress(&sd->Card, sd->Card.CurrentState->offset);

    sd->Card.currentROP = 0xffffffff;
//    VGA_WR08(sd->Card.PVIO, 0x3c3, 1);

    NVLoadDAC(&sd->Card);
    acc_reset(sd);

    ReleaseSemaphore(&sd->HWLock);
}

void DPMS(struct staticdata *sd, HIDDT_DPMSLevel state)
{
    UBYTE reg;
    
    ObtainSemaphore(&sd->HWLock);
    
    NVLockUnlock(sd,0);
    reg = CRTC_in(sd, 0x1a) & ~0xc0;
    
    switch(state)
    {
	case vHidd_Gfx_DPMSLevel_Standby:
	    reg |= 0x80;
	    break;
	case vHidd_Gfx_DPMSLevel_Suspend:
	    reg |= 0x40;
	    break;
	case vHidd_Gfx_DPMSLevel_Off:
	    reg |= 0xc0;
	    break;
	default:	// On
	    break;
    }
    
    CRTC_out(sd, 0x1a, reg);
    
    ReleaseSemaphore(&sd->HWLock);
}

void Protect(struct staticdata *sd, UBYTE protect)
{
    ObtainSemaphore(&sd->HWLock);
    
    if (protect)
    {
	UBYTE tmp = SEQ_in(sd, 1);
	SEQ_out(sd, 0, 1);
	SEQ_out(sd, 1, tmp | 0x20);
    }
    else
    {
	UBYTE tmp = SEQ_in(sd, 1);
	SEQ_out(sd, 1, tmp & ~0x20);
	SEQ_out(sd, 0, 3);
    }
    
    ReleaseSemaphore(&sd->HWLock);
}

static const UBYTE ROPTable[] = {
    [vHidd_GC_DrawMode_Clear]	    = 0x00,
    [vHidd_GC_DrawMode_And]	    = 0x88,
    [vHidd_GC_DrawMode_AndReverse]  = 0x44,
    [vHidd_GC_DrawMode_Copy]	    = 0xcc,
    [vHidd_GC_DrawMode_AndInverted] = 0x22,
    [vHidd_GC_DrawMode_NoOp]	    = 0xaa,
    [vHidd_GC_DrawMode_Xor]	    = 0x66,
    [vHidd_GC_DrawMode_Or]	    = 0xee,
    [vHidd_GC_DrawMode_Nor]	    = 0x11,
    [vHidd_GC_DrawMode_Equiv]	    = 0x99,
    [vHidd_GC_DrawMode_Invert]	    = 0x55,
    [vHidd_GC_DrawMode_OrReverse]   = 0xdd,
    [vHidd_GC_DrawMode_CopyInverted]= 0x33,
    [vHidd_GC_DrawMode_OrInverted]  = 0xbb,
    [vHidd_GC_DrawMode_Nand]	    = 0x77,
    [vHidd_GC_DrawMode_Set]	    = 0xff,
};

void NVDmaKickoff(struct Card *pNv)
{
    if(pNv->dmaCurrent != pNv->dmaPut) {
        pNv->dmaPut = pNv->dmaCurrent;
        WRITE_PUT(pNv,  pNv->dmaPut);
    }
}

/* There is a HW race condition with videoram command buffers.
   You can't jump to the location of your put offset.  We write put
   at the jump offset + SKIPS dwords with noop padding in between
   to solve this problem */
#define SKIPS  8

void
NVDmaWait (
   struct Card *pNv,
   int size
){
    int dmaGet;

    size++;

    while(pNv->dmaFree < size) {
       dmaGet = READ_GET(pNv);

	if(pNv->dmaPut >= dmaGet) {
           pNv->dmaFree = pNv->dmaMax - pNv->dmaCurrent;
           if(pNv->dmaFree < size) {
               NVDmaNext(pNv, 0x20000000);
               if(dmaGet <= SKIPS) {
                   if(pNv->dmaPut <= SKIPS) /* corner case - will be idle */
                      WRITE_PUT(pNv, SKIPS + 1);
                   do { dmaGet = READ_GET(pNv); }
                   while(dmaGet <= SKIPS);
               }
               WRITE_PUT(pNv, SKIPS);
               pNv->dmaCurrent = pNv->dmaPut = SKIPS;
               pNv->dmaFree = dmaGet - (SKIPS + 1);
           }
       } else
           pNv->dmaFree = dmaGet - pNv->dmaCurrent - 1;
    }
}

void NVSetPattern(
   struct staticdata *sd,
   ULONG clr0,
   ULONG clr1,
   ULONG pat0,
   ULONG pat1
)
{
    struct Card *pNv = &sd->Card;

    NVDmaStart(pNv, PATTERN_COLOR_0, 4);
    NVDmaNext (pNv, clr0);
    NVDmaNext (pNv, clr1);
    NVDmaNext (pNv, pat0);
    NVDmaNext (pNv, pat1);
}

void NVSetRopSolid(struct staticdata *sd, ULONG rop, ULONG planemask)
{
    struct Card *pNv = &sd->Card;

    if(planemask != ~0) {
        NVSetPattern(sd, 0, planemask, ~0, ~0);
        if(pNv->currentROP != (rop + 32)) {
           NVDmaStart(pNv, ROP_SET, 1);
           NVDmaNext (pNv, ROPTable[rop]);
           pNv->currentROP = rop + 32;
        }
    } else
    if (pNv->currentROP != rop) {
        if(pNv->currentROP >= 16)
             NVSetPattern(sd, ~0, ~0, ~0, ~0);
        NVDmaStart(pNv, ROP_SET, 1);
        NVDmaNext (pNv, ROPTable[rop]);
        pNv->currentROP = rop;
    }
}

void acc_reset(struct staticdata *sd)
{
    struct Card *pNv = &sd->Card;
    int pitch, i;

LOCK_HW

    pitch = pNv->CurrentState->width *
	(pNv->CurrentState->bitsPerPixel >> 3);

    sd->src_pitch = pitch;
    sd->dst_pitch = pitch;
    sd->src_offset = pNv->CurrentState->offset;
    sd->dst_offset = pNv->CurrentState->offset;

    pNv->dmaBase = (ULONG*)(&pNv->FrameBuffer[pNv->FbUsableSize]);

    for(i = 0; i < SKIPS; i++)
	pNv->dmaBase[i] = 0x00000000;

    pNv->dmaBase[0x0 + SKIPS] = 0x00040000;
    pNv->dmaBase[0x1 + SKIPS] = 0x80000010;
    pNv->dmaBase[0x2 + SKIPS] = 0x00042000;
    pNv->dmaBase[0x3 + SKIPS] = 0x80000011;
    pNv->dmaBase[0x4 + SKIPS] = 0x00044000;
    pNv->dmaBase[0x5 + SKIPS] = 0x80000012;
    pNv->dmaBase[0x6 + SKIPS] = 0x00046000;
    pNv->dmaBase[0x7 + SKIPS] = 0x80000013;
    pNv->dmaBase[0x8 + SKIPS] = 0x00048000;
    pNv->dmaBase[0x9 + SKIPS] = 0x80000014;
    pNv->dmaBase[0xA + SKIPS] = 0x0004A000;
    pNv->dmaBase[0xB + SKIPS] = 0x80000015;
    pNv->dmaBase[0xC + SKIPS] = 0x0004C000;
    pNv->dmaBase[0xD + SKIPS] = 0x80000016;
    pNv->dmaBase[0xE + SKIPS] = 0x0004E000;
    pNv->dmaBase[0xF + SKIPS] = 0x80000017;

    pNv->dmaPut = 0;
    pNv->dmaCurrent = 16 + SKIPS;
    pNv->dmaMax = 8191;
    pNv->dmaFree = pNv->dmaMax - pNv->dmaCurrent;

    switch(pNv->CurrentState->bpp) {
    case 32:
    case 24:
       sd->surface_format = SURFACE_FORMAT_DEPTH24;
       sd->pattern_format = PATTERN_FORMAT_DEPTH24;
       sd->rect_format    = RECT_FORMAT_DEPTH24;
       sd->line_format    = LINE_FORMAT_DEPTH24;
       break;
    case 16:
    case 15:
       sd->surface_format = SURFACE_FORMAT_DEPTH16;
       sd->pattern_format = PATTERN_FORMAT_DEPTH16;
       sd->rect_format    = RECT_FORMAT_DEPTH16;
       sd->line_format    = LINE_FORMAT_DEPTH16;
       break;
    default:
       sd->surface_format = SURFACE_FORMAT_DEPTH8;
       sd->pattern_format = PATTERN_FORMAT_DEPTH8;
       sd->rect_format    = RECT_FORMAT_DEPTH8;
       sd->line_format    = LINE_FORMAT_DEPTH8;
       break;
    }
   
    NVDmaStart(pNv, SURFACE_FORMAT, 4);
    NVDmaNext (pNv, sd->surface_format);
    NVDmaNext (pNv, sd->dst_pitch | (sd->src_pitch << 16));
    NVDmaNext (pNv, sd->src_offset);
    NVDmaNext (pNv, sd->dst_offset);

    NVDmaStart(pNv, PATTERN_FORMAT, 1);
    NVDmaNext (pNv, sd->pattern_format);

    NVDmaStart(pNv, RECT_FORMAT, 1);
    NVDmaNext (pNv, sd->rect_format);

    NVDmaStart(pNv, LINE_FORMAT, 1);
    NVDmaNext (pNv, sd->line_format);

    NVSetRopSolid(sd, vHidd_GC_DrawMode_Copy, ~0);

    NVDmaKickoff(pNv);

UNLOCK_HW
}

void NVSync(struct staticdata *sd)
{
    struct Card *pNv = &sd->Card;

    if(pNv->DMAKickoffCallback)
       (*pNv->DMAKickoffCallback)(sd);

    while(READ_GET(pNv) != pNv->dmaPut);

    while(pNv->PGRAPH[0x0700/4]);

    sd->gpu_busy = FALSE;
}

void NVDMAKickoffCallback(struct staticdata *sd)
{
   struct Card *pNv = &sd->Card;

   NVDmaKickoff(pNv);
   pNv->DMAKickoffCallback = NULL;
}

void NVSelectHead(struct staticdata *sd, UBYTE head)
{
    if (head)
    {
        sd->Card.PCIO = sd->Card.PCIO0 + 0x2000;
	sd->Card.PCRTC = sd->Card.PCRTC0 + 0x800;
	sd->Card.PRAMDAC = sd->Card.PRAMDAC0 + 0x800;
	sd->Card.PDIO = sd->Card.PDIO0 + 0x2000;
    }
    else
    {
        sd->Card.PCIO = sd->Card.PCIO0;
	sd->Card.PCRTC = sd->Card.PCRTC0;
	sd->Card.PRAMDAC = sd->Card.PRAMDAC0;
	sd->Card.PDIO = sd->Card.PDIO0;
    }
}

BOOL NVIsConnected (struct staticdata *sd, UBYTE output)
{
    NVPtr pNv = &sd->Card;
    volatile ULONG *PRAMDAC = pNv->PRAMDAC0;
    ULONG reg52C, reg608;
    BOOL present;
    int i;

    if(output) PRAMDAC += 0x800;

    reg52C = PRAMDAC[0x052C/4];
    reg608 = PRAMDAC[0x0608/4];

    PRAMDAC[0x0608/4] = reg608 & ~0x00010000;

    PRAMDAC[0x052C/4] = reg52C & 0x0000FEEE;
    
    //usleep(1000);
    for (i=0; i < 800000000; i++)
    {
    }
    
    PRAMDAC[0x052C/4] |= 1;

    pNv->PRAMDAC0[0x0610/4] = 0x94050140;
    pNv->PRAMDAC0[0x0608/4] |= 0x00001000;

    //usleep(1000);
    for (i=0; i < 800000000; i++)
    {
    }
 
    present = (PRAMDAC[0x0608/4] & (1 << 28)) ? TRUE : FALSE;

    pNv->PRAMDAC0[0x0608/4] &= 0x0000EFFF;

    PRAMDAC[0x052C/4] = reg52C;
    PRAMDAC[0x0608/4] = reg608;

    return present;
}

