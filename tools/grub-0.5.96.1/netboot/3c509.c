/**************************************************************************
ETHERBOOT -  BOOTP/TFTP Bootstrap Program

Author: Martin Renters.
  Date: Mar 22 1995

 This code is based heavily on David Greenman's if_ed.c driver and
  Andres Vega Garcia's if_ep.c driver.

 Copyright (C) 1993-1994, David Greenman, Martin Renters.
 Copyright (C) 1993-1995, Andres Vega Garcia.
 Copyright (C) 1995, Serge Babkin.
  This software may be used, modified, copied, distributed, and sold, in
  both source and binary form provided that the above copyright and these
  terms are retained. Under no circumstances are the authors responsible for
  the proper functioning of this software, nor do the authors assume any
  responsibility for damages incurred with its use.

3c509 support added by Serge Babkin (babkin@hq.icb.chel.su)

$Id$

***************************************************************************/

/* #define EDEBUG */

#include "etherboot.h"
#include "nic.h"
#include "cards.h"
#include "3c509.h"

static unsigned char	eth_vendor, eth_flags, eth_laar;
static unsigned short	eth_nic_base, eth_asic_base;
static char		bnc=0, utp=0; /* for 3C509 */

#ifdef	INCLUDE_3C529
/*
 * This table and several other pieces of the MCA support
 * code were shamelessly borrowed from the Linux kernel source.
 *
 * MCA support added by Adam Fritzler (mid@auk.cx)
 *
 */
struct el3_mca_adapters_struct {
        const char *name;
        int id;
};
static struct el3_mca_adapters_struct el3_mca_adapters[] = {
        { "3Com 3c529 EtherLink III (10base2)", 0x627c },
        { "3Com 3c529 EtherLink III (10baseT)", 0x627d },
        { "3Com 3c529 EtherLink III (test mode)", 0x62db },
        { "3Com 3c529 EtherLink III (TP or coax)", 0x62f6 },
        { "3Com 3c529 EtherLink III (TP)", 0x62f7 },
        { NULL, 0 },
};
#endif

/* a surrogate */

static void DELAY(int val)
{
	int c;

	for(c=0; c<val; c+=20) {
		twiddle();
	}
}

/**************************************************************************
ETH_RESET - Reset adapter
***************************************************************************/
static void t509_reset(struct nic *nic)
{
	int i;

	/***********************************************************
			Reset 3Com 509 card
	*************************************************************/

	if (eth_vendor != VENDOR_3C509)
		return;

	/* stop card */
	outw(RX_DISABLE, BASE + EP_COMMAND);
	outw(RX_DISCARD_TOP_PACK, BASE + EP_COMMAND);
	while (inw(BASE + EP_STATUS) & S_COMMAND_IN_PROGRESS);
	outw(TX_DISABLE, BASE + EP_COMMAND);
	outw(STOP_TRANSCEIVER, BASE + EP_COMMAND);
	outw(RX_RESET, BASE + EP_COMMAND);
	outw(TX_RESET, BASE + EP_COMMAND);
	outw(C_INTR_LATCH, BASE + EP_COMMAND);
	outw(SET_RD_0_MASK, BASE + EP_COMMAND);
	outw(SET_INTR_MASK, BASE + EP_COMMAND);
	outw(SET_RX_FILTER, BASE + EP_COMMAND);

	/*
	* initialize card
	*/
	while (inw(BASE + EP_STATUS) & S_COMMAND_IN_PROGRESS);

	GO_WINDOW(0);

	/* Disable the card */
	outw(0, BASE + EP_W0_CONFIG_CTRL);

	/* Configure IRQ to none */
	outw(SET_IRQ(0), BASE + EP_W0_RESOURCE_CFG);

	/* Enable the card */
	outw(ENABLE_DRQ_IRQ, BASE + EP_W0_CONFIG_CTRL);

	GO_WINDOW(2);

	/* Reload the ether_addr. */
	for (i = 0; i < ETHER_ADDR_SIZE; i++)
		outb(nic->node_addr[i], BASE + EP_W2_ADDR_0 + i);

	outw(RX_RESET, BASE + EP_COMMAND);
	outw(TX_RESET, BASE + EP_COMMAND);

	/* Window 1 is operating window */
	GO_WINDOW(1);
	for (i = 0; i < 31; i++)
		inb(BASE + EP_W1_TX_STATUS);

	/* get rid of stray intr's */
	outw(ACK_INTR | 0xff, BASE + EP_COMMAND);

	outw(SET_RD_0_MASK | S_5_INTS, BASE + EP_COMMAND);

	outw(SET_INTR_MASK, BASE + EP_COMMAND);

	outw(SET_RX_FILTER | FIL_INDIVIDUAL | FIL_BRDCST, BASE + EP_COMMAND);

	/* configure BNC */
	if (bnc) {
		outw(START_TRANSCEIVER, BASE + EP_COMMAND);
		DELAY(10000);
		}
	/* configure UTP */
	if (utp) {
		GO_WINDOW(4);
		outw(ENABLE_UTP, BASE + EP_W4_MEDIA_TYPE);
		GO_WINDOW(1);
		}

	/* start tranciever and receiver */
	outw(RX_ENABLE, BASE + EP_COMMAND);
	outw(TX_ENABLE, BASE + EP_COMMAND);

	/* set early threshold for minimal packet length */
	outw(SET_RX_EARLY_THRESH | ETH_MIN_PACKET, BASE + EP_COMMAND);

	outw(SET_TX_START_THRESH | 16, BASE + EP_COMMAND);
}

