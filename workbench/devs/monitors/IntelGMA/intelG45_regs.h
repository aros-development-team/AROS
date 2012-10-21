/*
 * intelG45_regs.h
 *
 *  Created on: Apr 17, 2010
 *      $Id$
 */

#include <stdint.h>

#ifndef INTELG45_REGS_H_
#define INTELG45_REGS_H_

#define G45_MGCC			0x52	/* Mirror of Dev0 GMCH Graphics Control */

#define G45_MGCC_IVD		0x0002	/* IGP VGA Disable */

#define G45_MGCC_GMS_MASK	0x00f0	/* Graphics Mode Select */
#define G45_MGCC_GMS_1M		0x0010
#define G45_MGCC_GMS_4M		0x0020
#define G45_MGCC_GMS_8M		0x0030
#define G45_MGCC_GMS_16M	0x0040
#define G45_MGCC_GMS_32M	0x0050
#define G45_MGCC_GMS_48M	0x0060
#define G45_MGCC_GMS_64M	0x0070

#define G45_BSM			0x5c
#define G45_MSAC			0x62
#define G45_MSAC2			0x66


#define G45_GATT_CONTROL				0x2020

/* Ring buffer registers */
#define G45_RING_TAIL					0x2030
#define G45_RING_TAIL_MASK				0x1ffff8

#define G45_RING_HEAD					0x2034
#define G45_RING_HEAD_WRAP_MASK		0x7ff
#define G45_RING_HEAD_WRAP_SHIFT		21
#define G45_RING_HEAD_MASK				0x1ffffc

#define G45_RING_BASE					0x2038
#define G45_RING_BASE_MASK				0xfffff000

#define G45_RING_CONTROL				0x203c
#define G45_RING_CONTROL_LENGTH_MASK	0x1ff
#define G45_RING_CONTROL_LENGTH_SHIFT	12
#define G45_RING_CONTROL_WAIT			0x00000800
#define G45_RING_CONTROL_REPORT_OFF	0x00000000
#define G45_RING_CONTROL_REPORT_64K	0x00000002
#define G45_RING_CONTROL_REPORT_128K	0x00000006
#define G45_RING_CONTROL_ENABLE		0x00000001

#define G45_GPIOA			0x5010
#define G45_GPIOB			0x5014
#define G45_GPIOC			0x5018
#define G45_GPIOD			0x501c
#define G45_GPIOE			0x5020
#define G45_GPIOF			0x5024
#define G45_GPIOG			0x5028
#define G45_GPIOH			0x502c

#define G45_GPIO_CLOCK_DIR_MASK	0x0001
#define G45_GPIO_CLOCK_DIR_VAL		0x0002
#define G45_GPIO_CLOCK_DATA_MASK	0x0004
#define G45_GPIO_CLOCK_DATA_VAL	0x0008
#define G45_GPIO_CLOCK_DATA_IN		0x0010
#define G45_GPIO_DATA_DIR_MASK		0x0100
#define G45_GPIO_DATA_DIR_VAL		0x0200
#define G45_GPIO_DATA_MASK			0x0400
#define G45_GPIO_DATA_VAL			0x0800
#define G45_GPIO_DATA_IN			0x1000

#define G45_GMBUS			0x5100

#define G45_PIPEASRC				0x6001c
#define G45_PIPEBSRC				0x6101c	
#define G45_PIPEACONF		0x70008
#define G45_PIPEBCONF       0x71008

#define G45_PIPECONF_ENABLE		0x80000000
#define G45_PIPECONF_ENABLED		0x40000000
#define G45_PIPECONF_DELAY_00		0
#define G45_PIPECONF_DELAY_01		0x08000000
#define G45_PIPECONF_DELAY_02		0x10000000
#define G45_PIPECONF_DELAY_03		0x18000000
#define G45_PIPECONF_FORCE_BORDER	0x02000000
#define G45_PIPECONF_10BIT_GAMMA	0x01000000
#define G45_PIPECONF_INTERLACE		0x00800000

#define G45_VGACNTRL		0x71400
#define G45_VGACNTRL_VGA_DISABLE	0x80000000

#define G45_DPLLA_CTRL		0x6014
#define G45_DPLLB_CTRL		0x6018

