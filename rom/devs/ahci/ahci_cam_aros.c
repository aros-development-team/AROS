/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/dos.h>

#include <dos/filehandler.h>

#include "ahci.h"

#include LC_LIBDEFS_FILE

static int ahci_RegisterPort(struct ahci_port *ap)
{
    struct AHCIBase *AHCIBase = ap->ap_sc->sc_dev->dev_AHCIBase;
    struct cam_sim *unit;

    unit = AllocPooled(AHCIBase->ahci_MemPool, sizeof(*unit));
    if (!unit)
        return ENOMEM;

    ap->ap_sim = unit;
    unit->sim_Port = ap;
    unit->sim_Unit = AHCIBase->ahci_UnitCount++;
    InitSemaphore(&unit->sim_Lock);
    NEWLIST(&unit->sim_ChangeInts);

    AddTail((struct List *)&AHCIBase->ahci_Units, (struct Node *)unit);

    return 0;
}

static int ahci_UnregisterPort(struct ahci_port *ap)
{
    struct ahci_softc *sc = ap->ap_sc;
    struct AHCIBase *AHCIBase;
    struct cam_sim *unit = ap->ap_sim;

    D(bug("ahci_UnregisterPort: %p\n", ap));

    if (sc == NULL) {
        D(bug("No softc?\n"));
        return 0;
    }

    AHCIBase = sc->sc_dev->dev_AHCIBase;

    /* FIXME: Stop IO on this device? */

    Remove((struct Node *)unit);
    FreePooled(AHCIBase->ahci_MemPool, unit, sizeof(*unit));

    return 0;
}

/* Add a bootnode using expansion.library */
static BOOL ahci_RegisterVolume(struct ahci_port *port)
{
    struct ata_port *at = port->ap_ata[0];
    struct ExpansionBase *ExpansionBase;
    struct DeviceNode *devnode;
    TEXT dosdevname[4] = "HA0";
    const ULONG DOS_ID = AROS_MAKE_ID('D','O','S','\001');
    const ULONG CDROM_ID = AROS_MAKE_ID('C','D','V','D');

    D(bug("ahci_RegisterVolume: port = %p, at = %p, unit = %d\n", port, at, port->ap_sim ? port->ap_sim->sim_Unit : -1));

    if (at == NULL || port->ap_type == ATA_PORT_T_NONE)
        return FALSE;

    /* This should be dealt with using some sort of volume manager or such. */
    switch (port->ap_type)
    {
        case ATA_PORT_T_DISK:
            break;
        case ATA_PORT_T_ATAPI:
            dosdevname[0] = 'C';
            break;
        default:
            D(bug("[AHCI>>]:-ahci_RegisterVolume called on unknown devicetype\n"));
            return FALSE;
    }

    ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library",
                                                        40L);

    if (ExpansionBase)
    {
        IPTR pp[4 + DE_BOOTBLOCKS + 1];

        if (port->ap_num < 10)
            dosdevname[2] += port->ap_num;
        else
            dosdevname[2] = 'A' + (port->ap_num - 10);
    
        pp[0] 		    = (IPTR)dosdevname;
        pp[1]		    = (IPTR)MOD_NAME_STRING;
        pp[2]		    = port->ap_sim->sim_Unit;
        pp[DE_TABLESIZE    + 4] = DE_BOOTBLOCKS;
        pp[DE_SIZEBLOCK    + 4] = at->at_identify.sector_size;
        pp[DE_NUMHEADS     + 4] = at->at_identify.nheads;
        pp[DE_SECSPERBLOCK + 4] = 1;
        pp[DE_BLKSPERTRACK + 4] = at->at_identify.nsectors;
        pp[DE_RESERVEDBLKS + 4] = 2;
        pp[DE_LOWCYL       + 4] = 0;
        pp[DE_HIGHCYL      + 4] = at->at_identify.ncyls-1;
        pp[DE_NUMBUFFERS   + 4] = 10;
        pp[DE_BUFMEMTYPE   + 4] = MEMF_PUBLIC;
        pp[DE_MAXTRANSFER  + 4] = 0x00200000;
        pp[DE_MASK         + 4] = ~3;
        pp[DE_BOOTPRI      + 4] = (port->ap_type == ATA_PORT_T_DISK) ? 0 : 10;
        pp[DE_DOSTYPE      + 4] = (port->ap_type == ATA_PORT_T_DISK) ? DOS_ID : CDROM_ID;
        pp[DE_BOOTBLOCKS   + 4] = 2;
    
        devnode = MakeDosNode(pp);

        if (devnode) {
            D(bug("[AHCI>>]:-ahci_RegisterVolume: '%s' C/H/S=%d/%d/%d, %s unit %d\n",
                        AROS_BSTR_ADDR(devnode->dn_Name), at->at_identify.ncyls, at->at_identify.nheads,  at->at_identify.nsectors, MOD_NAME_STRING, port->ap_sim->sim_Unit));
            AddBootNode(pp[DE_BOOTPRI + 4], 0, devnode, 0);
            D(bug("[AHCI>>]:-ahci_RegisterVolume: done\n"));
            return TRUE;
        }

        CloseLibrary((struct Library *)ExpansionBase);
    }

    return FALSE;
}


