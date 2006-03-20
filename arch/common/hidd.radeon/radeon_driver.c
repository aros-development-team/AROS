/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "ati.h"
#include "radeon.h"
#include "radeon_reg.h"
#include "radeon_bios.h"
#include "radeon_macros.h"

#define DEBUG 1
#include <aros/debug.h>

#define MAX(a,b)    ((a) > (b) ? (a) : (b))

static void usleep(ULONG usec)
{
    int i,j;
    for (i=0; i < usec; i++)
        for (j=0; j < 500; j++)
            asm volatile("nop");
}

/* Compute n/d with rounding */
static int RADEONDiv(int n, int d)
{
    return (n + (d / 2)) / d;
}

/* This function is required to workaround a hardware bug in some (all?)
 * revisions of the R300.  This workaround should be called after every
 * CLOCK_CNTL_INDEX register access.  If not, register reads afterward
 * may not be correct.
 */
void R300CGWorkaround(struct ati_staticdata *sd)
{
    ULONG         save, tmp;

    save = INREG(RADEON_CLOCK_CNTL_INDEX);
    tmp = save & ~(0x3f | RADEON_PLL_WR_EN);
    OUTREG(RADEON_CLOCK_CNTL_INDEX, tmp);
    tmp = INREG(RADEON_CLOCK_CNTL_DATA);
    OUTREG(RADEON_CLOCK_CNTL_INDEX, save);
}

/* Read PLL information */
unsigned RADEONINPLL(struct ati_staticdata *sd, int addr)
{
    ULONG   data;

    OUTREG8(RADEON_CLOCK_CNTL_INDEX, addr & 0x3f);
    data = INREG(RADEON_CLOCK_CNTL_DATA);
    if (sd->Card.R300CGWorkaround) R300CGWorkaround(sd);

    return data;
}

/* Blank screen */
static void RADEONBlank(struct ati_staticdata *sd)
{
    if (!sd->Card.IsSecondary) {
        switch(sd->Card.MonType2) {
        case MT_LCD:
        case MT_CRT:
        case MT_DFP:
            OUTREGP(RADEON_CRTC_EXT_CNTL,
                    RADEON_CRTC_DISPLAY_DIS,
                    ~(RADEON_CRTC_DISPLAY_DIS));
            break;

        case MT_NONE:
        default:
            break;
        }
    } else {
        OUTREGP(RADEON_CRTC2_GEN_CNTL,
                RADEON_CRTC2_DISP_DIS,
                ~(RADEON_CRTC2_DISP_DIS));
    }
}

/* Unblank screen */
static void RADEONUnblank(struct ati_staticdata *sd)
{
    if (!sd->Card.IsSecondary) {
        switch (sd->Card.MonType2) {
            case MT_LCD:
            case MT_CRT:
            case MT_DFP:
                OUTREGP(RADEON_CRTC_EXT_CNTL,
                        RADEON_CRTC_CRT_ON,
                        ~(RADEON_CRTC_DISPLAY_DIS));
                break;

            case MT_NONE:
            default:
                break;
        }
    } else {
        switch (sd->Card.MonType1) {
            case MT_LCD:
            case MT_DFP:
            case MT_CRT:
                OUTREGP(RADEON_CRTC2_GEN_CNTL,
                        0,
                        ~(RADEON_CRTC2_DISP_DIS));
                break;

            case MT_NONE:
            default:
                break;
        }
    }
}

static void RADEONPLLWaitForReadUpdateComplete(struct ati_staticdata *sd)
{
    int i = 0;

    /* FIXME: Certain revisions of R300 can't recover here.  Not sure of
       the cause yet, but this workaround will mask the problem for now.
       Other chips usually will pass at the very first test, so the
       workaround shouldn't have any effect on them. */
    for (i = 0;
         (i < 10000 &&
          RADEONINPLL(sd, RADEON_PPLL_REF_DIV) & RADEON_PPLL_ATOMIC_UPDATE_R);
         i++);
}

static void RADEONPLLWriteUpdate(struct ati_staticdata *sd)
{
    while (INPLL(sd, RADEON_PPLL_REF_DIV) & RADEON_PPLL_ATOMIC_UPDATE_R);

    OUTPLLP(sd, RADEON_PPLL_REF_DIV,
            RADEON_PPLL_ATOMIC_UPDATE_W,
            ~(RADEON_PPLL_ATOMIC_UPDATE_W));
}

/* Calculate display buffer watermark to prevent buffer underflow */
static void RADEONInitDispBandwidth(struct ati_staticdata *sd, struct CardState *mode)
{
    ULONG temp, data, mem_trcd, mem_trp, mem_tras, mem_trbs=0;
    float mem_tcas;
    int k1, c;
    ULONG MemTrcdExtMemCntl[4]     = {1, 2, 3, 4};
    ULONG MemTrpExtMemCntl[4]      = {1, 2, 3, 4};
    ULONG MemTrasExtMemCntl[8]     = {1, 2, 3, 4, 5, 6, 7, 8};

    ULONG MemTrcdMemTimingCntl[8]     = {1, 2, 3, 4, 5, 6, 7, 8};
    ULONG MemTrpMemTimingCntl[8]      = {1, 2, 3, 4, 5, 6, 7, 8};
    ULONG MemTrasMemTimingCntl[16]    = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};

    float MemTcas[8]  = {0, 1, 2, 3, 0, 1.5, 2.5, 0};
    float MemTcas2[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    float MemTrbs[8]  = {1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5};

    float mem_bw, peak_disp_bw;
    float min_mem_eff = 0.8;
    float sclk_eff, sclk_delay;
    float mc_latency_mclk, mc_latency_sclk, cur_latency_mclk, cur_latency_sclk;
    float disp_latency, disp_latency_overhead, disp_drain_rate, disp_drain_rate2;
    float pix_clk, pix_clk2; /* in MHz */
    int cur_size = 16;       /* in octawords */
    int critical_point;
    int stop_req, max_stop_req;

    /* R420 family not supported yet */
    if (sd->Card.Type == R420) return;

    /*
     * Determine if there is enough bandwidth for current display mode
     */
    mem_bw = sd->Card.mclk * (sd->Card.RamWidth / 8) * (sd->Card.IsDDR ? 2 : 1);
   
    pix_clk = mode->pixelc/1000.0;
    pix_clk2 = 0;

    peak_disp_bw = (pix_clk * mode->bpp);

    D(bug("[ATI] mem_bw=%d peak_disp_bw=%d mode->bpp=%d mode->pixelc=%d mode->HDisplay=%d\n", (ULONG)mem_bw, (ULONG)peak_disp_bw,
        mode->bpp, mode->pixelc, mode->HDisplay));

    if (peak_disp_bw >= mem_bw * min_mem_eff) {
        D(bug("[ATI] You may not have enough display bandwidth for current mode\n"
                   "[ATI] If you have flickering problem, try to lower resolution, refresh rate, or color depth\n"));
    }

    /*  CRTC1
        Set GRPH_BUFFER_CNTL register using h/w defined optimal values.
        GRPH_STOP_REQ <= MIN[ 0x7C, (CRTC_H_DISP + 1) * (bit depth) / 0x10 ]
    */
    stop_req = mode->HDisplay * mode->bpp / 16;

    /* setup Max GRPH_STOP_REQ default value */
    if (IS_RV100_VARIANT)
        max_stop_req = 0x5c;
    else
        max_stop_req  = 0x7c;
    if (stop_req > max_stop_req)
        stop_req = max_stop_req;

    /*  Get values from the EXT_MEM_CNTL register...converting its contents. */
    temp = INREG(RADEON_MEM_TIMING_CNTL);
    if ((sd->Card.Type == RV100) || sd->Card.IsIGP) { /* RV100, M6, IGPs */
        mem_trcd      = MemTrcdExtMemCntl[(temp & 0x0c) >> 2];
        mem_trp       = MemTrpExtMemCntl[ (temp & 0x03) >> 0];
        mem_tras      = MemTrasExtMemCntl[(temp & 0x70) >> 4];
    } else { /* RV200 and later */
        mem_trcd      = MemTrcdMemTimingCntl[(temp & 0x07) >> 0];
        mem_trp       = MemTrpMemTimingCntl[ (temp & 0x700) >> 8];
        mem_tras      = MemTrasMemTimingCntl[(temp & 0xf000) >> 12];
    }

    /* Get values from the MEM_SDRAM_MODE_REG register...converting its */
    temp = INREG(RADEON_MEM_SDRAM_MODE_REG);
    data = (temp & (7<<20)) >> 20;
    if ((sd->Card.Type == RV100) || sd->Card.IsIGP) { /* RV100, M6, IGPs */
        mem_tcas = MemTcas [data];
    } else {
        mem_tcas = MemTcas2 [data];
    }

    if (IS_R300_VARIANT) {

        /* on the R300, Tcas is included in Trbs.
        */
        temp = INREG(RADEON_MEM_CNTL);
        data = (R300_MEM_NUM_CHANNELS_MASK & temp);
        if (data == 2) {
            if (R300_MEM_USE_CD_CH_ONLY & temp) {
                temp  = INREG(R300_MC_IND_INDEX);
                temp &= ~R300_MC_IND_ADDR_MASK;
                temp |= R300_MC_READ_CNTL_CD_mcind;
                OUTREG(R300_MC_IND_INDEX, temp);
                temp  = INREG(R300_MC_IND_DATA);
                data = (R300_MEM_RBS_POSITION_C_MASK & temp);
            } else {
                temp = INREG(R300_MC_READ_CNTL_AB);
                data = (R300_MEM_RBS_POSITION_A_MASK & temp);
            }
        } else {
            temp = INREG(R300_MC_READ_CNTL_AB);
            data = (R300_MEM_RBS_POSITION_A_MASK & temp);
        }

        mem_trbs = MemTrbs[data];
        mem_tcas += mem_trbs;
    }

    if ((sd->Card.Type == RV100) || sd->Card.IsIGP) { /* RV100, M6, IGPs */
        /* DDR64 SCLK_EFF = SCLK for analysis */
        sclk_eff = sd->Card.sclk;
    } else {
        sclk_eff = sd->Card.sclk;
    }

    /* Find the memory controller latency for the display client.
    */
    if (IS_R300_VARIANT) {
        /*not enough for R350 ???*/
        /*
        if (!mode2) sclk_delay = 150;
        else {
            if (info->RamWidth == 256) sclk_delay = 87;
            else sclk_delay = 97;
        }
        */
        sclk_delay = 250;
    } else {
        if ((sd->Card.Type == RV100) || sd->Card.IsIGP) {
            if (sd->Card.IsDDR) sclk_delay = 41;
            else sclk_delay = 33;
        } else {
            if (sd->Card.RamWidth == 128) sclk_delay = 57;
            else sclk_delay = 41;
        }
    }

    mc_latency_sclk = sclk_delay / sclk_eff;

    if (sd->Card.IsDDR) {
        if (sd->Card.RamWidth == 32) {
            k1 = 40;
            c  = 3;
        } else {
            k1 = 20;
            c  = 1;
        }
    } else {
        k1 = 40;
        c  = 3;
    }
    mc_latency_mclk = ((2.0*mem_trcd + mem_tcas*c + 4.0*mem_tras + 4.0*mem_trp + k1) /
                       sd->Card.mclk) + (4.0 / sclk_eff);

    /*
      HW cursor time assuming worst case of full size colour cursor.
    */
    cur_latency_mclk = (mem_trp + MAX(mem_tras, (mem_trcd + 2*(cur_size - (sd->Card.IsDDR+1))))) / sd->Card.mclk;
    cur_latency_sclk = cur_size / sclk_eff;

    /*
      Find the total latency for the display data.
    */
    disp_latency_overhead = 8.0 / sd->Card.sclk;
    mc_latency_mclk = mc_latency_mclk + disp_latency_overhead + cur_latency_mclk;
    mc_latency_sclk = mc_latency_sclk + disp_latency_overhead + cur_latency_sclk;
    disp_latency = MAX(mc_latency_mclk, mc_latency_sclk);

    /*
      Find the drain rate of the display buffer.
    */
    disp_drain_rate = pix_clk / (16.0/mode->bpp);
    disp_drain_rate2 = 0;

    /*
      Find the critical point of the display buffer.
    */
    critical_point= (ULONG)(disp_drain_rate * disp_latency + 0.5);

    /* ???? */
    /*
    temp = (info->SavedReg.grph_buffer_cntl & RADEON_GRPH_CRITICAL_POINT_MASK) >> RADEON_GRPH_CRITICAL_POINT_SHIFT;
    if (critical_point < temp) critical_point = temp;
    */

    /*
      The critical point should never be above max_stop_req-4.  Setting
      GRPH_CRITICAL_CNTL = 0 will thus force high priority all the time.
    */
    if (max_stop_req - critical_point < 4) critical_point = 0;

    temp = sd->poweron_state->grph_buffer_cntl;
    temp &= ~(RADEON_GRPH_STOP_REQ_MASK);
    temp |= (stop_req << RADEON_GRPH_STOP_REQ_SHIFT);
    temp &= ~(RADEON_GRPH_START_REQ_MASK);
    if ((sd->Card.Type == R350) &&
        (stop_req > 0x15)) {
        stop_req -= 0x10;
    }
    temp |= (stop_req << RADEON_GRPH_START_REQ_SHIFT);

    temp |= RADEON_GRPH_BUFFER_SIZE;
    temp &= ~(RADEON_GRPH_CRITICAL_CNTL   |
              RADEON_GRPH_CRITICAL_AT_SOF |
              RADEON_GRPH_STOP_CNTL);
    /*
      Write the result into the register.
    */
    OUTREG(RADEON_GRPH_BUFFER_CNTL, ((temp & ~RADEON_GRPH_CRITICAL_POINT_MASK) |
                                     (critical_point << RADEON_GRPH_CRITICAL_POINT_SHIFT)));

    D(bug("[ATI] GRPH_BUFFER_CNTL from %x to %x\n",
               sd->poweron_state->grph_buffer_cntl, INREG(RADEON_GRPH_BUFFER_CNTL)));

}

