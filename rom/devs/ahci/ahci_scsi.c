/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <exec/errors.h>
#include <devices/scsidisk.h>
#include <devices/trackdisk.h>
#include <scsi/commands.h>
#include <scsi/values.h>

#include "ahci.h"
#include "ahci_scsi.h"

#undef offsetof
#undef container_of

#define offsetof(TYPE, MEMBER) ((IPTR) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({          \
    const typeof(((type *)0)->member) *__mptr = (ptr);    \
             (type *)((char *)__mptr - offsetof(type, member)); })


static inline UQUAD scsi_8btou64(UBYTE *addr)
{
    UQUAD res = 0;
    int i;

    for (i = 0; i < 8; i++, res <<= 8, res |= *(addr++));
    return res;
}

static inline ULONG scsi_4btoul(UBYTE *addr)
{
    ULONG res = 0;
    int i;

    for (i = 0; i < 4; i++, res <<= 8, res |= *(addr++));
    return res;
}

static inline ULONG scsi_3btoul(UBYTE *addr)
{
    ULONG res = 0;
    int i;

    for (i = 0; i < 3; i++, res <<= 8, res |= *(addr++));
    return res;
}

static inline ULONG scsi_2btoul(UBYTE *addr)
{
    UWORD res = 0;
    int i;

    for (i = 0; i < 2; i++, res <<= 8, res |= *(addr++));
    return res;
}

static inline void scsi_ulto4b(ULONG val, UBYTE *addr)
{
    int i;
    for (i = 3; i >= 0; i--, val >>= 8)
        addr[i] = val & 0xff;
}

/*
 * Construct dummy sense data for disks, and ATAPI devices
 * that do not support extended status
 *
 * In the io_complete, there should a REQUEST SENSE command
 * for ATAPI devices
 */
static void ahci_ata_sense(struct ata_xfer *xa,
        struct scsi_sense_data *sense_data)
{
    struct ata_fis_d2h *rfis = &xa->rfis;
    UBYTE asc = 0, asq = 0, key = 0;