int ahci_cam_attach(struct ahci_port *ap)
{
    int error;

    D(bug("ahci_cam_attach: port %p\n", ap));

    ahci_os_unlock_port(ap);
    lockmgr(&ap->ap_sim_lock, LK_EXCLUSIVE);
    error = ahci_RegisterPort(ap);
    lockmgr(&ap->ap_sim_lock, LK_RELEASE);
    ahci_os_lock_port(ap);
    if (error) {
            ahci_cam_detach(ap);
            return (EINVAL);
    }
    ap->ap_flags |= AP_F_BUS_REGISTERED;

    if (ap->ap_probe == ATA_PROBE_NEED_IDENT)
            error = ahci_cam_probe(ap, NULL);
    else
            error = 0;
    if (error) {
            ahci_cam_detach(ap);
            return (EIO);
    }
    ap->ap_flags |= AP_F_CAM_ATTACHED;

    return 0;
}

void ahci_cam_detach(struct ahci_port *ap)
{
    D(bug("ahci_cam_detach: port %p\n", ap));

    lockmgr(&ap->ap_sim_lock, LK_EXCLUSIVE);
    if (ap->ap_flags & AP_F_BUS_REGISTERED) {
            ap->ap_flags &= ~AP_F_BUS_REGISTERED;
    }
    if (ap->ap_sim) {
            ahci_UnregisterPort(ap);
    }
    lockmgr(&ap->ap_sim_lock, LK_RELEASE);
    ap->ap_flags &= ~AP_F_CAM_ATTACHED;
}

/*
 * The state of the port has changed.
 *
 * If at is NULL the physical port has changed state.
 * If at is non-NULL a particular target behind a PM has changed state.
 *
 * If found is -1 the target state must be queued to a non-interrupt context.
 * (only works with at == NULL).
 *
 * If found is 0 the target was removed.
 * If found is 1 the target was inserted.
 */
void ahci_cam_changed(struct ahci_port *ap, struct ata_port *atx, int found)
{
    D(bug("ahci_cam_changed: ap=%p, sim = %p, atx=%p, found=%d\n", ap, ap->ap_sim, atx, found));

    if (ap && ap->ap_sim && found == -1)
        ahci_RegisterVolume(ap);

    /* Mark the port scan as completed */
    ap->ap_flags |= AP_F_SCAN_COMPLETED;
}

static void ahci_strip_string(const char **basep, int *lenp)
{
	const char *base = *basep;
	int len = *lenp;

	while (len && (*base == 0 || *base == ' ')) {
		--len;
		++base;
	}
	while (len && (base[len-1] == 0 || base[len-1] == ' '))
		--len;
	*basep = base;
	*lenp = len;
}

static u_int16_t bswap16(u_int16_t word)
{
    return ((word << 8) & 0xff00) |
           ((word >> 8) & 0x00ff);
}

/*
 * Fix byte ordering so buffers can be accessed as
 * strings.
 */