/*
 * Powering done DAC, needed for DPMS problem with ViewSonic P817 (or its variant).
 *
 * Note for current DAC mapping when calling this function:
 * For most of cards:
 * single CRT:  Driver doesn't change the existing CRTC->DAC mapping, 
 *              CRTC1 could be driving either DAC or both DACs.
 * CRT+CRT:     CRTC1->TV DAC, CRTC2->Primary DAC
 * DFP/LCD+CRT: CRTC2->TV DAC, CRTC2->Primary DAC.
 * Some boards have two DACs reversed or don't even have a primary DAC,
 * this is reflected in pRADEONEnt->ReversedDAC. And radeon 7200 doesn't 
 * have a second DAC.
 * It's kind of messy, we'll need to redo DAC mapping part some day.
 */
static void RADEONDacPowerSet(struct ati_staticdata *sd, BOOL IsOn, BOOL IsPrimaryDAC)
{
    if (IsPrimaryDAC) {
        ULONG dac_cntl;
        ULONG dac_macro_cntl = 0;
        dac_cntl = INREG(RADEON_DAC_CNTL);
        if ((!sd->Card.IsMobility) || (sd->Card.Type == RV350))
            dac_macro_cntl = INREG(RADEON_DAC_MACRO_CNTL);
        if (IsOn) {
            dac_cntl &= ~RADEON_DAC_PDWN;
            dac_macro_cntl &= ~(RADEON_DAC_PDWN_R |
                                RADEON_DAC_PDWN_G |
                                RADEON_DAC_PDWN_B);
        } else {
            dac_cntl |= RADEON_DAC_PDWN;
            dac_macro_cntl |= (RADEON_DAC_PDWN_R |
                               RADEON_DAC_PDWN_G |
                               RADEON_DAC_PDWN_B);
        }
        OUTREG(RADEON_DAC_CNTL, dac_cntl);
        if ((!sd->Card.IsMobility) || (sd->Card.Type == RV350))
            OUTREG(RADEON_DAC_MACRO_CNTL, dac_macro_cntl);
    } else {
        if (sd->Card.Type != R200) {
            ULONG tv_dac_cntl = INREG(RADEON_TV_DAC_CNTL);
            if (IsOn) {
                tv_dac_cntl &= ~(RADEON_TV_DAC_RDACPD |
                                 RADEON_TV_DAC_GDACPD |
                                 RADEON_TV_DAC_BDACPD |
                                 RADEON_TV_DAC_BGSLEEP);
            } else {
                tv_dac_cntl |= (RADEON_TV_DAC_RDACPD |
                                RADEON_TV_DAC_GDACPD |
                                RADEON_TV_DAC_BDACPD |
                                RADEON_TV_DAC_BGSLEEP);
            }
            OUTREG(RADEON_TV_DAC_CNTL, tv_dac_cntl);
        } else {
            ULONG fp2_gen_cntl = INREG(RADEON_FP2_GEN_CNTL);
            if (IsOn) {
                fp2_gen_cntl |= RADEON_FP2_DVO_EN;
            } else {
                fp2_gen_cntl &= ~RADEON_FP2_DVO_EN;
            }
            OUTREG(RADEON_FP2_GEN_CNTL, fp2_gen_cntl);
        }
    }   
}

/* Define common registers for requested video mode */
static void RADEONInitCommonRegisters(struct ati_staticdata *sd, struct CardState *save, RADEONModeInfo *info)
{
    save->ovr_clr            = 0;
    save->ovr_wid_left_right = 0;
    save->ovr_wid_top_bottom = 0;
    save->ov0_scale_cntl     = 0;
    save->subpic_cntl        = 0;
    save->viph_control       = 0;
    save->i2c_cntl_1         = 0;
    save->rbbm_soft_reset    = 0;
    save->cap0_trig_cntl     = 0;
    save->cap1_trig_cntl     = 0;
    save->bus_cntl           = sd->Card.BusCntl;
    /*
     * If bursts are enabled, turn on discards
     * Radeon doesn't have write bursts
     */
    if (save->bus_cntl & (RADEON_BUS_READ_BURST))
    save->bus_cntl |= RADEON_BUS_RD_DISCARD_EN;
}

/* Define CRTC registers for requested video mode */
static BOOL RADEONInitCrtcRegisters(struct ati_staticdata *sd, struct CardState *save, RADEONModeInfo *mode)
{
    int  format;
    int  hsync_start;
    int  hsync_wid;
    int  vsync_wid;

    switch (mode->bpp) {
        case 15: format = 3; mode->bpp = 16; break;      /*  555 */
        case 16: format = 4; break;      /*  565 */
        case 24: format = 5; mode->bpp = 32; break;      /*  RGB */
        case 32: format = 6; break;      /* xRGB */
        default:
            return FALSE;
    }

    if (mode->bpp == 16)
        save->bpp = 2;
    else
        save->bpp = 4;

    if ((sd->Card.MonType1 == MT_DFP) ||
        (sd->Card.MonType1 == MT_LCD))
    {
        if (mode->Flags & RADEON_USE_RMX)
        {
            mode->HTotal        = mode->HDisplay + sd->Card.HBlank;
            mode->HSyncStart    = mode->HDisplay + sd->Card.HOverPlus;
            mode->HSyncEnd      = mode->HSyncStart + sd->Card.HSyncWidth;

            mode->VTotal        = mode->VDisplay + sd->Card.VBlank;
            mode->VSyncStart    = mode->VDisplay + sd->Card.VOverPlus;
            mode->VSyncEnd      = mode->VSyncStart + sd->Card.VSyncWidth;
            
            mode->pixelc        = sd->Card.DotClock;
            mode->Flags         = sd->Card.Flags | RADEON_USE_RMX;
        }
    }

    save->crtc_gen_cntl = (RADEON_CRTC_EXT_DISP_EN
               | RADEON_CRTC_EN
               | RADEON_CRTC_ARGB_EN
               | (format << 8)
               | ((mode->height < 400)
                  ? RADEON_CRTC_DBL_SCAN_EN
                  : 0));
                  
    if ((sd->Card.MonType1 == MT_DFP) ||
        (sd->Card.MonType1 == MT_LCD)) {
        save->crtc_ext_cntl = RADEON_VGA_ATI_LINEAR | RADEON_XCRT_CNT_EN;
        save->crtc_gen_cntl &= ~(RADEON_CRTC_DBL_SCAN_EN |
                     RADEON_CRTC_CSYNC_EN |
                     RADEON_CRTC_INTERLACE_EN);
    } else {
        save->crtc_ext_cntl = (RADEON_VGA_ATI_LINEAR |
                       RADEON_XCRT_CNT_EN |
                       RADEON_CRTC_CRT_ON);
    }
    
    save->dac_cntl = (RADEON_DAC_MASK_ALL
              | RADEON_DAC_VGA_ADR_EN
              | RADEON_DAC_8BIT_EN);

    save->crtc_h_total_disp = ((((mode->HTotal / 8) - 1) & 0x3ff)
                   | ((((mode->HDisplay / 8) - 1) & 0x1ff) << 16));

    hsync_wid = (mode->HSyncEnd - mode->HSyncStart) / 8;
    if (!hsync_wid) hsync_wid = 1;
    hsync_start = mode->HSyncStart - 8;

    save->crtc_h_sync_strt_wid = ((hsync_start & 0x1fff)
                  | ((hsync_wid & 0x3f) << 16));

    /* This works for double scan mode. */
    save->crtc_v_total_disp = (((mode->VTotal - 1) & 0xffff)
                   | ((mode->VDisplay - 1) << 16));
    
    vsync_wid = mode->VSyncEnd - mode->VSyncStart;
    if (!vsync_wid) vsync_wid = 1;

    save->crtc_v_sync_strt_wid = (((mode->VSyncStart - 1) & 0xfff)
                  | ((vsync_wid & 0x1f) << 16));

    save->crtc_offset      = mode->base;
    save->crtc_offset_cntl = INREG(RADEON_CRTC_OFFSET_CNTL);

    save->crtc_pitch  = (((mode->width * mode->bpp) +
              ((mode->bpp * 8) -1)) /
             (mode->bpp * 8));

    save->crtc_pitch |= save->crtc_pitch << 16;

    save->crtc_more_cntl = 0;
    if ((sd->Card.Type == RS100) ||
        (sd->Card.Type == RS200)) {
        /* This is to workaround the asic bug for RMX, some versions
           of BIOS dosen't have this register initialized correctly.
        */
        save->crtc_more_cntl |= RADEON_CRTC_H_CUTOFF_ACTIVE_EN;
    }

    save->surface_cntl = 0;
    save->disp_merge_cntl = sd->poweron_state->disp_merge_cntl;
    save->disp_merge_cntl &= ~RADEON_DISP_RGB_OFFSET_EN;

#if AROS_BIG_ENDIAN
    /* Alhought we current onlu use aperture 0, also setting aperture 1 should not harm -ReneR */
    switch (mode->bpp) {
        case 16:
            save->surface_cntl |= RADEON_NONSURF_AP0_SWP_16BPP;
            save->surface_cntl |= RADEON_NONSURF_AP1_SWP_16BPP;
            break;

        case 32:
            save->surface_cntl |= RADEON_NONSURF_AP0_SWP_32BPP;
            save->surface_cntl |= RADEON_NONSURF_AP1_SWP_32BPP;
            break;
    }
#endif

    return TRUE;
}