/**************************************************************************
ETH_TRANSMIT - Transmit a frame
***************************************************************************/
static char padmap[] = {
	0, 3, 2, 1};

static void t509_transmit(
struct nic *nic,
const char *d,			/* Destination */
unsigned int t,			/* Type */
unsigned int s,			/* size */
const char *p)			/* Packet */
{
	register unsigned int len;
	int pad;
	int status;

	if(eth_vendor != VENDOR_3C509)
		return;

#ifdef	EDEBUG
	printf("{l=%d,t=%x}",s+ETHER_HDR_SIZE,t);
#endif

	/* swap bytes of type */
	t= htons(t);

	len=s+ETHER_HDR_SIZE; /* actual length of packet */
	pad = padmap[len & 3];

	/*
	* The 3c509 automatically pads short packets to minimum ethernet length,
	* but we drop packets that are too large. Perhaps we should truncate
	* them instead?
	*/
	if (len + pad > ETH_MAX_PACKET) {
		return;
	}

	/* drop acknowledgements */
	while(( status=inb(BASE + EP_W1_TX_STATUS) )& TXS_COMPLETE ) {
		if(status & (TXS_UNDERRUN|TXS_MAX_COLLISION|TXS_STATUS_OVERFLOW)) {
			outw(TX_RESET, BASE + EP_COMMAND);
			outw(TX_ENABLE, BASE + EP_COMMAND);
		}

		outb(0x0, BASE + EP_W1_TX_STATUS);
	}

	while (inw(BASE + EP_W1_FREE_TX) < (unsigned short)len + pad + 4) {
		/* no room in FIFO */
	}

	outw(len, BASE + EP_W1_TX_PIO_WR_1);
	outw(0x0, BASE + EP_W1_TX_PIO_WR_1);	/* Second dword meaningless */

	/* write packet */
	outsw(BASE + EP_W1_TX_PIO_WR_1, d, ETHER_ADDR_SIZE/2);
	outsw(BASE + EP_W1_TX_PIO_WR_1, nic->node_addr, ETHER_ADDR_SIZE/2);
	outw(t, BASE + EP_W1_TX_PIO_WR_1);
	outsw(BASE + EP_W1_TX_PIO_WR_1, p, s / 2);
	if (s & 1)
		outb(*(p+s - 1), BASE + EP_W1_TX_PIO_WR_1);

	while (pad--)
		outb(0, BASE + EP_W1_TX_PIO_WR_1);	/* Padding */

	/* timeout after sending */
	DELAY(1000);
}

