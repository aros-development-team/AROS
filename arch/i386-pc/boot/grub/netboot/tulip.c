/*
    Tulip and clone Etherboot Driver
    By Marty Connor (mdc@thinguin.org)
    This software may be used and distributed according to the terms
    of the GNU Public License, incorporated herein by reference.

    Based on Ken Yap's Tulip Etherboot Driver and Donald Becker's
    Linux Tulip Driver. Supports N-Way speed auto-configuration on
    MX98715, MX98715A and MX98725. Support inexpensive PCI 10/100 cards
    based on the Macronix MX987x5 chip, such as the SOHOware Fast
    model SFA110A, and the LinkSYS model LNE100TX. The NetGear
    model FA310X, based on the LC82C168 chip is supported.
    The TRENDnet TE100-PCIA NIC which uses a genuine Intel 21143-PD
    chipset is supported.
    Also, Davicom DM9102's.

    Documentation and source code used:
      Source for Etherboot driver at
        http://etherboot.sourceforge.net/
      MX98715A Data Sheet and MX98715A Application Note
        on http://www.macronix.com/  (PDF format files)
      Source for Linux tulip driver at
        http://cesdis.gsfc.nasa.gov/linux/drivers/tulip.html

    Adapted by Ken Yap from
    FreeBSD netboot DEC 21143 driver
    Author: David Sharp
      date: Nov/98

    Some code fragments were taken from verious places, Ken Yap's
    etherboot, FreeBSD's if_de.c, and various Linux related files.
    DEC's manuals for the 21143 and SROM format were very helpful.
    The Linux de driver development page has a number of links to
    useful related information.  Have a look at:
    ftp://cesdis.gsfc.nasa.gov/pub/linux/drivers/tulip-devel.html
*/

/*********************************************************************/
/* Revision History                                                  */
/*********************************************************************/

/*
  16 Jul 2000  mdc     0.75b11
     Added support for ADMtek 0985 Centaur-P, a "Comet" tulip clone
     which is used on the LinkSYS LNE100TX v4.x cards.  We already
     support LNE100TX v2.0 cards, which use a different controller.
  04 Jul 2000	jam     ?
     Added test of status after receiving a packet from the card.
     Also uncommented the tulip_disable routine.  Stray packets
     seemed to be causing problems.
  27 Apr 2000	njl	?
  29 Feb 2000   mdc     0.75b7
     Increased reset delay to 3 seconds because Macronix cards seem to
     need more reset time before card comes back to a usable state.
  26 Feb 2000   mdc     0.75b6
     Added a 1 second delay after initializing the transmitter because
     some cards seem to need the time or they drop the first packet 
     transmitted.
  23 Feb 2000   mdc     0.75b5
     removed udelay code and used currticks() for more reliable delay
     code in reset pause and sanity timeouts.  Added function prototypes
     and TX debugging code.
  21 Feb 2000   mdc     patch to Etherboot 4.4.3
     Incorporated patches from Bob Edwards and Paul Mackerras of 
     Linuxcare's OZLabs to deal with inefficiencies in tulip_transmit
     and udelay.  We now wait for packet transmission to complete
     (or sanity timeout).
  04 Feb 2000   Robert.Edwards@anu.edu.au patch to Etherboot 4.4.2
     patch to tulip.c that implements the automatic selection of the MII
     interface on cards using the Intel/DEC 21143 reference design, in
     particular, the TRENDnet TE100-PCIA NIC which uses a genuine Intel
     21143-PD chipset.
  11 Jan 2000   mdc     0.75b4
     Added support for NetGear FA310TX card based on the LC82C168
     chip.  This should also support Lite-On LC82C168 boards.
     Added simple MII support. Re-arranged code to better modularize
     initializations.
  04 Dec 1999   mdc     0.75b3
     Added preliminary support for LNE100TX PCI cards.  Should work for
     PNIC2 cards. No MII support, but single interface (RJ45) tulip
     cards seem to not care.
  03 Dec 1999   mdc     0.75b2
     Renamed from mx987x5 to tulip, merged in original tulip init code
     from tulip.c to support other tulip compatible cards.
  02 Dec 1999   mdc     0.75b1
     Released Beta MX987x5 Driver for code review and testing to netboot
     and thinguin mailing lists.
*/