/* Define CRTC registers for requested video mode */
static BOOL RADEONInitCrtc2Registers(struct ati_staticdata *sd, struct CardState *save, RADEONModeInfo *mode)
{
    int  format;
    int  hsync_start;
    int  hsync_wid;
    int  vsync_wid;

    switch (mode->bpp) {
        case 15: format = 3; mode->bpp = 16; break;      /*  555 */
        case 16: format = 4; break;      /*  565 */
        case 24: format = 5; mode->bpp = 32; break;      /*  RGB */
        case 32: format = 6; break;      /* xRGB */
        default:
            return FALSE;
    }
    
    if (mode->bpp == 16)
        save->bpp = 2;
    else
        save->bpp = 4;
    
    save->crtc2_gen_cntl = (RADEON_CRTC2_EN
                | RADEON_CRTC2_CRT2_ON
                | RADEON_CRTC_ARGB_EN
                | (format << 8)
                | ((mode->height < 400)
                   ? RADEON_CRTC2_DBL_SCAN_EN
                   : 0));

    /* Turn CRT on in case the first head is a DFP */
    save->crtc_ext_cntl |= RADEON_CRTC_CRT_ON;
    save->dac2_cntl = sd->poweron_state->dac2_cntl;
    /* always let TVDAC drive CRT2, we don't support tvout yet */
    save->dac2_cntl |= RADEON_DAC2_DAC2_CLK_SEL;
    save->disp_output_cntl = sd->poweron_state->disp_output_cntl;
    if (sd->Card.Type == R200 || IS_R300_VARIANT) {
        save->disp_output_cntl &= ~(RADEON_DISP_DAC_SOURCE_MASK |
                    RADEON_DISP_DAC2_SOURCE_MASK);
        if (sd->Card.MonType2 != MT_CRT) {
            save->disp_output_cntl |= (RADEON_DISP_DAC_SOURCE_CRTC2 |
                       RADEON_DISP_DAC2_SOURCE_CRTC2);
        } else {
            if (sd->Card.ReversedDAC) {
                save->disp_output_cntl |= RADEON_DISP_DAC2_SOURCE_CRTC2;
            } else {
                save->disp_output_cntl |= RADEON_DISP_DAC_SOURCE_CRTC2;
            }
        }
    } else {
        save->disp_hw_debug = sd->poweron_state->disp_hw_debug;
        /* Turn on 2nd CRT */
        if (sd->Card.MonType2 != MT_CRT) {
            /* This is for some sample boards with the VGA port
               connected to the TVDAC, but BIOS doesn't reflect this.
               Here we configure both DACs to use CRTC2.
               Not sure if this happens in any retail board.
            */
            save->disp_hw_debug &= ~RADEON_CRT2_DISP1_SEL;
            save->dac2_cntl |= RADEON_DAC2_DAC_CLK_SEL;
        } else {
            if (sd->Card.ReversedDAC) {
                save->disp_hw_debug &= ~RADEON_CRT2_DISP1_SEL;
                save->dac2_cntl &= ~RADEON_DAC2_DAC_CLK_SEL;
            } else {
                save->disp_hw_debug |= RADEON_CRT2_DISP1_SEL;
                save->dac2_cntl |= RADEON_DAC2_DAC_CLK_SEL;
            }
        }
    }

    save->crtc2_h_total_disp =
           ((((mode->HTotal / 8) - 1) & 0x3ff)
         | ((((mode->HDisplay / 8) - 1) & 0x1ff) << 16));

    hsync_wid = (mode->HSyncEnd - mode->HSyncStart) / 8;
    if (!hsync_wid) hsync_wid = 1;
    hsync_start = mode->HSyncStart - 8;

    save->crtc2_h_sync_strt_wid = ((hsync_start & 0x1fff)
                   | ((hsync_wid & 0x3f) << 16));

    /* This works for double scan mode. */
    save->crtc2_v_total_disp = (((mode->VTotal - 1) & 0xffff)
                | ((mode->VDisplay - 1) << 16));   

    vsync_wid = mode->VSyncEnd - mode->VSyncStart;
    if (!vsync_wid) vsync_wid = 1;

    save->crtc2_v_sync_strt_wid = (((mode->VSyncStart - 1) & 0xfff)
                   | ((vsync_wid & 0x1f) << 16));

    save->crtc2_offset      = mode->base;
    save->crtc2_offset_cntl = INREG(RADEON_CRTC2_OFFSET_CNTL);
    /* this should be right */
    save->crtc2_pitch  = (((mode->width * mode->bpp) +
               ((mode->bpp * 8) -1)) /
              (mode->bpp * 8));
    save->crtc2_pitch |= save->crtc2_pitch << 16;

    save->disp2_merge_cntl = sd->poweron_state->disp2_merge_cntl;
    save->disp2_merge_cntl &= ~(RADEON_DISP2_RGB_OFFSET_EN);

    if (sd->Card.MonType2 == MT_DFP && sd->Card.IsSecondary) {
        save->crtc2_gen_cntl      = (RADEON_CRTC2_EN | (format << 8));
        save->fp2_h_sync_strt_wid = save->crtc2_h_sync_strt_wid;
        save->fp2_v_sync_strt_wid = save->crtc2_v_sync_strt_wid;
        save->fp2_gen_cntl        = sd->poweron_state->fp2_gen_cntl | RADEON_FP2_ON;

        if (sd->Card.Type == R200 || IS_R300_VARIANT) {
            save->fp2_gen_cntl   &= ~(R200_FP2_SOURCE_SEL_MASK |
                          RADEON_FP2_DVO_RATE_SEL_SDR);
    
            save->fp2_gen_cntl |= (R200_FP2_SOURCE_SEL_CRTC2 | 
                       RADEON_FP2_DVO_EN);
        } else {
            save->fp2_gen_cntl &= ~RADEON_FP2_SRC_SEL_MASK;
            save->fp2_gen_cntl |= RADEON_FP2_SRC_SEL_CRTC2;
        }
    
        save->fp2_gen_cntl |= RADEON_FP2_PANEL_FORMAT; /* 24 bit format */
    }

    return TRUE;
}

/* Define CRTC registers for requested video mode */
static void RADEONInitFPRegisters(struct ati_staticdata *sd, struct CardState *save, RADEONModeInfo *mode)
{
    int    xres = mode->HDisplay;
    int    yres = mode->VDisplay;
    float  Hratio, Vratio;

    /* If the FP registers have been initialized before for a panel,
     * but the primary port is a CRT, we need to reinitialize
     * FP registers in order for CRT to work properly
     */

    if ((sd->Card.MonType1 != MT_DFP) && (sd->Card.MonType1 != MT_LCD)) {
        save->fp_crtc_h_total_disp = sd->poweron_state->fp_crtc_h_total_disp;
        save->fp_crtc_v_total_disp = sd->poweron_state->fp_crtc_v_total_disp;
        save->fp_gen_cntl          = 0;
        save->fp_h_sync_strt_wid   = sd->poweron_state->fp_h_sync_strt_wid;
        save->fp_horz_stretch      = 0;
        save->fp_v_sync_strt_wid   = sd->poweron_state->fp_v_sync_strt_wid;
        save->fp_vert_stretch      = 0;
        save->lvds_gen_cntl        = sd->poweron_state->lvds_gen_cntl;
        save->lvds_pll_cntl        = sd->poweron_state->lvds_pll_cntl;
        save->tmds_pll_cntl        = sd->poweron_state->tmds_pll_cntl;
        save->tmds_transmitter_cntl= sd->poweron_state->tmds_transmitter_cntl;

        save->lvds_gen_cntl |= ( RADEON_LVDS_DISPLAY_DIS | (1 << 23));
        save->lvds_gen_cntl &= ~(RADEON_LVDS_BLON | RADEON_LVDS_ON);
        save->fp_gen_cntl &= ~(RADEON_FP_FPON | RADEON_FP_TMDS_EN);

        return;
    }

    if (sd->Card.PanelXRes == 0 || sd->Card.PanelYRes == 0) {
        Hratio = 1.0;
        Vratio = 1.0;
    } else {
        if (xres > sd->Card.PanelXRes) xres = sd->Card.PanelXRes;
        if (yres > sd->Card.PanelYRes) yres = sd->Card.PanelYRes;
    
        Hratio = (float)xres/(float)sd->Card.PanelXRes;
        Vratio = (float)yres/(float)sd->Card.PanelYRes;
    }
    
    if (Hratio == 1.0 || !(mode->Flags & RADEON_USE_RMX)) {
        save->fp_horz_stretch = sd->poweron_state->fp_horz_stretch;
        save->fp_horz_stretch &= ~(RADEON_HORZ_STRETCH_BLEND |
                       RADEON_HORZ_STRETCH_ENABLE);
        save->fp_horz_stretch &= ~(RADEON_HORZ_AUTO_RATIO |
                       RADEON_HORZ_PANEL_SIZE);
        save->fp_horz_stretch |= ((xres/8-1)<<16);
    } else {
        save->fp_horz_stretch =
            ((((unsigned long)(Hratio * RADEON_HORZ_STRETCH_RATIO_MAX +
                       0.5)) & RADEON_HORZ_STRETCH_RATIO_MASK)) |
            (sd->poweron_state->fp_horz_stretch & (RADEON_HORZ_PANEL_SIZE |
                          RADEON_HORZ_FP_LOOP_STRETCH |
                          RADEON_HORZ_AUTO_RATIO_INC));
        save->fp_horz_stretch |= (RADEON_HORZ_STRETCH_BLEND |
                      RADEON_HORZ_STRETCH_ENABLE);
    
        save->fp_horz_stretch &= ~(RADEON_HORZ_AUTO_RATIO |
                       RADEON_HORZ_PANEL_SIZE);
        save->fp_horz_stretch |= ((sd->Card.PanelXRes / 8 - 1) << 16);
    }

    if (Vratio == 1.0 || !(mode->Flags & RADEON_USE_RMX)) {
        save->fp_vert_stretch = sd->poweron_state->fp_vert_stretch;
        save->fp_vert_stretch &= ~(RADEON_VERT_STRETCH_ENABLE|
                       RADEON_VERT_STRETCH_BLEND);
        save->fp_vert_stretch &= ~(RADEON_VERT_AUTO_RATIO_EN |
                       RADEON_VERT_PANEL_SIZE);
        save->fp_vert_stretch |= ((yres-1) << 12);
    } else {
        save->fp_vert_stretch =
            (((((unsigned long)(Vratio * RADEON_VERT_STRETCH_RATIO_MAX +
                    0.5)) & RADEON_VERT_STRETCH_RATIO_MASK)) |
             (sd->poweron_state->fp_vert_stretch & (RADEON_VERT_PANEL_SIZE |
                           RADEON_VERT_STRETCH_RESERVED)));
        save->fp_vert_stretch |= (RADEON_VERT_STRETCH_ENABLE |
                      RADEON_VERT_STRETCH_BLEND);
    
        save->fp_vert_stretch &= ~(RADEON_VERT_AUTO_RATIO_EN |
                       RADEON_VERT_PANEL_SIZE);
        save->fp_vert_stretch |= ((sd->Card.PanelYRes-1) << 12);
    }

    save->fp_gen_cntl = (sd->poweron_state->fp_gen_cntl & (ULONG)
             ~(RADEON_FP_SEL_CRTC2 |
               RADEON_FP_RMX_HVSYNC_CONTROL_EN |
               RADEON_FP_DFP_SYNC_SEL |
               RADEON_FP_CRT_SYNC_SEL |
               RADEON_FP_CRTC_LOCK_8DOT |
               RADEON_FP_USE_SHADOW_EN |
               RADEON_FP_CRTC_USE_SHADOW_VEND |
               RADEON_FP_CRT_SYNC_ALT));
    save->fp_gen_cntl |= (RADEON_FP_CRTC_DONT_SHADOW_VPAR |
              RADEON_FP_CRTC_DONT_SHADOW_HEND );

    save->fp_gen_cntl |= RADEON_FP_PANEL_FORMAT;  /* 24 bit format */

    if (IS_R300_VARIANT || (sd->Card.Type == R200)) {
        save->fp_gen_cntl &= ~R200_FP_SOURCE_SEL_MASK;
        if (sd->Card.Flags & RADEON_USE_RMX) 
            save->fp_gen_cntl |= R200_FP_SOURCE_SEL_RMX;
        else
            save->fp_gen_cntl |= R200_FP_SOURCE_SEL_CRTC1;
    } else 
        save->fp_gen_cntl |= RADEON_FP_SEL_CRTC1;

    save->lvds_gen_cntl = sd->poweron_state->lvds_gen_cntl;
    save->lvds_pll_cntl = sd->poweron_state->lvds_pll_cntl;

    save->tmds_pll_cntl = sd->poweron_state->tmds_pll_cntl;
    save->tmds_transmitter_cntl= sd->poweron_state->tmds_transmitter_cntl;

    if (sd->Card.MonType1 == MT_LCD) {

        save->lvds_gen_cntl |= (RADEON_LVDS_ON | RADEON_LVDS_BLON);
        save->fp_gen_cntl   &= ~(RADEON_FP_FPON | RADEON_FP_TMDS_EN);

    } else if (sd->Card.MonType1 == MT_DFP) {
        int i;
        ULONG tmp = sd->poweron_state->tmds_pll_cntl & 0xfffff;
        for (i=0; i<4; i++) {
            if (sd->Card.tmds_pll[i].freq == 0) break;
            if (save->dot_clock_freq < sd->Card.tmds_pll[i].freq) {
                tmp = sd->Card.tmds_pll[i].value ;
                break;
            }
        }
        if (IS_R300_VARIANT || (sd->Card.Type == RV280)) {
            if (tmp & 0xfff00000)
                save->tmds_pll_cntl = tmp;
            else
                save->tmds_pll_cntl = (sd->poweron_state->tmds_pll_cntl & 0xfff00000) | tmp;
        } else save->tmds_pll_cntl = tmp;

        D(bug("[ATI] TMDS_PLL from %x to %x\n", 
             sd->poweron_state->tmds_pll_cntl, 
             save->tmds_pll_cntl));

        save->tmds_transmitter_cntl &= ~(RADEON_TMDS_TRANSMITTER_PLLRST);

        if (IS_R300_VARIANT || (sd->Card.Type == R200) || !sd->Card.HasCRTC2)
            save->tmds_transmitter_cntl &= ~(RADEON_TMDS_TRANSMITTER_PLLEN);
        else /* weird, RV chips got this bit reversed? */
            save->tmds_transmitter_cntl |= (RADEON_TMDS_TRANSMITTER_PLLEN);

        save->fp_gen_cntl   |= (RADEON_FP_FPON | RADEON_FP_TMDS_EN);
    }

    if (sd->Card.IsMobility) {
        /* To work correctly with laptop hotkeys.
         * Since there is no machnism for accessing ACPI evnets
         * and the driver currently doesn't know how to validate
         * a mode dynamically, we have to tell BIOS don't do
         * display switching after X has started.  
         * If LCD is on, lid close/open should still work 
         * with below settings
         */
        if (sd->Card.MonType1 == MT_LCD) {
            if (sd->Card.MonType2 == MT_CRT)
                save->bios_5_scratch = 0x0201;
            else if (sd->Card.MonType2 == MT_DFP)
                save->bios_5_scratch = 0x0801;
            else
                save->bios_5_scratch = sd->poweron_state->bios_5_scratch;
        } else {
            if (sd->Card.MonType2 == MT_CRT)
                save->bios_5_scratch = 0x0200;
            else if (sd->Card.MonType2 == MT_DFP)
                save->bios_5_scratch = 0x0800;
            else
                save->bios_5_scratch = 0x0; 
        }
        save->bios_4_scratch = 0x4;
        save->bios_6_scratch = sd->poweron_state->bios_6_scratch | 0x40000000;
    }

    save->fp_crtc_h_total_disp = save->crtc_h_total_disp;
    save->fp_crtc_v_total_disp = save->crtc_v_total_disp;
    save->fp_h_sync_strt_wid   = save->crtc_h_sync_strt_wid;
    save->fp_v_sync_strt_wid   = save->crtc_v_sync_strt_wid;
}

