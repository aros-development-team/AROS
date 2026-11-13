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

int8_t FEC_PHY_Find(struct FECUnit *unit)
{
	int8_t phy = 0;
	int8_t result = -1;

	for (phy=0; phy < 32; phy++)
	{
		int stat = FEC_MDIO_Read(unit, phy, 1);
		if (stat != -1)
		{
			if (stat != 0xffff && stat != 0x0000)
			{
				int advert = FEC_MDIO_Read(unit, phy, 4);
				D(bug("[FEC] MII transceiver %d status %4.4x advertising %4.4x\n",
						phy, stat, advert));

				result = phy;

				break;
			}
		}
	}

	return result;
}

int FEC_PHY_Link(struct FECUnit *unit)
{
	uint16_t reg;

	FEC_MDIO_Read(unit, unit->feu_phy_id, PHY_BMSR);
	reg = FEC_MDIO_Read(unit, unit->feu_phy_id, PHY_BMSR);

	if (reg & PHY_BMSR_LS)
		return 1;
	else
		return 0;
}

/*
 * FEC_PHY_Reset - Try to reset the PHY. Returns 0 on failure.
 */
int FEC_PHY_Reset(struct FECUnit *unit)
{
	uint16_t reg;
	uint32_t loop_cnt;

	/* set the reset signal. It should go away automaticaly within 0.5 seconds */
	reg = FEC_MDIO_Read(unit, unit->feu_phy_id, PHY_BMCR) | PHY_BMCR_RESET;
	FEC_MDIO_Write(unit, unit->feu_phy_id, PHY_BMCR, reg);

	loop_cnt = 0;

	/* Wait until either BMCR_RESET goes away or the timeout (0.5s) occurs */
	while((reg & PHY_BMCR_RESET) && loop_cnt++ < 1000)
	{
		reg = FEC_MDIO_Read(unit, unit->feu_phy_id, PHY_BMCR);
		FEC_UDelay(unit, 500);
	}

	if (reg & PHY_BMCR_RESET)
	{
		D(bug("[FEC] PHY Reset timed out\n"));
		return 0;
	}
	else
		return 1;
}

/* Get the PHY link speed */
int FEC_PHY_Speed(struct FECUnit *unit)
{
	uint16_t bmcr, anlpar;

	bmcr = FEC_MDIO_Read(unit, unit->feu_phy_id, PHY_BMCR);

	/* Check if auto negotiation is enabled */
	if (bmcr & PHY_BMCR_AUTON)
	{
		/* Get the autonegotiation result */
		anlpar = FEC_MDIO_Read(unit, unit->feu_phy_id, PHY_ANLPAR);

		return (anlpar & PHY_ANLPAR_100) ? _100BASET : _10BASET;
	}
	else
		return (bmcr & PHY_BMCR_100MB) ? _100BASET : _10BASET;
}

/* Check duplex */
int FEC_PHY_Duplex(struct FECUnit *unit)
{
	uint16_t bmcr, anlpar;

	bmcr = FEC_MDIO_Read(unit, unit->feu_phy_id, PHY_BMCR);

	/* Is autonegotiation enabled? */
	if (bmcr & PHY_BMCR_AUTON)
	{
		anlpar = FEC_MDIO_Read(unit, unit->feu_phy_id, PHY_ANLPAR);

		return (anlpar & (PHY_ANLPAR_10FD | PHY_ANLPAR_TXFD)) ? FULL : HALF;
	}

	return (bmcr & PHY_BMCR_DPLX) ? FULL : HALF;
}


/* Initiate autonegotiaition */
void FEC_PHY_Setup_Autonegotiation(struct FECUnit *unit)
{
	uint16_t bmcr;
	uint16_t adv;

	adv = FEC_MDIO_Read(unit, unit->feu_phy_id, PHY_ANAR);
	adv |= (PHY_ANLPAR_ACK | PHY_ANLPAR_RF | PHY_ANLPAR_T4 |
            PHY_ANLPAR_TXFD | PHY_ANLPAR_TX | PHY_ANLPAR_10FD |
            PHY_ANLPAR_10);
	FEC_MDIO_Write(unit, unit->feu_phy_id, PHY_ANAR, adv);

	adv = FEC_MDIO_Read(unit, unit->feu_phy_id, PHY_1000BTCR);
	adv |= 0x0300;
	FEC_MDIO_Write(unit, unit->feu_phy_id, PHY_1000BTCR, adv);

	/* Start/restart negotiation */
	bmcr = FEC_MDIO_Read(unit, unit->feu_phy_id, PHY_BMCR);
	bmcr |= (PHY_BMCR_AUTON | PHY_BMCR_RST_NEG);
	FEC_MDIO_Write(unit, unit->feu_phy_id, PHY_BMCR, bmcr);
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