#define G45_DPLL_VCO_ENABLE		0x80000000
#define G45_DPLL_DVO_HIGH_SPEED	0x40000000
#define G45_DPLL_VGA_MODE_DISABLE	0x10000000
#define G45_DPLL_MODE_LVDS			0x08000000
#define G45_DPLL_MODE_DAC_SERIAL	0x04000000
#define G45_DPLL_DAC_SERIAL_P2_DIV_5	0x01000000
#define G45_DPLL_DAC_SERIAL_P2_DIV_10	0x00000000
#define G45_DPLL_LVDS_P2_DIV_7		0x01000000
#define G45_DPLL_LVDS_P2_DIV_14	0x00000000
#define G45_DPLL_P1_MASK			0x00ff0000
#define G45_DPLL_P1_SHIFT			16
#define G45_DPLL_PHASE_MASK		0x00001e00
#define G45_DPLL_PHASE_SHIFT		9

#define G45_FPA0					0x6040
#define G45_FPA1					0x6044
#define G45_FPB0					0x6048
#define G45_FPB1					0x604c

#define G45_DSPACNTR				0x70180
#define G45_DSPBCNTR				0x71180

#define G45_DSPASIZE		0x70190
#define G45_DSPBSIZE		0x71190

#define G45_DSPCNTR_SEL_PIPE_B		(1<<24)
#define G45_DSPCNTR_PLANE_ENABLE	0x80000000
#define G45_DSPCNTR_GAMMA_ENABLE	0x40000000
#define G45_DSPCNTR_PIXEL_MASK		(0xf << 26)
#define G45_DSPCNTR_8BPP			(0x2 << 26)
#define G45_DSPCNTR_15BPP			(0x4 << 26)
#define G45_DSPCNTR_16BPP			(0x5 << 26)
#define G45_DSPCNTR_32BPP			(0x6 << 26)

#define G45_DSPALINOFF				0x70184
#define G45_DSPABASE G45_DSPALINOFF
#define G45_DSPASTRIDE				0x70188
#define G45_DSPASURF				0x7019c

#define G45_DSPBLINOFF				0x71184
#define G45_DSPBBASE G45_DSPBLINOFF
#define G45_DSPBSTRIDE				0x71188
#define G45_DSPBSURF 				0x7119C

#define G45_HTOTAL_A				0x60000
#define G45_HBLANK_A				0x60004
#define G45_HSYNC_A				0x60008
#define G45_VTOTAL_A				0x6000c
#define G45_VBLANK_A				0x60010
#define G45_VSYNC_A				0x60014

#define G45_HTOTAL_B	0x61000
#define G45_HBLANK_B	0x61004
#define G45_HSYNC_B 	0x61008
#define G45_VTOTAL_B	0x6100c
#define G45_VBLANK_B	0x61010
#define G45_VSYNC_B 	0x61014


#define G45_PIPEASRC				0x6001c
#define PIPEBCONF 0x71008
#define G45_ADPA					0x61100
#define G45_ADPA_MASK				0x3fff43e7
#define G45_ADPA_ENABLE			0x80000000
#define G45_ADPA_PIPESEL			0x40000000
#define G45_ADPA_VGA_SYNC			0x00008000
#define G45_ADPA_DPMS_ON			0x00000000
#define G45_ADPA_DPMS_OFF			0x00000c00
#define G45_ADPA_DPMS_MASK			0x00000c00
#define G45_ADPA_DPMS_STANDBY		0x00000800
#define G45_ADPA_DPMS_SUSPEND		0x00000400
#define G45_ADPA_VSYNC_PLUS		0x00000010
#define G45_ADPA_HSYNC_PLUS		0x00000008

#define G45_LVDS					0x61180
#define G45_LVDS_PORT_EN			(1 << 31)
#define G45_LVDS_PIPEB_SELECT		(1 << 30) /* Selects pipe B for LVDS data.  Must be set on pre-965. */

#define G45_PFIT_CONTROL			0x61230
#define G45_PFIT_ENABLE				(1 << 31)
#define G45_PFIT_PGM_RATIOS			0x61234

#define G45_CURACNTR				0x70080
#define G45_CURBCNTR				0x700C0
#define G45_CURCNTR_PIPE_A			0x00000000
#define G45_CURCNTR_PIPE_B			0x10000000
#define G45_CURCNTR_PIPE_C			0x20000000
#define G45_CURCNTR_PIPE_D			0x30000000
#define G45_CURCNTR_PIPE_MASK		0x30000000
#define G45_CURCNTR_POPUP_ENABLE	0x08000000
#define G45_CURCNTR_GAMMA_ENABLE	0x04000000
#define G45_CURCNTR_ROTATE_180		0x00008000
#define G45_CURCNTR_TYPE_MASK		0x00000027
#define G45_CURCNTR_TYPE_OFF		0x00000000
#define G45_CURCNTR_TYPE_ARGB		0x00000027		/* The only one used by AROS. 64x64 ARGB */

