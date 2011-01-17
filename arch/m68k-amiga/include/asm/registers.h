/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef REGISTERS_H
#define REGISTERS_H

#ifdef __PILOT_CODE__
#define WREG_L(addr)	*(Long *)(addr)
#define RREG_L(addr)	*(Long *)(addr)
#define WREG_W(addr)	*(Word *)(addr)
#define RREG_W(addr)	*(Word *)(addr)
#define WREG_B(addr)	*(Byte *)(addr)
#define RREG_B(addr)	*(Byte *)(addr)
#else
#include <exec/types.h>
#define WREG_L(addr)	*(ULONG *)(addr)
#define RREG_L(addr)	*(ULONG *)(addr)
#define WREG_W(addr)	*(UWORD *)(addr)
#define RREG_W(addr)	*(UWORD *)(addr)
#define WREG_B(addr)	*(UBYTE *)(addr)
#define RREG_B(addr)	*(UBYTE *)(addr)
#endif


/*
 * System Controler Registers
 */

#define SCR             0xfffff000
#define PCR             0xfffff003
#define IDR             0xfffff004
#define IODCR           0xfffff008

/*
 * Some Flags for the System Control registers
 */
#define BETO_F          (1<<7)
#define WPV_F           (1<<6)
#define PRV_F           (1<<5)
#define BETEN_F         (1<<4)
#define SO_F            (1<<3)
#define DMAP_F          (1<<2)
#define WDTH_F          (1<<0)


/*
 * Whatever is in chapter 6 in the downloadable 
 * dragonball documentation seems to be wrong.
 * Found the correct register description in the 
 * xcopilot source.
 */
#if 0

#define CSGBA		0xfffff100
#define CSGBB		0xfffff102
#define CSGBC		0xfffff104
#define CSGBD		0xfffff106
#define CSUGBA		0xfffff108
#define CSCR		0xfffff10A
#define CSA		0xfffff110
#define CSB		0xfffff112
#define CSC		0xfffff114
#define CSD		0xfffff116

#else

#define GRPBASEA	0xfffff100
#define GRPBASEB	0xfffff102
#define GRPBASEC	0xfffff104
#define GRPBASED	0xfffff100

#define GRPMASKA	0xfffff108
#define GRPMASKB	0xfffff10A
#define GRPMASKC	0xfffff10C
#define GRPMASKD	0xfffff10E

#define CSA0		0xfffff110
#define CSA1		0xfffff114
#define CSA2		0xfffff118
#define CSA3		0xfffff11c

#define CSC0		0xfffff130
#define CSC1		0xfffff134
#define CSC2		0xfffff138
#define CSC3		0xfffff13c

/*
 * CSA0-3, CSC0-3:
 *
 * Bit 0-2  : Wait states
 * Bit 3    : Read only (if '1')
 * Bit 4-7  : reserved
 * Bit 8-15 : AM (???)
 * Bit 16   : bus width
 * Bit 17-23: reserved
 * Bit 24-31: AC (???)
 */
#endif

#define PCTLR		0xfffff207

/*
 * Interrupt Controller Registers
 */
#define IVR		0xfffff300
#define ICR             0xfffff302
#define IMR             0xfffff304
#define IWR             0xfffff308
#define ISR             0xfffff30c
#define IPR             0xfffff310
#define ILCR            0xfffff314

/*
 * Port A:
 */
/*
 * Port B:
 */
/*
 * Port C:
 */
/*
 * Port D:
 */
#define PDDIR           0xfffff418
#define PDDATA          0xfffff419
#define PDPUEN          0xfffff41a
#define PDSEL           0xfffff41b
#define PDPOL           0xfffff41c
#define PDIRQEN         0xfffff41d
#define PDKBEN          0xfffff41e
#define PDIRQEG         0xfffff41f
/*
 * Port E:
 */
#define PEDIR           0xfffff420
#define PEDATA          0xfffff421
#define PEPUN           0xfffff422
#define PESEL           0xfffff423

/*
 * Port F: Turn LCD display on/off, ...
 */
#define PFDIR           0xfffff428
#define PFDATA          0xfffff429
#define PFPUEN          0xfffff42a
#define PFSEL           0xfffff42b

/*
 * General Purpose Timers Registers
 */
#define TCTL1           0xfffff600
#define TPRER1          0xfffff602
#define TCMP1           0xfffff604
#define TCR1            0xfffff606
#define TCN1            0xfffff608
#define TSTAT1          0xfffff60a

