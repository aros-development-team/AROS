#ifndef REGISTERS_H
#define REGISTERS_H

#include <exec/types.h>
#define WREG_L(addr)	*(ULONG *)addr
#define RREG_L(addr)	*(ULONG *)addr
#define WREG_W(addr)	*(UWORD *)addr
#define RREG_W(addr)	*(UWORD *)addr
#define WREG_B(addr)	*(UBYTE *)addr
#define RREG_B(addr)	*(UBYTE *)addr


/*
 * Interrupt Controller
 */

#define ICIP        0x90050000
#define ICMR        0x90050004
#define ICLR        0x90050008
#define ICCR        0x9005000c
#define ICFP        0x90050010
#define ICPR        0x90050020


/*
 * Real time clock
 */
#define RTAR        0x90010000
#define RCNR        0x90010004
#define RTTR        0x90010008
#define RTSR        0x90010010

/*
 * Operating System Timer
 */
#define OSMR0       0x90000000
#define OSMR1       0x90000004
#define OSMR2       0x90000008
#define OSMR3       0x9000000c
#define OSCR        0x90000010
#define OSSR        0x90000014
#define OWER        0x90000018
#define OIER        0x9000001c


/*
 * Serial Port 3 - UART
 */
#define UTCR0       0x80050000
#define UTCR1       0x80050004
#define UTCR2       0x80050008
#define UTCR3       0x8005000c
#define UTDR        0x80050014
#define UTSR0       0x8005001c
#define UTSR1       0x80050020

#define	PE_F        (1 << 0)
#define OES_F       (1 << 1)
#define SBS_F       (1 << 2)
#define DSS_F       (1 << 3)
#define SCE_F       (1 << 4)
#define RCE_F       (1 << 5)
#define TCE_F       (1 << 6)


#define RXE_F       (1 << 0)
#define TXE_F       (1 << 1)
#define BRK_F       (1 << 2)
#define RIE_F       (1 << 3)
#define TIE_F       (1 << 4)
#define LBM_F       (1 << 5)


#define PRE_F       (1 << 8)
#define FRE_F       (1 << 9)
#define ROR_F       (1 << 10)

#define TFS_F       (1 << 0)
#define RFS_F       (1 << 1)
#define RID_F       (1 << 2)
#define RBB_F       (1 << 3)
#define REB_F       (1 << 4)
#define EIF_F       (1 << 5)

#define TBY_F       (1 << 0)
#define RNE_F       (1 << 1)
#define TNF_F       (1 << 2)
//#define PRE_F       (1 << 3)
//#define FRE_F       (1 << 4)
//#define ROR_F       (1 << 5)

/*
 * LCD controller
 */
#define LCCR0       0xB0100000
#define LCSR        0xB0100004
#define DBAR1       0xB0100010
#define DCAR1       0xB0100014
#define DBAR2       0xB0100018
#define DCAR2       0xB010001c
#define LCCR1       0xB0100020
#define LCCR2       0xB0100024
#define LCCR3       0xB0100028

#define PBS_M       (1 << 13 | 1 << 12)

#define LEN_F       (1 << 0)
#define CMS_F       (1 << 1)
#define SDS_F       (1 << 2)
#define LDM_F       (1 << 3)
#define BAM_F       (1 << 4)
#define ERM_F       (1 << 5)
#define PAS_F       (1 << 7)
#define BLE_F       (1 << 8)
#define DPD_F       (1 << 9)
#define PDD_M       (0xff << 12)

#define PLL_M       (0x3ff)
#define HSW_M       (0x3f << 10)
#define ELW_M       (0xff << 16)
#define BLW_M       (0xff << 16)

#define LPP_M       (0x3ff)
#define VSW_M       (0x3f << 10)
#define EFW_M       (0xff << 16)
#define BFW_M       (0xff << 24)

#define PCD_M       (0xff << 0)
#define ACB_M       (0xff << 8)
#define API_M       (0x0f << 16)
#define VSP_F       (1 << 20)
#define HSP_F       (1 << 21)
#define PCP_F       (1 << 22)
#define OEP_F       (1 << 23)

#define LDD_F       (1 << 0)
#define BAU_F       (1 << 1)
#define BER_F       (1 << 2)
#define ABC_F       (1 << 3)
#define IOL_F       (1 << 4)
#define IUL_F       (1 << 5)
#define IOU_F       (1 << 6)
#define IUU_F       (1 << 7)
#define OOL_F       (1 << 8)
#define OUL_F       (1 << 9)
#define OOU_F       (1 << 10)
#define OUU_F       (1 << 11)

#endif