/**************************************************************************
ETH_POLL - Wait for a frame
***************************************************************************/
static int t509_poll(struct nic *nic)
{
	/* common variables */
	unsigned short type = 0;	/* used by EDEBUG */
	/* variables for 3C509 */
	short status, cst;
	register short rx_fifo;

	if(eth_vendor!=VENDOR_3C509)
		return 0;

	cst=inw(BASE + EP_STATUS);

#ifdef	EDEBUG
	if(cst & 0x1FFF)
		printf("-%x-",cst);
#endif

	if( (cst & S_RX_COMPLETE)==0 ) {
		/* acknowledge  everything */
		outw(ACK_INTR| (cst & S_5_INTS), BASE + EP_COMMAND);
		outw(C_INTR_LATCH, BASE + EP_COMMAND);

		return 0;
	}

	status = inw(BASE + EP_W1_RX_STATUS);
#ifdef	EDEBUG
	printf("*%x*",status);
#endif

	if (status & ERR_RX) {
		outw(RX_DISCARD_TOP_PACK, BASE + EP_COMMAND);
		return 0;
	}

	rx_fifo = status & RX_BYTES_MASK;
	if (rx_fifo==0)
		return 0;

		/* read packet */
#ifdef	EDEBUG
	printf("[l=%d",rx_fifo);
#endif
	insw(BASE + EP_W1_RX_PIO_RD_1, nic->packet, rx_fifo / 2);
	if(rx_fifo & 1)
		nic->packet[rx_fifo-1]=inb(BASE + EP_W1_RX_PIO_RD_1);
	nic->packetlen=rx_fifo;

	while(1) {
		status = inw(BASE + EP_W1_RX_STATUS);
#ifdef	EDEBUG
		printf("*%x*",status);
#endif
		rx_fifo = status & RX_BYTES_MASK;

		if(rx_fifo>0) {
			insw(BASE + EP_W1_RX_PIO_RD_1, nic->packet+nic->packetlen, rx_fifo / 2);
			if(rx_fifo & 1)
				nic->packet[nic->packetlen+rx_fifo-1]=inb(BASE + EP_W1_RX_PIO_RD_1);
			nic->packetlen+=rx_fifo;
#ifdef	EDEBUG
			printf("+%d",rx_fifo);
#endif
		}

		if(( status & RX_INCOMPLETE )==0) {
#ifdef	EDEBUG
			printf("=%d",nic->packetlen);
#endif
			break;
		}

		DELAY(1000);
	}

	/* acknowledge reception of packet */
	outw(RX_DISCARD_TOP_PACK, BASE + EP_COMMAND);
	while (inw(BASE + EP_STATUS) & S_COMMAND_IN_PROGRESS);
#ifdef	EDEBUG
	type = (nic->packet[12]<<8) | nic->packet[13];
	if(nic->packet[0]+nic->packet[1]+nic->packet[2]+nic->packet[3]+nic->packet[4]+
	    nic->packet[5] == 0xFF*ETHER_ADDR_SIZE)
		printf(",t=0x%x,b]",type);
	else
		printf(",t=0x%x]",type);
#endif
	return 1;
}


/*************************************************************************
	3Com 509 - specific routines
**************************************************************************/

static int
eeprom_rdy(void)
{
	int i;

	for (i = 0; is_eeprom_busy(IS_BASE) && i < MAX_EEPROMBUSY; i++);
	if (i >= MAX_EEPROMBUSY) {
		/* printf("3c509: eeprom failed to come ready.\n"); */
		printf("3c509: eeprom busy.\n"); /* memory in EPROM is tight */
		return (0);
	}
	return (1);
}

/*
 * get_e: gets a 16 bits word from the EEPROM. we must have set the window
 * before
 */
static int
get_e(int offset)
{
	if (!eeprom_rdy())
		return (0xffff);
	outw(EEPROM_CMD_RD | offset, IS_BASE + EP_W0_EEPROM_COMMAND);
	if (!eeprom_rdy())
		return (0xffff);
	return (inw(IS_BASE + EP_W0_EEPROM_DATA));
}

static int
send_ID_sequence(int port)
{
	int cx, al;

	for (al = 0xff, cx = 0; cx < 255; cx++) {
		outb(al, port);
		al <<= 1;
		if (al & 0x100)
			al ^= 0xcf;
	}
	return (1);
}


/*
 * We get eeprom data from the id_port given an offset into the eeprom.
 * Basically; after the ID_sequence is sent to all of the cards; they enter
 * the ID_CMD state where they will accept command requests. 0x80-0xbf loads
 * the eeprom data.  We then read the port 16 times and with every read; the
 * cards check for contention (ie: if one card writes a 0 bit and another
 * writes a 1 bit then the host sees a 0. At the end of the cycle; each card
 * compares the data on the bus; if there is a difference then that card goes
 * into ID_WAIT state again). In the meantime; one bit of data is returned in
 * the AX register which is conveniently returned to us by inb().  Hence; we
 * read 16 times getting one bit of data with each read.
 */