    if ((rfis->status & ATA_D2H_STATUS_BSY)) {
        key = SSD_KEY_ABORTED_COMMAND;
    } else {
        /* Decode error bits */
        switch (rfis->error & 0xff) {
        case ATA_D2H_ERROR_BBK|ATA_D2H_ERROR_UNK|ATA_D2H_ERROR_IDNF:
        case ATA_D2H_ERROR_BBK|ATA_D2H_ERROR_UNK|ATA_D2H_ERROR_IDNF|ATA_D2H_ERROR_AMNF:
                /* Device busy, aborted command */
        case ATA_D2H_ERROR_ABRT:
                /* Aborted command */
                key = SSD_KEY_ABORTED_COMMAND;
                asc = 0x00; asq = 0x00; /* no additional sense code */
                break;
        case ATA_D2H_ERROR_UNK|ATA_D2H_ERROR_MC|ATA_D2H_ERROR_AMNF:
                /* Hardware fault */
                key = SSD_KEY_HARDWARE_ERROR;
                asc = 0x44; asq = 0x00; /* internal target failure */
                break;
        case ATA_D2H_ERROR_BBK|ATA_D2H_ERROR_ABRT:
                /* Data partiy error */
                key = SSD_KEY_ABORTED_COMMAND;
                asc = 0x4b; asq = 0x00;   /* data phase error */
                break;
        case ATA_D2H_ERROR_MC:
                /* No media */
                key = SSD_KEY_NOT_READY;
                asc = 0x3a; asq = 0x00;   /* medium not present */
                break;
        case ATA_D2H_ERROR_MCR:
                /* Media change request */
                key = SSD_KEY_NOT_READY;
                asc = 0x04; asq = 0x03;    /* manual intervention required */
                break;
        case ATA_D2H_ERROR_MC|ATA_D2H_ERROR_IDNF|ATA_D2H_ERROR_ABRT|ATA_D2H_ERROR_TK0NF|ATA_D2H_ERROR_AMNF:
        case ATA_D2H_ERROR_MCR|ATA_D2H_ERROR_AMNF:
                /* Unit offline or not ready */
                key = SSD_KEY_NOT_READY;
                asc = 0x04; asq = 0x00;    /* unknown reason */
                break;
        case ATA_D2H_ERROR_AMNF:
                /* No address mark found */
                key = SSD_KEY_MEDIUM_ERROR;
                asc = 0x31; asq = 0x00;    /* medium format corrupted */
                break;
        case ATA_D2H_ERROR_TK0NF:
                /* Track 0 not found */
                key = SSD_KEY_HARDWARE_ERROR;
                asc = 0x02; asq = 0x00;    /* no seek complete */
                break;
        case ATA_D2H_ERROR_IDNF:
                /* Sector not found */
                key = SSD_KEY_ILLEGAL_REQUEST;
                asc = 0x21; asq = 0x00;    /* LBA out of range */
                break;
        case ATA_D2H_ERROR_BBK:
                /* Bad block (now defined as interface CRC in ATA8-ACS) */
                key = SSD_KEY_ABORTED_COMMAND;
                asc = 0x47; asq = 0x00;    /* SCSI partity error */
                break;
        case ATA_D2H_ERROR_UNK:
                /* ECC failure */
                key = SSD_KEY_MEDIUM_ERROR;
                if ((xa->fis->flags & ATA_H2D_FEATURES_DIR) == ATA_H2D_FEATURES_DIR_WRITE) {
                        asc = 0x10; asq = 0x00;    /* write fault */
                } else {
                        asc = 0x11; asq = 0x00;    /* read fault */
                }
                break;
        default:
                D(bug("ahci.device: No sense translation for ATA Error 0x%02x\n", rfis->error));
                switch (rfis->status) {
                case ATA_D2H_STATUS_DF:
                    key = SSD_KEY_HARDWARE_ERROR;
                    asc = 0x44; asq = 0x00;     /* Internal target failure */
                    break;
                case ATA_D2H_STATUS_DRQ:
                    key = SSD_KEY_ABORTED_COMMAND;
                    asc = 0x4B; asq = 0x00;     /* Data phase error */
                    break;
                case ATA_D2H_STATUS_CORR:
                    if ((xa->fis->flags & ATA_H2D_FEATURES_DIR) == ATA_H2D_FEATURES_DIR_WRITE) {
                        key = SSD_KEY_RECOVERED_ERROR;
                        asc = 0x0c; asq = 0x01; /* Recovered write with realloc */
                    } else {
                        key = SSD_KEY_RECOVERED_ERROR;
                        asc = 0x18; asq = 0x02; /* Recovered read  with realloc */
                    }
                    break;
                default:
                    D(bug("ahci.device: No sense translation for ATA Status 0x%02x\n", rfis->status));
                    key = SSD_KEY_ABORTED_COMMAND;
                    break;
                }
                break;
        }
    }

    memset(sense_data, 0, sizeof(*sense_data));

    sense_data->error_code = SSD_ERRCODE_VALID | SSD_CURRENT_ERROR;

    sense_data->flags = ((rfis->error & 0xF0) >> 4) | key;
    if (rfis->error & ATA_D2H_ERROR_AMNF)
        sense_data->flags |= SSD_ILI;
    sense_data->add_sense_code = asc;
    sense_data->add_sense_qual = asq;
}