/*********************************************************************/
/* Declarations                                                      */
/*********************************************************************/

#include "etherboot.h"
#include "nic.h"
#include "pci.h"
#include "cards.h"

#undef TULIP_DEBUG
#undef TULIP_DEBUG_WHERE

#define TX_TIME_OUT       2*TICKS_PER_SEC

typedef unsigned char  u8;
typedef   signed char  s8;
typedef unsigned short u16;
typedef   signed short s16;
typedef unsigned int   u32;
typedef   signed int   s32;

/* Register offsets for tulip device */
enum tulip_offsets {
   CSR0=0,     CSR1=0x08,  CSR2=0x10,  CSR3=0x18,  CSR4=0x20,  CSR5=0x28,
   CSR6=0x30,  CSR7=0x38,  CSR8=0x40,  CSR9=0x48, CSR10=0x50, CSR11=0x58,
  CSR12=0x60, CSR13=0x68, CSR14=0x70, CSR15=0x78, CSR16=0x80, CSR20=0xA0
};

#define DEC_21142_CSR6_TTM	0x00400000	/* Transmit Threshold Mode */
#define DEC_21142_CSR6_HBD	0x00080000	/* Heartbeat Disable */
#define DEC_21142_CSR6_PS	0x00040000	/* Port Select */

/* EEPROM Address width definitions */
#define EEPROM_ADDRLEN 6
#define EEPROM_SIZE    128              /* 2 << EEPROM_ADDRLEN */

/* Data Read from the EEPROM */
static unsigned char ee_data[EEPROM_SIZE];

/* The EEPROM commands include the alway-set leading bit. */
#define EE_WRITE_CMD    (5 << addr_len)
#define EE_READ_CMD     (6 << addr_len)
#define EE_ERASE_CMD    (7 << addr_len)

/* EEPROM_Ctrl bits. */
#define EE_SHIFT_CLK    0x02    /* EEPROM shift clock. */
#define EE_CS           0x01    /* EEPROM chip select. */
#define EE_DATA_WRITE   0x04    /* EEPROM chip data in. */
#define EE_WRITE_0      0x01
#define EE_WRITE_1      0x05
#define EE_DATA_READ    0x08    /* EEPROM chip data out. */
#define EE_ENB          (0x4800 | EE_CS)

/* Delay between EEPROM clock transitions.  Even at 33Mhz current PCI
   implementations don't overrun the EEPROM clock.  We add a bus
   turn-around to insure that this remains true.  */
#define eeprom_delay()  inl(ee_addr)

/* helpful macro if on a big_endian machine for changing byte order.
   not strictly needed on Intel */
#define le16_to_cpu(val) (val)

/* transmit and receive descriptor format */
struct txdesc {
  volatile unsigned long   status;         /* owner, status */
  unsigned long   buf1sz:11,      /* size of buffer 1 */
    buf2sz:11,                    /* size of buffer 2 */
    control:10;                   /* control bits */
  const unsigned char *buf1addr;  /* buffer 1 address */
  const unsigned char *buf2addr;  /* buffer 2 address */
};

struct rxdesc {
  volatile unsigned long   status;         /* owner, status */
  unsigned long   buf1sz:11,      /* size of buffer 1 */
    buf2sz:11,                    /* size of buffer 2 */
    control:10;                   /* control bits */
  unsigned char   *buf1addr;      /* buffer 1 address */
  unsigned char   *buf2addr;      /* buffer 2 address */
};

/* Size of transmit and receive buffers */
#define BUFLEN 1536

/*********************************************************************/
/* Global Storage                                                    */
/*********************************************************************/

/* PCI Bus parameters */
static unsigned short vendor, dev_id;
static unsigned long ioaddr;

/* Note: transmit and receive buffers must be longword aligned and
   longword divisable */

/* transmit descriptor and buffer */
static struct txdesc txd __attribute__ ((aligned(4)));
#ifndef	USE_INTERNAL_BUFFER
#define txb ((char *)0x10000 - BUFLEN)
#else
static unsigned char txb[BUFLEN] __attribute__ ((aligned(4)));
#endif

