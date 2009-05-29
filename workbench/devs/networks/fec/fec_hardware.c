/*
 * fec_hardware.c
 *
 *  Created on: May 21, 2009
 *      Author: misc
 */
#define DEBUG 1

#include <aros/debug.h>

#include <asm/io.h>


#include <proto/exec.h>
#include <inttypes.h>

#include "fec.h"

void FEC_UDelay(struct FECUnit *unit, uint32_t usec)
{
    unit->feu_TimerPort.mp_SigTask = FindTask(NULL);
    unit->feu_TimerRequest.tr_node.io_Command = TR_ADDREQUEST;
    unit->feu_TimerRequest.tr_time.tv_secs = usec / 1000000;
    unit->feu_TimerRequest.tr_time.tv_micro = usec % 1000000;

    DoIO((struct IORequest *)&unit->feu_TimerRequest);
}

int FEC_MDIO_Read(struct FECUnit *unit, int32_t phy_id, int32_t reg)
{
	int tries = 100;
	uint32_t request = FEC_MII_READ_FRAME;

	outl_be(FEC_IEVENT_MII, &unit->feu_regs->ievent);

	request |= (phy_id << FEC_MII_DATA_PA_SHIFT) & FEC_MII_DATA_PA_MSK;
	request |= (reg << FEC_MII_DATA_RA_SHIFT) & FEC_MII_DATA_RA_MSK;

	outl_be(request, &unit->feu_regs->mii_data);

	while(!(inl_be(&unit->feu_regs->ievent) & FEC_IEVENT_MII) && --tries)
	{
		FEC_UDelay(unit, 10);
	}

	if (tries == 0)
		return -1;

	return inl_be(&unit->feu_regs->mii_data) & FEC_MII_DATA_DATAMSK;
}

int FEC_MDIO_Write(struct FECUnit *unit, int32_t phy_id, int32_t reg, uint16_t data)
{
	uint32_t value = data;
	int tries = 100;

	outl_be(FEC_IEVENT_MII, &unit->feu_regs->ievent);

	value |= FEC_MII_WRITE_FRAME;
	value |= (phy_id << FEC_MII_DATA_PA_SHIFT) & FEC_MII_DATA_PA_MSK;
	value |= (reg << FEC_MII_DATA_RA_SHIFT) & FEC_MII_DATA_RA_MSK;

	outl_be(value, &unit->feu_regs->mii_data);

	while(!(inl_be(&unit->feu_regs->ievent) & FEC_IEVENT_MII) && --tries)
	{
		FEC_UDelay(unit, 10);
	}

	if (tries == 0)
		return -1;
	else
		return 0;
}

void FEC_PHY_Init(struct FECUnit *unit)
{
	/* Don't do much. Just adjust the MII speed */
	outl_be(unit->feu_phy_speed, &unit->feu_regs->mii_speed);
}

void FEC_HW_Init(struct FECUnit *unit)
{
	int i;

	/* Reset the hardware */
	outl_be(FEC_ECNTRL_RESET, &unit->feu_regs->ecntrl);
	for (i=0; i < 20; i++)
	{
		if ((inl_be(&unit->feu_regs->ecntrl) & FEC_ECNTRL_RESET) == 0)
			break;

		FEC_UDelay(unit, 10);
	}

	/* set pause to 0x20 frames */
	outl_be(FEC_OP_PAUSE_OPCODE | 0x20, &unit->feu_regs->op_pause);

    /* high service request will be deasserted when there's < 7 bytes in fifo
     * low service request will be deasserted when there's < 4*7 bytes in fifo
     */
    outl_be(FEC_FIFO_CNTRL_FRAME | FEC_FIFO_CNTRL_LTG_7, &unit->feu_regs->rfifo_cntrl);
    outl_be(FEC_FIFO_CNTRL_FRAME | FEC_FIFO_CNTRL_LTG_7, &unit->feu_regs->tfifo_cntrl);

    /* alarm when <= x bytes in FIFO */
    outl_be(0x0000030c, &unit->feu_regs->rfifo_alarm);
    outl_be(0x00000100, &unit->feu_regs->tfifo_alarm);

    /* begin transmittion when 256 bytes are in FIFO (or EOF or FIFO full) */
    outl_be(FEC_FIFO_WMRK_256B, &unit->feu_regs->x_wmrk);

    /* enable crc generation */
    outl_be(FEC_XMIT_FSM_APPEND_CRC | FEC_XMIT_FSM_ENABLE_CRC, &unit->feu_regs->xmit_fsm);
    outl_be(0x00000000, &unit->feu_regs->iaddr1);     /* No individual filter */
    outl_be(0x00000000, &unit->feu_regs->iaddr2);     /* No individual filter */

    FEC_PHY_Init(unit);
}

void FEC_Reset_Stats(struct FECUnit *unit)
{
	uint32_t *ptr = &unit->feu_regs->rmon_t_drop;

	outl_be(FEC_MIB_DISABLE, &unit->feu_regs->mib_control);
	while (ptr < &unit->feu_regs->reserved10[0])
	{
		outl_be(0, ptr);
		ptr++;
	}
	outl_be(0, &unit->feu_regs->mib_control);
}
