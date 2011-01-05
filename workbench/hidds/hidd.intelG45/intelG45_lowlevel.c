/*
 * intelG45_lowlevel.c
 *
 *  Created on: May 2, 2010
 *      $Id$
 */

#define DEBUG 0
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <utility/tagitem.h>

#include <hidd/graphics.h>
#include <hidd/i2c.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <stdint.h>
#include <stdlib.h>


#include LC_LIBDEFS_FILE

#include "intelG45_intern.h"
#include "intelG45_regs.h"

typedef struct {
	 uint8_t	N;
	 uint8_t	M1;
	 uint8_t	M2;
	 uint8_t	P1;
	 uint8_t	P2;
	 uint32_t	VCO;
	 uint32_t	PixelClock;
} GMA_PLL_t;

BOOL delay_ms(struct g45staticdata *sd, uint32_t msec)
{
	/* Allocate a signal within this task context */

	sd->tr.tr_node.io_Message.mn_ReplyPort->mp_SigBit = SIGB_SINGLE;
	sd->tr.tr_node.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);

	/* Specify the request */
	sd->tr.tr_node.io_Command = TR_ADDREQUEST;
	sd->tr.tr_time.tv_secs = msec / 1000;
	sd->tr.tr_time.tv_micro = 1000 * (msec % 1000);

	/* Wait */
	DoIO((struct IORequest *)&sd->tr);

	sd->tr.tr_node.io_Message.mn_ReplyPort->mp_SigTask = NULL;
}

BOOL delay_us(struct g45staticdata *sd, uint32_t usec)
{
	/* Allocate a signal within this task context */

	sd->tr.tr_node.io_Message.mn_ReplyPort->mp_SigBit = SIGB_SINGLE;
	sd->tr.tr_node.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);

	/* Specify the request */
	sd->tr.tr_node.io_Command = TR_ADDREQUEST;
	sd->tr.tr_time.tv_secs = usec / 1000000;
	sd->tr.tr_time.tv_micro = (usec % 1000000);

	/* Wait */
	DoIO((struct IORequest *)&sd->tr);

	sd->tr.tr_node.io_Message.mn_ReplyPort->mp_SigTask = NULL;
}
static BOOL calc_pll_and_validate(GMA_PLL_t *pll)
{
	uint32_t	m, p;

	/* requirement: M1 > M2 */
	if (pll->M1 <= pll->M2)
		return FALSE;

	/* M1 range: 10 .. 20 */
	if (pll->M1 < 10 || pll->M1 > 20)
		return FALSE;

	/* M2 range: 5 .. 9 */
	if (pll->M2 < 5 || pll->M2 > 9)
		return FALSE;

	m = 5 * ((uint32_t)pll->M1 + 2) + (pll->M2 + 2);

	/* m range: 70 .. 120 */
	if (m < 70 || m > 120)
		return FALSE;

	/* N range: 3 .. 8 */
	if (pll->N < 3 || pll->N > 8)
		return FALSE;

	pll->VCO = (96000 * m) / (pll->N + 2);

	if (pll->VCO < 1400000 || pll->VCO > 2800000)
		return FALSE;

	if (pll->P1 < 1 || pll->P1 > 8)
		return FALSE;

	p = (uint32_t)pll->P1 * (uint32_t)pll->P2;

	/* p range: 5 .. 80 for sDVO/DAC mode */
	if (p < 5 || p > 80)
		return FALSE;

	pll->PixelClock = pll->VCO / p;

	if (pll->PixelClock < 20000 || pll->PixelClock > 400000)
		return FALSE;

	return TRUE;
}