/* receive descriptor(s) and buffer(s) */
#define NRXD 4
static struct rxdesc rxd[NRXD] __attribute__ ((aligned(4)));
#ifndef	USE_INTERNAL_BUFFER
#define rxb ((char *)0x10000 - NRXD * BUFLEN - BUFLEN)
#else
static unsigned char rxb[NRXD * BUFLEN] __attribute__ ((aligned(4)));
#endif
static int rxd_tail;

/* buffer for ethernet header */
static unsigned char ehdr[ETHER_HDR_SIZE];


/*********************************************************************/
/* Function Prototypes                                               */
/*********************************************************************/
static void whereami(const char *str);
static int lc82c168_mdio_read(int phy_id, int location);
static void lc82c168_mdio_write(int phy_id, int location, int value);
static void lc82c168_do_mii();
static int read_eeprom(unsigned long ioaddr, int location, int addr_len);
struct nic *tulip_probe(struct nic *nic, unsigned short *io_addrs,
			struct pci_device *pci);
static void tulip_init_ring(struct nic *nic);
static void tulip_reset(struct nic *nic);
static void tulip_transmit(struct nic *nic, const char *d, unsigned int t,
			   unsigned int s, const char *p);
static int tulip_poll(struct nic *nic);
static void tulip_disable(struct nic *nic);
static void whereami (const char *str);
#ifdef	TULIP_DEBUG
static void tulip_more(void);
#endif /* TULIP_DEBUG */
static void tulip_wait(unsigned int nticks);


/*********************************************************************/
/* Utility Routines                                                  */
/*********************************************************************/

static inline void whereami (const char *str)
{
#ifdef	TULIP_DEBUG_WHERE
  printf("%s\n", str);
  /* sleep(2); */
#endif
}

#ifdef	TULIP_DEBUG
static void tulip_more()
{
  printf("\n\n-- more --");
  while (!iskey())
    /* wait */;
  getchar();
  printf("\n\n");
}
#endif /* TULIP_DEBUG */

static void tulip_wait(unsigned int nticks)
{
  unsigned int to = currticks() + nticks;
  while (currticks() < to)
    /* wait */ ;
}


/*********************************************************************/
/* Media Descriptor Code                                             */
/*********************************************************************/
static int lc82c168_mdio_read(int phy_id, int location)
{
  int i = 1000;
  int retval = 0;

  whereami("mdio_read\n");

  outl(0x60020000 | (phy_id<<23) | (location<<18), ioaddr + 0xA0);
  inl(ioaddr + 0xA0);
  inl(ioaddr + 0xA0);
  while (--i > 0)
    if ( ! ((retval = inl(ioaddr + 0xA0)) & 0x80000000))
      return retval & 0xFFFF;
  if (i == 0) printf("mdio read timeout!\n");
  return 0xFFFF;
}

static void lc82c168_mdio_write(int phy_id, int location, int value)
{
  int i = 1000;
  int cmd = (0x5002 << 16) | (phy_id << 23) | (location<<18) | value;

  whereami("mdio_write\n");

  outl(cmd, ioaddr + 0xA0);
  do
    if ( ! (inl(ioaddr + 0xA0) & 0x80000000))
      break;
  while (--i > 0);
  if (i == 0) printf("mdio write timeout!\n");
  return;
}

static void lc82c168_do_mii(void)
{
  int phy, phy_idx;

  whereami("do_mii\n");

  for (phy = 0, phy_idx = 0; phy < 32 && phy_idx < 4; phy++) {

    int mii_status = lc82c168_mdio_read(phy, 1);

    if ((mii_status & 0x8301) == 0x8001 ||
        ((mii_status & 0x8000) == 0  && (mii_status & 0x7800) != 0)) {

      int mii_reg0 = lc82c168_mdio_read(phy, 0);
      int mii_advert = lc82c168_mdio_read(phy, 4);
      int mii_reg4 = ((mii_status >> 6) & 0x01E1) | 1;

      phy_idx++;
      printf("%s:  MII trcvr #%d "
             "config %x status %x advertising %x reg4 %x.\n",
             "LC82C168", phy, mii_reg0, mii_status, mii_advert, mii_reg4);

      lc82c168_mdio_write(phy, 0, mii_reg0 | 0x1000);
      if (mii_advert != mii_reg4)
        lc82c168_mdio_write(phy, 4, mii_reg4);
    }
  }
#ifdef	TULIP_DEBUG
  printf("mii_cnt = %d\n", phy_idx);
#endif

}