static void RADEONInitPLLRegisters(struct ati_staticdata *sd, struct CardState *save, RADEONModeInfo *mode)
{
    double freq = mode->pixelc/10.0;
    
    struct {
        int divider;
        int bitvalue;
    } *post_div, post_divs[]   = {
                /* From RAGE 128 VR/RAGE 128 GL Register
                 * Reference Manual (Technical Reference
                 * Manual P/N RRG-G04100-C Rev. 0.04), page
                 * 3-17 (PLL_DIV_[3:0]).
                 */
        {  1, 0 },              /* VCLK_SRC                 */
        {  2, 1 },              /* VCLK_SRC/2               */
        {  4, 2 },              /* VCLK_SRC/4               */
        {  8, 3 },              /* VCLK_SRC/8               */
        {  3, 4 },              /* VCLK_SRC/3               */
        { 16, 5 },              /* VCLK_SRC/16              */
        {  6, 6 },              /* VCLK_SRC/6               */
        { 12, 7 },              /* VCLK_SRC/12              */
        {  0, 0 }
    };

    if (freq > sd->Card.pll.max_pll_freq)      freq = sd->Card.pll.max_pll_freq;
    if (freq * 12 < sd->Card.pll.min_pll_freq) freq = sd->Card.pll.min_pll_freq / 12;

    for (post_div = &post_divs[0]; post_div->divider; ++post_div) {
        save->pll_output_freq = post_div->divider * freq;
    
        if (save->pll_output_freq >= sd->Card.pll.min_pll_freq
            && save->pll_output_freq <= sd->Card.pll.max_pll_freq) break;
    }

    if (!post_div->divider) {
        save->pll_output_freq = freq;
        post_div = &post_divs[0];
    }

    save->dot_clock_freq = freq;
    save->feedback_div   = RADEONDiv(sd->Card.pll.reference_div
                     * save->pll_output_freq,
                     sd->Card.pll.reference_freq);
    save->post_div       = post_div->divider;

    D(bug("[ATI] dc=%d, of=%d, fd=%d, pd=%d\n",
           save->dot_clock_freq,
           save->pll_output_freq,
           save->feedback_div,
           save->post_div));

    save->ppll_ref_div   = sd->Card.pll.reference_div;
    save->ppll_div_3     = (save->feedback_div | (post_div->bitvalue << 16));
    save->htotal_cntl    = 0;
}

/* Define PLL2 registers for requested video mode */
static void RADEONInitPLL2Registers(struct ati_staticdata *sd, struct CardState *save, RADEONModeInfo *mode)
{
    double freq = mode->pixelc/10.0;

    struct {
        int divider;
        int bitvalue;
    } *post_div, post_divs[]   = {
                /* From RAGE 128 VR/RAGE 128 GL Register
                 * Reference Manual (Technical Reference
                 * Manual P/N RRG-G04100-C Rev. 0.04), page
                 * 3-17 (PLL_DIV_[3:0]).
                 */
        {  1, 0 },              /* VCLK_SRC                 */
        {  2, 1 },              /* VCLK_SRC/2               */
        {  4, 2 },              /* VCLK_SRC/4               */
        {  8, 3 },              /* VCLK_SRC/8               */
        {  3, 4 },              /* VCLK_SRC/3               */
        {  6, 6 },              /* VCLK_SRC/6               */
        { 12, 7 },              /* VCLK_SRC/12              */
        {  0, 0 }
    };

    if (freq > sd->Card.pll.max_pll_freq)      freq = sd->Card.pll.max_pll_freq;
    if (freq * 12 < sd->Card.pll.min_pll_freq) freq = sd->Card.pll.min_pll_freq / 12;

    for (post_div = &post_divs[0]; post_div->divider; ++post_div) {
        save->pll_output_freq_2 = post_div->divider * freq;
        if (save->pll_output_freq_2 >= sd->Card.pll.min_pll_freq
            && save->pll_output_freq_2 <= sd->Card.pll.max_pll_freq) break;
    }

    if (!post_div->divider) {
        save->pll_output_freq_2 = freq;
        post_div = &post_divs[0];
    }

    save->dot_clock_freq_2 = freq;
    save->feedback_div_2   = RADEONDiv(sd->Card.pll.reference_div
                       * save->pll_output_freq_2,
                       sd->Card.pll.reference_freq);
    save->post_div_2       = post_div->divider;

    D(bug("[ATI] dc=%d, of=%d, fd=%d, pd=%d\n",
           save->dot_clock_freq_2,
           save->pll_output_freq_2,
           save->feedback_div_2,
           save->post_div_2));

    save->p2pll_ref_div    = sd->Card.pll.reference_div;
    save->p2pll_div_0      = (save->feedback_div_2 |
                  (post_div->bitvalue << 16));
    save->htotal_cntl2     = 0;
}

static void RADEONGetVRamType(struct ati_staticdata *sd)
{
    ULONG tmp;

    if (sd->Card.IsIGP || (sd->Card.Type >= R300) ||
        (INREG(RADEON_MEM_SDRAM_MODE_REG) & (1<<30)))
        sd->Card.IsDDR = TRUE;
    else
        sd->Card.IsDDR = FALSE;

    tmp = INREG(RADEON_MEM_CNTL);
    if (IS_R300_VARIANT) {
        tmp &=  R300_MEM_NUM_CHANNELS_MASK;
        switch (tmp) {
            case 0: sd->Card.RamWidth = 64; break;
            case 1: sd->Card.RamWidth = 128; break;
            case 2: sd->Card.RamWidth = 256; break;
            default: sd->Card.RamWidth = 128; break;
        }
    } else if ((sd->Card.Type == RV100) ||
               (sd->Card.Type == RS100) ||
               (sd->Card.Type == RS200)){
        if (tmp & RV100_HALF_MODE) sd->Card.RamWidth = 32;
        else sd->Card.RamWidth = 64;
    } else {
        if (tmp & RADEON_MEM_NUM_CHANNELS_MASK) sd->Card.RamWidth = 128;
        else sd->Card.RamWidth = 64;
    }

    /* This may not be correct, as some cards can have half of channel disabled
     * ToDo: identify these cases
     */
}

struct RADEONInt10Save {
    ULONG MEM_CNTL;
    ULONG MEMSIZE;
    ULONG MPP_TB_CONFIG;
};

static void RADEONPreInt10Save(struct ati_staticdata *sd, void **pPtr)
{
    ULONG CardTmp;
    static struct RADEONInt10Save SaveStruct = { 0, 0, 0 };

    /* Save the values and zap MEM_CNTL */
    SaveStruct.MEM_CNTL = INREG(RADEON_MEM_CNTL);
    SaveStruct.MEMSIZE = INREG(RADEON_CONFIG_MEMSIZE);
    SaveStruct.MPP_TB_CONFIG = INREG(RADEON_MPP_TB_CONFIG);

    /*
     * Zap MEM_CNTL and set MPP_TB_CONFIG<31:24> to 4
     */
    OUTREG(RADEON_MEM_CNTL, 0);
    CardTmp = SaveStruct.MPP_TB_CONFIG & 0x00ffffffu;
    CardTmp |= 0x04 << 24;
    OUTREG(RADEON_MPP_TB_CONFIG, CardTmp);

    *pPtr = (void *)&SaveStruct;
}

static void RADEONPostInt10Check(struct ati_staticdata *sd, void *ptr)
{
    struct RADEONInt10Save *pSave = ptr;
    ULONG CardTmp;

    /* If we don't have a valid (non-zero) saved MEM_CNTL, get out now */
    if (!pSave || !pSave->MEM_CNTL)
        return;

    /*
     * If either MEM_CNTL is currently zero or inconistent (configured for
     * two channels with the two channels configured differently), restore
     * the saved registers.
     */
    CardTmp = INREG(RADEON_MEM_CNTL);
    if (!CardTmp ||
        ((CardTmp & 1) &&
         (((CardTmp >> 8) & 0xff) != ((CardTmp >> 24) & 0xff)))) {
        /* Restore the saved registers */
        D(bug("[ATI] Restoring MEM_CNTL (%08lx), setting to %08lx\n",
                   (unsigned long)CardTmp, (unsigned long)pSave->MEM_CNTL));
        OUTREG(RADEON_MEM_CNTL, pSave->MEM_CNTL);

        CardTmp = INREG(RADEON_CONFIG_MEMSIZE);
        if (CardTmp != pSave->MEMSIZE) {
            D(bug("[ATI] Restoring CONFIG_MEMSIZE (%08lx), setting to %08lx\n",
                       (unsigned long)CardTmp, (unsigned long)pSave->MEMSIZE));
            OUTREG(RADEON_CONFIG_MEMSIZE, pSave->MEMSIZE);
        }
    }

    CardTmp = INREG(RADEON_MPP_TB_CONFIG);
    if ((CardTmp & 0xff000000u) != (pSave->MPP_TB_CONFIG & 0xff000000u)) {
        D(bug("[ATI] Restoring MPP_TB_CONFIG<31:24> (%02lx), setting to %02lx\n",
                   (unsigned long)CardTmp >> 24,
                   (unsigned long)pSave->MPP_TB_CONFIG >> 24));
        CardTmp &= 0x00ffffffu;
        CardTmp |= (pSave->MPP_TB_CONFIG & 0xff000000u);
        OUTREG(RADEON_MPP_TB_CONFIG, CardTmp);
    }
}

