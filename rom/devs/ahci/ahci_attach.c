/*
 * (MPSAFE)
 *
 * Copyright (c) 2006 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *
 * Copyright (c) 2009 The DragonFly Project.  All rights reserved.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Matthew Dillon <dillon@backplane.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $OpenBSD: ahci.c,v 1.147 2009/02/16 21:19:07 miod Exp $
 */

#include "ahci.h"

static int	ahci_vt8251_attach(device_t);
static int	ahci_ati_sb600_attach(device_t);
static int	ahci_ati_sb700_attach(device_t);
static int	ahci_nvidia_mcp_attach(device_t);
static int	ahci_pci_attach(device_t);
static int	ahci_pci_detach(device_t);

static const struct ahci_device ahci_devices[] = {
	{ PCI_VENDOR_VIATECH,	PCI_PRODUCT_VIATECH_VT8251_SATA,
	    ahci_vt8251_attach, ahci_pci_detach, "ViaTech-VT8251-SATA" },
	{ PCI_VENDOR_ATI,	PCI_PRODUCT_ATI_SB600_SATA,
	    ahci_ati_sb600_attach, ahci_pci_detach, "ATI-SB600-SATA" },
	{ PCI_VENDOR_ATI,	PCI_PRODUCT_ATI_SB700_SATA,
	    ahci_ati_sb700_attach, ahci_pci_detach, "ATI-SB700-SATA" },
	{ PCI_VENDOR_NVIDIA,	PCI_PRODUCT_NVIDIA_MCP65_AHCI_2,
	    ahci_nvidia_mcp_attach, ahci_pci_detach, "NVidia-MCP65-SATA" },
	{ PCI_VENDOR_NVIDIA,	PCI_PRODUCT_NVIDIA_MCP67_AHCI_1,
	    ahci_nvidia_mcp_attach, ahci_pci_detach, "NVidia-MCP67-SATA" },
	{ PCI_VENDOR_NVIDIA,	PCI_PRODUCT_NVIDIA_MCP77_AHCI_5,
	    ahci_nvidia_mcp_attach, ahci_pci_detach, "NVidia-MCP77-SATA" },
	{ PCI_VENDOR_NVIDIA,	PCI_PRODUCT_NVIDIA_MCP79_AHCI_1,
	    ahci_nvidia_mcp_attach, ahci_pci_detach, "NVidia-MCP79-SATA" },
	{ 0, 0,
	    ahci_pci_attach, ahci_pci_detach, "AHCI-PCI-SATA" }
};

/*
 * Match during probe and attach.  The device does not yet have a softc.
 */
const struct ahci_device *
ahci_lookup_device(device_t dev)
{
	const struct ahci_device *ad;
	u_int16_t vendor = pci_get_vendor(dev);
	u_int16_t product = pci_get_device(dev);
	u_int8_t class = pci_get_class(dev);
	u_int8_t subclass = pci_get_subclass(dev);
	u_int8_t progif = pci_read_config(dev, PCIR_PROGIF, 1);
	int is_ahci;

	/*
	 * Generally speaking if the pci device does not identify as
	 * AHCI we skip it.
	 */
	if (class == PCIC_STORAGE && subclass == PCIS_STORAGE_SATA &&
	    progif == PCIP_STORAGE_SATA_AHCI_1_0) {
		is_ahci = 1;
	} else {
		is_ahci = 0;
	}

	for (ad = &ahci_devices[0]; ad->ad_vendor; ++ad) {
		if (ad->ad_vendor == vendor && ad->ad_product == product)
			return (ad);
	}

	/*
	 * Last ad is the default match if the PCI device matches SATA.
	 */
	if (is_ahci == 0)
		ad = NULL;
	return (ad);
}

/*
 * Attach functions.  They all eventually fall through to ahci_pci_attach().
 */
static int
ahci_vt8251_attach(device_t dev)
{
	struct ahci_softc *sc = device_get_softc(dev);

	sc->sc_flags |= AHCI_F_NO_NCQ;
	return (ahci_pci_attach(dev));
}

