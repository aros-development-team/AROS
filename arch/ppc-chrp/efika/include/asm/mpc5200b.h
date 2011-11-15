#ifndef ASM_MPC5200B_H
#define ASM_MPC5200B_H

#include <asm/cpu.h>

#define __BV32(num)	((0x80000000 >> (num)))
#define __BV16(num)	((0x8000 >> (num)))
#define __BV08(num) ((0x80 >> (num)))

typedef struct regs {
    uint32_t    gpr[32];
    uint32_t    srr0;
    uint32_t    srr1;
    uint32_t    ctr;
    uint32_t    lr;
    uint32_t    xer;
    uint32_t    ccr;
    uint32_t    dar;
    uint32_t    dsisr;
} regs_t;

typedef struct fpuregs {
    double      fpr[32];
    uint64_t    fpscr;
} fpuregs_t;

typedef struct context {
    regs_t      cpu;
    fpuregs_t   fpu;
} context_t;

/* Machine State Register */
#define MSR_POW 0x00040000
#define MSR_TGPR 0x00020000
#define MSR_ILE	0x00010000
#define MSR_EE  0x00008000
#define MSR_PR  0x00004000
#define MSR_FP  0x00002000
#define MSR_ME  0x00001000
#define MSR_FE0 0x00000800
#define MSR_SE	0x00000400
#define MSR_BE  0x00000200
#define MSR_FE1 0x00000100
#define MSR_IP	0x00000040
#define MSR_IS  0x00000020
#define MSR_DS  0x00000010
#define MSR_RI	0x00000002
#define MSR_LE	0x00000001

/* SPR registers */
#define XER     0x001   /* Integer Exception Register */
#define LR      0x008   /* Link Register */
#define CTR     0x009   /* Count Register */
#define DEC     0x016   /* Decrementer */
#define SDR1	0x019	/* MMU base address */
#define SRR0    0x01A   /* Save/Restore Register 0 */
#define SRR1    0x01B   /* Save/Restore Register 1 */
#define TBLU    0x10C   /* Time Base Lower */
#define TBUU    0x10D   /* Time Base Upper */
#define SPRG0   0x110   /* Special Purpose Register General 0 */
#define SPRG1   0x111   /* Special Purpose Register General 1 */
#define SPRG2   0x112   /* Special Purpose Register General 2 */
#define SPRG3   0x113   /* Special Purpose Register General 3 */
#define SPRG4   0x114   /* Special Purpose Register General 4 */
#define SPRG5   0x115   /* Special Purpose Register General 5 */
#define SPRG6   0x116   /* Special Purpose Register General 6 */
#define SPRG7   0x117   /* Special Purpose Register General 7 */

#define DMISS	0x3d0
#define DCMP	0x3d1
#define HASH1	0xd32
#define HASH2	0x3d3
#define IMISS	0x3d4
#define ICMP	0x3d5
#define RPA		0x3d6

static inline struct KernelBase *getKernelBase()
{
    return (struct KernelBase *)rdspr(SPRG4);
}

static inline struct ExecBase *getSysBase()
{
    return (struct ExecBase *)rdspr(SPRG5);
}

/* Interrupt controller */

typedef struct {
	uint32_t	ictl_pim;		/* Peripheral interrupt mask register */
	uint32_t	ictl_ppri[3];	/* Peripheral priority and HI/LO select register */
	uint32_t	ictl_ee;		/* External enable and external types register */
	uint32_t	ictl_cpmim;		/* Critical Priority and Main Interrupt Mask Register */
	uint32_t	ictl_mip[2];	/* Main Interrupt Priority and INT/SMI Select Register */
	uint32_t	__pad1;
	uint32_t	ictl_pmce;		/* PerStat, MainStat, CritStat Encoded Register */
	uint32_t	ictl_cis;		/* Critical Interrupt Status All Register */
	uint32_t	ictl_mis;		/* Main Interrupt Status All Register */
	uint32_t	ictl_pis;		/* Peripheral Interrupt Status All Register */
	uint32_t	__pad2;
	uint32_t	ictl_bes;		/* Bus Error Status Register */
	uint32_t	__pad3;
	uint32_t	ictl_mie;		/* Main Interrupt Emulation All Register */
	uint32_t	ictl_pie;		/* Peripheral Interrupt Emulation All Register */
	uint32_t	ictl_iie;		/* IRQ Interrupt Emulation All Register */
} mpc5200b_ictl_t;