/*********************************************************************/
/* EEPROM Reading Code                                               */
/*********************************************************************/
/* EEPROM routines adapted from the Linux Tulip Code */
/* Reading a serial EEPROM is a "bit" grungy, but we work our way
   through:->.
*/
static int read_eeprom(unsigned long ioaddr, int location, int addr_len)
{
  int i;
  unsigned short retval = 0;
  long ee_addr = ioaddr + CSR9;
  int read_cmd = location | EE_READ_CMD;

  whereami("read_eeprom\n");

  outl(EE_ENB & ~EE_CS, ee_addr);
  outl(EE_ENB, ee_addr);

  /* Shift the read command bits out. */
  for (i = 4 + addr_len; i >= 0; i--) {
    short dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
    outl(EE_ENB | dataval, ee_addr);
    eeprom_delay();
    outl(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
    eeprom_delay();
  }
  outl(EE_ENB, ee_addr);

  for (i = 16; i > 0; i--) {
    outl(EE_ENB | EE_SHIFT_CLK, ee_addr);
    eeprom_delay();
    retval = (retval << 1) | ((inl(ee_addr) & EE_DATA_READ) ? 1 : 0);
    outl(EE_ENB, ee_addr);
    eeprom_delay();
  }

  /* Terminate the EEPROM access. */
  outl(EE_ENB & ~EE_CS, ee_addr);
  return retval;
}

/*********************************************************************/
/* tulip_init_ring - setup the tx and rx descriptors                */
/*********************************************************************/
static void tulip_init_ring(struct nic *nic)
{
  int i;

  /* setup the transmit descriptor */
  txd.buf1addr = &txb[0];
  txd.buf2addr = &txb[0];         /* just in case */
  txd.buf1sz   = 192;             /* setup packet must be 192 bytes */
  txd.buf2sz   = 0;
  txd.control  = 0x028;           /* setup packet + TER */
  txd.status   = 0x80000000;      /* give ownership to device */

  /* construct perfect filter frame with mac address as first match
     and broadcast address for all others */
  for (i=0; i<192; i++) txb[i] = 0xFF;
  txb[0] = nic->node_addr[0];
  txb[1] = nic->node_addr[1];
  txb[4] = nic->node_addr[2];
  txb[5] = nic->node_addr[3];
  txb[8] = nic->node_addr[4];
  txb[9] = nic->node_addr[5];

  /* setup receive descriptor */
  for (i=0; i<NRXD; i++) {
    rxd[i].buf1addr = &rxb[i * BUFLEN];
    rxd[i].buf2addr = 0;        /* not used */
    rxd[i].buf1sz   = BUFLEN;
    rxd[i].buf2sz   = 0;        /* not used */
    rxd[i].control  = 0x0;
    rxd[i].status   = 0x80000000;   /* give ownership to device */
  }

  /* Set Receive end of ring on last descriptor */
  rxd[NRXD - 1].control = 0x008;
  rxd_tail = 0;
}

/*********************************************************************/
/* eth_reset - Reset adapter                                         */
/*********************************************************************/
static void tulip_reset(struct nic *nic)
{
  unsigned long to, csr6;
  u32 addr_low, addr_high;

  whereami("tulip_reset\n");

  /* Stop Tx and RX */
  outl(inl(ioaddr + CSR6) & ~0x00002002, ioaddr + CSR6);

  if (vendor == PCI_VENDOR_ID_MACRONIX && dev_id == PCI_DEVICE_ID_MX987x5) {
    /* set up 10-BASE-T Control Port */
    outl(0xFFFFFFFF, ioaddr + CSR14);
    /* set up 10-BASE-T Status Port  */
    outl(0x00001000, ioaddr + CSR12);
    /* Set Operation Control Register (CSR6) for MX987x5
       to allow N-Way Active Speed selection, and
       start the chip's Tx to process setup frame.
       While it is possible to force speed selection,
       this is probably more useful most of the time.
    */
    outl(0x01A80200, ioaddr + CSR6);

  } else if (vendor == PCI_VENDOR_ID_LINKSYS && dev_id == PCI_DEVICE_ID_LC82C115) {
    /* This is MX987x5 init code. It seems to work for the LNE100TX
       but should be replaced when we figure out the right way
       to do this initialization
    */
    outl(0xFFFFFFFF, ioaddr + CSR14);
    outl(0x00001000, ioaddr + CSR12);
    outl(0x01A80200, ioaddr + CSR6);

  } else if (vendor == PCI_VENDOR_ID_LINKSYS && dev_id == PCI_DEVICE_ID_DEC_TULIP) {

    lc82c168_do_mii();

  } else if (vendor == PCI_VENDOR_ID_DEC && dev_id == PCI_DEVICE_ID_DEC_21142) {
    /* nothing */

  } else if (vendor == PCI_VENDOR_ID_ADMTEK && dev_id == PCI_DEVICE_ID_ADMTEK_0985) {
    /* nothing */

  } else {
    /* If we don't know what to do with the card, set to 10Mbps half-duplex */

    outl(0x00000000, ioaddr + CSR13);
    outl(0x7F3F0000, ioaddr + CSR14);
    outl(0x08000008, ioaddr + CSR15);
    outl(0x00000000, ioaddr + CSR13);
    outl(0x00000001, ioaddr + CSR13);
    outl(0x02404000, ioaddr + CSR6);
    outl(0x08AF0008, ioaddr + CSR15);
    outl(0x00050008, ioaddr + CSR15);

  }

  /* Reset the chip, holding bit 0 set at least 50 PCI cycles. */
  outl(0x00000001, ioaddr + CSR0);

  tulip_wait(3*TICKS_PER_SEC);

  /* turn off reset and set cache align=16lword, burst=unlimit */
  outl(0x01A08000, ioaddr + CSR0);

  /* set up transmit and receive descriptors */
  tulip_init_ring(nic);

  /* set up multicast hash address for Comet (ADKTEK 0985) */
  /* possibly not needed for Etherboot, but seems to do no harm  -mdc */
  if (vendor == PCI_VENDOR_ID_ADMTEK && dev_id == PCI_DEVICE_ID_ADMTEK_0985) {
    addr_low  = nic->node_addr[0] + (nic->node_addr[1] << 8)
      + (nic->node_addr[2] << 16) + (nic->node_addr[3] << 24);
    addr_high = nic->node_addr[4] + (nic->node_addr[5] << 8);
    outl(addr_low,  ioaddr + 0xA4);
    outl(addr_high, ioaddr + 0xA8);
    outl(0, ioaddr + 0xAC);
    outl(0, ioaddr + 0xB0);
  }

  /* Point to receive descriptor */
  outl((unsigned long)&rxd[0], ioaddr + CSR3);
  outl((unsigned long)&txd   , ioaddr + CSR4);

  csr6 = 0x02404000;

  /* Chip specific init code */

  if (vendor == PCI_VENDOR_ID_MACRONIX && dev_id == PCI_DEVICE_ID_MX987x5) {
    csr6 = 0x01880200;
    /* Set CSR16 and CSR20 to values that allow device modification */
    outl(0x0B3C0000 | inl(ioaddr + CSR16), ioaddr + CSR16);
    outl(0x00011000 | inl(ioaddr + CSR20), ioaddr + CSR20);

  } else if (vendor == PCI_VENDOR_ID_LINKSYS && dev_id == PCI_DEVICE_ID_LC82C115) {
    /* This is MX987x5 init code. It seems to work for the LNE100TX
       but should be replaced when we figure out the right way
       to do this initialization.
    */
    csr6 = 0x01880200;
    outl(0x0B3C0000 | inl(ioaddr + CSR16), ioaddr + CSR16);
    outl(0x00011000 | inl(ioaddr + CSR20), ioaddr + CSR20);

  } else if (vendor == PCI_VENDOR_ID_LINKSYS && dev_id == PCI_DEVICE_ID_DEC_TULIP) {

    csr6 = 0x814C0000;
    outl(0x00000001, ioaddr + CSR15);

  } else if (vendor == PCI_VENDOR_ID_DEC && dev_id == PCI_DEVICE_ID_DEC_21142) {
     /* check SROM for evidence of an MII interface */
     /* get Controller_0 Info Leaf Offset from SROM - assume already in ee_data */
     int offset = ee_data [27] + (ee_data [28] << 8);

     /* check offset range and if we have an extended type 3 Info Block */
     if ((offset >= 30) && (offset < 120) && (ee_data [offset + 3] > 128) &&
       (ee_data [offset + 4] == 3)) {
       /* must have an MII interface - disable heartbeat, select port etc. */
       csr6 |= (DEC_21142_CSR6_HBD | DEC_21142_CSR6_PS);
       csr6 &= ~(DEC_21142_CSR6_TTM);
     }
  } else if (vendor == PCI_VENDOR_ID_DAVICOM && dev_id == PCI_DEVICE_ID_DM9102){
      /* setup CR12 */
      outl(0x180, ioaddr + CSR12);    /* Let bit 7 output port */
      outl(0x80, ioaddr + CSR12);     /* RESET DM9102 phyxcer */
      outl(0x0, ioaddr + CSR12);      /* Clear RESET signal */

  } else if (vendor == PCI_VENDOR_ID_ADMTEK && dev_id == PCI_DEVICE_ID_ADMTEK_0985) {
    /* nothing */

  }

  /* set the chip's operating mode */
  outl(csr6, ioaddr + CSR6);

  /* Start Tx */
  outl(inl(ioaddr + CSR6) | 0x00002000, ioaddr + CSR6);
  /* immediate transmit demand */
  outl(0, ioaddr + CSR1);

  to = currticks() + TX_TIME_OUT;
  while ((txd.status & 0x80000000) && (currticks() < to))
    /* wait */ ;

  if (currticks() >= to) {
    printf ("TX Setup Timeout!\n");
  }

#ifdef TULIP_DEBUG
  printf("txd.status = %X\n", txd.status);
  printf("ticks = %d\n", currticks() - (to - TX_TIME_OUT));
  tulip_more();
#endif

  /* enable RX */
  outl(inl(ioaddr + CSR6) | 0x00000002, ioaddr + CSR6);
  /* immediate poll demand */
  outl(0, ioaddr + CSR2);
}


/*********************************************************************/
/* eth_transmit - Transmit a frame                                   */
/*********************************************************************/
static void tulip_transmit(struct nic *nic, const char *d, unsigned int t,
                           unsigned int s, const char *p)
{
  unsigned long to;

  whereami("tulip_transmit\n");

  /* Stop Tx */
  outl(inl(ioaddr + CSR6) & ~0x00002000, ioaddr + CSR6);

  /* setup ethernet header */
  memcpy(ehdr, d, ETHER_ADDR_SIZE);
  memcpy(&ehdr[ETHER_ADDR_SIZE], nic->node_addr, ETHER_ADDR_SIZE);
  ehdr[ETHER_ADDR_SIZE*2] = (t >> 8) & 0xFF;
  ehdr[ETHER_ADDR_SIZE*2+1] = t & 0xFF;

  /* setup the transmit descriptor */
  txd.buf1addr = &ehdr[0];        /* ethernet header */
  txd.buf1sz   = ETHER_HDR_SIZE;
  txd.buf2addr = p;               /* packet to transmit */
  txd.buf2sz   = s;
  txd.control  = 0x00000188;      /* LS+FS+TER */
  txd.status   = 0x80000000;      /* give ownership to device */

  /* Point to transmit descriptor */
  outl((unsigned long)&txd, ioaddr + CSR4);

  /* Start Tx */
  outl(inl(ioaddr + CSR6) |  0x00002000, ioaddr + CSR6);

  /* immediate transmit demand */
  outl(0, ioaddr + CSR1);

  to = currticks() + TX_TIME_OUT;
  while ((txd.status & 0x80000000) && (currticks() < to))
    /* wait */ ;

  if (currticks() >= to) {
    printf ("TX Timeout!\n");
  }

}

/*********************************************************************/
/* eth_poll - Wait for a frame                                       */
/*********************************************************************/
static int tulip_poll(struct nic *nic)
{
  whereami("tulip_poll\n");

  if (rxd[rxd_tail].status & 0x80000000)
    return 0;

  whereami("tulip_poll got one\n");

  nic->packetlen = (rxd[rxd_tail].status & 0x3FFF0000) >> 16;

  if( rxd[rxd_tail].status & 0x00008000){
      rxd[rxd_tail].status = 0x80000000;
      rxd_tail++;
      if (rxd_tail == NRXD) rxd_tail = 0;
      return 0;
  }

  /* copy packet to working buffer */
  /* XXX - this copy could be avoided with a little more work
     but for now we are content with it because the optimised
     memcpy is quite fast */

  memcpy(nic->packet, rxb + rxd_tail * BUFLEN, nic->packetlen);

  /* return the descriptor and buffer to receive ring */
  rxd[rxd_tail].status = 0x80000000;
  rxd_tail++;
  if (rxd_tail == NRXD) rxd_tail = 0;

  return 1;
}

/*********************************************************************/
/* eth_disable - Disable the interface                               */
/*********************************************************************/
static void tulip_disable(struct nic *nic)
{
  whereami("tulip_disable\n");

  /* disable interrupts */
  outl(0x00000000, ioaddr + CSR7);

  /* Stop the chip's Tx and Rx processes. */
  outl(inl(ioaddr + CSR6) & ~0x00002002, ioaddr + CSR6);

  /* Clear the missed-packet counter. */
  (volatile unsigned long)inl(ioaddr + CSR8);
}

/*********************************************************************/
/* eth_probe - Look for an adapter                                   */
/*********************************************************************/
struct nic *tulip_probe(struct nic *nic, unsigned short *io_addrs,
                          struct pci_device *pci)
{
  unsigned int i;
  u32 l1, l2;

  whereami("tulip_probe\n");

  if (io_addrs == 0 || *io_addrs == 0)
    return 0;

  vendor  = pci->vendor;
  dev_id  = pci->dev_id;
  ioaddr  = *io_addrs;

  /* wakeup chip */
  pcibios_write_config_dword(0, pci->devfn, 0x40, 0x00000000);

  /* Stop the chip's Tx and Rx processes. */
  outl(inl(ioaddr + CSR6) & ~0x00002002, ioaddr + CSR6);

  /* Clear the missed-packet counter. */
  (volatile unsigned long)inl(ioaddr + CSR8);

  /* Get MAC Address */

  /* Hardware Address retrieval method for LC82C168 */
  if (vendor == PCI_VENDOR_ID_LINKSYS && dev_id == PCI_DEVICE_ID_DEC_TULIP) {
    for (i = 0; i < 3; i++) {
      int value, boguscnt = 100000;
      outl(0x600 | i, ioaddr + 0x98);
      do
        value = inl(ioaddr + CSR9);
      while (value < 0  && --boguscnt > 0);
      nic->node_addr[i*2]     = (u8)((value >> 8) & 0xFF);
      nic->node_addr[i*2 + 1] = (u8)( value       & 0xFF);
    }
  } else if (vendor == PCI_VENDOR_ID_ADMTEK &&
            dev_id == PCI_DEVICE_ID_ADMTEK_0985) {
    l1 = inl(ioaddr + 0xA4);
    l2 = inl(ioaddr + 0xA8);
    nic->node_addr[0] = (l1      ) & 0xFF;
    nic->node_addr[1] = (l1 >>  8) & 0xFF;
    nic->node_addr[2] = (l1 >> 16) & 0xFF;
    nic->node_addr[3] = (l1 >> 24) & 0xFF;
    nic->node_addr[4] = (l2      ) & 0xFF;
    nic->node_addr[5] = (l2 >>  8) & 0xFF;
  } else {
    /* read EEPROM data */
    for (i = 0; i < sizeof(ee_data)/2; i++)
      ((unsigned short *)ee_data)[i] =
        le16_to_cpu(read_eeprom(ioaddr, i, EEPROM_ADDRLEN));

    /* extract MAC address from EEPROM buffer */
    for (i=0; i<6; i++)
      nic->node_addr[i] = ee_data[20+i];
  }

  printf("Tulip %b:%b:%b:%b:%b:%b at ioaddr 0x%x\n",
    nic->node_addr[0],nic->node_addr[1],nic->node_addr[2],nic->node_addr[3],
    nic->node_addr[4],nic->node_addr[5],ioaddr);

  /* initialize device */
  tulip_reset(nic);

  nic->reset    = tulip_reset;
  nic->poll     = tulip_poll;
  nic->transmit = tulip_transmit;
  nic->disable  = tulip_disable;

  return nic;
}