static int
ahci_ati_sb600_attach(device_t dev)
{
	struct ahci_softc *sc = device_get_softc(dev);
	pcireg_t magic;
	u_int8_t subclass = pci_get_subclass(dev);
	u_int8_t revid;

	if (subclass == PCIS_STORAGE_IDE) {
		revid = pci_read_config(dev, PCIR_REVID, 1);
		magic = pci_read_config(dev, AHCI_PCI_ATI_SB600_MAGIC, 4);
		pci_write_config(dev, AHCI_PCI_ATI_SB600_MAGIC,
				 magic | AHCI_PCI_ATI_SB600_LOCKED, 4);
		pci_write_config(dev, PCIR_REVID,
				 (PCIC_STORAGE << 24) |
				 (PCIS_STORAGE_SATA << 16) |
				 (PCIP_STORAGE_SATA_AHCI_1_0 << 8) |
				 revid, 4);
		pci_write_config(dev, AHCI_PCI_ATI_SB600_MAGIC, magic, 4);
	}

	sc->sc_flags |= AHCI_F_IGN_FR;
	return (ahci_pci_attach(dev));
}

static int
ahci_ati_sb700_attach(device_t dev)
{
	struct ahci_softc *sc = device_get_softc(dev);

	sc->sc_flags |= AHCI_F_IGN_FR;
	sc->sc_flags |= AHCI_F_NO_PM;
	return (ahci_pci_attach(dev));
}

static int
ahci_nvidia_mcp_attach(device_t dev)
{
	struct ahci_softc *sc = device_get_softc(dev);

	sc->sc_flags |= AHCI_F_IGN_FR;
	return (ahci_pci_attach(dev));
}

