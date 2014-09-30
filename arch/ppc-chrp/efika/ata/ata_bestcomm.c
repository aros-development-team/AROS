/*
    Copyright © 2009-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <aros/debug.h>
#include <asm/mpc5200b.h>
#include <inttypes.h>



#define MAX_DMA_BUFFERS		4
#define MAX_DMA_TRANSFER	256*512

/* Buffer descriptor */
typedef struct {
	uint32_t	status;
	void		*data1;
	void		*data2;
} ata_bd2_t;

#define SDMA_FLAGS_NONE		0x0000
#define SDMA_FLAGS_ENABLE	0x0001
#define SDMA_FLAGS_BD2		0x0002
#define SDMA_BD_READY		0x40000000UL

typedef struct {
	uint32_t	enable;
	uint32_t	bd_base;
	uint32_t	bd_last;
	uint32_t	bd_start;
	uint32_t	buffer_size;
} ata_ivar_t;

typedef struct {
	int16_t	pad0;
	int16_t	incr_bytes;
	int16_t	pad1;
	int16_t	incr_dst;
	int16_t	pad2;
	int16_t	incr_src;
} ata_inc_t;

uint8_t *sram = NULL;
volatile bestcomm_t *bestcomm = NULL;
volatile bestcomm_tdt_t *tdt = NULL;
uint32_t bestcomm_taskid = 0xffffffff;
volatile uint32_t *bestcomm_taskcode;
volatile uint32_t *bestcomm_vartable;
volatile ata_bd2_t *bd2;
uint16_t idx, odx, num_bd;
uint32_t flags;

volatile ata_ivar_t *ivar;
volatile ata_inc_t	*inc;

void bestcomm_init()
{
	D(bug("[ATA] bestcomm_init()\n"));

	tdt = (bestcomm_tdt_t *)bestcomm->bc_taskBar;

	D(bug("[ATA] taskBar at %08x\n", tdt));

	bestcomm_taskcode = (tdt[bestcomm_taskid].start);
	bestcomm_vartable = (tdt[bestcomm_taskid].var);

	ivar = (ata_ivar_t *)bestcomm_vartable;
	inc = (ata_inc_t *)(bestcomm_vartable + 24);

	D(bug("[ATA] looking on ivar at %08x:\n", ivar));
	D(bug("[ATA]   enable=%08x bd_base=%08x bd_last=%08x bd_start=%08x buffer_size=%08x\n",
			ivar->enable, ivar->bd_base, ivar->bd_last, ivar->bd_start, ivar->buffer_size));
}
