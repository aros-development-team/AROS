/*
 * intelG45_lowlevel.c
 *
 *  Created on: May 2, 2010
 *      Author: misc
 */

#define DEBUG 1
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

		state->fp = clock.N << 16 | clock.M1 << 8 | clock.M2;

		state->dpll = (6 << G45_DPLL_PHASE_SHIFT) | G45_DPLL_VGA_MODE_DISABLE | G45_DPLL_MODE_DAC_SERIAL;
		if (clock.P2 >= 10)
			state->dpll |= G45_DPLL_DAC_SERIAL_P2_DIV_10;
		else
			state->dpll |= G45_DPLL_DAC_SERIAL_P2_DIV_5;
		state->dpll |= (1 << (clock.P1 - 1)) << G45_DPLL_P1_SHIFT;


		state->dspcntr = 0; // <- TODO!
		state->pipeconf = 0; // <- TODO!

	}
}

void G45_LoadState(struct g45staticdata *sd, GMAState_t *state)
{

}

void G45_SaveState(struct g45staticdata *sd, GMAState_t *state)
{

}