static void RADEONGetPanelInfo(struct ati_staticdata *sd)
{
}

static void RADEONGetClockInfo(struct ati_staticdata *sd)
{
    RADEONPLLRec *pll = &sd->Card.pll;
    double min_dotclock;

    if (RADEONGetClockInfoFromBIOS(sd)) {
        if (pll->reference_div < 2) {
            /* retrive it from register setting for fitting into current PLL algorithm.
               We'll probably need a new routine to calculate the best ref_div from BIOS
               provided min_input_pll and max_input_pll
            */
            ULONG tmp;
            tmp = RADEONINPLL(sd, RADEON_PPLL_REF_DIV);
            if (IS_R300_VARIANT ||
                (sd->Card.Type == RS300)) {
                pll->reference_div = (tmp & R300_PPLL_REF_DIV_ACC_MASK) >> R300_PPLL_REF_DIV_ACC_SHIFT;
            } else {
                pll->reference_div = tmp & RADEON_PPLL_REF_DIV_MASK;
            }

            if (pll->reference_div < 2) pll->reference_div = 12;
        }

    } else {
        D(bug("[ATI] Video BIOS not detected, using default clock settings!\n"));

        if (sd->Card.IsIGP)
            pll->reference_freq = 1432;
        else
            pll->reference_freq = 2700;

        pll->reference_div = 12;
        pll->min_pll_freq = 12500;
        pll->max_pll_freq = 35000;
        pll->xclk = 10300;

        sd->Card.sclk = 200.00;
        sd->Card.mclk = 200.00;
    }

    D(bug("[ATI] PLL parameters: rf=%d rd=%d min=%ld max=%ld; xclk=%d\n",
                pll->reference_freq,
                pll->reference_div,
                pll->min_pll_freq, pll->max_pll_freq, pll->xclk));
}

static BOOL RADEONQueryConnectedMonitors(struct ati_staticdata *sd)
{
    const char *s;
    BOOL ignore_edid = TRUE;
    int i = 0, second = 0, max_mt;

    const char *MonTypeName[7] =
    {
        "AUTO",
        "NONE",
        "CRT",
        "LVDS",
        "TMDS",
        "CTV",
        "STV"
    };

    const RADEONMonitorType MonTypeID[7] =
    {
        MT_UNKNOWN, /* this is just a dummy value for AUTO DETECTION */
        MT_NONE,    /* NONE -> NONE */
        MT_CRT,     /* CRT -> CRT */
        MT_LCD,     /* Laptop LCDs are driven via LVDS port */
        MT_DFP,     /* DFPs are driven via TMDS */
        MT_CTV,     /* CTV -> CTV */
        MT_STV,     /* STV -> STV */
    };

    const char *TMDSTypeName[3] =
    {
        "NONE",
        "Internal",
        "External"
    };

    const char *DDCTypeName[5] =
    {
        "NONE",
        "MONID",
        "DVI_DDC",
        "VGA_DDC",
        "CRT2_DDC"
    };

    const char *DACTypeName[3] =
    {
        "Unknown",
        "Primary",
        "TVDAC/ExtDAC",
    };

    const char *ConnectorTypeName[8] =
    {
        "None",
        "Proprietary",
        "VGA",
        "DVI-I",
        "DVI-D",
        "CTV",
        "STV",
        "Unsupported"
    };

    const char *ConnectorTypeNameATOM[10] =
    {
        "None",
        "VGA",
        "DVI-I",
        "DVI-D",
        "DVI-A",
        "STV",
        "CTV",
        "LVDS",
        "Digital",
        "Unsupported"
    };

    max_mt = 5;

/*
    if(info->IsSecondary) {
        info->DisplayType = (RADEONMonitorType)pRADEONEnt->MonType2;
        if(info->DisplayType == MT_NONE) return FALSE;
        return TRUE;
    }
*/

    /* We first get the information about all connectors from BIOS.
     * This is how the card is phyiscally wired up.
     * The information should be correct even on a OEM card.
     * If not, we may have problem -- need to use MonitorLayout option.
     */
    for (i = 0; i < 2; i++) {
        sd->Card.PortInfo[i].MonType = MT_UNKNOWN;
//        sd->Card.PortInfo[i].MonInfo = NULL;
        sd->Card.PortInfo[i].DDCType = DDC_NONE_DETECTED;
        sd->Card.PortInfo[i].DACType = DAC_UNKNOWN;
        sd->Card.PortInfo[i].TMDSType = TMDS_UNKNOWN;
        sd->Card.PortInfo[i].ConnectorType = CONNECTOR_NONE;
    }

    if (!RADEONGetConnectorInfoFromBIOS(sd)) {
        /* Below is the most common setting, but may not be true */
        sd->Card.PortInfo[0].MonType = MT_UNKNOWN;
//        sd->Card.PortInfo[0].MonInfo = NULL;
        sd->Card.PortInfo[0].DDCType = DDC_DVI;
        sd->Card.PortInfo[0].DACType = DAC_TVDAC;
        sd->Card.PortInfo[0].TMDSType = TMDS_INT;
        sd->Card.PortInfo[0].ConnectorType = CONNECTOR_DVI_D;

        sd->Card.PortInfo[1].MonType = MT_UNKNOWN;
//        sd->Card.PortInfo[1].MonInfo = NULL;
        sd->Card.PortInfo[1].DDCType = DDC_VGA;
        sd->Card.PortInfo[1].DACType = DAC_PRIMARY;
        sd->Card.PortInfo[1].TMDSType = TMDS_EXT;
        sd->Card.PortInfo[1].ConnectorType = CONNECTOR_CRT;
    }
    
    /* always make TMDS_INT port first*/
    if (sd->Card.PortInfo[1].TMDSType == TMDS_INT) {
        RADEONConnector connector;
        connector = sd->Card.PortInfo[0];
        sd->Card.PortInfo[0] = sd->Card.PortInfo[1];
        sd->Card.PortInfo[1] = connector;
    } else if ((sd->Card.PortInfo[0].TMDSType != TMDS_INT &&
                sd->Card.PortInfo[1].TMDSType != TMDS_INT)) {
        /* no TMDS_INT port, make primary DAC port first */
        if (sd->Card.PortInfo[1].DACType == DAC_PRIMARY) {
            RADEONConnector connector;
            connector = sd->Card.PortInfo[0];
            sd->Card.PortInfo[0] = sd->Card.PortInfo[1];
            sd->Card.PortInfo[1] = connector;
        }
    }
    
    if (sd->Card.HasSingleDAC) {
        /* For RS300/RS350/RS400 chips, there is no primary DAC. Force VGA port to use TVDAC*/
        if (sd->Card.PortInfo[0].ConnectorType == CONNECTOR_CRT) {
            sd->Card.PortInfo[0].DACType = DAC_TVDAC;
            sd->Card.PortInfo[1].DACType = DAC_PRIMARY;
        } else {
            sd->Card.PortInfo[1].DACType = DAC_TVDAC;
            sd->Card.PortInfo[0].DACType = DAC_PRIMARY;
        }
    } else if (!sd->Card.HasCRTC2) {
        sd->Card.PortInfo[0].DACType = DAC_PRIMARY;
    }
    
    if(((!sd->Card.HasCRTC2) || sd->Card.IsDellServer)) {
        if (sd->Card.PortInfo[0].MonType == MT_UNKNOWN) {
/*
            if((sd->Card.PortInfo[0].MonType = RADEONDisplayDDCConnected(pScrn, DDC_DVI, &sd->Card.PortInfo[0])));
            else if((sd->Card.PortInfo[0].MonType = RADEONDisplayDDCConnected(pScrn, DDC_VGA, &sd->Card.PortInfo[0])));
            else if((sd->Card.PortInfo[0].MonType = RADEONDisplayDDCConnected(pScrn, DDC_CRT2, &sd->Card.PortInfo[0])));
            else
*/
                sd->Card.PortInfo[0].MonType = MT_CRT; 
        }
        sd->Card.MonType1 = sd->Card.PortInfo[0].MonType;
//        pRADEONEnt->MonInfo1 = sd->Card.PortInfo[0].  .MonInfo;
        sd->Card.MonType2 = MT_NONE;
//        pRADEONEnt->MonInfo2 = NULL;

        D(bug("[ATI] Primary:\n Monitor   -- %s\n Connector -- %s\n DAC Type  -- %s\n TMDS Type -- %s\n DDC Type  -- %s\n",
                   MonTypeName[sd->Card.PortInfo[0].MonType+1],
                   sd->Card.IsAtomBios ?
                   ConnectorTypeNameATOM[sd->Card.PortInfo[0].ConnectorType]:
                   ConnectorTypeName[sd->Card.PortInfo[0].ConnectorType],
                   DACTypeName[sd->Card.PortInfo[0].DACType+1],
                   TMDSTypeName[sd->Card.PortInfo[0].TMDSType+1],
                   DDCTypeName[sd->Card.PortInfo[0].DDCType]));

        return TRUE;
    }

    if (sd->Card.PortInfo[0].MonType == MT_UNKNOWN || sd->Card.PortInfo[1].MonType == MT_UNKNOWN) {

        /* Primary Head (DVI or Laptop Int. panel)*/
        /* A ddc capable display connected on DVI port */
        if (sd->Card.PortInfo[0].MonType == MT_UNKNOWN) {
//            if((sd->Card.PortInfo[0].MonType = RADEONDisplayDDCConnected(pScrn, pRADEONEnt->PortInfo[0].DDCType, &pRADEONEnt->PortInfo[0])));
//            else 
            if (sd->Card.IsMobility &&
                     (INREG(RADEON_BIOS_4_SCRATCH) & 4)) {
                /* non-DDC laptop panel connected on primary */
                sd->Card.PortInfo[0].MonType = MT_LCD;
            } else {
                /* CRT on DVI, TODO: not reliable, make it always return false for now*/
//                sd->Card.PortInfo[0].MonType = RADEONCrtIsPhysicallyConnected(pScrn, !(pRADEONEnt->PortInfo[0].DACType));
            }
        }

        /* Secondary Head (mostly VGA, can be DVI on some OEM boards)*/
        if (sd->Card.PortInfo[1].MonType == MT_UNKNOWN) {
//            if((sd->Card.PortInfo[1].MonType =
//                RADEONDisplayDDCConnected(pScrn, pRADEONEnt->PortInfo[1].DDCType, &pRADEONEnt->PortInfo[1])));
//            else 
            if (sd->Card.IsMobility &&
                     (INREG(RADEON_FP2_GEN_CNTL) & RADEON_FP2_ON)) {
                /* non-DDC TMDS panel connected through DVO */
                sd->Card.PortInfo[1].MonType = MT_DFP;
            } 
            //else
//                sd->Card.PortInfo[1].MonType = RADEONCrtIsPhysicallyConnected(pScrn, !(pRADEONEnt->PortInfo[1].DACType));
        
        }
    }

    sd->Card.MonType1 = sd->Card.PortInfo[0].MonType;
    sd->Card.MonType2 = sd->Card.PortInfo[1].MonType;
    if (sd->Card.PortInfo[0].MonType == MT_NONE) {
        if (sd->Card.PortInfo[1].MonType == MT_NONE) {
            sd->Card.MonType1 = MT_CRT;
        } else {
            RADEONConnector tmp;
            sd->Card.MonType1 = sd->Card.PortInfo[1].MonType;
            tmp = sd->Card.PortInfo[0];
            sd->Card.PortInfo[0] = sd->Card.PortInfo[1];
            sd->Card.PortInfo[1] = tmp;
        }
        sd->Card.MonType2 = MT_NONE;
    }
    sd->Card.ReversedDAC = FALSE;
    sd->Card.OverlayOnCRTC2 = FALSE;

    if (sd->Card.MonType2 != MT_NONE) {

        if (sd->Card.PortInfo[1].DACType == DAC_TVDAC) {
            D(bug("[ATI] Reversed DAC decteced\n"));
            sd->Card.ReversedDAC = TRUE;
        }
    } else {
        sd->Card.HasSecondary = FALSE;
    }
    D(bug("[ATI] Primary:\n Monitor   -- %s\n Connector -- %s\n DAC Type  -- %s\n TMDS Type -- %s\n DDC Type  -- %s\n",
               MonTypeName[sd->Card.PortInfo[0].MonType+1],
               sd->Card.IsAtomBios ?
               ConnectorTypeNameATOM[sd->Card.PortInfo[0].ConnectorType]:
               ConnectorTypeName[sd->Card.PortInfo[0].ConnectorType],
               DACTypeName[sd->Card.PortInfo[0].DACType+1],
               TMDSTypeName[sd->Card.PortInfo[0].TMDSType+1],
               DDCTypeName[sd->Card.PortInfo[0].DDCType]));

    D(bug("[ATI] Secondary:\n Monitor   -- %s\n Connector -- %s\n DAC Type  -- %s\n TMDS Type -- %s\n DDC Type  -- %s\n",
               MonTypeName[sd->Card.PortInfo[1].MonType+1],
               sd->Card.IsAtomBios ?
               ConnectorTypeNameATOM[sd->Card.PortInfo[1].ConnectorType]:
               ConnectorTypeName[sd->Card.PortInfo[1].ConnectorType],
               DACTypeName[sd->Card.PortInfo[1].DACType+1],
               TMDSTypeName[sd->Card.PortInfo[1].TMDSType+1],
               DDCTypeName[sd->Card.PortInfo[1].DDCType]));

    return TRUE;
}