static int
ahci_pci_attach(device_t dev)
{
	struct ahci_softc *sc = device_get_softc(dev);
	struct ahci_port *ap;
	const char *gen;
	u_int32_t pi, reg;
	u_int32_t cap, cap2;
	bus_addr_t addr;
	int i;
	int error, fbs;
	const char *revision;

	if (pci_read_config(dev, PCIR_COMMAND, 2) & 0x0400) {
		device_printf(dev, "BIOS disabled PCI interrupt, "
				   "re-enabling\n");
		pci_write_config(dev, PCIR_COMMAND,
			pci_read_config(dev, PCIR_COMMAND, 2) & ~0x0400, 2);
	}


	/*
	 * Map the AHCI controller's IRQ and BAR(5) (hardware registers)
	 */
	sc->sc_dev = dev;
	sc->sc_rid_irq = AHCI_IRQ_RID;
	sc->sc_irq = bus_alloc_resource_any(dev, SYS_RES_IRQ, &sc->sc_rid_irq,
					    RF_SHAREABLE | RF_ACTIVE);
	if (sc->sc_irq == NULL) {
		device_printf(dev, "unable to map interrupt\n");
		ahci_pci_detach(dev);
		return (ENXIO);
	}

	/*
	 * When mapping the register window store the tag and handle
	 * separately so we can use the tag with per-port bus handle
	 * sub-spaces.
	 */
	sc->sc_rid_regs = PCIR_BAR(5);
	sc->sc_regs = bus_alloc_resource_any(dev, SYS_RES_MEMORY,
					     &sc->sc_rid_regs, RF_ACTIVE);
	if (sc->sc_regs == NULL) {
		device_printf(dev, "unable to map registers\n");
		ahci_pci_detach(dev);
		return (ENXIO);
	}
	sc->sc_iot = rman_get_bustag(sc->sc_regs);
	sc->sc_ioh = rman_get_bushandle(sc->sc_regs);

	/*
	 * Initialize the chipset and then set the interrupt vector up
	 */
	device_printf(dev, "device flags 0x%x\n", sc->sc_flags);
	error = ahci_init(sc);
	if (error) {
		ahci_pci_detach(dev);
		return (ENXIO);
	}

	/*
	 * Get the AHCI capabilities and max number of concurrent
	 * command tags and set up the DMA tags.  Adjust the saved
	 * sc_cap according to override flags.
	 */
	cap = sc->sc_cap = ahci_read(sc, AHCI_REG_CAP);
	if (sc->sc_flags & AHCI_F_NO_NCQ)
		sc->sc_cap &= ~AHCI_REG_CAP_SNCQ;
	if (sc->sc_flags & AHCI_F_NO_PM)
		cap &= ~AHCI_REG_CAP_SPM;
	if (sc->sc_flags & AHCI_F_FORCE_FBSS)
		sc->sc_cap |= AHCI_REG_CAP_FBSS;

	/*
	 * We assume at least 4 commands.
	 */
	sc->sc_ncmds = AHCI_REG_CAP_NCS(cap);
	if (sc->sc_ncmds < 4) {
		device_printf(dev, "NCS must probe a value >= 4\n");
		ahci_pci_detach(dev);
		return (ENXIO);
	}

	addr = (cap & AHCI_REG_CAP_S64A) ?
		BUS_SPACE_MAXADDR : BUS_SPACE_MAXADDR_32BIT;

	/*
	 * DMA tags for allocation of DMA memory buffers, lists, and so
	 * forth.  These are typically per-port.
	 *
	 * When FIS-based switching is supported we need a rfis for
	 * each target (4K total).  The spec also requires 4K alignment
	 * for this case.
	 */
	fbs = (cap & AHCI_REG_CAP_FBSS) ? 16 : 1;
	error = 0;

	error += bus_dma_tag_create(
			NULL,				/* parent tag */
			256 * fbs,			/* alignment */
			PAGE_SIZE,			/* boundary */
			addr,				/* loaddr? */
			BUS_SPACE_MAXADDR,		/* hiaddr */
			NULL,				/* filter */
			NULL,				/* filterarg */
			sizeof(struct ahci_rfis) * fbs,	/* [max]size */
			1,				/* maxsegs */
			sizeof(struct ahci_rfis) * fbs,	/* maxsegsz */
			0,				/* flags */
			&sc->sc_tag_rfis);		/* return tag */

	error += bus_dma_tag_create(
			NULL,				/* parent tag */
			32,				/* alignment */
			1024,				/* boundary */
			addr,				/* loaddr? */
			BUS_SPACE_MAXADDR,		/* hiaddr */
			NULL,				/* filter */
			NULL,				/* filterarg */
			sc->sc_ncmds * sizeof(struct ahci_cmd_hdr),
			1,				/* maxsegs */
			sc->sc_ncmds * sizeof(struct ahci_cmd_hdr),
			0,				/* flags */
			&sc->sc_tag_cmdh);		/* return tag */

	/*
	 * NOTE: ahci_cmd_table is sized to a power of 2
	 */
	error += bus_dma_tag_create(
			NULL,				/* parent tag */
			sizeof(struct ahci_cmd_table),	/* alignment */
			1024,				/* boundary */
			addr,				/* loaddr? */
			BUS_SPACE_MAXADDR,		/* hiaddr */
			NULL,				/* filter */
			NULL,				/* filterarg */
			sc->sc_ncmds * sizeof(struct ahci_cmd_table),
			1,				/* maxsegs */
			sc->sc_ncmds * sizeof(struct ahci_cmd_table),
			0,				/* flags */
			&sc->sc_tag_cmdt);		/* return tag */

	/*
	 * The data tag is used for later dmamaps and not immediately
	 * allocated.
	 */
	error += bus_dma_tag_create(
			NULL,				/* parent tag */
			4,				/* alignment */
			0,				/* boundary */
			addr,				/* loaddr? */
			BUS_SPACE_MAXADDR,		/* hiaddr */
			NULL,				/* filter */
			NULL,				/* filterarg */
			4096 * 1024,			/* maxiosize */
			AHCI_MAX_PRDT,			/* maxsegs */
			65536,				/* maxsegsz */
			0,				/* flags */
			&sc->sc_tag_data);		/* return tag */

	if (error) {
		device_printf(dev, "unable to create dma tags\n");
		ahci_pci_detach(dev);
		return (ENXIO);
	}

	switch (cap & AHCI_REG_CAP_ISS) {
	case AHCI_REG_CAP_ISS_G1:
		gen = "1 (1.5Gbps)";
		break;
	case AHCI_REG_CAP_ISS_G2:
		gen = "2 (3Gbps)";
		break;
	case AHCI_REG_CAP_ISS_G3:
		gen = "3 (6Gbps)";
		break;
	default:
		gen = "unknown";
		break;
	}

	/* check the revision */
	reg = ahci_read(sc, AHCI_REG_VS);

	switch (reg) {
	case AHCI_REG_VS_0_95:
		revision = "AHCI 0.95";
		break;
	case AHCI_REG_VS_1_0:
		revision = "AHCI 1.0";
		break;
	case AHCI_REG_VS_1_1:
		revision = "AHCI 1.1";
		break;
	case AHCI_REG_VS_1_2:
		revision = "AHCI 1.2";
		break;
	case AHCI_REG_VS_1_3:
		revision = "AHCI 1.3";
		break;
	case AHCI_REG_VS_1_4:
		revision = "AHCI 1.4";
		break;
	case AHCI_REG_VS_1_5:
		revision = "AHCI 1.5";	/* future will catch up to us */
		break;
	default:
		device_printf(sc->sc_dev,
			      "Warning: Unknown AHCI revision 0x%08x\n", reg);
		revision = "AHCI <unknown>";
		break;
	}
	sc->sc_vers = reg;

	if (reg >= AHCI_REG_VS_1_3) {
		cap2 = ahci_read(sc, AHCI_REG_CAP2);
		device_printf(dev,
			      "%s cap 0x%b cap2 0x%b, %d ports, "
			      "%d tags/port, gen %s\n",
			      revision,
			      cap, AHCI_FMT_CAP,
			      cap2, AHCI_FMT_CAP2,
			      AHCI_REG_CAP_NP(cap), sc->sc_ncmds, gen);
	} else {
		cap2 = 0;
		device_printf(dev,
			      "%s cap 0x%b, %d ports, "
			      "%d tags/port, gen %s\n",
			      revision,
			      cap, AHCI_FMT_CAP,
			      AHCI_REG_CAP_NP(cap), sc->sc_ncmds, gen);
	}
	sc->sc_cap2 = cap2;

	pi = ahci_read(sc, AHCI_REG_PI);
	DPRINTF(AHCI_D_VERBOSE, "%s: ports implemented: 0x%08x\n",
	    DEVNAME(sc), pi);

#ifdef AHCI_COALESCE
	/* Naive coalescing support - enable for all ports. */
	if (cap & AHCI_REG_CAP_CCCS) {
		u_int16_t		ccc_timeout = 20;
		u_int8_t		ccc_numcomplete = 12;
		u_int32_t		ccc_ctl;

		/* disable coalescing during reconfiguration. */
		ccc_ctl = ahci_read(sc, AHCI_REG_CCC_CTL);
		ccc_ctl &= ~0x00000001;
		ahci_write(sc, AHCI_REG_CCC_CTL, ccc_ctl);

		sc->sc_ccc_mask = 1 << AHCI_REG_CCC_CTL_INT(ccc_ctl);
		if (pi & sc->sc_ccc_mask) {
			/* A conflict with the implemented port list? */
			kprintf("%s: coalescing interrupt/implemented port list "
			    "conflict, PI: %08x, ccc_mask: %08x\n",
			    DEVNAME(sc), pi, sc->sc_ccc_mask);
			sc->sc_ccc_mask = 0;
			goto noccc;
		}

		/* ahci_port_start will enable each port when it starts. */
		sc->sc_ccc_ports = pi;
		sc->sc_ccc_ports_cur = 0;

		/* program thresholds and enable overall coalescing. */
		ccc_ctl &= ~0xffffff00;
		ccc_ctl |= (ccc_timeout << 16) | (ccc_numcomplete << 8);
		ahci_write(sc, AHCI_REG_CCC_CTL, ccc_ctl);
		ahci_write(sc, AHCI_REG_CCC_PORTS, 0);
		ahci_write(sc, AHCI_REG_CCC_CTL, ccc_ctl | 1);
	}
noccc:
#endif
	/*
	 * Allocate per-port resources
	 *
	 * Ignore attach errors, leave the port intact for
	 * rescan and continue the loop.
	 *
	 * All ports are attached in parallel but the CAM scan-bus
	 * is held up until all ports are attached so we get a deterministic
	 * order.
	 */
	for (i = 0; error == 0 && i < AHCI_MAX_PORTS; i++) {
		if ((pi & (1 << i)) == 0) {
			/* dont allocate stuff if the port isnt implemented */
			continue;
		}
		error = ahci_port_alloc(sc, i);
	}

	/*
	 * Setup the interrupt vector and enable interrupts.  Note that
	 * since the irq may be shared we do not set it up until we are
	 * ready to go.
	 */
	if (error == 0) {
		error = bus_setup_intr(dev, sc->sc_irq, INTR_MPSAFE,
				       ahci_intr, sc,
				       &sc->sc_irq_handle, NULL);
	}

	if (error) {
		device_printf(dev, "unable to install interrupt\n");
		ahci_pci_detach(dev);
		return (ENXIO);
	}

	/*
	 * Before marking the sc as good, which allows the interrupt
	 * subsystem to operate on the ports, wait for all the port threads
	 * to get past their initial pre-probe init.  Otherwise an interrupt
	 * may try to process the port before it has been initialized.
	 */
	for (i = 0; i < AHCI_MAX_PORTS; i++) {
		if ((ap = sc->sc_ports[i]) != NULL) {
			while (ap->ap_signal & AP_SIGF_THREAD_SYNC)
				ahci_os_sleep(100);
		}
	}

	/*
	 * Master interrupt enable, and call ahci_intr() in case we race
	 * our AHCI_F_INT_GOOD flag.
	 */
	crit_enter();
	ahci_write(sc, AHCI_REG_GHC, AHCI_REG_GHC_AE | AHCI_REG_GHC_IE);
	sc->sc_flags |= AHCI_F_INT_GOOD;
	crit_exit();
	ahci_intr(sc);

	/*
	 * All ports are probing in parallel.  Wait for them to finish
	 * and then issue the cam attachment and bus scan serially so
	 * the 'da' assignments are deterministic.
	 */
	for (i = 0; i < AHCI_MAX_PORTS; i++) {
		if ((ap = sc->sc_ports[i]) != NULL) {
			while (ap->ap_signal & AP_SIGF_INIT)
			    ahci_os_sleep(100);
			ahci_os_lock_port(ap);
			if (ahci_cam_attach(ap) == 0) {
				ahci_cam_changed(ap, NULL, -1);
				ahci_os_unlock_port(ap);
				while ((ap->ap_flags & AP_F_SCAN_COMPLETED) == 0) {
					ahci_os_sleep(100);
				}
			} else {
				ahci_os_unlock_port(ap);
			}
		}
	}

	return(0);
}

