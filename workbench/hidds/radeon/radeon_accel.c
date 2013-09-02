/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "ati.h"
#include "radeon.h"
#include "radeon_reg.h"
#include "radeon_bios.h"
#include "radeon_accel.h"
#include "radeon_macros.h"


#define DEBUG 1
#include <aros/debug.h>

struct __ROP RADEON_ROP[] = {
    { RADEON_ROP3_ZERO, RADEON_ROP3_ZERO }, /* GXclear        */
    { RADEON_ROP3_DSa,  RADEON_ROP3_DPa  }, /* Gxand          */
    { RADEON_ROP3_SDna, RADEON_ROP3_PDna }, /* GXandReverse   */
    { RADEON_ROP3_S,    RADEON_ROP3_P    }, /* GXcopy         */
    { RADEON_ROP3_DSna, RADEON_ROP3_DPna }, /* GXandInverted  */
    { RADEON_ROP3_D,    RADEON_ROP3_D    }, /* GXnoop         */
    { RADEON_ROP3_DSx,  RADEON_ROP3_DPx  }, /* GXxor          */
    { RADEON_ROP3_DSo,  RADEON_ROP3_DPo  }, /* GXor           */
    { RADEON_ROP3_DSon, RADEON_ROP3_DPon }, /* GXnor          */
    { RADEON_ROP3_DSxn, RADEON_ROP3_PDxn }, /* GXequiv        */
    { RADEON_ROP3_Dn,   RADEON_ROP3_Dn   }, /* GXinvert       */
    { RADEON_ROP3_SDno, RADEON_ROP3_PDno }, /* GXorReverse    */
    { RADEON_ROP3_Sn,   RADEON_ROP3_Pn   }, /* GXcopyInverted */
    { RADEON_ROP3_DSno, RADEON_ROP3_DPno }, /* GXorInverted   */
    { RADEON_ROP3_DSan, RADEON_ROP3_DPan }, /* GXnand         */
    { RADEON_ROP3_ONE,  RADEON_ROP3_ONE  }  /* GXset          */
};

/* The FIFO has 64 slots.  This routines waits until at least `entries'
 * of these slots are empty.
 */
void RADEONWaitForFifoFunction(struct ati_staticdata *sd, int entries)
{
    int i;
    
    for (;;) {
        for (i = 0; i < RADEON_TIMEOUT; i++) {
            sd->Card.FIFOSlots =
                INREG(RADEON_RBBM_STATUS) & RADEON_RBBM_FIFOCNT_MASK;
            if (sd->Card.FIFOSlots == 64) sd->Card.Busy = FALSE;
            if (sd->Card.FIFOSlots >= entries) return;
        }
        D(bug("[ATI] FIFO timed out: %d entries, stat=0x%08x\n",
                     INREG(RADEON_RBBM_STATUS) & RADEON_RBBM_FIFOCNT_MASK,
                     INREG(RADEON_RBBM_STATUS)));
        RADEONEngineReset(sd);
        RADEONEngineRestore(sd);
    }
}

/* The FIFO has 64 slots.  This routines waits until at least `entries'
 * of these slots are empty.
 */
void RADEONWaitForIdleMMIO(struct ati_staticdata *sd)
{
    int i;
    
    /* Wait for the engine to go idle */
    RADEONWaitForFifoFunction(sd, 64);
    
    for (;;) {
        for (i = 0; i < RADEON_TIMEOUT; i++) {
            if (!(INREG(RADEON_RBBM_STATUS) & RADEON_RBBM_ACTIVE)) {
                RADEONEngineFlush(sd);
                sd->Card.Busy = FALSE;
                return;
            }
        }
        D(bug("[ATI] Idle timed out: %d entries, stat=0x%08x\n",
                     INREG(RADEON_RBBM_STATUS) & RADEON_RBBM_FIFOCNT_MASK,
                     INREG(RADEON_RBBM_STATUS)));
        RADEONEngineReset(sd);
        RADEONEngineRestore(sd);
    }
}

/* Flush all dirty data in the Pixel Cache to memory */
void RADEONEngineFlush(struct ati_staticdata *sd)
{
    int            i;

    OUTREGP(RADEON_RB2D_DSTCACHE_CTLSTAT,
            RADEON_RB2D_DC_FLUSH_ALL,
            ~RADEON_RB2D_DC_FLUSH_ALL);
    for (i = 0; i < RADEON_TIMEOUT; i++) {
        if (!(INREG(RADEON_RB2D_DSTCACHE_CTLSTAT) & RADEON_RB2D_DC_BUSY))
            break;
    }
}