/*
 * Accesible functions
 */

void InitMode(struct ati_staticdata *sd, struct CardState *save,
                ULONG width, ULONG height, UBYTE bpp, ULONG pixelc, IPTR base,
                ULONG HDisplay, ULONG VDisplay, 
                ULONG HSyncStart, ULONG HSyncEnd, ULONG HTotal,
                ULONG VSyncStart, ULONG VSyncEnd, ULONG VTotal)
{
    RADEONModeInfo info = {
        width, height, bpp, pixelc, base,
        HDisplay, VDisplay, HSyncStart, HSyncEnd, HTotal,
        VSyncStart, VSyncEnd, VTotal };
      
    RADEONInitCommonRegisters(sd, save, &info);

    if (sd->Card.IsSecondary)
    {
        RADEONInitCrtc2Registers(sd, save, &info);
        RADEONInitPLL2Registers(sd, save, &info);
    } 
    else
    {
        RADEONInitCrtcRegisters(sd, save, &info);
        RADEONInitPLLRegisters(sd, save, &info);        
    }
    RADEONInitFPRegisters(sd, save, &info);
    
    save->pixelc = info.pixelc;
    save->HDisplay = info.HDisplay;
}

void ShowHideCursor(struct ati_staticdata *sd, BOOL visible)
{
    D(bug("[ATI] ShowHideCursor: %s\n", visible ? "show":"hide"));

    if (visible)
    {
        if (sd->Card.IsSecondary)
            OUTREGP(RADEON_CRTC2_GEN_CNTL, RADEON_CRTC2_CUR_EN, ~RADEON_CRTC2_CUR_EN);
        else    
            OUTREGP(RADEON_CRTC_GEN_CNTL, RADEON_CRTC_CUR_EN, ~RADEON_CRTC_CUR_EN);
    }
    else
    {
        if (sd->Card.IsSecondary)
            OUTREGP(RADEON_CRTC2_GEN_CNTL, 0, ~RADEON_CRTC2_CUR_EN);
        else        
            OUTREGP(RADEON_CRTC_GEN_CNTL, 0, ~RADEON_CRTC_CUR_EN);
    }
}

void SaveState(struct ati_staticdata *sd, struct CardState *save)
{
    save->dp_datatype      = INREG(RADEON_DP_DATATYPE);
    save->rbbm_soft_reset  = INREG(RADEON_RBBM_SOFT_RESET);
    save->clock_cntl_index = INREG(RADEON_CLOCK_CNTL_INDEX);
    if (sd->Card.R300CGWorkaround) R300CGWorkaround(sd);

    // Common registers
    save->ovr_clr            = INREG(RADEON_OVR_CLR);
    save->ovr_wid_left_right = INREG(RADEON_OVR_WID_LEFT_RIGHT);
    save->ovr_wid_top_bottom = INREG(RADEON_OVR_WID_TOP_BOTTOM);
    save->ov0_scale_cntl     = INREG(RADEON_OV0_SCALE_CNTL);
    save->subpic_cntl        = INREG(RADEON_SUBPIC_CNTL);
    save->viph_control       = INREG(RADEON_VIPH_CONTROL);
    save->i2c_cntl_1         = INREG(RADEON_I2C_CNTL_1);
    save->gen_int_cntl       = INREG(RADEON_GEN_INT_CNTL);
    save->cap0_trig_cntl     = INREG(RADEON_CAP0_TRIG_CNTL);
    save->cap1_trig_cntl     = INREG(RADEON_CAP1_TRIG_CNTL);
    save->bus_cntl           = INREG(RADEON_BUS_CNTL);
    save->surface_cntl       = INREG(RADEON_SURFACE_CNTL);
    save->grph_buffer_cntl   = INREG(RADEON_GRPH_BUFFER_CNTL);
    save->grph2_buffer_cntl  = INREG(RADEON_GRPH2_BUFFER_CNTL);

    if (!sd->Card.IsSecondary)
    {
        // PLL
        save->ppll_ref_div = RADEONINPLL(sd, RADEON_PPLL_REF_DIV);
        save->ppll_div_3   = RADEONINPLL(sd, RADEON_PPLL_DIV_3);
        save->htotal_cntl  = RADEONINPLL(sd, RADEON_HTOTAL_CNTL);

        // CRTC    
        save->crtc_gen_cntl        = INREG(RADEON_CRTC_GEN_CNTL);
        save->crtc_ext_cntl        = INREG(RADEON_CRTC_EXT_CNTL);
        save->dac_cntl             = INREG(RADEON_DAC_CNTL);
        save->crtc_h_total_disp    = INREG(RADEON_CRTC_H_TOTAL_DISP);
        save->crtc_h_sync_strt_wid = INREG(RADEON_CRTC_H_SYNC_STRT_WID);
        save->crtc_v_total_disp    = INREG(RADEON_CRTC_V_TOTAL_DISP);
        save->crtc_v_sync_strt_wid = INREG(RADEON_CRTC_V_SYNC_STRT_WID);
        save->crtc_offset          = INREG(RADEON_CRTC_OFFSET);
        save->crtc_offset_cntl     = INREG(RADEON_CRTC_OFFSET_CNTL);
        save->crtc_pitch           = INREG(RADEON_CRTC_PITCH);
        save->disp_merge_cntl      = INREG(RADEON_DISP_MERGE_CNTL);
        save->crtc_more_cntl       = INREG(RADEON_CRTC_MORE_CNTL);
    
        if (sd->Card.IsDellServer) {
            save->tv_dac_cntl      = INREG(RADEON_TV_DAC_CNTL);
            save->dac2_cntl        = INREG(RADEON_DAC_CNTL2);
            save->disp_hw_debug    = INREG (RADEON_DISP_HW_DEBUG);
            save->crtc2_gen_cntl   = INREG(RADEON_CRTC2_GEN_CNTL);
        }
    
        // Flat panel
        save->fp_crtc_h_total_disp = INREG(RADEON_FP_CRTC_H_TOTAL_DISP);
        save->fp_crtc_v_total_disp = INREG(RADEON_FP_CRTC_V_TOTAL_DISP);
        save->fp_gen_cntl          = INREG(RADEON_FP_GEN_CNTL);
        save->fp_h_sync_strt_wid   = INREG(RADEON_FP_H_SYNC_STRT_WID);
        save->fp_horz_stretch      = INREG(RADEON_FP_HORZ_STRETCH);
        save->fp_v_sync_strt_wid   = INREG(RADEON_FP_V_SYNC_STRT_WID);
        save->fp_vert_stretch      = INREG(RADEON_FP_VERT_STRETCH);
        save->lvds_gen_cntl        = INREG(RADEON_LVDS_GEN_CNTL);
        save->lvds_pll_cntl        = INREG(RADEON_LVDS_PLL_CNTL);
        save->tmds_pll_cntl        = INREG(RADEON_TMDS_PLL_CNTL);
        save->tmds_transmitter_cntl= INREG(RADEON_TMDS_TRANSMITTER_CNTL);
        save->bios_4_scratch       = INREG(RADEON_BIOS_4_SCRATCH);
        save->bios_5_scratch       = INREG(RADEON_BIOS_5_SCRATCH);
        save->bios_6_scratch       = INREG(RADEON_BIOS_6_SCRATCH);
    
        if (sd->Card.Type == RV280) {
            /* bit 22 of TMDS_PLL_CNTL is read-back inverted */
            save->tmds_pll_cntl ^= (1 << 22);
        }
    }
    else
    {
        // CRTC2
        save->dac2_cntl             = INREG(RADEON_DAC_CNTL2);
        save->disp_output_cntl      = INREG(RADEON_DISP_OUTPUT_CNTL);
        save->disp_hw_debug         = INREG (RADEON_DISP_HW_DEBUG);
    
        save->crtc2_gen_cntl        = INREG(RADEON_CRTC2_GEN_CNTL);
        save->crtc2_h_total_disp    = INREG(RADEON_CRTC2_H_TOTAL_DISP);
        save->crtc2_h_sync_strt_wid = INREG(RADEON_CRTC2_H_SYNC_STRT_WID);
        save->crtc2_v_total_disp    = INREG(RADEON_CRTC2_V_TOTAL_DISP);
        save->crtc2_v_sync_strt_wid = INREG(RADEON_CRTC2_V_SYNC_STRT_WID);
        save->crtc2_offset          = INREG(RADEON_CRTC2_OFFSET);
        save->crtc2_offset_cntl     = INREG(RADEON_CRTC2_OFFSET_CNTL);
        save->crtc2_pitch           = INREG(RADEON_CRTC2_PITCH);
    
        save->fp2_h_sync_strt_wid   = INREG (RADEON_FP_H2_SYNC_STRT_WID);
        save->fp2_v_sync_strt_wid   = INREG (RADEON_FP_V2_SYNC_STRT_WID);
        save->fp2_gen_cntl          = INREG (RADEON_FP2_GEN_CNTL);
        save->disp2_merge_cntl      = INREG(RADEON_DISP2_MERGE_CNTL);
    
        // PLL2
        save->p2pll_ref_div = RADEONINPLL(sd, RADEON_P2PLL_REF_DIV);
        save->p2pll_div_0   = RADEONINPLL(sd, RADEON_P2PLL_DIV_0);
        save->htotal_cntl2  = RADEONINPLL(sd, RADEON_HTOTAL2_CNTL);
    }
}

