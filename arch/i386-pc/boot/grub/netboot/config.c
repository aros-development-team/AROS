/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2001  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Based on "src/config.c" in etherboot-4.6.4.  */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

#include	"etherboot.h"
#include	"nic.h"

#undef	INCLUDE_PCI
#if	defined(INCLUDE_NS8390) || defined(INCLUDE_EEPRO100) || defined(INCLUDE_LANCE) || defined(INCLUDE_EPIC100) || defined(INCLUDE_TULIP) || defined(INCLUDE_OTULIP) || defined(INCLUDE_3C90X) ||  defined(INCLUDE_3C595) || defined(INCLUDE_RTL8139) || defined(INCLUDE_VIA_RHINE)
	/* || others later */
#if	defined(ETHERBOOT32)		/* only for 32 bit machines */
#define	INCLUDE_PCI
#include	"pci.h"
static unsigned short	pci_ioaddrs[16];

static struct pci_device	pci_nic_list[] = {
#ifdef	INCLUDE_NS8390
	{ PCI_VENDOR_ID_REALTEK,	PCI_DEVICE_ID_REALTEK_8029,
		"Realtek 8029", 0, 0, 0},
	{ PCI_VENDOR_ID_WINBOND2,	PCI_DEVICE_ID_WINBOND2_89C940,
		"Winbond NE2000-PCI", 0, 0, 0},
	{ PCI_VENDOR_ID_COMPEX,		PCI_DEVICE_ID_COMPEX_RL2000,
		"Compex ReadyLink 2000", 0, 0, 0},
	{ PCI_VENDOR_ID_KTI,		PCI_DEVICE_ID_KTI_ET32P2,
		"KTI ET32P2", 0, 0, 0},
	{ PCI_VENDOR_ID_NETVIN,		PCI_DEVICE_ID_NETVIN_NV5000SC,
		"NetVin NV5000SC", 0, 0, 0},
#endif
#ifdef	INCLUDE_3C90X
	{ PCI_VENDOR_ID_3COM,		PCI_DEVICE_ID_3COM_3C900TPO,
		"3Com900-TPO", 0, 0, 0},
	{ PCI_VENDOR_ID_3COM,		PCI_DEVICE_ID_3COM_3C900COMBO,
		"3Com900-Combo", 0, 0, 0},
	{ PCI_VENDOR_ID_3COM,		PCI_DEVICE_ID_3COM_3C905TX,
		"3Com905-TX", 0, 0, 0},
	{ PCI_VENDOR_ID_3COM,		PCI_DEVICE_ID_3COM_3C905T4,
		"3Com905-T4", 0, 0, 0},