void G45_InitMode(struct g45staticdata *sd, GMAState_t *state,
		uint16_t width, uint16_t height, uint8_t depth, uint32_t pixelclock, intptr_t framebuffer,
        uint16_t hdisp, uint16_t vdisp, uint16_t hstart, uint16_t hend, uint16_t htotal,
        uint16_t vstart, uint16_t vend, uint16_t vtotal, uint32_t flags)
{
	D(bug("[GMA] InitMode %dx%dx%d @ %dHz\n", hdisp, vdisp, depth, ((pixelclock / (uint32_t)htotal) * 1000) / ((uint32_t)vtotal)));
	GMA_PLL_t clock, t;
	uint32_t err = pixelclock;

	clock.PixelClock = 0;

	/*
	 * Brute force determination of PLL settings. Iterate through all available configurations and select the most
	 * suitable one. I know, nested loops, but there is really no better way to do it
	 */
	if (pixelclock <= 270000)
		t.P2 = 10;
	else
		t.P2 = 5;

	for (t.M1 = 10; t.M1 <= 20; t.M1++)
	{
		for (t.M2 = 5; t.M2 <= 9; t.M2++)
		{
			for (t.N = 3; t.N <= 8; t.N++)
			{
				for (t.P1=1; t.P1 <= 8; t.P1++)
				{
					if (calc_pll_and_validate(&t) == TRUE)
					{
						int this_err = abs(pixelclock - t.PixelClock);
						if (this_err < err)
						{
							clock = t;
							err = this_err;
						}
					}
				}
			}
		}
	}

	if (clock.PixelClock)
	{
		D(bug("[GMA] PixelClock: Found: %d kHz, Requested: %d kHz\n", clock.PixelClock, pixelclock));
		D(bug("[GMA] VCO: %d MHz\n", clock.VCO / 1000));
		state->fp = clock.N << 16 | clock.M1 << 8 | clock.M2;

		if (clock.VCO >= 1900000 && clock.VCO < 2400000)
			state->fp |= 1 << 24;
		else if (clock.VCO >= 2500000 && clock.VCO < 3000000)
			state->fp |= 2 << 24;
		else if (clock.VCO > 3100000)
			state->fp |= 3 << 24;

		state->dpll = (10 << G45_DPLL_PHASE_SHIFT) | G45_DPLL_VGA_MODE_DISABLE | G45_DPLL_MODE_DAC_SERIAL;
		if (clock.P2 >= 10)
			state->dpll |= G45_DPLL_DAC_SERIAL_P2_DIV_10;
		else
			state->dpll |= G45_DPLL_DAC_SERIAL_P2_DIV_5;
		state->dpll |= (1 << (clock.P1 - 1)) << G45_DPLL_P1_SHIFT;

		state->dspcntr = readl(sd->Card.MMIO + (sd->pipe == PIPE_A ? G45_DSPACNTR:G45_DSPBCNTR)) & 0x000f0000;

		if (depth <= 8)
		{
			state->dspcntr |= G45_DSPCNTR_8BPP;
			state->dspstride = width;
		}
		else if (depth == 15)
		{
			state->dspcntr |= G45_DSPCNTR_15BPP;
			state->dspstride = width * 2;
		}
		else if (depth == 16)
		{
			state->dspcntr |= G45_DSPCNTR_16BPP;
			state->dspstride = width * 2;
		}
		else
		{
			state->dspcntr |= G45_DSPCNTR_32BPP;
			state->dspstride = width * 4;
		}

		state->pipeconf = G45_PIPECONF_ENABLE;
		state->dpll |= G45_DPLL_VCO_ENABLE;
		state->dspcntr |= G45_DSPCNTR_PLANE_ENABLE;
		if(sd->pipe == PIPE_B ) state->dspcntr |= G45_DSPCNTR_SEL_PIPE_B;

		state->htotal = (hdisp - 1) | ((htotal - 1) << 16);
		state->hblank = (hdisp - 1) | ((htotal - 1) << 16);
		state->hsync = (hstart - 1) | ((hend - 1) << 16);

		state->vtotal = (vdisp - 1) | ((vtotal - 1) << 16);
		state->vblank = (vdisp - 1) | ((vtotal - 1) << 16);
		state->vsync = (vstart - 1) | ((vend - 1) << 16);

		state->pipesrc = (vdisp - 1) | ((hdisp - 1) << 16);
		state->dspsurf = 0;
		state->dsplinoff = framebuffer;
		state->dspstride = (state->dspstride + 63) & ~63;

		state->adpa = 0;

		if (flags & vHidd_Sync_HSyncPlus)
			state->adpa |= G45_ADPA_HSYNC_PLUS;
		if (flags & vHidd_Sync_VSyncPlus)
			state->adpa |= G45_ADPA_VSYNC_PLUS;

		D(bug("[GMA] dpll=%08x\n", state->dpll));
	}
}