static void ahci_io_complete(struct ata_xfer *xa)
{
    struct IORequest *io = xa->atascsi_private;
    const int sense_length = offsetof(struct scsi_sense_data, extra_bytes[0]);
    struct cam_sim *unit = (struct cam_sim *)io->io_Unit;

    switch (xa->state) {
    case ATA_S_COMPLETE:
        if (io->io_Command == HD_SCSICMD) {
            struct SCSICmd *scsi = IOStdReq(io)->io_Data;
            scsi->scsi_Status = SCSI_GOOD;
            scsi->scsi_Actual = xa->datalen;
            IOStdReq(io)->io_Actual = sizeof(*scsi);
        } else {
            IOStdReq(io)->io_Actual = xa->datalen;
        }
        break;
    case ATA_S_ERROR:
        if (io->io_Command == HD_SCSICMD) {
            struct SCSICmd *scsi = IOStdReq(io)->io_Data;
            D(bug("Error on HD_SCSICMD\n"));
            scsi->scsi_Status = SCSI_CHECK_CONDITION;
            scsi->scsi_Actual = 0;
            IOStdReq(io)->io_Actual = sizeof(*scsi);
            if (scsi->scsi_Flags & (SCSIF_AUTOSENSE | SCSIF_OLDAUTOSENSE)) {
                D(bug("SCSIF_AUTOSENSE desired\n"));
                if (scsi->scsi_SenseData && scsi->scsi_SenseLength >= sense_length) {
                    ahci_ata_sense(xa, (void *)scsi->scsi_SenseData);
                    scsi->scsi_SenseActual = sense_length;
                    D(bug("SCSI Sense: KCQ = 0x%02x 0x%02x 0x%02x\n",
                                scsi->scsi_SenseData[2],
                                scsi->scsi_SenseData[12],
                                scsi->scsi_SenseData[13]));
                }
            }
        } else {
            io->io_Error = TDERR_SeekError;
            IOStdReq(io)->io_Actual = 0;
        }
        break;
    case ATA_S_TIMEOUT:
        if (io->io_Command == HD_SCSICMD) {
            struct SCSICmd *scsi = IOStdReq(io)->io_Data;
            scsi->scsi_Status = SCSI_BUSY;
            scsi->scsi_Actual = 0;
            IOStdReq(io)->io_Actual = sizeof(*scsi);
        } else {
            io->io_Error = IOERR_UNITBUSY;
            IOStdReq(io)->io_Actual = 0;
        }
        break;
    default:
        io->io_Error = IOERR_NOCMD;
        break;
    }

    ahci_ata_put_xfer(xa);

    ObtainSemaphore(&unit->sim_Lock);
    Remove(&io->io_Message.mn_Node);
    ReleaseSemaphore(&unit->sim_Lock);

    ASSERT(!(io->io_Flags & IOF_QUICK));

    D(bug("[AHCI%02ld] IO %p Final, io_Flags = %d, io_Error = %d\n", unit->sim_Unit, io, io->io_Flags, io->io_Error));

    ReplyMsg(&io->io_Message);
}


/*
 * Simulate page inquiries for disk attachments.
 */
static BYTE ahci_scsi_page_inquiry(struct ahci_port *ap, struct ata_port *at, struct SCSICmd *scsi)
{
    union {
        struct scsi_vpd_supported_page_list    list;
        struct scsi_vpd_unit_serial_number    serno;
        UBYTE                    buf[256];
    } page;
    scsi_cdb_t cdb;
    int i;
    int j;
    int len;

    cdb = (APTR)scsi->scsi_Command;

    switch(cdb->inquiry.page_code) {
    case SVPD_SUPPORTED_PAGE_LIST:
        i = 0;
        page.list.device = T_DIRECT;
        page.list.page_code = SVPD_SUPPORTED_PAGE_LIST;
        page.list.list[i++] = SVPD_SUPPORTED_PAGE_LIST;
        page.list.list[i++] = SVPD_UNIT_SERIAL_NUMBER;
        page.list.length = i;
        len = offsetof(struct scsi_vpd_supported_page_list, list[3]);
        break;
    case SVPD_UNIT_SERIAL_NUMBER:
        i = 0;
        j = sizeof(at->at_identify.serial);
        for (i = 0; i < j && at->at_identify.serial[i] == ' '; ++i)
            ;
        while (j > i && at->at_identify.serial[j-1] == ' ')
            --j;
        page.serno.device = T_DIRECT;
        page.serno.page_code = SVPD_UNIT_SERIAL_NUMBER;
        page.serno.length = j - i;
        CopyMem(at->at_identify.serial + i,
              page.serno.serial_num, j - i);
        len = offsetof(struct scsi_vpd_unit_serial_number,
                   serial_num[j-i]);
        break;
    default:
        return IOERR_NOCMD;
        break;
    }

    if (len > scsi->scsi_Length)
        return IOERR_BADLENGTH;

    memset(scsi->scsi_Data + len, 0, scsi->scsi_Length - len);
    CopyMem(&page, scsi->scsi_Data, len);
    scsi->scsi_Actual = len;

    return 0;
}