#define MPC5200B_IRQ0		0	/* IRQ 0 */
#define MPC5200B_ST0		1	/* Slice Timer 0 */
#define MPC5200B_HI_INT		2	/* HI_int */
#define MPC5200B_WAKEUP		3	/* WakeUp from deep-sleep */

#define MPC5200B_ST1		4	/* Slice Timer 1 */
#define MPC5200B_IRQ1		5	/* IRQ 1 */
#define MPC5200B_IRQ2		6	/* IRQ 2 */
#define MPC5200B_IRQ3		7	/* IRQ 3 */
#define MPC5200B_LO_INT		8	/* LO_int */
#define MPC5200B_RTC_PINT	9	/* RTC Periodic Int */
#define MPC5200B_RTC_SINT	10	/* RTC Stopwatch and Alarm Int */
#define MPC5200B_GPIO_STD	11	/* GPIO interrupts */
#define MPC5200B_GPIO_WKUP	12	/* GPIO wakeup */
#define MPC5200B_TMR0		13	/* Timer 0 */
#define MPC5200B_TMR1		14	/* Timer 1 */
#define MPC5200B_TMR2		15	/* Timer 2 */
#define MPC5200B_TMR3		16	/* Timer 3 */
#define MPC5200B_TMR4		17	/* Timer 4 */
#define MPC5200B_TMR5		18	/* Timer 5 */
#define MPC5200B_TMR6		19	/* Timer 6 */
#define MPC5200B_TMR7		20	/* Timer 7 */

#define MPC5200B_BESTCOMM	21
#define MPC5200B_PSC1		22
#define MPC5200B_PSC2		23
#define MPC5200B_PSC3		24
#define MPC5200B_PSC6		25
#define MPC5200B_ETHER		26
#define MPC5200B_USB		27
#define MPC5200B_ATA		28
#define MPC5200B_PCICM		29
#define MPC5200B_PCISC_RX	30
#define MPC5200B_PCISC_TX	31
#define MPC5200B_PSC4		32
#define MPC5200B_PSC5		33
#define MPC5200B_SPI_MODF	34
#define MPC5200B_SPI_SPIF	35
#define MPC5200B_I2C1		36
#define MPC5200B_I2C2		37
#define MPC5200B_CAN1		38
#define MPC5200B_CAN2		39
#define MPC5200B_XLB		42
#define MPC5200B_BDLC		43
#define MPC5200B_BESTCOMMLP	44

#define PER_BESTCOMM		0
#define PER_PSC1			1
#define PER_PSC2			2
#define PER_PSC3			3
#define PER_PSC6			4
#define PER_ETHER			5
#define PER_USB				6
#define PER_ATA				7
#define PER_PCICM			8
#define PER_PCISC_RX		9
#define PER_PCISC_TX		10
#define PER_PSC4			11
#define PER_PSC5			12
#define PER_SPI_MODF		13
#define PER_SPI_SPIF		14
#define PER_I2C1			15
#define PER_I2C2			16
#define PER_CAN1			17
#define PER_CAN2			18
#define PER_XLB				21
#define PER_BDLC			22
#define PER_BESTCOMMLP		23

