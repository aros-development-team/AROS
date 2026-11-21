/*
    Copyright (C) 2011-2026, The AROS Development Team. All rights reserved

    Desc: Simple HD_SCSICMD emulator.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include <exec/types.h>
#include <exec/exec.h>
#include <devices/scsidisk.h>

#include <string.h>

#include "ata.h"

#undef UtilityBase

static void wl(UBYTE *p, ULONG v)
{
    p[0] = v >> 24;
    p[1] = v >> 16;
    p[2] = v >> 8;
    p[3] = v;
}

static void ww(UBYTE *p, UWORD v)
{
    p[0] = v >> 8;
    p[1] = v;
}

static ULONG rl(UBYTE *p)
{
    return ((ULONG)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | (p[3]);
}

static UBYTE scsi_inquiry(struct ata_Unit *unit, struct SCSICmd *cmd, ULONG *outlen)
{
    struct Library *UtilityBase = unit->au_Bus->ab_Base->ata_UtilityBase;
    UBYTE *cmdbuf = cmd->scsi_Command;
    UBYTE out[36] = {0};
    ULONG len;
    
    if ((cmdbuf[1] & 1) || cmdbuf[2] != 0)
        return 0xff;
    len = cmdbuf[4];
    if (cmdbuf[1] >> 5)
        return 0xff; /* no lun supported */
    out[2] = 2; /* supports SCSI-2 */
    out[3] = 2; /* response data format */
    out[4] = sizeof(out) - 5; /* additional length */
    out[7] = 0x20; /* 16 bit bus */
    if (len > cmd->scsi_Length)
        len = cmd->scsi_Length;
    *outlen = len < sizeof(out) ? len : sizeof(out);
    SetMem(out + 8, ' ', 8 + 16 + 4);
    CopyMem(unit->au_Model, out + 8, strlen(unit->au_Model) > 16 + 8 ? 16 + 8 : strlen(unit->au_Model));
    CopyMem(unit->au_FirmwareRev, out + 8 + 16, strlen(unit->au_FirmwareRev) > 4 ? 4 : strlen(unit->au_FirmwareRev));
    if (*outlen)
        CopyMem(out, cmd->scsi_Data, *outlen);
    return 0;
}

static UBYTE scsi_modesense(struct ata_Unit *unit, struct SCSICmd *cmd, ULONG *outlen)
{
    UBYTE *cmdbuf = cmd->scsi_Command;
    UBYTE out[36] = {0};
    UBYTE pcode = cmdbuf[2] & 0x3f;
    ULONG blocksize = 1UL << unit->au_SectorShift;
    ULONG len = 4;
    UBYTE *p;

    if (!(cmdbuf[1] & 8)) {
        UQUAD blocks = unit->au_Capacity48;
        out[3] = 8;
        wl(out + 4, blocks > 0xffffff ? 0xffffff : (ULONG)blocks);
        wl(out + 8, blocksize);
        len += 8;
    }
    p = out + len;
    if (pcode == 0) {
        p[2] = 0x20;
        len += 4;
    } else if (pcode == 3) {
        p[0] = 3;
        p[1] = 22;
        p[3] = 1;
        ww(p + 10, unit->au_Sectors);
        ww(p + 12, blocksize);
        p[15] = 1; /* interleave */
        p[20] = 0x80;
        len += 24;
    } else if (pcode == 4) {
        p[0] = 4;
        wl(p + 1, unit->au_Cylinders);
        p[1] = 22;
        p[5] = unit->au_Heads;
        wl(p + 13, unit->au_Cylinders);
        ww(p + 20, 5400);
        len += 24;
    } else {
        return 0xff;
    }
    out[0] = len - 1;
    if (len > cmdbuf[4])
        len = cmdbuf[4];
    if (len > cmd->scsi_Length)
        len = cmd->scsi_Length;
    if (len)
        CopyMem(out, cmd->scsi_Data, len);
    *outlen = len;
    return 0;
}

static UBYTE scsi_readcapacity(struct ata_Unit *unit, struct SCSICmd *cmd, ULONG *outlen)
{
    UBYTE *cmdbuf = cmd->scsi_Command;
    UBYTE out[8];
    UQUAD blocks = unit->au_Capacity48;
    UQUAD last;
    ULONG lba = rl(cmdbuf + 2);

    if (!blocks || (!(cmdbuf[8] & 1) && lba))
        return 0xff;
    last = blocks - 1;
    if (cmdbuf[8] & 1) {
        ULONG cylsize = (ULONG)unit->au_Heads * unit->au_Sectors;
        if (lba >= blocks)
            return 0xff;
        if (cylsize) {
            UQUAD end = (UQUAD)lba + cylsize - (lba % cylsize) - 1;
            if (end < last)
                last = end;
        }
    }
    wl(out, last > 0xffffffffULL ? 0xffffffffUL : (ULONG)last);
    wl(out + 4, 1UL << unit->au_SectorShift);
    *outlen = cmd->scsi_Length < sizeof(out) ? cmd->scsi_Length : sizeof(out);
    if (*outlen)
        CopyMem(out, cmd->scsi_Data, *outlen);
    return 0;
}