void G45_LoadState(struct g45staticdata *sd, GMAState_t *state)
{
	int i;
	D(bug("[GMA] LoadState\n"));

	LOCK_HW
	DO_FLUSH();
	
	if( sd->pipe == PIPE_B )
	{
		
		bug("[GMA]load lvds\n");

		writel(readl(sd->Card.MMIO + G45_VGACNTRL) | G45_VGACNTRL_VGA_DISABLE, sd->Card.MMIO + G45_VGACNTRL);
		
		writel(((sd->lvds_fixed.hdisp - 1) << 16) | (sd->lvds_fixed.vdisp - 1), sd->Card.MMIO + G45_PIPEBSRC);
		writel(((sd->lvds_fixed.vdisp - 1) << 16) | (sd->lvds_fixed.hdisp - 1), sd->Card.MMIO + G45_DSPBSIZE);
		
		writel(	state->pipeconf , sd->Card.MMIO + G45_PIPEBCONF);
		readl(sd->Card.MMIO + G45_PIPEBCONF);
		delay_ms(sd, 20);

		writel( state->dspcntr , sd->Card.MMIO + G45_DSPBCNTR );
		delay_ms(sd, 20);

		LONG offset = sd->VisibleBitmap->yoffset * sd->VisibleBitmap->pitch +
		              sd->VisibleBitmap->xoffset * sd->VisibleBitmap->bpp;
		writel( state->dspstride , sd->Card.MMIO + G45_DSPBSTRIDE );
		writel( state->dsplinoff - offset , sd->Card.MMIO + G45_DSPBLINOFF );
		readl( sd->Card.MMIO + G45_DSPBLINOFF );

		delay_ms(sd, 20);
	 
		ULONG i;
		for (i = 0; i < 256; i++) {
			writel( (i << 16) |(i << 8) | i , sd->Card.MMIO + 0x0a800 + 4 * i);//PALETTE_B
		}

	}else{
		
		uint32_t tmp;

		writel(readl(sd->Card.MMIO + 0x61140) & ~(1 << 29), sd->Card.MMIO + 0x61140);
		writel(readl(sd->Card.MMIO + 0x61160) & ~(1 << 29), sd->Card.MMIO + 0x61160);

		writel(readl(sd->Card.MMIO + 0x61140) & ~(1 << 31), sd->Card.MMIO + 0x61140);
		writel(readl(sd->Card.MMIO + 0x61160) & ~(1 << 31), sd->Card.MMIO + 0x61160);

	//	/* Stop cursor */
	//	writel(0, sd->Card.MMIO + 0x70080);
	//	delay_ms(sd, 20);

		/* Disable pipe */
		writel(readl(sd->Card.MMIO + G45_PIPEACONF) & ~G45_PIPECONF_ENABLE, sd->Card.MMIO + G45_PIPEACONF);

		for (i=0; i < 100; i++)
		{
			if ((readl(sd->Card.MMIO + G45_PIPEACONF) & G45_PIPECONF_ENABLED) == 0)
				break;

			/* Disable pipe again and again*/
			writel(readl(sd->Card.MMIO + G45_PIPEACONF) & ~G45_PIPECONF_ENABLE, sd->Card.MMIO + G45_PIPEACONF);
			delay_ms(sd, 10);
		}

		/* Disable DAC */
		writel(readl(sd->Card.MMIO + G45_ADPA) & ~G45_ADPA_ENABLE, sd->Card.MMIO + G45_ADPA);

		/* Disable planes */
		writel(readl(sd->Card.MMIO + G45_DSPACNTR) & ~G45_DSPCNTR_PLANE_ENABLE, sd->Card.MMIO + G45_DSPACNTR);
		writel(readl(sd->Card.MMIO + G45_DSPBCNTR) & ~G45_DSPCNTR_PLANE_ENABLE, sd->Card.MMIO + G45_DSPBCNTR);

		/* "VBLANK" delay */
		delay_ms(sd, 20);

		/* Stop sync */
		writel((readl(sd->Card.MMIO + G45_ADPA) & G45_ADPA_MASK) | G45_ADPA_DPMS_OFF, sd->Card.MMIO + G45_ADPA);

		/* Disable VGA */
		writel(readl(sd->Card.MMIO + G45_VGACNTRL) | G45_VGACNTRL_VGA_DISABLE, sd->Card.MMIO + G45_VGACNTRL);

		/* Clear PIPE status */
		D(bug("[GMA] Old PIPEA Status = %08x\n", readl(sd->Card.MMIO + 0x70024)));
		writel(0xffff, sd->Card.MMIO + 0x70024);

		/* Use all 96 fifo entries for PIPE A */
		writel(95 | (95 << 7), sd->Card.MMIO + 0x70030);

		/* unprotect some fields */
		writel(0xabcd0000, sd->Card.MMIO + 0x61204);

	//	tmp = readl(sd->Card.MMIO + G45_DPLLA_CTRL);
	//	D(bug("[GMA] dpll before=%08x\n", tmp));
	//	tmp &= ~G45_DPLL_VCO_ENABLE;
	//	D(bug("[GMA] writing dpll=%08x\n", tmp));
	//	writel(tmp, sd->Card.MMIO + G45_DPLLA_CTRL);
	//	D(bug("[GMA] dpll after=%08x\n", readl(sd->Card.MMIO + G45_DPLLA_CTRL)));

		delay_us(sd, 150);

	//	writel(state->dspsurf, sd->Card.MMIO + G45_DSPASURF);

		tmp = readl(sd->Card.MMIO + G45_PIPEACONF);
		writel(tmp & ~(3 << 18), sd->Card.MMIO + G45_PIPEACONF);
		writel(readl(sd->Card.MMIO + G45_DSPACNTR) | 0x80000000, sd->Card.MMIO + G45_DSPACNTR);
		writel(0, sd->Card.MMIO + G45_DSPASURF);
		writel(readl(sd->Card.MMIO + G45_DSPACNTR) & 0x7fffffff, sd->Card.MMIO + G45_DSPACNTR);
		writel(0, sd->Card.MMIO + G45_DSPASURF);
		writel(tmp, sd->Card.MMIO + G45_PIPEACONF);

		if (state->dpll & G45_DPLL_VCO_ENABLE)
		{
			writel(state->fp, sd->Card.MMIO + G45_FPA0);
			writel(state->fp, sd->Card.MMIO + G45_FPA1);
			writel(state->dpll & ~G45_DPLL_VCO_ENABLE, sd->Card.MMIO + G45_DPLLA_CTRL);
			(void)readl(sd->Card.MMIO + G45_DPLLA_CTRL);
			delay_ms(sd, 1);
			writel(state->dpll & ~G45_DPLL_VCO_ENABLE, sd->Card.MMIO + G45_DPLLA_CTRL);
			(void)readl(sd->Card.MMIO + G45_DPLLA_CTRL);
			delay_ms(sd, 1);
	//		writel(state->dpll & ~G45_DPLL_VCO_ENABLE, sd->Card.MMIO + G45_DPLLA_CTRL);
	//		(void)readl(sd->Card.MMIO + G45_DPLLA_CTRL);
	//		delay_ms(sd, 1);
	//		writel(state->dpll & ~G45_DPLL_VCO_ENABLE, sd->Card.MMIO + G45_DPLLA_CTRL);
	//		(void)readl(sd->Card.MMIO + G45_DPLLA_CTRL);
	//		delay_ms(sd, 1);
	//		writel(state->dpll & ~G45_DPLL_VCO_ENABLE, sd->Card.MMIO + G45_DPLLA_CTRL);
	//		(void)readl(sd->Card.MMIO + G45_DPLLA_CTRL);
	//		delay_ms(sd, 1);
	//		writel(state->dpll & ~G45_DPLL_VCO_ENABLE, sd->Card.MMIO + G45_DPLLA_CTRL);
	//		(void)readl(sd->Card.MMIO + G45_DPLLA_CTRL);
	//		delay_ms(sd, 1);
		}

		writel(state->fp, sd->Card.MMIO + G45_FPA0);
		writel(state->fp, sd->Card.MMIO + G45_FPA1);
		writel(state->dpll, sd->Card.MMIO + G45_DPLLA_CTRL);
		(void)readl(sd->Card.MMIO + G45_DPLLA_CTRL);
		delay_us(sd, 150);
		writel(state->dpll, sd->Card.MMIO + G45_DPLLA_CTRL);
		D(bug("[GMA] writing dpll=%08x, got %08x\n", state->dpll, readl(sd->Card.MMIO + G45_DPLLA_CTRL)));
		delay_us(sd, 150);

		/* protect on again */
		writel(0x00000000, sd->Card.MMIO + 0x61204);

		/* Enable DAC*/
		writel((readl(sd->Card.MMIO + G45_ADPA) & G45_ADPA_MASK) | G45_ADPA_DPMS_OFF | G45_ADPA_ENABLE, sd->Card.MMIO + G45_ADPA);

		writel(state->htotal, sd->Card.MMIO + G45_HTOTAL_A);
		writel(state->hblank, sd->Card.MMIO + G45_HBLANK_A);
		writel(state->hsync, sd->Card.MMIO + G45_HSYNC_A);
		writel(state->vtotal, sd->Card.MMIO + G45_VTOTAL_A);
		writel(state->vblank, sd->Card.MMIO + G45_VBLANK_A);
		writel(state->vsync, sd->Card.MMIO + G45_VSYNC_A);

		writel(state->pipesrc, sd->Card.MMIO + G45_PIPEASRC);
		writel(state->pipesrc, sd->Card.MMIO + 0x70190);
		writel(state->pipeconf, sd->Card.MMIO + G45_PIPEACONF);
		(void)readl(sd->Card.MMIO + G45_PIPEACONF);

		LONG offset = sd->VisibleBitmap->yoffset * sd->VisibleBitmap->pitch +
					  sd->VisibleBitmap->xoffset * sd->VisibleBitmap->bpp;

		writel(state->dspsurf, sd->Card.MMIO + G45_DSPASURF);
		writel(state->dspstride, sd->Card.MMIO + G45_DSPASTRIDE);
		writel(state->dsplinoff - offset , sd->Card.MMIO + G45_DSPALINOFF);

		/* Enable DAC */
		writel((readl(sd->Card.MMIO + G45_ADPA) & ~G45_ADPA_DPMS_MASK) | G45_ADPA_DPMS_ON, sd->Card.MMIO + G45_ADPA);

		/* Adjust Sync pulse polarity */
		writel((readl(sd->Card.MMIO + G45_ADPA) & ~(G45_ADPA_VSYNC_PLUS | G45_ADPA_HSYNC_PLUS)) | state->adpa, sd->Card.MMIO + G45_ADPA);

		D(bug("[GMA] Loaded state. dpll=%08x %08x %08x %08x %08x %08x %08x\n", state->dpll,
				readl(sd->Card.MMIO + G45_HTOTAL_A),
				readl(sd->Card.MMIO + G45_HBLANK_A),
				readl(sd->Card.MMIO + G45_HSYNC_A),
				readl(sd->Card.MMIO + G45_VTOTAL_A),
				readl(sd->Card.MMIO + G45_VBLANK_A),
				readl(sd->Card.MMIO + G45_VSYNC_A)));

		if (state->dpll != readl(sd->Card.MMIO + G45_DPLLA_CTRL))
		{
			D(bug("[GMA] DPLL mismatch!\n"));
			writel(state->dpll, sd->Card.MMIO + G45_DPLLA_CTRL);
			(void)readl(sd->Card.MMIO + G45_DPLLA_CTRL);
		}

		delay_ms(sd, 20);

		writel(state->dspcntr, sd->Card.MMIO + G45_DSPACNTR);
	}
	
	UNLOCK_HW
}