#define ICTL_PIM_BESTCOMM	__BV32(PER_BESTCOMM)
#define ICTL_PIM_PSC1		__BV32(PER_PSC1)
#define ICTL_PIM_PSC2		__BV32(PER_PSC2)
#define ICTL_PIM_PSC3		__BV32(PER_PSC3)
#define ICTL_PIM_PSC6		__BV32(PER_PSC6)
#define ICTL_PIM_ETHER		__BV32(PER_ETHER)
#define ICTL_PIM_USB		__BV32(PER_USB)
#define ICTL_PIM_ATA		__BV32(PER_ATA)
#define ICTL_PIM_PCICM		__BV32(PER_PCICM)
#define ICTL_PIM_PCISC_RX	__BV32(PER_PCISC_RX)
#define ICTL_PIM_PCISC_TX	__BV32(PER_PCISC_TX)
#define ICTL_PIM_PSC4		__BV32(PER_PSC4)
#define ICTL_PIM_PSC5		__BV32(PER_PSC5)
#define ICTL_PIM_SPI_MODF	__BV32(PER_SPI_MODF)
#define ICTL_PIM_SPI_SPIF	__BV32(PER_SPI_SPIF)
#define ICTL_PIM_I2C1		__BV32(PER_I2C1)
#define ICTL_PIM_I2C2		__BV32(PER_I2C2)
#define ICTL_PIM_CAN1		__BV32(PER_CAN1)
#define ICTL_PIM_CAN2		__BV32(PER_CAN2)
#define ICTL_PIM_XLB		__BV32(PER_XLB)
#define ICTL_PIM_BDLC		__BV32(PER_BDLC)
#define ICTL_PIM_BESTCOMMLP	__BV32(PER_BESTCOMMLP)

#define ICTL_EE_MEE			__BV32(19)
#define ICTL_EE_CEB			__BV32(31)

/* Slice timer */
typedef struct {
	volatile uint32_t 	slt_tc;	/* Terminal count register */
	volatile uint32_t	slt_cf;	/* Control field register */
	volatile uint32_t	slt_cv;	/* Count value register. Read only! */
	volatile uint32_t	slt_ts;	/* Timer Status register */
} slt_t;

#define SLT_CF_RUNWAIT		0x04000000	/* Run/Wait */
#define SLT_CF_INTRENA		0x02000000	/* Interrupt enable */
#define SLT_CF_ENABLE		0x01000000	/* Enable/Disable timer */

#define SLT_TS_ST			0x01000000	/* Terminal count reached. Write 1 to clear */


/* Ata device */
typedef struct {
	volatile uint32_t	ata_config;
	volatile uint32_t	ata_status;

	volatile uint32_t	ata_pio1;
	volatile uint32_t 	ata_pio2;
	volatile uint32_t	ata_dma1;
	volatile uint32_t 	ata_dma2;
	volatile uint32_t	ata_udma1;
	volatile uint32_t	ata_udma2;
	volatile uint32_t	ata_udma3;
	volatile uint32_t	ata_udma4;
	volatile uint32_t 	ata_udma5;
	volatile uint32_t	ata_invalid;
} ata_5k2_t;


/* bestcomm */

typedef struct {
	uint32_t	bc_taskBar;
	uint32_t	bc_currentPointer;
	uint32_t	bc_endPointer;
	uint32_t	bc_variablePointer;
	uint32_t	bc_interruptVector;
	uint32_t	bc_interruptPending;
	uint32_t	bc_interruptMask;
	uint16_t	bc_tcr[16];
	uint8_t		bc_ipr[32];
	uint32_t	bc_requestMuxControl;
	uint32_t	bc_taskSize[2];
} bestcomm_t;

/* Bestcomm's task description table */
typedef struct {
	uint32_t	start;
	uint32_t	stop;
	uint32_t	var;
	uint32_t	fdt;
	uint32_t	exec_status;
	uint32_t	mvtp;
	uint32_t	context;
	uint32_t	litbase;
} bestcomm_tdt_t;

#endif /* ASM_MPC5200B_H */