#define G45_CURABASE				0x70084		/* Base address of cursor (4K aligned) and trigger for update operations */
#define G45_CURBBASE				0x700C4	
#define G45_CURAPOS				0x70088			/* Cursor position */
#define G45_CURBPOS				0x700C8

#define G45_CURPOS_XSHIFT			0
#define G45_CURPOS_YSHIFT			16
#define G45_CURPOS_SIGN			0x8000

#define   MI_READ_FLUSH		(1 << 0)
#define   MI_EXE_FLUSH		(1 << 1)
#define   MI_NO_WRITE_FLUSH	(1 << 2)

#define PIPE_A 0
#define PIPE_B 1

#define readl(addr) ( *(volatile uint32_t *) (addr) )
#define readw(addr) ( *(volatile uint16_t *) (addr) )
#define readb(addr) ( *(volatile uint8_t *)  (addr) )

#define writeb(b,addr) do { (*(volatile uint8_t *)  (addr)) = (b); } while (0)
#define writew(b,addr) do { (*(volatile uint16_t *) (addr)) = (b); } while (0)
#define writel(b,addr) do { (*(volatile uint32_t *) (addr)) = (b); } while (0)

#define OUT_RING(n) do { \
	writel((n), &sd->RingBufferPhys[sd->RingBufferTail]); \
	sd->RingBufferTail += 4; \
	sd->RingBufferTail %= sd->RingBufferSize; \
} while(0)

#define START_RING(n) do { \
	uint32_t head, tail, space; \
	do { \
		head = readl(sd->Card.MMIO + G45_RING_HEAD) & G45_RING_HEAD_MASK; \
		tail = sd->RingBufferTail; \
		if (tail >= head) \
			space = sd->RingBufferSize - (tail - head); \
		else \
			space = head - tail; \
		if (space > 256) space-= 256; \
		else space = 0; \
	} while(space < (n)*4); \
} while(0)

#define ADVANCE_RING() do { sd->RingActive = 1; writel(sd->RingBufferTail, sd->Card.MMIO + G45_RING_TAIL); } while(0)

#define WAIT_IDLE() if (sd->RingActive) do { \
	uint32_t head, tail; \
	do { \
		head = readl(sd->Card.MMIO + G45_RING_HEAD) & G45_RING_HEAD_MASK; \
		tail = readl(sd->Card.MMIO + G45_RING_TAIL) & G45_RING_TAIL_MASK; \
	} while(head != tail); sd->RingActive = 0; \
} while(0)

struct __ROP {
    int rop;
    int pattern;
};

extern struct __ROP ROP_table[];

#define ROP3_ZERO             0x00000000
#define ROP3_DSa              0x00880000
#define ROP3_SDna             0x00440000
#define ROP3_S                0x00cc0000
#define ROP3_DSna             0x00220000
#define ROP3_D                0x00aa0000
#define ROP3_DSx              0x00660000
#define ROP3_DSo              0x00ee0000
#define ROP3_DSon             0x00110000
#define ROP3_DSxn             0x00990000
#define ROP3_Dn               0x00550000
#define ROP3_SDno             0x00dd0000
#define ROP3_Sn               0x00330000
#define ROP3_DSno             0x00bb0000
#define ROP3_DSan             0x00770000
#define ROP3_ONE              0x00ff0000
#define ROP3_DPa              0x00a00000
#define ROP3_PDna             0x00500000
#define ROP3_P                0x00f00000
#define ROP3_DPna             0x000a0000
#define ROP3_D                0x00aa0000
#define ROP3_DPx              0x005a0000
#define ROP3_DPo              0x00fa0000
#define ROP3_DPon             0x00050000
#define ROP3_PDxn             0x00a50000
#define ROP3_PDno             0x00f50000
#define ROP3_Pn               0x000f0000
#define ROP3_DPno             0x00af0000
#define ROP3_DPan             0x005f0000
 //  | (1 << 4) | (1 << 0)
#define DO_FLUSH() do { if (sd->RingActive) { START_RING(2); OUT_RING((4 << 23)); OUT_RING(0); ADVANCE_RING(); WAIT_IDLE(); } } while(0)

#endif /* INTELG45_REGS_H_ */