/*
 * Device unload / detachment
 */
static int
ahci_pci_detach(device_t dev)
{
	struct ahci_softc *sc = device_get_softc(dev);
	struct ahci_port *ap;
	int	i;

	/*
	 * Disable the controller and de-register the interrupt, if any.
	 *
	 * XXX interlock last interrupt?
	 */
	sc->sc_flags &= ~AHCI_F_INT_GOOD;
	if (sc->sc_regs)
		ahci_write(sc, AHCI_REG_GHC, 0);

	if (sc->sc_irq_handle) {
		bus_teardown_intr(dev, sc->sc_irq, sc->sc_irq_handle);
		sc->sc_irq_handle = NULL;
	}

	/*
	 * Free port structures and DMA memory
	 */
	for (i = 0; i < AHCI_MAX_PORTS; i++) {
		ap = sc->sc_ports[i];
		if (ap) {
			ahci_cam_detach(ap);
			ahci_port_free(sc, i);
		}
	}

	/*
	 * Clean up the bus space
	 */
	if (sc->sc_irq) {
		bus_release_resource(dev, SYS_RES_IRQ,
				     sc->sc_rid_irq, sc->sc_irq);
		sc->sc_irq = NULL;
	}
	if (sc->sc_regs) {
		bus_release_resource(dev, SYS_RES_MEMORY,
				     sc->sc_rid_regs, sc->sc_regs);
		sc->sc_regs = NULL;
	}

	if (sc->sc_tag_rfis) {
		bus_dma_tag_destroy(sc->sc_tag_rfis);
		sc->sc_tag_rfis = NULL;
	}
	if (sc->sc_tag_cmdh) {
		bus_dma_tag_destroy(sc->sc_tag_cmdh);
		sc->sc_tag_cmdh = NULL;
	}
	if (sc->sc_tag_cmdt) {
		bus_dma_tag_destroy(sc->sc_tag_cmdt);
		sc->sc_tag_cmdt = NULL;
	}
	if (sc->sc_tag_data) {
		bus_dma_tag_destroy(sc->sc_tag_data);
		sc->sc_tag_data = NULL;
	}

	return (0);
}