void G45_SaveState(struct g45staticdata *sd, GMAState_t *state)
{

}

IPTR AllocBitmapArea(struct g45staticdata *sd, ULONG width, ULONG height,
    ULONG bpp, BOOL must_have)
{
    IPTR result;

    LOCK_HW

    Forbid();
    result = (IPTR)Allocate(&sd->CardMem, 1024+((width * bpp + 63) & ~63) * height);
    Permit();

    if (result)
    	result +=512;

    D(bug("[GMA] AllocBitmapArea(%dx%d@%d) = %p\n",
	width, height, bpp, result));
    /*
	If Allocate failed, make the 0xffffffff as return. If it succeeded, make
	the memory pointer relative to the begin of GFX memory
    */
    if (result == 0) result--;
    else result -= (IPTR)sd->Card.Framebuffer;

    UNLOCK_HW

    /* Generic thing. Will be extended later */
    return result;
}

VOID FreeBitmapArea(struct g45staticdata *sd, IPTR bmp, ULONG width, ULONG height,
    ULONG bpp)
{
    APTR ptr = (APTR)(bmp + sd->Card.Framebuffer-512);

    LOCK_HW

    D(bug("[GMA] FreeBitmapArea(%p,%dx%d@%d)\n",
	bmp, width, height, bpp));

    Forbid();
    Deallocate(&sd->CardMem, ptr, 1024+((width * bpp + 63) & ~63) * height);
    Permit();

    UNLOCK_HW
}