/* Reset graphics card to known state */
void RADEONEngineReset(struct ati_staticdata *sd)
{
    ULONG   clock_cntl_index;
    ULONG   mclk_cntl;
    ULONG   rbbm_soft_reset;
    ULONG   host_path_cntl;

    RADEONEngineFlush(sd);

    clock_cntl_index = INREG(RADEON_CLOCK_CNTL_INDEX);
    if (sd->Card.R300CGWorkaround) R300CGWorkaround(sd);
    
    mclk_cntl = RADEONINPLL(sd, RADEON_MCLK_CNTL);

    host_path_cntl = INREG(RADEON_HOST_PATH_CNTL);
    rbbm_soft_reset = INREG(RADEON_RBBM_SOFT_RESET);

    if (IS_R300_VARIANT) {
        ULONG tmp;

        OUTREG(RADEON_RBBM_SOFT_RESET, (rbbm_soft_reset |
                                        RADEON_SOFT_RESET_CP |
                                        RADEON_SOFT_RESET_HI |
                                        RADEON_SOFT_RESET_E2));
        INREG(RADEON_RBBM_SOFT_RESET);
        OUTREG(RADEON_RBBM_SOFT_RESET, 0);
        tmp = INREG(RADEON_RB2D_DSTCACHE_MODE);
        OUTREG(RADEON_RB2D_DSTCACHE_MODE, tmp | (1 << 17)); /* FIXME */
    } else {
        OUTREG(RADEON_RBBM_SOFT_RESET, (rbbm_soft_reset |
                                        RADEON_SOFT_RESET_CP |
                                        RADEON_SOFT_RESET_SE |
                                        RADEON_SOFT_RESET_RE |
                                        RADEON_SOFT_RESET_PP |
                                        RADEON_SOFT_RESET_E2 |
                                        RADEON_SOFT_RESET_RB));
        INREG(RADEON_RBBM_SOFT_RESET);
        OUTREG(RADEON_RBBM_SOFT_RESET, (rbbm_soft_reset & (ULONG)
                                        ~(RADEON_SOFT_RESET_CP |
                                          RADEON_SOFT_RESET_SE |
                                          RADEON_SOFT_RESET_RE |
                                          RADEON_SOFT_RESET_PP |
                                          RADEON_SOFT_RESET_E2 |
                                          RADEON_SOFT_RESET_RB)));
        INREG(RADEON_RBBM_SOFT_RESET);
    }

    OUTREG(RADEON_HOST_PATH_CNTL, host_path_cntl | RADEON_HDP_SOFT_RESET);
    INREG(RADEON_HOST_PATH_CNTL);
    OUTREG(RADEON_HOST_PATH_CNTL, host_path_cntl);

    if (IS_R300_VARIANT)
        OUTREG(RADEON_RBBM_SOFT_RESET, rbbm_soft_reset);

    OUTREG(RADEON_CLOCK_CNTL_INDEX, clock_cntl_index);
    OUTPLL(RADEON_MCLK_CNTL, mclk_cntl);
    if (sd->Card.R300CGWorkaround) R300CGWorkaround(sd);
    
    OUTREG(RADEON_RB3D_CNTL, 0);
}

/* Restore the acceleration hardware to its previous state */
void RADEONEngineRestore(struct ati_staticdata *sd)
{
    RADEONWaitForFifo(sd, 1);

    /* NOTE: The following RB2D_DSTCACHE_MODE setting will cause the
     * R300 to hang.  ATI does not see a reason to change it from the
     * default BIOS settings (even on non-R300 cards).  This setting
     * might be removed in future versions of the Radeon driver.
     */

    /* Turn of all automatic flushing - we'll do it all */
    if (!IS_R300_VARIANT)
        OUTREG(RADEON_RB2D_DSTCACHE_MODE, 0);

    RADEONWaitForFifo(sd, 1);
#if AROS_BIG_ENDIAN
    OUTREGP(RADEON_DP_DATATYPE,
            RADEON_HOST_BIG_ENDIAN_EN,
            ~RADEON_HOST_BIG_ENDIAN_EN);
#else
    OUTREGP(RADEON_DP_DATATYPE, 0, ~RADEON_HOST_BIG_ENDIAN_EN);
#endif

    RADEONWaitForFifo(sd, 1);
    OUTREG(RADEON_DEFAULT_SC_BOTTOM_RIGHT, (RADEON_DEFAULT_SC_RIGHT_MAX
                                            | RADEON_DEFAULT_SC_BOTTOM_MAX));

    RADEONWaitForFifo(sd, 7);
    OUTREG(RADEON_DST_LINE_START,    0);
    OUTREG(RADEON_DST_LINE_END,      0);
    OUTREG(RADEON_DP_BRUSH_FRGD_CLR, 0xffffffff);
    OUTREG(RADEON_DP_BRUSH_BKGD_CLR, 0x00000000);
    OUTREG(RADEON_DP_SRC_FRGD_CLR,   0xffffffff);
    OUTREG(RADEON_DP_SRC_BKGD_CLR,   0x00000000);
    OUTREG(RADEON_DP_WRITE_MASK,     0xffffffff);

    RADEONWaitForIdleMMIO(sd);
}