#define TCTL2           0xfffff60c
#define TPRER2          0xfffff60e
#define TCMP2           0xfffff610
#define TCR2            0xfffff612
#define TCN2            0xfffff614
#define TSTAT2          0xfffff616


#define FRR_F           (1<<8)
#define CAP_M           (1<<7|1<<6)
#define OM_F            (1<<5)
#define IRQEN           (1<<4)
#define CLKSOURCE_M     (1<<3|1<<2|1<<1)
#define TEN_F           (1<<0)

#define CAPT_F          (1<<1)
#define COMP_F          (1<<0)


/*
 * Serial Peripheral Interface 1 and 2
 */
#define SPIRXD          0xfffff700
#define SPITXD          0xfffff702
#define SPICONT1        0xfffff704
#define SPIINTCS        0xfffff706
#define SPITEST         0xfffff708
#define SPISPC          0xfffff70a

#define SPIDATA2        0xfffff800
#define SPICONT2        0xfffff802

/*
 * Some flags for SPI 2
 */
#define SPI_ENABLE_F    (1<<9)
#define SPI_XCH_F       (1<<8)
#define SPI_IRQ_F       (1<<7)
#define SPI_IRQEN_F     (1<<6)
#define SPI_PHA_F       (1<<5)
#define SPI_POL_F       (1<<4)

/*
 * UART 1 & 2
 */

#define USTCNT1		0xfffff900
#define UBAUD1          0xfffff902
#define URX1            0xfffff904
#define UTX1            0xfffff906
#define UMISC1          0xfffff908
#define NIPR1           0xfffff90a

#define USTCNT2		0xfffff910
#define UBAUD2          0xfffff912
#define URX2            0xfffff914
#define UTX2            0xfffff916
#define UMISC2          0xfffff918
#define NIPR2           0xfffff91a

/*
 * Flags for USTCNT1/2 register
 */
#define UEN_F		(1 << 15)
#define RXEN_F		(1 << 14)
#define TXEN_F		(1 << 13)
#define CLKM_F		(1 << 12)
#define PARITY_EN_F	(1 << 11)
#define ODD_F		(1 << 10)
#define STOP_F		(1 <<  9)
#define EITHER8OR7_F	(1 <<  8)
#define ODEN_F		(1 <<  7)
#define CTSD_F		(1 <<  6)
#define RXFE_F		(1 <<  5)
#define RXHE_F		(1 <<  4)
#define RXRE_F		(1 <<  3)
#define TXEE_F		(1 <<  2)
#define TXHE_F		(1 <<  1)
#define TXAE_F		(1 <<  0)

/*
 * Flags for UBAUD1/2 register
 */
#define UCLKDIR_F	(1 << 13)
#define BAUD_SRC_F	(1 << 11)

/*
 * Flagss for URX1/2 register
 */
#define FIFO_FULL_F	(1 << 15)
#define FIFO_HALF_F	(1 << 14)
#define DATA_READY_F	(1 << 13)
#define OLD_DATA_F	(1 << 12)
#define OVRUN_F		(1 << 11)
#define FRAME_ERROR_F	(1 << 10)
#define BREAK_F		(1 <<  9)
#define PARITY_ERROR_F	(1 <<  8)

/*
 * Flags for URX1/2 register
 */
#define FIFO_EMPTY_F	(1 << 15)
#define FIFO_HALF_F	(1 << 14)
#define TX_AVAIL_F	(1 << 13)
#define SEND_BREAK_F	(1 << 12)
#define NOCTS1_F	(1 << 11)
#define BUSY_F		(1 << 10)
#define CTS1_STAT_F	(1 <<  9)
#define CTS1_DELTA_F	(1 <<  8)

/*
 * Flags for the UMISC1/2 register
 */
#define BAUD_TEST_F	(1 << 15)
#define CLKSRC_F	(1 << 14)
#define FORCE_PERR_F	(1 << 13)
#define LOOP_F		(1 << 12)
#define BAUD_RESET_F	(1 << 11)
#define IRTEST_F	(1 << 10)
#define RTS1_CONT_F	(1 <<  7)
#define RTS1_F		(1 <<  6)
#define IRDAEN_F	(1 <<  5)
#define IRDA_LOOP_F	(1 <<  4)
#define RXPOL_F		(1 <<  3)
#define TXPOL_F		(1 <<  2)