/*
 * Convert the SCSI command to an ata_xfer command in xa
 * for ATA_PORT_T_DISK operations.  Set the completion function
 * to convert the response back, then dispatch to the OpenBSD AHCI
 * layer.
 *
 * AHCI DISK commands only support a limited command set, and we
 * fake additional commands to make it play nice.
 *
 * Return TRUE if no need to wait for a reply
 */
BOOL ahci_scsi_disk_io(struct IORequest *io, struct SCSICmd *scsi)
{
    struct cam_sim *unit = (struct cam_sim *)io->io_Unit;
    struct ahci_port *ap = unit->sim_Port;
    struct ata_port  *at = ap->ap_ata[0];
    struct ata_xfer *xa;
    struct ata_fis_h2d *fis;
    union {
        struct scsi_inquiry_data inquiry_data;
        struct scsi_read_capacity_data read_capacity_data;
    } *rdata = (APTR)scsi->scsi_Command;
    ULONG rdata_len = scsi->scsi_CmdLength;
    u_int64_t capacity;
    u_int64_t lba;
    u_int32_t count;
    BOOL done = FALSE;
    scsi_cdb_t cdb = (APTR)scsi->scsi_Command;

    xa = ahci_ata_get_xfer(ap, at);

    switch(cdb->generic.opcode) {
    case SCSI_REQUEST_SENSE:
        /*
         * Auto-sense everything, so explicit sense requests
         * return no-sense.
         */
        io->io_Error = HFERR_BadStatus;
        done = TRUE;
        break;
    case SCSI_INQUIRY:
        done = TRUE;
        /*
         * Inquiry supported features
         *
         * [opcode, byte2, page_code, length, control]
         */
        if (cdb->inquiry.byte2 & SI_EVPD) {
            io->io_Error = ahci_scsi_page_inquiry(ap, at, scsi);
        } else {
            memset(rdata, 0, rdata_len);
            if (rdata_len < SHORT_INQUIRY_LENGTH) {
                io->io_Error = IOERR_BADLENGTH;
                break;
            }
            if (rdata_len > sizeof(rdata->inquiry_data))
                rdata_len = sizeof(rdata->inquiry_data);
            rdata->inquiry_data.device = T_DIRECT;
            /* Mark as removable if ATA has the 'removable' tag */
            rdata->inquiry_data.dev_qual2 = (at->at_identify.config ? 0x80 : 0);
            rdata->inquiry_data.version = SCSI_REV_SPC2;
            rdata->inquiry_data.response_format = 2;
            rdata->inquiry_data.additional_length = 32;

            /* 
             * Use the vendor specific area to set the TRIM status
             */
            if (at->at_identify.support_dsm) {
                rdata->inquiry_data.vendor_specific[0] =
                    at->at_identify.support_dsm &ATA_SUPPORT_DSM_TRIM;
                rdata->inquiry_data.vendor_specific[1] =
                    at->at_identify.max_dsm_blocks;
            }
            CopyMem("SATA    ", rdata->inquiry_data.vendor, 8);
            CopyMem(at->at_identify.model,
                  rdata->inquiry_data.product,
                  sizeof(rdata->inquiry_data.product));
            CopyMem(at->at_identify.firmware,
                  rdata->inquiry_data.revision,
                  sizeof(rdata->inquiry_data.revision));
        }
        break;
    case SCSI_DA_READ_CAPACITY:
        done = TRUE;
        if (rdata_len < sizeof(rdata->read_capacity_data)) {
            io->io_Error = IOERR_BADLENGTH;
            break;
        }

        capacity = at->at_capacity;

        memset(rdata, 0, rdata_len);
        rdata_len = sizeof(rdata->read_capacity_data);
        if (capacity > 0xFFFFFFFFU)
            capacity = 0xFFFFFFFFU;
        memset(&rdata->read_capacity_data, 0, rdata_len);
        scsi_ulto4b((u_int32_t)capacity - 1,
                rdata->read_capacity_data.addr);
        scsi_ulto4b(512, rdata->read_capacity_data.length);
        break;
    case SCSI_DA_SYNCHRONIZE_CACHE:
        /*
         * Synchronize cache.  Specification says this can take
         * greater then 30 seconds so give it at least 45.
         */
        fis = xa->fis;
        fis->flags = ATA_H2D_FLAGS_CMD;
        fis->command = ATA_C_FLUSH_CACHE;
        fis->device = 0;
        if (xa->timeout < 45000)
            xa->timeout = 45000;
        xa->datalen = 0;
        xa->flags = 0;
        xa->complete = ahci_io_complete;
        break;
    case SCSI_TEST_UNIT_READY:
        /*
         * Just silently return success
         */
        done = TRUE;
        rdata_len = 0;
        break;
    default:
        switch(cdb->generic.opcode) {
        case SCSI_DA_READ_6:
            lba = scsi_3btoul(cdb->rw_6.addr) & 0x1FFFFF;
            count = cdb->rw_6.length ? cdb->rw_6.length : 0x100;
            xa->flags = ATA_F_READ;
            break;
        case SCSI_DA_READ_10:
            lba = scsi_4btoul(cdb->rw_10.addr);
            count = scsi_2btoul(cdb->rw_10.length);
            xa->flags = ATA_F_READ;
            break;
        case SCSI_DA_READ_12:
            lba = scsi_4btoul(cdb->rw_12.addr);
            count = scsi_4btoul(cdb->rw_12.length);
            xa->flags = ATA_F_READ;
            break;
        case SCSI_DA_READ_16:
            lba = scsi_8btou64(cdb->rw_16.addr);
            count = scsi_4btoul(cdb->rw_16.length);
            xa->flags = ATA_F_READ;
            break;
        case SCSI_DA_WRITE_6:
            lba = scsi_3btoul(cdb->rw_6.addr) & 0x1FFFFF;
            count = cdb->rw_6.length ? cdb->rw_6.length : 0x100;
            xa->flags = ATA_F_WRITE;
            break;
        case SCSI_DA_WRITE_10:
            lba = scsi_4btoul(cdb->rw_10.addr);
            count = scsi_2btoul(cdb->rw_10.length);
            xa->flags = ATA_F_WRITE;
            break;
        case SCSI_DA_WRITE_12:
            lba = scsi_4btoul(cdb->rw_12.addr);
            count = scsi_4btoul(cdb->rw_12.length);
            xa->flags = ATA_F_WRITE;
            break;
        case SCSI_DA_WRITE_16:
            lba = scsi_8btou64(cdb->rw_16.addr);
            count = scsi_4btoul(cdb->rw_16.length);
            xa->flags = ATA_F_WRITE;
            break;
        default:
            io->io_Error = IOERR_NOCMD;
            done = TRUE;
            break;
        }
        if (done)
            break;

        fis = xa->fis;
        fis->flags = ATA_H2D_FLAGS_CMD;
        fis->lba_low = (u_int8_t)lba;
        fis->lba_mid = (u_int8_t)(lba >> 8);
        fis->lba_high = (u_int8_t)(lba >> 16);
        fis->device = ATA_H2D_DEVICE_LBA;

        /*
         * NCQ only for direct-attached disks, do not currently
         * try to use NCQ with port multipliers.
         */
        if (at->at_ncqdepth > 1 &&
            ap->ap_type == ATA_PORT_T_DISK &&
            (ap->ap_sc->sc_cap & AHCI_REG_CAP_SNCQ)) {
            /*
             * Use NCQ - always uses 48 bit addressing
             */
            xa->flags |= ATA_F_NCQ;
            fis->command = (xa->flags & ATA_F_WRITE) ?
                    ATA_C_WRITE_FPDMA : ATA_C_READ_FPDMA;
            fis->lba_low_exp = (u_int8_t)(lba >> 24);
            fis->lba_mid_exp = (u_int8_t)(lba >> 32);
            fis->lba_high_exp = (u_int8_t)(lba >> 40);
            fis->sector_count = xa->tag << 3;
            fis->features = (u_int8_t)count;
            fis->features_exp = (u_int8_t)(count >> 8);
        } else if (count > 0x100 || lba > 0x0FFFFFFFU) {
            /*
             * Use LBA48
             */
            fis->command = (xa->flags & ATA_F_WRITE) ?
                    ATA_C_WRITEDMA_EXT : ATA_C_READDMA_EXT;
            fis->lba_low_exp = (u_int8_t)(lba >> 24);
            fis->lba_mid_exp = (u_int8_t)(lba >> 32);
            fis->lba_high_exp = (u_int8_t)(lba >> 40);
            fis->sector_count = (u_int8_t)count;
            fis->sector_count_exp = (u_int8_t)(count >> 8);
        } else {
            /*
             * Use LBA
             *
             * NOTE: 256 sectors is supported, stored as 0.
             */
            fis->command = (xa->flags & ATA_F_WRITE) ?
                    ATA_C_WRITEDMA : ATA_C_READDMA;
            fis->device |= (u_int8_t)(lba >> 24) & 0x0F;
            fis->sector_count = (u_int8_t)count;
        }

        xa->data = scsi->scsi_Data;
        xa->datalen = scsi->scsi_Length;
        xa->complete = ahci_io_complete;
        xa->timeout = 1000;    /* milliseconds */
#if 0
        if (xa->timeout > 10000)    /* XXX - debug */
            xa->timeout = 10000;
#endif
        break;
    }

    /*
     * If the request is still in progress the xa and FIS have
     * been set up (except for the PM target), and must be dispatched.
     * Otherwise the request was completed.
     */
    if (!done) {
        io->io_Flags &= ~IOF_QUICK;
        KKASSERT(xa->complete != NULL);
        xa->atascsi_private = io;
        ahci_os_lock_port(ap);
        xa->fis->flags |= at->at_target;
        ahci_ata_cmd(xa);
        ahci_os_unlock_port(ap);
    } else {
        IOStdReq(io)->io_Actual = sizeof(*scsi);
        ahci_ata_put_xfer(xa);
    }

    return done;
}