static void
ata_fix_identify(struct ata_identify *id)
{
	u_int16_t	*swap;
	int		i;

	swap = (u_int16_t *)id->serial;
	for (i = 0; i < sizeof(id->serial) / sizeof(u_int16_t); i++)
		swap[i] = bswap16(swap[i]);

	swap = (u_int16_t *)id->firmware;
	for (i = 0; i < sizeof(id->firmware) / sizeof(u_int16_t); i++)
		swap[i] = bswap16(swap[i]);

	swap = (u_int16_t *)id->model;
	for (i = 0; i < sizeof(id->model) / sizeof(u_int16_t); i++)
		swap[i] = bswap16(swap[i]);
}

/*
 * Dummy done callback for xa.
 */
static void ahci_ata_dummy_done(struct ata_xfer *xa)
{
}

/*
 * Setting the transfer mode is irrelevant for the SATA transport
 * but some (atapi) devices seem to need it anyway.  In addition
 * if we are running through a SATA->PATA converter for some reason
 * beyond my comprehension we might have to set the mode.
 *
 * We only support DMA modes for SATA attached devices, so don't bother
 * with legacy modes.
 */
static int
ahci_set_xfer(struct ahci_port *ap, struct ata_port *atx)
{
	struct ata_port *at;
	struct ata_xfer	*xa;
	u_int16_t mode;
	u_int16_t mask;

	at = atx ? atx : ap->ap_ata[0];

	/*
	 * Figure out the supported UDMA mode.  Ignore other legacy modes.
	 */
	mask = le16toh(at->at_identify.ultradma);
	if ((mask & 0xFF) == 0 || mask == 0xFFFF)
		return(0);
	mask &= 0xFF;
	mode = 0x4F;
	while ((mask & 0x8000) == 0) {
		mask <<= 1;
		--mode;
	}

	/*
	 * SATA atapi devices often still report a dma mode, even though
	 * it is irrelevant for SATA transport.  It is also possible that
	 * we are running through a SATA->PATA converter and seeing the
	 * PATA dma mode.
	 *
	 * In this case the device may require a (dummy) SETXFER to be
	 * sent before it will work properly.
	 */
	xa = ahci_ata_get_xfer(ap, atx);
	xa->complete = ahci_ata_dummy_done;
	xa->fis->command = ATA_C_SET_FEATURES;
	xa->fis->features = ATA_SF_SETXFER;
	xa->fis->flags = ATA_H2D_FLAGS_CMD | at->at_target;
	xa->fis->sector_count = mode;
	xa->flags = ATA_F_PIO | ATA_F_POLL;
	xa->timeout = 1000;
	xa->datalen = 0;
	if (ahci_ata_cmd(xa) != ATA_S_COMPLETE) {
		kprintf("%s: Unable to set dummy xfer mode \n",
			ATANAME(ap, atx));
	} else if (bootverbose) {
		kprintf("%s: Set dummy xfer mode to %02x\n",
			ATANAME(ap, atx), mode);
	}
	ahci_ata_put_xfer(xa);
	return(0);
}


/*
 * DISK-specific probe after initial ident
 */