static int
get_eeprom_data(int id_port, int offset)
{
	int i, data = 0;
	outb(0x80 + offset, id_port);
	DELAY(10000);
	for (i = 0; i < 16; i++)
		data = (data << 1) | (inw(id_port) & 1);
	return (data);
}

static void t509_disable(struct nic *nic)
{
	outb(0xc0, EP_ID_PORT);
}

/**************************************************************************
ETH_PROBE - Look for an adapter
***************************************************************************/
#ifdef	INCLUDE_3C529
struct nic *t529_probe(struct nic *nic, unsigned short *probe_addrs)
#else
struct nic *t509_probe(struct nic *nic, unsigned short *probe_addrs)
#endif
{
	/* common variables */
	int i;
	int failcount;

#ifdef	INCLUDE_3C529
	struct el3_mca_adapters_struct *mcafound = NULL;
	int mca_pos4 = 0, mca_pos5 = 0, mca_irq = 0;
#endif

	t509_disable(nic);		/* in case board was active */
					/* note that nic is not used */

	for (failcount=0; failcount<4000; failcount++) {
		int data, j, io_base, id_port = EP_ID_PORT;
		unsigned short k;
		int ep_current_tag = EP_LAST_TAG + 1;
		short *p;
#ifdef	INCLUDE_3C529
		int curboard;
#endif

		id_port = EP_ID_PORT;
		ep_current_tag = EP_LAST_TAG + 1;
		eth_vendor = VENDOR_NONE;

	/*********************************************************
			Search for 3Com 509 card
	***********************************************************/
#ifdef	INCLUDE_3C529
		/*
		 * XXX: We should really check to make sure we have an MCA
		 * bus controller before going ahead with this...
		 *
		 * For now, we avoid any hassle by making it a compile
		 * time option.
		 *
		 */
		printf("\nWarning: Assuming presence of MCA bus\n");

                /* Make sure motherboard setup is off */
                outb_p(0xff, MCA_MOTHERBOARD_SETUP_REG);

		/* Cycle through slots */
		for(curboard=0; curboard<MCA_MAX_SLOT_NR; curboard++) {
			int boardid;
			int curcard;

			outb_p(0x8|(curboard&0xf), MCA_ADAPTER_SETUP_REG);

			boardid = inb_p(MCA_POS_REG(0));
			boardid += inb_p(MCA_POS_REG(1)) << 8;

			curcard = 0;
			while (el3_mca_adapters[curcard].name) {
				if (el3_mca_adapters[curcard].id == boardid) {
					mcafound = &el3_mca_adapters[curcard];

					mca_pos4 = inb_p(MCA_POS_REG(4));
					mca_pos5 = inb_p(MCA_POS_REG(5));

					goto donewithdetect;
				}
				else
					curcard++;
			}

		}
	donewithdetect:
		/* Kill all setup modes */
		outb_p(0, MCA_ADAPTER_SETUP_REG);

		if (mcafound) {
			eth_vendor = VENDOR_3C509;

			eth_nic_base = ((short)((mca_pos4&0xfc)|0x02)) << 8;
			mca_irq = mca_pos5 & 0x0f;
			ep_current_tag--;
		}
		else
			printf("MCA Card not found\n");
#endif
	/* Look for the EISA boards, leave them activated */
	/* search for the first card, ignore all others */
	for(j = 1; j < 16 && eth_vendor==VENDOR_NONE ; j++) {
		io_base = (j * EP_EISA_START) | EP_EISA_W0;
		if (inw(io_base + EP_W0_MFG_ID) != MFG_ID)
			continue;

		/* we must found 0x1f if the board is EISA configurated */
		if ((inw(io_base + EP_W0_ADDRESS_CFG) & 0x1f) != 0x1f)
			continue;

		/* Reset and Enable the card */
		outb(W0_P4_CMD_RESET_ADAPTER, io_base + EP_W0_CONFIG_CTRL);
		DELAY(10000); /* we must wait at least 10 ms */
		outb(W0_P4_CMD_ENABLE_ADAPTER, io_base + EP_W0_CONFIG_CTRL);

		/*
		 * Once activated, all the registers are mapped in the range
		 * x000 - x00F, where x is the slot number.
		 */
		eth_nic_base = j * EP_EISA_START;
		eth_vendor = VENDOR_3C509;
	}
	ep_current_tag--;

	/* Look for the ISA boards. Init and leave them actived */
	/* search for the first card, ignore all others */
	outb(0xc0, id_port);	/* Global reset */
	DELAY(10000);
	for (i = 0; i < EP_MAX_BOARDS && eth_vendor==VENDOR_NONE; i++) {
		outb(0, id_port);
		outb(0, id_port);
		send_ID_sequence(id_port);

		data = get_eeprom_data(id_port, EEPROM_MFG_ID);
		if (data != MFG_ID)
			break;

		/* resolve contention using the Ethernet address */
		for (j = 0; j < 3; j++)
			data = get_eeprom_data(id_port, j);

		eth_nic_base =
		    (get_eeprom_data(id_port, EEPROM_ADDR_CFG) & 0x1f) * 0x10 + 0x200;
		outb(ep_current_tag, id_port);	/* tags board */
		outb(ACTIVATE_ADAPTER_TO_CONFIG, id_port);
		eth_vendor = VENDOR_3C509;
		ep_current_tag--;
	}

	if(eth_vendor != VENDOR_3C509)
		goto no3c509;

	/*
	* The iobase was found and MFG_ID was 0x6d50. PROD_ID should be
	* 0x9[0-f]50
	*/
	GO_WINDOW(0);
	k = get_e(EEPROM_PROD_ID);
#ifdef	INCLUDE_3C529
	/*
	 * On MCA, the PROD_ID matches the MCA card ID (POS0+POS1)
	 */
	if (mcafound) {
		if (mcafound->id != k) {
			printf("MCA: PROD_ID in EEPROM does not match MCA card ID! (%x != %x)\n", k, mcafound->id);
			goto no3c509;
		}
	} else { /* for ISA/EISA */
		if ((k & 0xf0ff) != (PROD_ID & 0xf0ff))
			goto no3c509;
	}
#else
	if ((k & 0xf0ff) != (PROD_ID & 0xf0ff))
		goto no3c509;
#endif

#ifdef	INCLUDE_3C529
	if (mcafound) {
		printf("%s board found on MCA at 0x%x IRQ %d -",
		       mcafound->name, eth_nic_base, mca_irq);
	} else {
#endif
		if(eth_nic_base >= EP_EISA_START) {
			printf("3C5x9 board on EISA at 0x%x - ",eth_nic_base);
		} else {
			printf("3C5x9 board on ISA at 0x%x - ",eth_nic_base);
		}
#ifdef	INCLUDE_3C529
	}
#endif

	/* test for presence of connectors */
	i = inw(IS_BASE + EP_W0_CONFIG_CTRL);
	j = (inw(IS_BASE + EP_W0_ADDRESS_CFG) >> 14) & 0x3;

	switch(j) {
		case 0:
			if(i & IS_UTP) {
				printf("10baseT\n");
				utp=1;
				}
			else {
				printf("10baseT not present\n");
				eth_vendor=VENDOR_NONE;
				goto no3c509;
				}

			break;
		case 1:
			if(i & IS_AUI)
				printf("10base5\n");
			else {
				printf("10base5 not present\n");
				eth_vendor=VENDOR_NONE;
				goto no3c509;
				}

			break;
		case 3:
			if(i & IS_BNC) {
				printf("10base2\n");
				bnc=1;
				}
			else {
				printf("10base2 not present\n");
				eth_vendor=VENDOR_NONE;
				goto no3c509;
				}

			break;
		default:
			printf("unknown connector\n");
			eth_vendor=VENDOR_NONE;
			goto no3c509;
		}
	/*
	* Read the station address from the eeprom
	*/
	p = (unsigned short *) nic->node_addr;
	for (i = 0; i < 3; i++) {
		GO_WINDOW(0);
		p[i] = htons(get_e(i));
		GO_WINDOW(2);
		outw(ntohs(p[i]), BASE + EP_W2_ADDR_0 + (i * 2));
	}

	printf("Ethernet address: ");
	for(i=0; i<5; i++) {
		printf("%b:",nic->node_addr[i]);
	}
	printf("%b\n",nic->node_addr[i]);

	t509_reset(nic);
	nic->reset = t509_reset;
	nic->poll = t509_poll;
	nic->transmit = t509_transmit;
	nic->disable = t509_disable;
	return nic;
no3c509:
	eth_vendor = VENDOR_NONE;
	printf("(probe fail)");
	}
	return 0;
}

/*
 * Local variables:
 *  c-basic-offset: 8
 * End:
 */