void LoadState(struct ati_staticdata *sd, struct CardState *restore)
{
#if AROS_BIG_ENDIAN
    RADEONWaitForFifo(sd, 1);
    OUTREG(RADEON_RBBM_GUICNTL, RADEON_HOST_DATA_SWAP_NONE);
#endif

    RADEONBlank(sd);

    OUTREG(RADEON_CLOCK_CNTL_INDEX, restore->clock_cntl_index);
    if (sd->Card.R300CGWorkaround) R300CGWorkaround(sd);
    OUTREG(RADEON_RBBM_SOFT_RESET,  restore->rbbm_soft_reset);
    OUTREG(RADEON_DP_DATATYPE,      restore->dp_datatype);
    OUTREG(RADEON_GRPH_BUFFER_CNTL, restore->grph_buffer_cntl);
    OUTREG(RADEON_GRPH2_BUFFER_CNTL, restore->grph2_buffer_cntl);

    if (!sd->Card.HasCRTC2)
    {
        // Common
        OUTREG(RADEON_OVR_CLR,            restore->ovr_clr);
        OUTREG(RADEON_OVR_WID_LEFT_RIGHT, restore->ovr_wid_left_right);
        OUTREG(RADEON_OVR_WID_TOP_BOTTOM, restore->ovr_wid_top_bottom);
        OUTREG(RADEON_OV0_SCALE_CNTL,     restore->ov0_scale_cntl);
        OUTREG(RADEON_SUBPIC_CNTL,        restore->subpic_cntl);
        OUTREG(RADEON_VIPH_CONTROL,       restore->viph_control);
        OUTREG(RADEON_I2C_CNTL_1,         restore->i2c_cntl_1);
        OUTREG(RADEON_GEN_INT_CNTL,       restore->gen_int_cntl);
        OUTREG(RADEON_CAP0_TRIG_CNTL,     restore->cap0_trig_cntl);
        OUTREG(RADEON_CAP1_TRIG_CNTL,     restore->cap1_trig_cntl);
        OUTREG(RADEON_BUS_CNTL,           restore->bus_cntl);
        OUTREG(RADEON_SURFACE_CNTL,       restore->surface_cntl);

        // CRTC
        OUTREG(RADEON_CRTC_GEN_CNTL, restore->crtc_gen_cntl);
    
        OUTREGP(RADEON_CRTC_EXT_CNTL,
                restore->crtc_ext_cntl,
                RADEON_CRTC_VSYNC_DIS |
                RADEON_CRTC_HSYNC_DIS |
                RADEON_CRTC_DISPLAY_DIS);
    
        OUTREGP(RADEON_DAC_CNTL,
                restore->dac_cntl,
                RADEON_DAC_RANGE_CNTL |
                RADEON_DAC_BLANKING);
    
        OUTREG(RADEON_CRTC_H_TOTAL_DISP,    restore->crtc_h_total_disp);
        OUTREG(RADEON_CRTC_H_SYNC_STRT_WID, restore->crtc_h_sync_strt_wid);
        OUTREG(RADEON_CRTC_V_TOTAL_DISP,    restore->crtc_v_total_disp);
        OUTREG(RADEON_CRTC_V_SYNC_STRT_WID, restore->crtc_v_sync_strt_wid);
        OUTREG(RADEON_CRTC_OFFSET,          restore->crtc_offset);
        OUTREG(RADEON_CRTC_OFFSET_CNTL,     restore->crtc_offset_cntl);
        OUTREG(RADEON_CRTC_PITCH,           restore->crtc_pitch);
        OUTREG(RADEON_DISP_MERGE_CNTL,      restore->disp_merge_cntl);
        OUTREG(RADEON_CRTC_MORE_CNTL,       restore->crtc_more_cntl);
    
        if (sd->Card.IsDellServer) {
            OUTREG(RADEON_TV_DAC_CNTL, restore->tv_dac_cntl);
            OUTREG(RADEON_DISP_HW_DEBUG, restore->disp_hw_debug);
            OUTREG(RADEON_DAC_CNTL2, restore->dac2_cntl);
            OUTREG(RADEON_CRTC2_GEN_CNTL, restore->crtc2_gen_cntl);
        }

        // Flat Panel
        unsigned long  tmp;
    
        OUTREG(RADEON_FP_CRTC_H_TOTAL_DISP, restore->fp_crtc_h_total_disp);
        OUTREG(RADEON_FP_CRTC_V_TOTAL_DISP, restore->fp_crtc_v_total_disp);
        OUTREG(RADEON_FP_H_SYNC_STRT_WID,   restore->fp_h_sync_strt_wid);
        OUTREG(RADEON_FP_V_SYNC_STRT_WID,   restore->fp_v_sync_strt_wid);
        OUTREG(RADEON_TMDS_PLL_CNTL,        restore->tmds_pll_cntl);
        OUTREG(RADEON_TMDS_TRANSMITTER_CNTL,restore->tmds_transmitter_cntl);
        OUTREG(RADEON_FP_HORZ_STRETCH,      restore->fp_horz_stretch);
        OUTREG(RADEON_FP_VERT_STRETCH,      restore->fp_vert_stretch);
        OUTREG(RADEON_FP_GEN_CNTL,          restore->fp_gen_cntl);
        OUTREG(RADEON_GRPH_BUFFER_CNTL,
               INREG(RADEON_GRPH_BUFFER_CNTL) & ~0x7f0000);

        if (sd->Card.IsMobility) {
            OUTREG(RADEON_BIOS_4_SCRATCH, restore->bios_4_scratch);
            OUTREG(RADEON_BIOS_5_SCRATCH, restore->bios_5_scratch);
            OUTREG(RADEON_BIOS_6_SCRATCH, restore->bios_6_scratch);
        }

        if (sd->Card.MonType1 != MT_DFP) {
            unsigned long tmpPixclksCntl = RADEONINPLL(sd, RADEON_PIXCLKS_CNTL);
    
            if (sd->Card.IsMobility || sd->Card.IsIGP) {
                /* Asic bug, when turning off LVDS_ON, we have to make sure
                   RADEON_PIXCLK_LVDS_ALWAYS_ON bit is off
                */
                if (!(restore->lvds_gen_cntl & RADEON_LVDS_ON)) {
                    OUTPLLP(sd, RADEON_PIXCLKS_CNTL, 0, ~RADEON_PIXCLK_LVDS_ALWAYS_ONb);
                }
            }
    
            tmp = INREG(RADEON_LVDS_GEN_CNTL);
            if ((tmp & (RADEON_LVDS_ON | RADEON_LVDS_BLON)) ==
                (restore->lvds_gen_cntl & (RADEON_LVDS_ON | RADEON_LVDS_BLON))) {
                OUTREG(RADEON_LVDS_GEN_CNTL, restore->lvds_gen_cntl);
            } else {
                if (restore->lvds_gen_cntl & (RADEON_LVDS_ON | RADEON_LVDS_BLON)) {
                    usleep(sd->Card.PanelPwrDly * 1000);
                    OUTREG(RADEON_LVDS_GEN_CNTL, restore->lvds_gen_cntl);
                } else {
                    OUTREG(RADEON_LVDS_GEN_CNTL,
                           restore->lvds_gen_cntl | RADEON_LVDS_BLON);
                    usleep(sd->Card.PanelPwrDly * 1000);
                    OUTREG(RADEON_LVDS_GEN_CNTL, restore->lvds_gen_cntl);
                }
            }
    
            if (sd->Card.IsMobility || sd->Card.IsIGP) {
                if (!(restore->lvds_gen_cntl & RADEON_LVDS_ON)) {
                    OUTPLL(RADEON_PIXCLKS_CNTL, tmpPixclksCntl);
                }
            }
        }
        
        // PLL

        if (sd->Card.IsMobility) {
            /* A temporal workaround for the occational blanking on certain laptop panels.
               This appears to related to the PLL divider registers (fail to lock?).
               It occurs even when all dividers are the same with their old settings.
               In this case we really don't need to fiddle with PLL registers.
               By doing this we can avoid the blanking problem with some panels.
            */
            if ((restore->ppll_ref_div == (RADEONINPLL(sd, RADEON_PPLL_REF_DIV) & RADEON_PPLL_REF_DIV_MASK)) &&
                (restore->ppll_div_3 == (RADEONINPLL(sd, RADEON_PPLL_DIV_3) & (RADEON_PPLL_POST3_DIV_MASK | RADEON_PPLL_FB3_DIV_MASK))))
                return;
        }
        
        OUTPLLP(sd, RADEON_VCLK_ECP_CNTL,
                RADEON_VCLK_SRC_SEL_CPUCLK,
                ~(RADEON_VCLK_SRC_SEL_MASK));
    
        OUTPLLP(sd,
                RADEON_PPLL_CNTL,
                RADEON_PPLL_RESET
                | RADEON_PPLL_ATOMIC_UPDATE_EN
                | RADEON_PPLL_VGA_ATOMIC_UPDATE_EN,
                ~(RADEON_PPLL_RESET
                  | RADEON_PPLL_ATOMIC_UPDATE_EN
                  | RADEON_PPLL_VGA_ATOMIC_UPDATE_EN));
    
        OUTREGP(RADEON_CLOCK_CNTL_INDEX,
                RADEON_PLL_DIV_SEL,
                ~(RADEON_PLL_DIV_SEL));

        if (IS_R300_VARIANT ||
            (sd->Card.Type == RS300)) {
            if (restore->ppll_ref_div & R300_PPLL_REF_DIV_ACC_MASK) {
                /* When restoring console mode, use saved PPLL_REF_DIV
                 * setting.
                 */
                OUTPLLP(sd, RADEON_PPLL_REF_DIV,
                        restore->ppll_ref_div,
                        0);
            } else {
                /* R300 uses ref_div_acc field as real ref divider */
                OUTPLLP(sd, RADEON_PPLL_REF_DIV,
                        (restore->ppll_ref_div << R300_PPLL_REF_DIV_ACC_SHIFT),
                        ~R300_PPLL_REF_DIV_ACC_MASK);
            }
        } else {
            OUTPLLP(sd, RADEON_PPLL_REF_DIV,
                    restore->ppll_ref_div,
                    ~RADEON_PPLL_REF_DIV_MASK);
        }

        OUTPLLP(sd, RADEON_PPLL_DIV_3,
                restore->ppll_div_3,
                ~RADEON_PPLL_FB3_DIV_MASK);
    
        OUTPLLP(sd, RADEON_PPLL_DIV_3,
                restore->ppll_div_3,
                ~RADEON_PPLL_POST3_DIV_MASK);
    
        RADEONPLLWriteUpdate(sd);
        RADEONPLLWaitForReadUpdateComplete(sd);
    
        OUTPLL(RADEON_HTOTAL_CNTL, restore->htotal_cntl);
    
        OUTPLLP(sd, RADEON_PPLL_CNTL,
                0,
                ~(RADEON_PPLL_RESET
                  | RADEON_PPLL_SLEEP
                  | RADEON_PPLL_ATOMIC_UPDATE_EN
                  | RADEON_PPLL_VGA_ATOMIC_UPDATE_EN));

        usleep(50000); /* Let the clock to lock */
    
        OUTPLLP(sd, RADEON_VCLK_ECP_CNTL,
                RADEON_VCLK_SRC_SEL_PPLLCLK,
                ~(RADEON_VCLK_SRC_SEL_MASK));
    }
    else
    {
    }

    RADEONUnblank(sd);
    
    RADEONInitDispBandwidth(sd, restore);
}