/*
 * Offsets
 */
#define O_USTCNT	0
#define O_UBAUD		2
#define O_URX		4
#define O_UTX		6
#define O_UMISC		8
#define O_NIPR		10

/*
 * LCD Controller Registers
 */
#define LSSA		0xfffffa00
#define LVPW            0xfffffa05
#define LXMAX           0xfffffa08
#define LYMAX           0xfffffa0a
#define LCXP            0xfffffa18
#define LCYP            0xfffffa1a

#define LCWCH           0xfffffa1c
#define LBLKC           0xfffffa1f
#define LPICF           0xfffffa20
#define LPOLCF          0xfffffa21
#define LACDRC          0xfffffa23
#define LPXCD           0xfffffa25
#define LCKCON          0xfffffa27
#define LRRA            0xfffffa28
#define LOTCR           0xfffffa2b
#define LPOSR           0xfffffa2d
#define LFRCM           0xfffffa31
#define LGPMR           0xfffffa33
#define PWMR            0xfffffa36
#define RMCR            0xfffffa38


/*
 * Real time clock registers
 */

#define RTCTIME         0xfffffb00
#define RTCAKRM         0xfffffb04
#define WATCHDOG        0xfffffb0a
#define RTCCTL          0xfffffb0c
#define RTCISR          0xfffffb0e
#define RTCIENR         0xfffffb10
#define STPWCH          0xfffffb12
#define DAYR            0xfffffb1a
#define DAYALARM        0xfffffb1c

#define HOURS_M         (1<<28|1<<27|1<<26|1<<25|1<<24)
#define MINUTES_M       (1<<21|1<<20|1<<19|1<<18|1<<17|1<<16)
#define SECONDS_M       (1<<5|1<<4|1<<3|1<<2|1<<1|1<<0)

#define DAYSAL_M        0x01ff

#define CNTR_M          (0x03 << 8)
#define INTF_F          (0x01 << 7)
#define ISEL_F          (0x01 << 1)
#define EN_F            (0x01 << 0)

#define RTCEN_F         (0x01 << 7)
#define REFREQ_F        (0x01 << 5)

#define DRAMMC		0xfffffc00
#define DRAMC		0xfffffc00
#define SDCTRL		0xfffffc00
#define SDPWDN		0xfffffc00

/*
 * Flags used by the Interrupt controller - again the dragonball 
 * doc is all wrong - have a look at the xcopilot sources.
 */
 
/* Flags for IMR, ISR, IPR */
#define SPI2_F		(0x01 << 0)
#define TMR2_F		(0x01 << 1)
#define UART1_F		(0x01 << 2)
#define WDT_F		(0x01 << 3)
#define RTC_F		(0x01 << 4)
#define LCDC_F		(0x01 << 5)
#define KB_F		(0x01 << 6)
#define PWM1_F		(0x01 << 7)
#define INT0_F		(0x01 << 8)
#define INT1_F		(0x01 << 9)
#define INT2_F		(0x01 << 10)
#define INT3_F		(0x01 << 11)
#define INT4_F		(0x01 << 12)
#define INT5_F		(0x01 << 13)
#define INT6_F		(0x01 << 14)
#define INT7_F		(0x01 << 15)
#define IRQ1_F		(0x01 << 16)
#define IRQ2_F		(0x01 << 17)
#define IRQ3_F		(0x01 << 18)
#define IRQ6_F		(0x01 << 19)
#define PEN_F		(0x01 << 20)
#define SPIS_F		(0x01 << 21)
#define TMR1_F		(0x01 << 22)
#define IRQ7_F		(0x01 << 23)

/* Flags for ICR */
#define POL5_F		(0x01 << 7)
#define ET6_F		(0x01 << 8)
#define ET3_F		(0x01 << 9)
#define ET2_F		(0x01 << 10)
#define ET1_F		(0x01 << 11)
#define POL6_F		(0x01 << 12)
#define POL3_F		(0x01 << 13)
#define POL2_F		(0x01 << 14)
#define POL1_F		(0x01 << 15)
#define POL1_F		(0x01 << 15)
#define POL1_F		(0x01 << 15)
#define POL1_F		(0x01 << 15)
#define POL1_F		(0x01 << 15)
#define POL1_F		(0x01 << 15)
#define POL1_F		(0x01 << 15)
#define POL1_F		(0x01 << 15)

#endif