	{ PCI_VENDOR_ID_3COM,		0x9004,
		"3Com900B-TPO", 0, 0, 0},
	{ PCI_VENDOR_ID_3COM,		0x9005,
		"3Com900B-Combo", 0, 0, 0},
	{ PCI_VENDOR_ID_3COM,		0x9006,
		"3Com900B-2/T", 0, 0, 0},
	{ PCI_VENDOR_ID_3COM,		0x900A,
		"3Com900B-FL", 0, 0, 0},
	{ PCI_VENDOR_ID_3COM,		PCI_DEVICE_ID_3COM_3C905B_TX,
		"3Com905B-TX", 0, 0, 0},
	{ PCI_VENDOR_ID_3COM,		0x9056,
		"3Com905B-T4", 0, 0, 0},
	{ PCI_VENDOR_ID_3COM,		0x905A,
		"3Com905B-FL", 0, 0, 0},
	{ PCI_VENDOR_ID_3COM,		PCI_DEVICE_ID_3COM_3C905C_TXM,
		"3Com905C-TXM", 0, 0, 0},

#endif
#ifdef	INCLUDE_3C595
	{ PCI_VENDOR_ID_3COM,		PCI_DEVICE_ID_3COM_3C590,
		"3Com590", 0, 0, 0},
	{ PCI_VENDOR_ID_3COM,		PCI_DEVICE_ID_3COM_3C595,
		"3Com595", 0, 0, 0},
	{ PCI_VENDOR_ID_3COM,		PCI_DEVICE_ID_3COM_3C595_1,
		"3Com595", 0, 0, 0},
	{ PCI_VENDOR_ID_3COM,		PCI_DEVICE_ID_3COM_3C595_2,
		"3Com595", 0, 0, 0},
#endif
#ifdef	INCLUDE_EEPRO100
	{ PCI_VENDOR_ID_INTEL,		PCI_DEVICE_ID_INTEL_82557,
		"Intel EtherExpressPro100", 0, 0, 0},
#endif
#ifdef	INCLUDE_EPIC100
	{ PCI_VENDOR_ID_SMC,		PCI_DEVICE_ID_SMC_EPIC100,
		"SMC EtherPowerII", 0, 0, 0},
#endif
#ifdef	INCLUDE_LANCE
	{ PCI_VENDOR_ID_AMD,		PCI_DEVICE_ID_AMD_LANCE,
		"AMD Lance/PCI", 0, 0, 0},
#endif
#ifdef	INCLUDE_RTL8139
	{ PCI_VENDOR_ID_REALTEK,	PCI_DEVICE_ID_REALTEK_8139,
		"Realtek 8139", 0, 0, 0},
        { PCI_VENDOR_ID_SMC_1211,       PCI_DEVICE_ID_SMC_1211,
                "SMC EZ10/100", 0, 0, 0},
#endif
#ifdef	INCLUDE_OTULIP
	{ PCI_VENDOR_ID_DEC,		PCI_DEVICE_ID_DEC_TULIP,
		"Digital Tulip", 0, 0, 0},
	{ PCI_VENDOR_ID_DEC,		PCI_DEVICE_ID_DEC_TULIP_FAST,
		"Digital Tulip Fast", 0, 0, 0},
	{ PCI_VENDOR_ID_DEC,		PCI_DEVICE_ID_DEC_TULIP_PLUS,
		"Digital Tulip+", 0, 0, 0},
	{ PCI_VENDOR_ID_DEC,		PCI_DEVICE_ID_DEC_21142,
		"Digital Tulip 21142", 0, 0, 0},
#endif
#ifdef	INCLUDE_TULIP
	{ PCI_VENDOR_ID_DEC,		PCI_DEVICE_ID_DEC_TULIP,
		"Digital Tulip", 0, 0, 0},
	{ PCI_VENDOR_ID_DEC,		PCI_DEVICE_ID_DEC_TULIP_FAST,
		"Digital Tulip Fast", 0, 0, 0},
	{ PCI_VENDOR_ID_DEC,		PCI_DEVICE_ID_DEC_TULIP_PLUS,
		"Digital Tulip+", 0, 0, 0},
	{ PCI_VENDOR_ID_DEC,		PCI_DEVICE_ID_DEC_21142,
		"Digital Tulip 21142", 0, 0, 0},
	{ PCI_VENDOR_ID_MACRONIX,	PCI_DEVICE_ID_MX987x5,
		"Macronix MX987x5", 0, 0, 0},
	{ PCI_VENDOR_ID_LINKSYS,	PCI_DEVICE_ID_LC82C115,
		"LinkSys LNE100TX", 0, 0, 0},
	{ PCI_VENDOR_ID_LINKSYS,	PCI_DEVICE_ID_DEC_TULIP,
		"Netgear FA310TX", 0, 0, 0},
	{ PCI_VENDOR_ID_DAVICOM, PCI_DEVICE_ID_DM9102,
		"Davicom 9102", 0, 0, 0},
	{ PCI_VENDOR_ID_ADMTEK, PCI_DEVICE_ID_ADMTEK_0985,
		"ADMtek Centaur-P", 0, 0, 0},
#endif
#ifdef	INCLUDE_VIA_RHINE
	{ PCI_VENDOR_ID_VIATEC,	PCI_DEVICE_ID_VIA_RHINE_I,
		"VIA 3043", 0, 0, 0},
	{ PCI_VENDOR_ID_VIATEC,	PCI_DEVICE_ID_VIA_86C100A,
		"VIA 86C100A", 0, 0, 0},
#endif
/* other PCI NICs go here */
	{0, 0, NULL, 0, 0, 0}
};
#endif	/* ETHERBOOT32 */
#endif	/* INCLUDE_*PCI */

#include "cards.h"

#if defined(GRUB) && defined(INCLUDE_PCI)
struct pci_dispatch_table
{
  unsigned short vendor;
  unsigned short dev_id;
  struct nic *(*eth_probe) (struct nic *, unsigned short *,
			    struct pci_device *);
};