void DPMS(struct ati_staticdata *sd, HIDDT_DPMSLevel level)
{
    int mask1     = (RADEON_CRTC_DISPLAY_DIS |
                     RADEON_CRTC_HSYNC_DIS |
                     RADEON_CRTC_VSYNC_DIS);
    int mask2     = (RADEON_CRTC2_DISP_DIS |
                     RADEON_CRTC2_VSYNC_DIS |
                     RADEON_CRTC2_HSYNC_DIS);

    switch (level)
    {
        case vHidd_Gfx_DPMSLevel_On:
            /* Screen: On; HSync: On, VSync: On */
            if (sd->Card.IsSecondary)
                OUTREGP(RADEON_CRTC2_GEN_CNTL, 0, ~mask2);
            else {
                OUTREGP(RADEON_CRTC_EXT_CNTL, 0, ~mask1);
            }
            break;
        
        case vHidd_Gfx_DPMSLevel_Standby:
             /* Screen: Off; HSync: Off, VSync: On */
            if (sd->Card.IsSecondary)
                OUTREGP(RADEON_CRTC2_GEN_CNTL,
                        RADEON_CRTC2_DISP_DIS | RADEON_CRTC2_HSYNC_DIS,
                        ~mask2);
            else {
                OUTREGP(RADEON_CRTC_EXT_CNTL,
                        RADEON_CRTC_DISPLAY_DIS | RADEON_CRTC_HSYNC_DIS,
                        ~mask1);
            }
            break;
        
        case vHidd_Gfx_DPMSLevel_Suspend:
            /* Screen: Off; HSync: On, VSync: Off */
            if (sd->Card.IsSecondary)
                OUTREGP(RADEON_CRTC2_GEN_CNTL,
                        RADEON_CRTC2_DISP_DIS | RADEON_CRTC2_VSYNC_DIS,
                        ~mask2);
            else {
                OUTREGP(RADEON_CRTC_EXT_CNTL,
                        RADEON_CRTC_DISPLAY_DIS | RADEON_CRTC_VSYNC_DIS,
                        ~mask1);
            }
            break;
       
        case vHidd_Gfx_DPMSLevel_Off:
            /* Screen: Off; HSync: Off, VSync: Off */
            if (sd->Card.IsSecondary)
                OUTREGP(RADEON_CRTC2_GEN_CNTL, mask2, ~mask2);
            else {
                OUTREGP(RADEON_CRTC_EXT_CNTL, mask1, ~mask1);
            }
            break;
    }
    if (level == vHidd_Gfx_DPMSLevel_On)
    {
        if (sd->Card.IsSecondary) {
            if (sd->Card.MonType2 == MT_DFP) {
                OUTREGP (RADEON_FP2_GEN_CNTL, 0, ~RADEON_FP2_BLANK_EN);
                OUTREGP (RADEON_FP2_GEN_CNTL, RADEON_FP2_ON, ~RADEON_FP2_ON);
                if (sd->Card.Type >= R200) {
                    OUTREGP (RADEON_FP2_GEN_CNTL, RADEON_FP2_DVO_EN, ~RADEON_FP2_DVO_EN);
                }
            } else if (sd->Card.MonType2 == MT_CRT) {
                RADEONDacPowerSet(sd, TRUE, !sd->Card.ReversedDAC);
            }
        } else {
            if (sd->Card.MonType1 == MT_DFP) {
                OUTREGP (RADEON_FP_GEN_CNTL, (RADEON_FP_FPON | RADEON_FP_TMDS_EN),
                         ~(RADEON_FP_FPON | RADEON_FP_TMDS_EN));
            } else if (sd->Card.MonType1 == MT_LCD) {

                OUTREGP (RADEON_LVDS_GEN_CNTL, RADEON_LVDS_BLON, ~RADEON_LVDS_BLON);
                usleep (sd->Card.PanelPwrDly * 1000);
                OUTREGP (RADEON_LVDS_GEN_CNTL, RADEON_LVDS_ON, ~RADEON_LVDS_ON);
            } else if (sd->Card.MonType1 == MT_CRT) {
                if (sd->Card.HasSecondary) {
                    RADEONDacPowerSet(sd, TRUE, sd->Card.ReversedDAC);
                } else {
                    RADEONDacPowerSet(sd, TRUE, TRUE);
                    if (sd->Card.HasCRTC2)
                        RADEONDacPowerSet(sd, TRUE, FALSE);
                }
            }
        }
    }   
    else if ((level == vHidd_Gfx_DPMSLevel_Off) ||
             (level == vHidd_Gfx_DPMSLevel_Suspend) ||
             (level == vHidd_Gfx_DPMSLevel_Standby))
    {
        if (sd->Card.IsSecondary) {
            if (sd->Card.MonType2 == MT_DFP) {
                OUTREGP (RADEON_FP2_GEN_CNTL, RADEON_FP2_BLANK_EN, ~RADEON_FP2_BLANK_EN);
                OUTREGP (RADEON_FP2_GEN_CNTL, 0, ~RADEON_FP2_ON);
                if (sd->Card.Type >= R200) {
                    OUTREGP (RADEON_FP2_GEN_CNTL, 0, ~RADEON_FP2_DVO_EN);
                }
            } else if (sd->Card.Type == MT_CRT) {
                RADEONDacPowerSet(sd, FALSE, !sd->Card.ReversedDAC);
            }
        } else {
            if (sd->Card.MonType1 == MT_DFP) {
                OUTREGP (RADEON_FP_GEN_CNTL, 0, ~(RADEON_FP_FPON | RADEON_FP_TMDS_EN));
            } else if (sd->Card.MonType1 == MT_LCD) {
                unsigned long tmpPixclksCntl = RADEONINPLL(sd, RADEON_PIXCLKS_CNTL);

                if (sd->Card.IsMobility || sd->Card.IsIGP) {
                    /* Asic bug, when turning off LVDS_ON, we have to make sure
                       RADEON_PIXCLK_LVDS_ALWAYS_ON bit is off
                    */
                    OUTPLLP(sd, RADEON_PIXCLKS_CNTL, 0, ~RADEON_PIXCLK_LVDS_ALWAYS_ONb);
                }

                OUTREGP (RADEON_LVDS_GEN_CNTL, 0,
                         ~(RADEON_LVDS_BLON | RADEON_LVDS_ON));

                if (sd->Card.IsMobility || sd->Card.IsIGP) {
                    OUTPLL(RADEON_PIXCLKS_CNTL, tmpPixclksCntl);
                }
            } else if (sd->Card.MonType1 == MT_CRT) {
                if (sd->Card.HasSecondary) {
                    RADEONDacPowerSet(sd, FALSE, sd->Card.ReversedDAC);
                } else {
                    /* single CRT, turning both DACs off, we don't really know 
                     * which DAC is actually connected.
                     */
                    RADEONDacPowerSet(sd, FALSE, TRUE);
                    if (sd->Card.HasCRTC2) /* don't apply this to old radeon (singel CRTC) card */
                        RADEONDacPowerSet(sd, FALSE, FALSE);
                }
            }
        }
    }
}

BOOL RADEONInit(struct ati_staticdata *sd)
{
    APTR int10_save = NULL;
    
    sd->Card.IsSecondary = FALSE;
    
    RADEONPreInt10Save(sd, &int10_save);
    RADEONPostInt10Check(sd, int10_save);

D(bug("[ATI] Radeon init\n"));

    sd->Card.HasCRTC2 = TRUE;
    sd->Card.IsMobility = FALSE;
    sd->Card.IsIGP = FALSE;
    sd->Card.IsDellServer = FALSE;
    sd->Card.HasSingleDAC = FALSE;

    D(bug("[ATI] flags:"));
    
    switch (sd->Card.ProductID)
    {
        case 0x4c59:
        case 0x4c5a:
        case 0x4c57:
        case 0x4c58:
        case 0x4c64:
        case 0x4c66:
        case 0x4c67:
        case 0x5c61:
        case 0x5c63:
        case 0x4e50:
        case 0x4e51:
        case 0x4e52:
        case 0x4e53:
        case 0x4e54:
        case 0x4e56:
        case 0x3150:
        case 0x3154:
        case 0x5460:
        case 0x5464:
        case 0x4a4e:
            sd->Card.IsMobility = TRUE;
            D(bug(" IsMobility\n"));
            break;
        
        case 0x4336:
        case 0x4337:
            sd->Card.IsMobility = TRUE;
            D(bug(" IsMobility\n"));
        case 0x4136:
        case 0x4237:
            sd->Card.IsIGP = TRUE;
            D(bug(" IsIGP\n"));
            break;
        
        case 0x5835:
        case 0x7835:
            sd->Card.IsMobility = TRUE;
            D(bug(" IsMobility\n"));
        case 0x5834:
        case 0x7834:
            sd->Card.IsIGP = TRUE;
            sd->Card.HasSingleDAC = TRUE;
            D(bug(" IsIGP HasSingleDAC\n"));
            break;
        
        default:
            sd->Card.HasCRTC2 = FALSE;
    }
    D(bug("\n"));

    if ((sd->Card.Type == RS100) ||
        (sd->Card.Type == RS200) ||
        (sd->Card.Type == RS300)) {
        ULONG tom = INREG(RADEON_NB_TOM);

        sd->Card.FbUsableSize = (((tom >> 16) -
                            (tom & 0xffff) + 1) << 6) * 1024;

        OUTREG(RADEON_CONFIG_MEMSIZE, sd->Card.FbUsableSize);
    } else {
        /* There are different HDP mapping schemes depending on single/multi funciton setting,
         * chip family, HDP mode, and the generation of HDP mapping scheme.
         * To make things simple, we only allow maximum 128M addressable FB. Anything more than
         * 128M is configured as invisible FB to CPU that can only be accessed from chip side.
         */
        sd->Card.FbUsableSize = INREG(RADEON_CONFIG_MEMSIZE);
        if (sd->Card.FbUsableSize > 128*1024*1024) sd->Card.FbUsableSize = 128*1024*1024;
        if ((sd->Card.Type == RV350) ||
            (sd->Card.Type == RV380) ||
            (sd->Card.Type == R420)) {
            OUTREGP (RADEON_HOST_PATH_CNTL, (1<<23), ~(1<<23));
        }
    }

    /* Some production boards of m6 will return 0 if it's 8 MB */
    if (sd->Card.FbUsableSize == 0) sd->Card.FbUsableSize = 8192*1024;
    
#if 0
    if (sd->Card.IsSecondary) {
        /* FIXME: For now, split FB into two equal sections. This should
         * be able to be adjusted by user with a config option. */
        RADEONEntPtr pRADEONEnt = RADEONEntPriv(pScrn);
        RADEONInfoPtr  info1;
        
        pScrn->videoRam /= 2;
        pRADEONEnt->pPrimaryScrn->videoRam = pScrn->videoRam;
        
        info1 = RADEONPTR(pRADEONEnt->pPrimaryScrn);
        info1->FbMapSize  = pScrn->videoRam * 1024;
        info->LinearAddr += pScrn->videoRam * 1024;
        info1->MergedFB = FALSE;
    }
#endif

    sd->Card.R300CGWorkaround = (sd->Card.Type == R300 &&
         (INREG(RADEON_CONFIG_CNTL) & RADEON_CFG_ATI_REV_ID_MASK)
         == RADEON_CFG_ATI_REV_A11);    
         
    D(bug("[ATI] R300CGWorkaroung = %s\n", sd->Card.R300CGWorkaround? "Yes":"No"));
    
    sd->Card.MemCntl            = INREG(RADEON_SDRAM_MODE_REG);
    sd->Card.BusCntl            = INREG(RADEON_BUS_CNTL);

    sd->Card.DDCReg             = RADEON_GPIO_DVI_DDC;

    RADEONGetVRamType(sd);

    D(bug("[ATI] Video memory = %dMiB (%d bit %s SDRAM)\n", sd->Card.FbUsableSize >> 20,
        sd->Card.RamWidth, sd->Card.IsDDR ? "DDR":"SDR"));

    /* RADEONPreInitDDC */
    sd->Card.DDC1 = FALSE;
    sd->Card.DDC2 = FALSE;
    sd->Card.DDCBios = FALSE;

    RADEONGetBIOSInfo(sd);
    RADEONQueryConnectedMonitors(sd);
    RADEONGetClockInfo(sd);
    RADEONGetPanelInfo(sd);

    if (sd->Card.MonType1 == MT_UNKNOWN)
        sd->Card.MonType1 = MT_CRT;

    return TRUE;
}

/*
    Allocates some memory area on GFX card, which may be sufficient for bitmap
    with given size and depth. The must_have bit may be defined but doesn't
    have to. If it is TRUE, the allocator will do everything to get the memory -
    eg. it will throw other bitmaps away from it or it will shift them within
    GFX memory
*/

IPTR AllocBitmapArea(struct ati_staticdata *sd, ULONG width, ULONG height,
    ULONG bpp, BOOL must_have)
{
    IPTR result;
    ULONG size = (((width * bpp + 255) & ~255) * height + 1023) & ~1023;

    LOCK_HW
    
    Forbid();
    result = (IPTR)Allocate(&sd->CardMem, size);
    Permit();

    D(bug("[ATI] AllocBitmapArea(%dx%d@%d) = %p\n", width, height, bpp, result));
    
    /*
        If Allocate failed, make the 0xffffffff as return. If it succeeded, make
        the memory pointer relative to the begin of GFX memory
    */
    if (result == 0) --result;
    else result -= (IPTR)sd->Card.FrameBuffer;

    UNLOCK_HW

    /* Generic thing. Will be extended later */
    return result;
}

VOID FreeBitmapArea(struct ati_staticdata *sd, IPTR bmp, ULONG width, ULONG height, ULONG bpp)
{
    APTR ptr = (APTR)(bmp + sd->Card.FrameBuffer);
    ULONG size = (((width * bpp + 255) & ~255) * height + 1023) & ~1023;

    LOCK_HW

    D(bug("[ATI] FreeBitmapArea(%p,%dx%d@%d)\n",
    bmp, width, height, bpp));

    Forbid();
    Deallocate(&sd->CardMem, ptr, size);
    Permit();
    
    UNLOCK_HW
}