static int
ahci_cam_probe_disk(struct ahci_port *ap, struct ata_port *atx)
{
	struct ata_port *at;
	struct ata_xfer	*xa;

	at = atx ? atx : ap->ap_ata[0];

	/*
	 * Set dummy xfer mode
	 */
	ahci_set_xfer(ap, atx);

	/*
	 * Enable write cache if supported
	 *
	 * NOTE: "WD My Book" external disk devices have a very poor
	 *	 daughter board between the the ESATA and the HD.  Sending
	 *	 any ATA_C_SET_FEATURES commands will break the hardware port
	 *	 with a fatal protocol error.  However, this device also
	 *	 indicates that WRITECACHE is already on and READAHEAD is
	 *	 not supported so we avoid the issue.
	 */
	if ((at->at_identify.cmdset82 & ATA_IDENTIFY_WRITECACHE) &&
	    (at->at_identify.features85 & ATA_IDENTIFY_WRITECACHE) == 0) {
		xa = ahci_ata_get_xfer(ap, atx);
		xa->complete = ahci_ata_dummy_done;
		xa->fis->command = ATA_C_SET_FEATURES;
		xa->fis->features = ATA_SF_WRITECACHE_EN;
		/* xa->fis->features = ATA_SF_LOOKAHEAD_EN; */
		xa->fis->flags = ATA_H2D_FLAGS_CMD | at->at_target;
		xa->fis->device = 0;
		xa->flags = ATA_F_PIO | ATA_F_POLL;
		xa->timeout = 1000;
		xa->datalen = 0;
		if (ahci_ata_cmd(xa) == ATA_S_COMPLETE)
			at->at_features |= ATA_PORT_F_WCACHE;
		else
			kprintf("%s: Unable to enable write-caching\n",
				ATANAME(ap, atx));
		ahci_ata_put_xfer(xa);
	}

	/*
	 * Enable readahead if supported
	 */
	if ((at->at_identify.cmdset82 & ATA_IDENTIFY_LOOKAHEAD) &&
	    (at->at_identify.features85 & ATA_IDENTIFY_LOOKAHEAD) == 0) {
		xa = ahci_ata_get_xfer(ap, atx);
		xa->complete = ahci_ata_dummy_done;
		xa->fis->command = ATA_C_SET_FEATURES;
		xa->fis->features = ATA_SF_LOOKAHEAD_EN;
		xa->fis->flags = ATA_H2D_FLAGS_CMD | at->at_target;
		xa->fis->device = 0;
		xa->flags = ATA_F_PIO | ATA_F_POLL;
		xa->timeout = 1000;
		xa->datalen = 0;
		if (ahci_ata_cmd(xa) == ATA_S_COMPLETE)
			at->at_features |= ATA_PORT_F_RAHEAD;
		else
			kprintf("%s: Unable to enable read-ahead\n",
				ATANAME(ap, atx));
		ahci_ata_put_xfer(xa);
	}

	/*
	 * FREEZE LOCK the device so malicious users can't lock it on us.
	 * As there is no harm in issuing this to devices that don't
	 * support the security feature set we just send it, and don't bother
	 * checking if the device sends a command abort to tell us it doesn't
	 * support it
	 */
	if ((at->at_identify.cmdset82 & ATA_IDENTIFY_SECURITY) &&
	    (at->at_identify.securestatus & ATA_SECURE_FROZEN) == 0 &&
	    (AhciNoFeatures & (1 << ap->ap_num)) == 0) {
		xa = ahci_ata_get_xfer(ap, atx);
		xa->complete = ahci_ata_dummy_done;
		xa->fis->command = ATA_C_SEC_FREEZE_LOCK;
		xa->fis->flags = ATA_H2D_FLAGS_CMD | at->at_target;
		xa->flags = ATA_F_PIO | ATA_F_POLL;
		xa->timeout = 1000;
		xa->datalen = 0;
		if (ahci_ata_cmd(xa) == ATA_S_COMPLETE)
			at->at_features |= ATA_PORT_F_FRZLCK;
		else
			kprintf("%s: Unable to set security freeze\n",
				ATANAME(ap, atx));
		ahci_ata_put_xfer(xa);
	}

	return (0);
}

/*
 * ATAPI-specific probe after initial ident
 */
static int
ahci_cam_probe_atapi(struct ahci_port *ap, struct ata_port *atx)
{
	ahci_set_xfer(ap, atx);
	return(0);
}


/*
 * Once the AHCI port has been attached we need to probe for a device or
 * devices on the port and setup various options.
 *
 * If at is NULL we are probing the direct-attached device on the port,
 * which may or may not be a port multiplier.
 */