static struct pci_dispatch_table PCI_NIC[] =
{
# ifdef INCLUDE_NS8390
  { PCI_VENDOR_ID_REALTEK,  PCI_DEVICE_ID_REALTEK_8029,    nepci_probe },
  { PCI_VENDOR_ID_WINBOND2, PCI_DEVICE_ID_WINBOND2_89C940, nepci_probe },
  { PCI_VENDOR_ID_COMPEX,   PCI_DEVICE_ID_COMPEX_RL2000,   nepci_probe },
  { PCI_VENDOR_ID_KTI,      PCI_DEVICE_ID_KTI_ET32P2,      nepci_probe },
  { PCI_VENDOR_ID_NETVIN,   PCI_DEVICE_ID_NETVIN_NV5000SC, nepci_probe },
# endif /* INCLUDE_NS8390 */
# ifdef INCLUDE_3C90X
  { PCI_VENDOR_ID_3COM,	    PCI_DEVICE_ID_3COM_3C900TPO,   a3c90x_probe },
  { PCI_VENDOR_ID_3COM,     PCI_DEVICE_ID_3COM_3C900COMBO, a3c90x_probe },
  { PCI_VENDOR_ID_3COM,     PCI_DEVICE_ID_3COM_3C905TX,    a3c90x_probe },
  { PCI_VENDOR_ID_3COM,     PCI_DEVICE_ID_3COM_3C905T4,    a3c90x_probe },
  { PCI_VENDOR_ID_3COM,     0x9004,                        a3c90x_probe },
  { PCI_VENDOR_ID_3COM,     0x9005,                        a3c90x_probe },
  { PCI_VENDOR_ID_3COM,     0x9006,                        a3c90x_probe },
  { PCI_VENDOR_ID_3COM,     0x900A,                        a3c90x_probe },
  { PCI_VENDOR_ID_3COM,     PCI_DEVICE_ID_3COM_3C905B_TX,  a3c90x_probe },
  { PCI_VENDOR_ID_3COM,     0x9056,                        a3c90x_probe },
  { PCI_VENDOR_ID_3COM,     0x905A,                        a3c90x_probe },
  { PCI_VENDOR_ID_3COM,     PCI_DEVICE_ID_3COM_3C905C_TXM, a3c90x_probe },
# endif /* INCLUDE_3C90X */
# ifdef	INCLUDE_3C595
  { PCI_VENDOR_ID_3COM,     PCI_DEVICE_ID_3COM_3C590,      t595_probe },
  { PCI_VENDOR_ID_3COM,     PCI_DEVICE_ID_3COM_3C595,      t595_probe },
  { PCI_VENDOR_ID_3COM,     PCI_DEVICE_ID_3COM_3C595_1,    t595_probe },
  { PCI_VENDOR_ID_3COM,     PCI_DEVICE_ID_3COM_3C595_2,    t595_probe },
# endif /* INCLUDE_3C595 */
# ifdef	INCLUDE_EEPRO100
  { PCI_VENDOR_ID_INTEL,    PCI_DEVICE_ID_INTEL_82557,     eepro100_probe },
# endif /* INCLUDE_EEPRO100 */
# ifdef	INCLUDE_EPIC100
  { PCI_VENDOR_ID_SMC,      PCI_DEVICE_ID_SMC_EPIC100,     epic100_probe },
# endif /* INCLUDE_EPIC100 */
# ifdef	INCLUDE_LANCE
  { PCI_VENDOR_ID_AMD,      PCI_DEVICE_ID_AMD_LANCE,       lancepci_probe },
# endif /* INCLUDE_LANCE */
# ifdef	INCLUDE_RTL8139
  { PCI_VENDOR_ID_REALTEK,  PCI_DEVICE_ID_REALTEK_8139,    rtl8139_probe },
  { PCI_VENDOR_ID_SMC_1211, PCI_DEVICE_ID_SMC_1211,        rtl8139_probe },
# endif /* INCLUDE_RTL8139 */
# ifdef	INCLUDE_OTULIP
  { PCI_VENDOR_ID_DEC,      PCI_DEVICE_ID_DEC_TULIP,       otulip_probe },
  { PCI_VENDOR_ID_DEC,      PCI_DEVICE_ID_DEC_TULIP_FAST,  otulip_probe },
  { PCI_VENDOR_ID_DEC,      PCI_DEVICE_ID_DEC_TULIP_PLUS,  otulip_probe },
  { PCI_VENDOR_ID_DEC,      PCI_DEVICE_ID_DEC_21142,       otulip_probe },
# endif /* INCLUDE_OTULIP */
# ifdef	INCLUDE_TULIP
  { PCI_VENDOR_ID_DEC,      PCI_DEVICE_ID_DEC_TULIP,       tulip_probe },
  { PCI_VENDOR_ID_DEC,      PCI_DEVICE_ID_DEC_TULIP_FAST,  tulip_probe },
  { PCI_VENDOR_ID_DEC,      PCI_DEVICE_ID_DEC_TULIP_PLUS,  tulip_probe },
  { PCI_VENDOR_ID_DEC,      PCI_DEVICE_ID_DEC_21142,       tulip_probe },
  { PCI_VENDOR_ID_MACRONIX, PCI_DEVICE_ID_MX987x5,         tulip_probe },
  { PCI_VENDOR_ID_LINKSYS,  PCI_DEVICE_ID_LC82C115,        tulip_probe },
  { PCI_VENDOR_ID_LINKSYS,  PCI_DEVICE_ID_DEC_TULIP,       tulip_probe },
  { PCI_VENDOR_ID_DAVICOM,  PCI_DEVICE_ID_DM9102,          tulip_probe },
  { PCI_VENDOR_ID_ADMTEK,   PCI_DEVICE_ID_ADMTEK_0985,     tulip_probe },
# endif /* INCLUDE_TULIP */
# ifdef	INCLUDE_VIA_RHINE
  { PCI_VENDOR_ID_VIATEC,   PCI_DEVICE_ID_VIA_RHINE_I,     rhine_probe },
  { PCI_VENDOR_ID_VIATEC,   PCI_DEVICE_ID_VIA_86C100A,     rhine_probe },
# endif /* INCLUDE_VIA_RHINE */
  { 0,                      0,                             0 }
};
#endif /* GRUB && INCLUDE_PCI */