/*
 * Convert the SCSI command in ccb to an ata_xfer command in xa
 * for ATA_PORT_T_ATAPI operations.  Set the completion function
 * to convert the response back, then dispatch to the OpenBSD AHCI
 * layer.
 */
BOOL ahci_scsi_atapi_io(struct IORequest *io, struct SCSICmd *scsi)
{
    struct cam_sim *unit = (struct cam_sim *)io->io_Unit;
    struct ahci_port *ap = unit->sim_Port;
    struct ata_port  *at = ap->ap_ata[0];
    struct ata_xfer *xa;
    struct ata_fis_h2d *fis;
    scsi_cdb_t cdbs;
    scsi_cdb_t cdbd;
    uint32_t offset, len;
    int flags;

    if (scsi->scsi_Flags & SCSIF_READ) {
        flags = ATA_F_PACKET | ATA_F_READ;
    } else {
        flags = ATA_F_PACKET | ATA_F_WRITE;
    }

    /*
     * Special handling to get the rfis back into host memory while
     * still allowing the chip to run commands in parallel to
     * ATAPI devices behind a PM.
     */
    if (scsi->scsi_Flags & (SCSIF_AUTOSENSE | SCSIF_OLDAUTOSENSE))
        flags |= ATA_F_AUTOSENSE;

    /*
     * The command has to fit in the packet command buffer.
     */
    if (scsi->scsi_CmdLength < 6 || scsi->scsi_CmdLength > 16) {
        io->io_Error = IOERR_NOCMD;
        return TRUE;
    }

    /*
     * Initialize the XA and FIS.  It is unclear how much of
     * this has to mimic the equivalent ATA command.
     *
     * XXX not passing NULL at for direct attach!
     */
    xa = ahci_ata_get_xfer(ap, at);
    if (xa == NULL) {
        io->io_Error = IOERR_UNITBUSY;
        return TRUE;
    }
    fis = xa->fis;

    fis->flags = ATA_H2D_FLAGS_CMD | at->at_target;
    fis->command = ATA_C_PACKET;
    fis->device = ATA_H2D_DEVICE_LBA;
    fis->sector_count = xa->tag << 3;
    if (flags & (ATA_F_READ | ATA_F_WRITE)) {
        if (flags & ATA_F_WRITE) {
            fis->features = ATA_H2D_FEATURES_DMA |
                       ATA_H2D_FEATURES_DIR_WRITE;
        } else {
            fis->features = ATA_H2D_FEATURES_DMA |
                       ATA_H2D_FEATURES_DIR_READ;
        }
    } else {
        fis->lba_mid = 0;
        fis->lba_high = 0;
    }
    fis->control = ATA_FIS_CONTROL_4BIT;

    xa->flags = flags;
    xa->data = scsi->scsi_Data;
    xa->datalen = scsi->scsi_Length;
    xa->timeout = 1000;    /* milliseconds */

    /*
     * Copy the cdb to the packetcmd buffer in the FIS using a
     * convenient pointer in the xa.
     *
     * Zero-out any trailing bytes in case the ATAPI device cares.
     */
    cdbs = (void *)scsi->scsi_Command;
    CopyMem(cdbs, xa->packetcmd, scsi->scsi_CmdLength);
    if (scsi->scsi_CmdLength < 16)
        memset(xa->packetcmd + scsi->scsi_CmdLength, 0, 16 - scsi->scsi_CmdLength);

#if 0
    kprintf("opcode %d cdb_len %d dxfer_len %d\n",
        cdbs->generic.opcode,
        scsi->scsi_CmdLength, scsi->scsi_Length);
#endif

    /*
     * Some ATAPI commands do not actually follow the SCSI standard.
     */
    cdbd = (void *)xa->packetcmd;

    switch(cdbd->generic.opcode) {
    case SCSI_REQUEST_SENSE:
        /*
         * Force SENSE requests to the ATAPI sense length.
         *
         * It is unclear if this is needed or not.
         */
        if (cdbd->sense.length == SSD_FULL_SIZE) {
            if (bootverbose) {
                kprintf("%s: Shortening sense request\n",
                    PORTNAME(ap));
            }
            cdbd->sense.length = offsetof(struct scsi_sense_data,
                              extra_bytes[0]);
        }
        break;
    case SCSI_INQUIRY:
        /*
         * Some ATAPI devices can't handle long inquiry lengths,
         * don't ask me why.  Truncate the inquiry length.
         */
        if (cdbd->inquiry.page_code == 0 &&
            cdbd->inquiry.length > SHORT_INQUIRY_LENGTH) {
            cdbd->inquiry.length = SHORT_INQUIRY_LENGTH;
        }
        break;
    case SCSI_DA_READ_6:
    case SCSI_DA_WRITE_6:
        offset = (cdbd->rw_6.addr[0] << 16) |
                 (cdbd->rw_6.addr[1] << 8) |
                 (cdbd->rw_6.addr[2]);
        len    = cdbd->rw_6.length ? cdbd->rw_6.length : 0x100;
        /*
         * Convert *_6 to *_10 commands.  Most ATAPI devices
         * cannot handle the SCSI READ_6 and WRITE_6 commands.
         */
        cdbd->rw_10.opcode |= 0x20;
        cdbd->rw_10.control = cdbs->rw_6.control;
        cdbd->rw_10.byte2 = 0;
        cdbd->rw_10.reserved = 0;

        cdbd->rw_10.addr[0] = (offset >> 24) & 0xff;
        cdbd->rw_10.addr[1] = (offset >> 16) & 0xff;
        cdbd->rw_10.addr[2] = (offset >>  8) & 0xff;
        cdbd->rw_10.addr[3] = (offset >>  0) & 0xff;

        cdbd->rw_10.length[0] = (len >> 8) & 0xff;
        cdbd->rw_10.length[1] = (len >> 0) & 0xff;
        break;
    default:
        break;
    }

    /*
     * And dispatch
     */
    io->io_Flags &= ~IOF_QUICK;
    xa->complete = ahci_io_complete;
    xa->atascsi_private = io;
    ahci_os_lock_port(ap);
    ahci_ata_cmd(xa);
    ahci_os_unlock_port(ap);

    return FALSE;
}