BOOL adpa_Enabled(struct g45staticdata *sd)
{
	return ( readl( sd->Card.MMIO + G45_ADPA ) & G45_ADPA_ENABLE) ? TRUE : FALSE;
}

BOOL lvds_Enabled(struct g45staticdata *sd)
{
	return ( readl( sd->Card.MMIO + G45_LVDS ) & G45_LVDS_PORT_EN) ? TRUE : FALSE;
}

void GetSync(struct g45staticdata *sd,struct Sync *sync,ULONG pipe)
{
    ULONG htot =  readl(sd->Card.MMIO + (pipe == PIPE_A ? G45_HTOTAL_A : G45_HTOTAL_B));
    ULONG hsync = readl(sd->Card.MMIO + (pipe == PIPE_A ? G45_HSYNC_A : G45_HSYNC_B));
    ULONG vtot =  readl(sd->Card.MMIO + (pipe == PIPE_A ? G45_VTOTAL_A : G45_VTOTAL_B));
    ULONG vsync = readl(sd->Card.MMIO + (pipe == PIPE_A ? G45_VSYNC_A : G45_VSYNC_B));

    sync->pixelclock = 48; // dummy value
	sync->flags =0;
	
    sync->hdisp = (htot & 0xffff) + 1;
    sync->htotal = ((htot & 0xffff0000) >> 16) + 1;
    sync->hstart = (hsync & 0xffff) + 1;
    sync->hend = ((hsync & 0xffff0000) >> 16) + 1;
    sync->vdisp = (vtot & 0xffff) + 1;
    sync->vtotal = ((vtot & 0xffff0000) >> 16) + 1;
    sync->vstart = (vsync & 0xffff) + 1;
    sync->vend = ((vsync & 0xffff0000) >> 16) + 1;

	sync->width = sync->hdisp;
	sync->height = sync->vdisp;
	
    ULONG dsp_cntr = readl(sd->Card.MMIO + (sd->pipe == 0 ? G45_DSPACNTR : G45_DSPBCNTR));
	
    switch (dsp_cntr & G45_DSPCNTR_PIXEL_MASK) {
        case G45_DSPCNTR_8BPP:
            sync->depth = 8;
            break;
        case G45_DSPCNTR_16BPP:
            sync->depth = 16;
        break;
        case G45_DSPCNTR_32BPP:
            sync->depth = 32;
            break;
        default:
            bug("[GMA] GetSync: Unknown pixel format.\n");
    }
	bug("[GMA] GetSync: %d %d %d %d\n",
		sync->hdisp,
		sync->hstart,
		sync->hend,
		sync->htotal);

	bug("               %d %d %d %d\n",
		sync->vdisp,
		sync->vstart,
		sync->vend,
		sync->vtotal);
}
