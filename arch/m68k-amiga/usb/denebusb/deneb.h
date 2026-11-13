#ifndef DENEB_H
#define DENEB_H

/*
 *----------------------------------------------------------------------------
 *             Includes for Deneb USB Controller
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <hodges@in.tum.de>
 *
 * History
 *
 *  18-11-2005 - Initial
 *
 *
 */

#include <exec/types.h>

/*
 * --------------------- MACH registers ------------------------
 * Warning: These are LONG offsets!
 */

#ifndef ZORRO_II
#define MACH_USBREGS        (0x100000>>2)
#define MACH_CLOCKPORT      (0x200000>>2)

#define MACH_DMAMEMORY      (0x000000>>2)
#define MACH_DMAADDRESS     (0x010000>>2)
#define MACH_DMACTRL        (0x020000>>2)
#define MACH_LCDATA         (0x080000>>2) /* logic catch data for debug */
#define MACH_CTRL           (0x0e0000>>2)
#define MACH_DMAINT         (0x0c0000>>2)
#define MACH_CLKPORTCTRL    (0x0d0000>>2) /* clock port control register */

#else
#define MACH_CLOCKPORT_Z2   (0x0200>>1) // WORD offset
#define MACH_FASTZORROMAGIC (0x02fa>>1) // WORD offset
#define MACH_CLKPORTCTRL_Z2 (0x02fc>>1) // WORD offset
#define MACH_CTRL_Z2        (0x02fe>>1) // WORD offset
#endif

/* MACH_CTRL bits */
#define MCB_INT2             0    /* Use INT2 instead of INT6 */
#define MCB_INTEN            1    /* Enable interrupts */
#define MCB_RESET            2    /* Reset HW */
#define MCB_DMAINTEN         3    /* Enable DMA interrupt */

#define MCF_INT2            (1UL<<MCB_INT2)
#define MCF_INTEN           (1UL<<MCB_INTEN)
#define MCF_RESET           (1UL<<MCB_RESET)
#define MCF_DMAINTEN        (1UL<<MCB_DMAINTEN)

/* MACH_CLKPORTCTRL bits */

#ifndef ZORRO_II

#define MCPCS_WRITEPULSE      0    /* Write Pulse length */
#define MCPCS_READPULSE       9    /* Read Pulse length */
#define MCPCS_CSDELAY        18    /* Chip Select delay */
#define MCPCS_ADRSETUP       23    /* Address Setup time */
#define MCPCB_INTEN          28    /* Enable interrupts */
#define MCPCB_INT2           29    /* Use INT2 instead of INT6 */
#define MCPCB_STATUS         31    /* Clockport status bit from hardware */

#define MCPCM_WRITEPULSE     (((1UL<<9)-1)<<MCPCS_WRITEPULSE)
#define MCPCM_READPULSE      (((1UL<<9)-1)<<MCPCS_READPULSE)
#define MCPCM_CSDELAY        (((1UL<<5)-1)<<MCPCS_CSDELAY)
#define MCPCM_ADRSETUP       (((1UL<<5)-1)<<MCPCS_ADRSETUP)
#define MCPCF_INTEN          (1UL<<MCPCB_INTEN)
#define MCPCF_INT2           (1UL<<MCPCB_INT2)

#else

#define MCPCB_FAST_Z2        15    /* Enable Fast Zorro II */
#define MCPCB_RESET           2    /* Clockport reset */
#define MCPCB_INTEN           1    /* Enable interrupts */
#define MCPCB_INT2            0    /* Use INT2 instead of INT6 */

#define MCPCF_FAST_Z2        (1UL<<MCPCB_FAST_Z2)
#define MCPCF_RESET          (1UL<<MCPCB_RESET)
#define MCPCF_INTEN          (1UL<<MCPCB_INTEN)
#define MCPCF_INT2           (1UL<<MCPCB_INT2)

#endif

#ifndef ZORRO_II

/* MACH_DMAMEMORY bits */
#define MMS_ISPADDRESS       2    /* Address in ISP (longword offsets) */
#define MMB_DMADONE         16    /* DMA done bit */
#define MMB_DAMINT          17    /* DMA interrupt */
#define MMS_LENGTHBYTES     16    /* Length of DMA transfers in bytes */
#define MMS_LENGTH          18    /* Length of DMA transfers in longwords */

#define MMM_ISPADDRESS      (((1UL<<14)-1)<<MMS_ISPADDRESS)
#define MMM_LENGTH          (((1UL<<14)-1)<<MMS_LENGTH)
#define MMF_DMADONE         (1UL<<MMB_DMADONE)
#define MMF_DAMINT          (1UL<<MMB_DAMINT)

/* MACH_DMAADDRESS bits */
#define MAB_ENABLE           0    /* Enable DMA Transfer */
#define MAB_READFROMRAM      1    /* 1=Copy from Zorro Memory into ISP, 0=Copy from ISP into Zorro Memory */
#define MAS_ADDRESS          2    /* Address on Zorro Bus */

#define MAF_ENABLE          (1UL<<MAB_ENABLE)
#define MAF_READFROMRAM     (1UL<<MAB_READFROMRAM)
#define MAM_ADDRESS         (((1UL<<30)-1)<<MAS_ADDRESS)

/* MACH_DMAINT bits */
#define MIB_DMAINT           0    /* DMA finished (r/wc) */
#define MIB_ISPINT           1    /* ISP1760 interrupt status (r/wc) */

#define MIF_DMAINT          (1UL<<MIB_DMAINT)
#define MIF_ISPINT          (1UL<<MIB_ISPINT)

/* MACH_DMACTRL bits */
#define MXB_BUSTER11         0    /* Buster 11 fix */
#define MXS_QUANTUM         16    /* number of data word per DMA block, 0 => 1, ...., 255 => 256 */
#define MXS_TICKS           24    /* number of ticks (4xE7M) between unregister und register pulses (/BRn) */

#define MXF_BUSTER11        (1UL<<MXB_BUSTER11)
#define MXM_QUANTUM         (((1UL<<8)-1)<<MXS_QUANTUM)
#define MXM_TICKS           (((1UL<<6)-1)<<MXS_TICKS)

/* MACH_LCCTRL bits */
#define MLS_ADDRESSTRIGGER  20
#define MLB_READTRIGGER     17
#define MLB_ARM             16
#define MLB_DATAREADY       15
#define MLB_RECORDING       14
#define MLB_ARMED           13
#define MLS_STARTFRAME       0

#define MLM_ADDRESSTRIGGER  (((1UL<<12)-1)<<MLS_ADDRESSTRIGGER)
#define MLF_READTRIGGER     (1UL<<MLB_READTRIGGER)
#define MLF_ARM             (1UL<<MLB_ARM)
#define MLF_DATAREADY       (1UL<<MLB_DATAREADY)
#define MLF_RECORDING       (1UL<<MLB_RECORDING)
#define MLF_ARMED           (1UL<<MLB_ARMED)
#define MLM_STARTFRAME      (((1UL<<8)-1)<<MLS_STARTFRAME)

#endif

#endif /* DENEB_H */