int
ahci_cam_probe(struct ahci_port *ap, struct ata_port *atx)
{
	struct ata_port	*at;
	struct ata_xfer	*xa;
	u_int64_t	capacity;
	u_int64_t	capacity_bytes;
	int		model_len;
	int		firmware_len;
	int		serial_len;
	int		error;
	int		devncqdepth;
	int		i;
	const char	*model_id;
	const char	*firmware_id;
	const char	*serial_id;
	const char	*wcstr;
	const char	*rastr;
	const char	*scstr;
	const char	*type;

	error = EIO;

	/*
	 * A NULL atx indicates a probe of the directly connected device.
	 * A non-NULL atx indicates a device connected via a port multiplier.
	 * We need to preserve atx for calls to ahci_ata_get_xfer().
	 *
	 * at is always non-NULL.  For directly connected devices we supply
	 * an (at) pointing to target 0.
	 */
	if (atx == NULL) {
		at = ap->ap_ata[0];	/* direct attached - device 0 */
		if (ap->ap_type == ATA_PORT_T_PM) {
			kprintf("%s: Found Port Multiplier\n",
				ATANAME(ap, atx));
			return (0);
		}
		at->at_type = ap->ap_type;
	} else {
		at = atx;
		if (atx->at_type == ATA_PORT_T_PM) {
			kprintf("%s: Bogus device, reducing port count to %d\n",
				ATANAME(ap, atx), atx->at_target);
			if (ap->ap_pmcount > atx->at_target)
				ap->ap_pmcount = atx->at_target;
			goto err;
		}
	}
	if (ap->ap_type == ATA_PORT_T_NONE)
		goto err;
	if (at->at_type == ATA_PORT_T_NONE)
		goto err;

	/*
	 * Issue identify, saving the result
	 */
	xa = ahci_ata_get_xfer(ap, atx);
	xa->complete = ahci_ata_dummy_done;
	xa->data = &at->at_identify;
	xa->datalen = sizeof(at->at_identify);
	xa->flags = ATA_F_READ | ATA_F_PIO | ATA_F_POLL;
	xa->fis->flags = ATA_H2D_FLAGS_CMD | at->at_target;

	switch(at->at_type) {
	case ATA_PORT_T_DISK:
		xa->fis->command = ATA_C_IDENTIFY;
		type = "DISK";
		break;
	case ATA_PORT_T_ATAPI:
		xa->fis->command = ATA_C_ATAPI_IDENTIFY;
		xa->flags |= ATA_F_AUTOSENSE;
		type = "ATAPI";
		break;
	default:
		xa->fis->command = ATA_C_ATAPI_IDENTIFY;
		type = "UNKNOWN(ATAPI?)";
		break;
	}
	xa->fis->features = 0;
	xa->fis->device = 0;
	xa->timeout = 1000;

	if (ahci_ata_cmd(xa) != ATA_S_COMPLETE) {
		kprintf("%s: Detected %s device but unable to IDENTIFY\n",
			ATANAME(ap, atx), type);
		ahci_ata_put_xfer(xa);
		goto err;
	}
	ahci_ata_put_xfer(xa);

	ata_fix_identify(&at->at_identify);

	/*
	 * Read capacity using SATA probe info.
	 */
	if (le16toh(at->at_identify.cmdset83) & 0x0400) {
		/* LBA48 feature set supported */
		capacity = 0;
		for (i = 3; i >= 0; --i) {
			capacity <<= 16;
			capacity +=
			    le16toh(at->at_identify.addrsecxt[i]);
		}
	} else {
		capacity = le16toh(at->at_identify.addrsec[1]);
		capacity <<= 16;
		capacity += le16toh(at->at_identify.addrsec[0]);
	}
	if (capacity == 0)
		capacity = 1024 * 1024 / 512;
	at->at_capacity = capacity;
	if (atx == NULL)
		ap->ap_probe = ATA_PROBE_GOOD;

	capacity_bytes = capacity * 512;

	/*
	 * Negotiate NCQ, throw away any ata_xfer's beyond the negotiated
	 * number of slots and limit the number of CAM ccb's to one less
	 * so we always have a slot available for recovery.
	 *
	 * NCQ is not used if ap_ncqdepth is 1 or the host controller does
	 * not support it, and in that case the driver can handle extra
	 * ccb's.
	 *
	 * NCQ is currently used only with direct-attached disks.  It is
	 * not used with port multipliers or direct-attached ATAPI devices.
	 *
	 * Remember at least one extra CCB needs to be reserved for the
	 * error ccb.
	 */
	if ((ap->ap_sc->sc_cap & AHCI_REG_CAP_SNCQ) &&
	    ap->ap_type == ATA_PORT_T_DISK &&
	    (le16toh(at->at_identify.satacap) & (1 << 8))) {
		at->at_ncqdepth = (le16toh(at->at_identify.qdepth) & 0x1F) + 1;
		devncqdepth = at->at_ncqdepth;
		if (at->at_ncqdepth > ap->ap_sc->sc_ncmds)
			at->at_ncqdepth = ap->ap_sc->sc_ncmds;
		if (at->at_ncqdepth > 1) {
			for (i = 0; i < ap->ap_sc->sc_ncmds; ++i) {
				xa = ahci_ata_get_xfer(ap, atx);
				if (xa->tag < at->at_ncqdepth) {
					xa->state = ATA_S_COMPLETE;
					ahci_ata_put_xfer(xa);
				}
			}
#if 0
			if (at->at_ncqdepth >= ap->ap_sc->sc_ncmds) {
				cam_sim_set_max_tags(ap->ap_sim,
						     at->at_ncqdepth - 1);
			}
#endif
		}
	} else {
		devncqdepth = 0;
	}

	model_len = sizeof(at->at_identify.model);
	model_id = at->at_identify.model;
	ahci_strip_string(&model_id, &model_len);

	firmware_len = sizeof(at->at_identify.firmware);
	firmware_id = at->at_identify.firmware;
	ahci_strip_string(&firmware_id, &firmware_len);

	serial_len = sizeof(at->at_identify.serial);
	serial_id = at->at_identify.serial;
	ahci_strip_string(&serial_id, &serial_len);

	/*
	 * Generate informatiive strings.
	 *
	 * NOTE: We do not automatically set write caching, lookahead,
	 *	 or the security state for ATAPI devices.
	 */
	if (at->at_identify.cmdset82 & ATA_IDENTIFY_WRITECACHE) {
		if (at->at_identify.features85 & ATA_IDENTIFY_WRITECACHE)
			wcstr = "enabled";
		else if (at->at_type == ATA_PORT_T_ATAPI)
			wcstr = "disabled";
		else
			wcstr = "enabling";
	} else {
		    wcstr = "notsupp";
	}

	if (at->at_identify.cmdset82 & ATA_IDENTIFY_LOOKAHEAD) {
		if (at->at_identify.features85 & ATA_IDENTIFY_LOOKAHEAD)
			rastr = "enabled";
		else if (at->at_type == ATA_PORT_T_ATAPI)
			rastr = "disabled";
		else
			rastr = "enabling";
	} else {
		    rastr = "notsupp";
	}

	if (at->at_identify.cmdset82 & ATA_IDENTIFY_SECURITY) {
		if (at->at_identify.securestatus & ATA_SECURE_FROZEN)
			scstr = "frozen";
		else if (at->at_type == ATA_PORT_T_ATAPI)
			scstr = "unfrozen";
		else if (AhciNoFeatures & (1 << ap->ap_num))
			scstr = "<disabled>";
		else
			scstr = "freezing";
	} else {
		    scstr = "notsupp";
	}

	kprintf("%s: Found %s \"%*.*s %*.*s\" serial=\"%*.*s\"\n"
		"%s: tags=%d/%d satacap=%04x satafea=%04x NCQ=%s "
		"capacity=%lld.%02dMB\n",

		ATANAME(ap, atx),
		type,
		model_len, model_len, model_id,
		firmware_len, firmware_len, firmware_id,
		serial_len, serial_len, serial_id,

		ATANAME(ap, atx),
		devncqdepth, ap->ap_sc->sc_ncmds,
		at->at_identify.satacap,
		at->at_identify.satafsup,
		(at->at_ncqdepth > 1 ? "YES" : "NO"),
		(long long)capacity_bytes / (1024 * 1024),
		(int)(capacity_bytes % (1024 * 1024)) * 100 / (1024 * 1024)
	);
	kprintf("%s: f85=%04x f86=%04x f87=%04x WC=%s RA=%s SEC=%s\n",
		ATANAME(ap, atx),
		at->at_identify.features85,
		at->at_identify.features86,
		at->at_identify.features87,
		wcstr,
		rastr,
		scstr
	);

	/*
	 * Additional type-specific probing
	 */
	switch(at->at_type) {
	case ATA_PORT_T_DISK:
		error = ahci_cam_probe_disk(ap, atx);
		break;
	case ATA_PORT_T_ATAPI:
		error = ahci_cam_probe_atapi(ap, atx);
		break;
	default:
		error = EIO;
		break;
	}
err:
	if (error) {
		at->at_probe = ATA_PROBE_FAILED;
		if (atx == NULL)
			ap->ap_probe = at->at_probe;
	} else {
		at->at_probe = ATA_PROBE_GOOD;
		if (atx == NULL)
			ap->ap_probe = at->at_probe;
	}
	return (error);
}