BYTE SCSIEmu(struct ata_Unit *unit, struct SCSICmd *cmd)
{
    struct Library *UtilityBase = unit->au_Bus->ab_Base->ata_UtilityBase;
    ULONG len, offset;
    UQUAD end;
    ULONG scsi_len;
    UWORD scsi_sense_len = (cmd->scsi_Flags & (1 << SCSIB_OLDAUTOSENSE)) ? 4 :
        (cmd->scsi_Flags & (1 << SCSIB_AUTOSENSE)) ? cmd->scsi_SenseLength : 0;
    UBYTE *cmdbuf = cmd->scsi_Command;
    UBYTE sense[32];
    UWORD senselen;
    UBYTE err, status;
    UBYTE key = 0, asc = 0;
 
    cmd->scsi_Actual = 0;
    cmd->scsi_CmdActual = 0;
    cmd->scsi_SenseActual = 0;
    cmd->scsi_Status = 0;
    if (!cmdbuf || (cmd->scsi_Length && !cmd->scsi_Data) ||
        (scsi_sense_len && !cmd->scsi_SenseData))
        return IOERR_BADADDRESS;
    if (!cmd->scsi_CmdLength)
        return IOERR_BADLENGTH;
    /* Check the command group before reading any operands. */
    switch (cmdbuf[0] >> 5) {
    case 0: len = 6; break;
    case 1:
    case 2: len = 10; break;
    case 5: len = 12; break;
    default: len = 1; break;
    }
    if (cmd->scsi_CmdLength < len)
        return IOERR_BADLENGTH;

    err = 0;
    status = 0;
    scsi_len = 0;
    senselen = 0;
    if (scsi_sense_len > sizeof sense)
        scsi_sense_len = sizeof sense;
    switch(cmdbuf[0])
    {
        case 0x00: /* TEST UNIT READY */
        break;

        case 0x08: /* READ (6) */
        case 0x0a: /* WRITE (6) */
        case 0x28: /* READ (10) */
        case 0x2a: /* WRITE (10) */
        case 0xa8: /* READ (12) */
        case 0xaa: /* WRITE (12) */
        if (cmdbuf[0] < 0x20) {
            offset = ((cmdbuf[1] & 31) << 16) | (cmdbuf[2] << 8) | cmdbuf[3];
            len = cmdbuf[4];
            if (!len)
                len = 256;
        } else {
            offset = rl(cmdbuf + 2);
            if (cmdbuf[0] < 0x80)
                len = rl(cmdbuf + 7 - 2) & 0xffff;
            else
                len = rl(cmdbuf + 6);
        }
        if (!len)
            break;
        if (len > (cmd->scsi_Length >> unit->au_SectorShift))
            return IOERR_BADLENGTH;
        end = (UQUAD)offset + len;
        if (end > unit->au_Capacity48)
            return IOERR_BADADDRESS;
        /* Words 60-61 report a sector count, capped at 0x0fffffff. */
        if (end <= unit->au_Capacity && end <= 0x0fffffffUL) {
            err = (cmdbuf[0] & 2 ? unit->au_Write32 : unit->au_Read32)
                (unit, offset, len, cmd->scsi_Data, &scsi_len);
        } else {
            if (!(unit->au_UseModes & AF_XFER_48BIT))
                return IOERR_BADADDRESS;
            err = (cmdbuf[0] & 2 ? unit->au_Write64 : unit->au_Read64)
                (unit, offset, len, cmd->scsi_Data, &scsi_len);
        }
        break;

        case 0x37: /* READ DEFECT DATA */
        key = 0; /* NO SENSE */
        asc = 0x1c; /* DEFECT LIST NOT FOUND */
        break;

        case 0x12: /* INQUIRY */
        err = scsi_inquiry(unit, cmd, &scsi_len);
        break;
        case 0x1a: /* MODE SENSE(6) */
        err = scsi_modesense(unit, cmd, &scsi_len);
        break;
        case 0x25: /* READ CAPACITY */
        err = scsi_readcapacity(unit, cmd, &scsi_len);
        break;

        case 0x1d: /* SEND DIAGNOSTICS */
        break;
        case 0x35: /* SYNCHRONIZE CACHE */
        err = ata_FlushCache(unit);
        break;
        
        default:
        err = 0xff;
        break;
    }
    
    if (err == 0xff) {
        key = 5; /* ILLEGAL REQUEST */
        asc = 0x24; /* ILLEGAL FIELD IN CDB */
        err = TDERR_NotSpecified;
    }
    /* a non zero additional sense code means CHECK CONDITION with sense data */
    if (asc) {
        status = 2; /* CHECK CONDITION */
        senselen = 32;
        SetMem(sense, 0, senselen);
        sense[0] = 0x70;
        sense[2] = key;
        sense[12] = asc;
    }
    
    if (senselen && scsi_sense_len) {
        if (senselen > scsi_sense_len)
            senselen = scsi_sense_len;
        CopyMem(sense, cmd->scsi_SenseData, senselen);
        cmd->scsi_SenseActual = senselen;
    }
    cmd->scsi_Status = status;
    cmd->scsi_CmdActual = status != 0 ? 0 : cmd->scsi_CmdLength;
    cmd->scsi_Actual = scsi_len;
    
    return err;
}