struct dispatch_table
{
	const char	*nic_name;
#ifdef	INCLUDE_PCI
	struct nic	*(*eth_probe)(struct nic *, unsigned short *,
		struct pci_device *);
#else
	struct nic	*(*eth_probe)(struct nic *, unsigned short *);
#endif	/* INCLUDE_PCI */
	unsigned short	*probe_ioaddrs;		/* for probe overrides */
};

/*
 *	NIC probing is in order of appearance in this table.
 *	If for some reason you want to change the order,
 *	just rearrange the entries (bracketed by the #ifdef/#endif)
 */
static struct dispatch_table	NIC[] =
{
#ifdef	INCLUDE_RTL8139
	{ "RTL8139", rtl8139_probe, pci_ioaddrs },
#endif
#ifdef	INCLUDE_WD
	{ "WD", wd_probe, 0 },
#endif
#ifdef	INCLUDE_3C503
	{ "3C503", t503_probe, 0 },
#endif
#ifdef	INCLUDE_NE
	{ "NE*000", ne_probe, 0 },
#endif
#ifdef	INCLUDE_3C509
	{ "3C5x9", t509_probe, 0 },
#endif
#ifdef	INCLUDE_3C529
	{ "3C5x9", t529_probe, 0 },
#endif
#ifdef	INCLUDE_3C595
	{ "3C595", t595_probe, pci_ioaddrs },
#endif
#ifdef	INCLUDE_3C90X
	{ "3C90X", a3c90x_probe, pci_ioaddrs },
#endif
#ifdef	INCLUDE_EEPRO100
	{ "EEPRO100", eepro100_probe, pci_ioaddrs },
#endif
#ifdef	INCLUDE_EPIC100
	{ "EPIC100", epic100_probe, pci_ioaddrs },
#endif
#ifdef	INCLUDE_OTULIP
	{ "OTulip", otulip_probe, pci_ioaddrs },
#endif
#ifdef	INCLUDE_TULIP
	{ "Tulip", tulip_probe, pci_ioaddrs },
#endif
#ifdef	INCLUDE_CS89X0
	{ "CS89x0", cs89x0_probe, 0 },
#endif
#ifdef	INCLUDE_NE2100
	{ "NE2100", ne2100_probe, 0 },
#endif
#ifdef	INCLUDE_NI6510
	{ "NI6510", ni6510_probe, 0 },
#endif
#ifdef	INCLUDE_SK_G16
	{ "SK_G16", SK_probe, 0 },
#endif
#ifdef	INCLUDE_3C507
	{ "3C507", t507_probe, 0 },
#endif
#ifdef	INCLUDE_NI5210
	{ "NI5210", ni5210_probe, 0 },
#endif
#ifdef	INCLUDE_EXOS205
	{ "EXOS205", exos205_probe, 0 },
#endif
#ifdef	INCLUDE_SMC9000
	{ "SMC9000", smc9000_probe, 0 },
#endif
#ifdef	INCLUDE_TIARA
	{ "TIARA", tiara_probe, 0 },
#endif
#ifdef	INCLUDE_DEPCA
	{ "DEPCA", depca_probe, 0 },
#endif
#ifdef	INCLUDE_NS8390
	{ "NE2000/PCI", nepci_probe, pci_ioaddrs },
#endif
#ifdef	INCLUDE_LANCE
	{ "LANCE/PCI", lancepci_probe, pci_ioaddrs },
#endif
#ifdef	INCLUDE_VIA_RHINE
	{ "VIA 86C100", rhine_probe, pci_ioaddrs },
#endif
	/* this entry must always be last to mark the end of list */
	{ 0, 0, 0 }
};

#define	NIC_TABLE_SIZE	(sizeof(NIC)/sizeof(NIC[0]))

static int eth_dummy(struct nic *nic)
{
	return (0);
}

static char	packet[ETH_MAX_PACKET];

struct nic	nic =
{
#ifdef	ETHERBOOT32
	(void (*)(struct nic *))eth_dummy,	/* reset */
	eth_dummy,				/* poll */
	(void (*)(struct nic *, const char *,
		unsigned int, unsigned int,
		const char *))eth_dummy,		/* transmit */
	(void (*)(struct nic *))eth_dummy,	/* disable */
#endif
/* bcc has problems with complicated casts */
#ifdef	ETHERBOOT16
	eth_dummy,
	eth_dummy,
	eth_dummy,
	eth_dummy,
#endif
#ifdef	T503_AUI
	1,			/* aui */
#else
	0,			/* no aui */
#endif
	arptable[ARP_CLIENT].node,	/* node_addr */
	packet,			/* packet */
	0,			/* packetlen */
	0			/* priv_data */
};

#ifndef GRUB
void print_config(void)
{
	struct dispatch_table	*t;

#ifdef	ETHERBOOT32
	printf("Etherboot/32 version " VERSION " (GPL) for ");
#endif
#ifdef	ETHERBOOT16
	/* Must be all on line or bcc can't handle concatenation */
	printf("Etherboot/16 version " VERSION " (GPL) for ");
#endif
	for (t = NIC; t->nic_name != 0; ++t)
		printf("[%s]", t->nic_name);
	putchar('\n');
}
#endif /* ! GRUB */

void eth_reset(void)
{
	(*nic.reset)(&nic);
}

int eth_probe(void)
{
	struct pci_device	*p;
	struct dispatch_table	*t;
#ifdef GRUB
	static int probed = 0;
#endif /* GRUB */

#ifdef GRUB
        /* If already probed, don't try to probe it any longer.  */
        if (probed)
          return 1;

	/* Clear the ready flag.  */
        network_ready = 0;
        /* Clear the ARP table.  */
        grub_memset ((char *) arptable, 0,
                     MAX_ARP * sizeof (struct arptable_t));
#endif /* GRUB */

	p = 0;
#ifdef	INCLUDE_PCI
	
# ifdef GRUB
        /* In GRUB, the ROM info is initialized here.  */
        rom = *((struct rom_info *) ROM_INFO_LOCATION);
# endif /* GRUB */
	
 	eth_pci_init(pci_nic_list);
	pci_ioaddrs[0] = 0;
	pci_ioaddrs[1] = 0;
	/* at this point we have a list of possible PCI candidates
	   we just pick the first one with a non-zero ioaddr */
	for (p = pci_nic_list; p->vendor != 0; ++p) {
		if (p->ioaddr != 0) {
			pci_ioaddrs[0] = p->ioaddr;
			break;
		}
	}
#endif
	printf("Probing...");
	
#if defined(GRUB) && defined(INCLUDE_PCI)
	if (p->vendor)
	  {
	    struct pci_dispatch_table *pt;

	    for (pt = PCI_NIC; pt->eth_probe != 0; pt++)
	      if (p->vendor == pt->vendor && p->dev_id == pt->dev_id)
		{
		  printf ("[%s]", p->name);
		  if ((pt->eth_probe) (&nic, pci_ioaddrs, p))
		    {
		      probed = 1;
		      return 1;
		    }
		}
	  }
#endif /* GRUB && INCLUDE_PCI */
	
	for (t = NIC; t->nic_name != 0; ++t)
	{
		printf("[%s]", t->nic_name);
#ifdef	INCLUDE_PCI
		if ((*t->eth_probe)(&nic, t->probe_ioaddrs, p)) {
#else
		if ((*t->eth_probe)(&nic, t->probe_ioaddrs)) {
#endif	/* INCLUDE_PCI */
#ifdef GRUB
		  probed = 1;
#endif /* GRUB */
			return (1);
		}
	}
	return (0);
}

int eth_poll(void)
{
	return ((*nic.poll)(&nic));
}

void eth_transmit(const char *d, unsigned int t, unsigned int s, const void *p)
{
	(*nic.transmit)(&nic, d, t, s, p);
	twiddle();
}

void eth_disable(void)
{
	(*nic.disable)(&nic);
}
