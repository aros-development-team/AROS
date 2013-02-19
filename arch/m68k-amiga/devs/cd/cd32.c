/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <aros/debug.h>

#include <aros/symbolsets.h>

#include <devices/cd.h>

#include <proto/exec.h>
#include <proto/alib.h>

#include <hardware/intbits.h>

#include "chinon.h"
#include "cd32.h"

#include "cd_intern.h"

static inline ULONG readl(ULONG addr)
{
    return *((volatile ULONG *)addr);
}

static inline VOID writel(ULONG value, ULONG addr)
{
    *((volatile ULONG *)addr) = value;
}

static inline UWORD readw(ULONG addr)
{
    return *((volatile UWORD *)addr);
}

static inline VOID writew(UWORD value, ULONG addr)
{
    *((volatile UWORD *)addr) = value;
}

static inline UBYTE readb(ULONG addr)
{
    return *((volatile UBYTE *)addr);
}

static inline VOID writeb(UBYTE value, ULONG addr)
{
    *((volatile UBYTE *)addr) = value;
}

struct CD32Mode1 {
    ULONG cs_Sector;
    UBYTE cs_Reserved[8];
    UBYTE cs_MinuteBCD;
    UBYTE cs_SecondBCD;
    UBYTE cs_FrameBCD;
    UBYTE cs_Mode;
    UBYTE cs_Data[2048];
    UBYTE cs_EDC[4];
    UBYTE cs_Reserved2[8];
    UBYTE cs_ECC[276];
};

struct CD32Unit {
    LIBBASETYPE *cu_CDBase;
    struct CD32Data {
        UBYTE Data[0xc00];
        UBYTE Error[0x400];
    } *cu_Data; /* x16 of these */
    struct CD32Misc {
        UBYTE Response[0x100];
        UBYTE Subcode[0x100];
        UBYTE Command[0x100];
        UBYTE Reserved[0x100];
    } *cu_Misc;
    struct Interrupt cu_Interrupt;
    UBYTE cu_TxHead, cu_RxHead;
    struct Task *cu_Task;
    struct CDInfo cu_CDInfo;
    struct QCode cu_QCode;
    ULONG cu_TotalSectors;
    UBYTE cu_Sequence;
    UBYTE cu_Muted;
    union CDTOC cu_CDTOC[100];
    ULONG cu_IntEnable;
    ULONG cu_ChangeNum;
};

static inline void CD32_Mute(struct CD32Unit *cu, int muted)
{
    UBYTE val;
   
    Forbid();
    val = readb(0xbfe001);
    if (!muted) {
        val |= 1;
    } else {
        val &= ~1;
    }
    writeb(val, 0xbfe001);
    Permit();

    cu->cu_Muted = muted ? 1 : 0;
}


UBYTE dec2bcd(UBYTE dec)
{
    if (dec > 99)
        return 99;
    return ((dec / 10) << 4) | (dec % 10);
}

static void sec2msf(LONG sec, UBYTE *msf)
{
    msf[0] = dec2bcd(sec / (60 * 75));
    sec = sec % (60 * 75);
    msf[1] = dec2bcd(sec / 75);
    msf[2] = dec2bcd(sec % 75);
}

static ULONG msf2sec(struct RMSF *msf)
{
    return ((msf->Minute * 60) + msf->Second) * 75 + msf->Frame;
}

static VOID CD32_IntEnable(struct CD32Unit *cu, ULONG mask)
{
    cu->cu_IntEnable |= mask;
    writel(cu->cu_IntEnable, AKIKO_CDINTENA);
}

static VOID CD32_IntDisable(struct CD32Unit *cu, ULONG mask)
{
    cu->cu_IntEnable &= ~mask;
    writel(cu->cu_IntEnable, AKIKO_CDINTENA);
}

static AROS_INTH1(CD32_Interrupt, struct CD32Unit *, cu)
{
    const UBYTE resp_len[16] = {
        1, 2, 2, 2, 2, 2, 15, 20, 0, 0, 2, 0, 0, 0, 0, 0
    };
    ULONG status;
    UBYTE rxtail;

    AROS_INTFUNC_INIT

    status = readl(AKIKO_CDINTREQ);
    if (!status)
        return FALSE;

    if (status & AKIKO_CDINT_RXDMA) {
        rxtail = readb(AKIKO_CDRXINX);
        if (((cu->cu_RxHead+1)&0xff) == rxtail) {
            /* Add the correct length for the full response */
            UBYTE len = resp_len[cu->cu_Misc->Response[cu->cu_RxHead] & 0xf];
            if (len == 0) {
                D(bug("%s: Insane response byte 0x%02x\n", cu->cu_Misc->Response[cu->cu_RxHead]));
            }
            writeb(cu->cu_RxHead+len+1, AKIKO_CDRXCMP);
            return TRUE;
        }
        CD32_IntDisable(cu, AKIKO_CDINT_RXDMA);
        D(bug("%s: Signal %p\n", __func__, cu->cu_Task));
        Signal(cu->cu_Task, SIGF_SINGLE);
    }

    if (status & AKIKO_CDINT_TXDMA) {
        CD32_IntDisable(cu, AKIKO_CDINT_TXDMA);
    }

#if 0
    if (status & AKIKO_CDINT_OVERFLOW) {
        CD32_IntDisable(cu, AKIKO_CDINT_OVERFLOW);
        Signal(cu->cu_Task, SIGF_SINGLE);
    }
#endif

    if (status & AKIKO_CDINT_PBX) {
        UWORD pbx = readw(AKIKO_CDPBX);
        writew(0, AKIKO_CDPBX);
        if (pbx < 0x000f)
            Signal(cu->cu_Task, SIGF_SINGLE);
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

static UBYTE bcd2dec(UBYTE bcd)
{
    return (bcd & 0xf) + ((bcd >> 4) & 0xf) * 10;
}

static VOID CD32_UpdateTOC(struct CD32Unit *cu)
{
    struct QCode *qc = &cu->cu_QCode;

    D(bug("%s: Index 0x%02x\n", __func__, qc->Index));
    switch (qc->Index) {
    case 0xA0:
        cu->cu_CDTOC[0].Summary.FirstTrack = bcd2dec(qc->DiskPosition.MSF.Minute);
        break;
    case 0xA1:
        cu->cu_CDTOC[0].Summary.LastTrack  = bcd2dec(qc->DiskPosition.MSF.Minute);
        break;
    case 0xA2:
        cu->cu_CDTOC[0].Summary.LeadOut = qc->DiskPosition;
        cu->cu_CDInfo.Status |= CDSTSF_TOC;
        break;
    default:
        if (((qc->Index & 0xf) <= 9) && ((qc->Index & 0xf0) <= 0x90) && 
            (qc->Index != 0)) {
            int track = bcd2dec(qc->Index);
            cu->cu_CDTOC[track].Entry.CtlAdr = qc->CtlAdr;
            cu->cu_CDTOC[track].Entry.Track  = track;
            cu->cu_CDTOC[track].Entry.Position = qc->DiskPosition;
            if (track == 1 && qc->CtlAdr == 0x41) {
                cu->cu_CDInfo.Status |= CDSTSF_CDROM;
            }
        } else {
            D(bug("%s: Illegal track number 0x%02x ignored\n", __func__, qc->Index));
        }
        break;
    }
}

static LONG CD32_Cmd(struct CD32Unit *cu, UBYTE *cmd, LONG cmd_len, UBYTE *resp, LONG resp_len)
{
    UBYTE csum, RxTail;
    int i;

    cu->cu_Sequence++;
    if (cu->cu_Sequence >= 16)
        cu->cu_Sequence = 1;

    cmd[0] = (cmd[0] & 0xf) | (cu->cu_Sequence << 4);

    csum = 0;
    for (i = 0; i < cmd_len; i++) {
        /* NOTE: We are relying on the fact that cu_TxHead is a
         *       UBYTE, so that it wraps at 0x100
         */
        cu->cu_Misc->Command[cu->cu_TxHead++] = cmd[i];
        csum += cmd[i];
    }

    cu->cu_Misc->Command[cu->cu_TxHead++] = ~csum;

    /* Just wait for the RX to complete of the status */
    CD32_IntEnable(cu, AKIKO_CDINT_RXDMA | AKIKO_CDINT_TXDMA);
    writel(AKIKO_CDFLAG_TXD | AKIKO_CDFLAG_RXD | AKIKO_CDFLAG_CAS | AKIKO_CDFLAG_PBX | AKIKO_CDFLAG_MSB, AKIKO_CDFLAG);

    /* Trigger the command by updating AKIKO_CDTXCMP */
    SetSignal(0, SIGF_SINGLE);
    writeb((cu->cu_RxHead + 1) & 0xff, AKIKO_CDRXCMP);
    writeb(cu->cu_TxHead, AKIKO_CDTXCMP);

    for (;;) {
        UBYTE RxHead = cu->cu_RxHead;

        D(bug("%s: %02x Wait...\n", __func__, cmd[0]));
        Wait(SIGF_SINGLE);
        D(bug("%s: %02x Waited.\n", __func__, cmd[0]));

        RxTail = readb(AKIKO_CDRXINX);
        D(bug("%s: RxHead=%02x RxTail=%02x\n", __func__, RxHead, RxTail));

        if (cu->cu_Misc->Response[RxHead] == cmd[0])
            break;

        D(bug("%s: Found unexpected: 0x%02x (%d)\n", __func__, cu->cu_Misc->Response[RxHead], (RxTail + 256 - RxHead) & 0xff));

        csum = 0;
        while (RxHead != RxTail) {
            /* NOTE: We are relying on the fact that cu_RxHead is a
             *       UBYTE, so that it wraps at 0x100
             */
            UBYTE val = cu->cu_Misc->Response[RxHead++];
            D(bug("%02x ", val));
            csum += val;
        }
        D(bug(" (%02x)\n", csum));

        if (csum != 0xff) {
            D(bug("%s: Checksum failed on RX\n", __func__));
            return CDERR_NotSpecified;
        }

        RxHead = cu->cu_RxHead;
        cu->cu_RxHead = RxTail;

        switch (cu->cu_Misc->Response[RxHead]) {
        case CHCD_MEDIA:
            RxHead++;
            if (cu->cu_Misc->Response[RxHead] == 0x83) {
                if (!(cu->cu_CDInfo.Status & CDSTSF_DISK)) {
                    cu->cu_ChangeNum++;
                     cu->cu_CDInfo.Status |= CDSTSF_CLOSED | CDSTSF_DISK | CDSTSF_SPIN;
                }
            } else {
                if (cu->cu_CDInfo.Status & CDSTSF_DISK) {
                    cu->cu_ChangeNum++;
                    cu->cu_CDInfo.Status = 0;
                }
            }
            break;
        case CHCD_SUBQ:
            RxHead++;
            RxHead++;
            RxHead++;
            cu->cu_QCode.CtlAdr  = cu->cu_Misc->Response[RxHead++];
            cu->cu_QCode.Track   = cu->cu_Misc->Response[RxHead++];
            cu->cu_QCode.Index   = cu->cu_Misc->Response[RxHead++];
            cu->cu_QCode.Zero    = 0;
            cu->cu_QCode.TrackPosition.MSF.Reserved = 0;
            cu->cu_QCode.TrackPosition.MSF.Minute   = cu->cu_Misc->Response[RxHead++];
            cu->cu_QCode.TrackPosition.MSF.Second   = cu->cu_Misc->Response[RxHead++];
            cu->cu_QCode.TrackPosition.MSF.Frame    = cu->cu_Misc->Response[RxHead++];
            cu->cu_QCode.DiskPosition.MSF.Reserved = cu->cu_Misc->Response[RxHead++];
            cu->cu_QCode.DiskPosition.MSF.Minute   = cu->cu_Misc->Response[RxHead++];
            cu->cu_QCode.DiskPosition.MSF.Second   = cu->cu_Misc->Response[RxHead++];
            cu->cu_QCode.DiskPosition.MSF.Frame    = cu->cu_Misc->Response[RxHead++];
            if (!(cu->cu_CDInfo.Status & CDSTSF_TOC))
                CD32_UpdateTOC(cu);
                
            break;

        default:
            D(bug("%s: Command mismatch: got 0x%02x, expected 0x%02x\n", __func__, cu->cu_Misc->Response[RxHead], cmd[0]));
            return CDERR_InvalidState;
        }

        writeb((RxTail + 1) & 0xff, AKIKO_CDRXCMP);
    }

    D(bug("%s: Found expected: 0x%02x (%d)\n", __func__, cu->cu_Misc->Response[cu->cu_RxHead], (RxTail + 256 - cu->cu_RxHead) & 0xff));

    csum = 0;
    while (cu->cu_RxHead != RxTail) {
        /* NOTE: We are relying on the fact that cu_RxHead is a
         *       UBYTE, so that it wraps at 0x100
         */
        UBYTE val = cu->cu_Misc->Response[cu->cu_RxHead++];
        D(bug("%02x ", val));
        if (resp_len > 0) {
            *(resp++) = val;
            resp_len--;
        }
        csum += val;
    }
    D(bug("(%02x)\n", csum));

    if (csum != 0xff) {
        D(bug("%s: Checksum failed on RX\n", __func__));
        return CDERR_NotSpecified;
    }

    D(bug("%s:  -- RXHead %2x --\n", __func__, cu->cu_RxHead));

    return 0;
}

static VOID CD32_Led(struct CD32Unit *cu, BOOL led_on)
{
    UBYTE cmd[2] = { CHCD_LED, (led_on ? 1 : 0) | 0x80 };
    UBYTE resp[2];

    CD32_Cmd(cu, cmd, 2, resp, 2);
}

static LONG CD32_CmdRead(struct CD32Unit *cu, LONG sect_start, LONG sectors, void (*copy_sector)(APTR sector, APTR priv), APTR priv)
{
    LONG err;
    UBYTE cmd[12], resp[2];
    UBYTE cmd_pause[1] = { CHCD_PAUSE };
    UBYTE cmd_unpause[1] = { CHCD_UNPAUSE };
    int i;
    UWORD pbx;

    while (sectors > 0) {
        cu->cu_CDInfo.Status &= ~(CDSTSF_PLAYING | CDSTSF_PAUSED | CDSTSF_SEARCH | CDSTSF_DIRECTION);

        cmd[0] = CHCD_MULTI;
        sec2msf(sect_start, &cmd[1]);
        sec2msf(sect_start + 16, &cmd[4]);
        cmd[7] = 0x80;  /* Data read */
        cmd[8] = (cu->cu_CDInfo.PlaySpeed >= 150) ? 0x40 : 0x00;
        cmd[8] = 0x00;
        cmd[9] = 0x00;
        cmd[10] = 0x04;  /* 0x04 to enable the motor */
        cmd[11] = 0x00;

        writew(0xffff, AKIKO_CDPBX);

        /* Start the read */
        CD32_Led(cu, TRUE);
        CD32_Cmd(cu, cmd_unpause, 1, resp, sizeof(resp));
        err = CD32_Cmd(cu, cmd, sizeof(cmd), resp, sizeof(resp));
        if (err)
            return err;

        if ((resp[1] & 2) != 2)
            return CDERR_SeekError;


        /* Wait for (most) sectors to come in */
        CD32_IntEnable(cu, AKIKO_CDINT_PBX);

        writel(readl(AKIKO_CDFLAG) | AKIKO_CDFLAG_ENABLE, AKIKO_CDFLAG);
        Wait(SIGF_SINGLE);

        CD32_Cmd(cu, cmd_pause, 1, resp, sizeof(resp));
        CD32_Led(cu, FALSE);
        pbx = readw(AKIKO_CDPBX);

        if (pbx == 0) {
            D(bug("%s: Overflow during read\n", __func__));
            break;
        }

        for (i = 15; i >= 0; i--) {
            if (!(pbx & (1 << i))) {
                D(bug("%s: SECTOR: %d\n", __func__, *(ULONG *)(&cu->cu_Data[i])));
                copy_sector(&cu->cu_Data[i], priv);
                sectors--;
                sect_start++;
                if (sectors <= 0)
                    return 0;
            }
        }

    }

    return -1;
}

static UBYTE CD32_Status(struct CD32Unit *cu)
{
    UBYTE cmd[1], res[21];

    cu->cu_Task = FindTask(NULL);
    D(bug("%s: %p\n", __func__, cu));
    cmd[0] = CHCD_STATUS;
    res[1] = 0x80;
    CD32_Cmd(cu, cmd, 1, res, 20);
    res[20] = 0;
    D(bug("%s: Drive \"%s\", state 0x%02x\n", __func__, &res[2], res[1]));

    if (res[1] & CHERR_BADCOMMAND) {
        if (cu->cu_CDInfo.Status & CDSTSF_DISK) {
            cu->cu_ChangeNum++;
            cu->cu_CDInfo.Status = 0;
        }
    } else if (res[1] & CHERR_DISKPRESENT) {
        if (!(cu->cu_CDInfo.Status & CDSTSF_DISK)) {
            cu->cu_ChangeNum++;
            cu->cu_CDInfo.Status |= CDSTSF_CLOSED | CDSTSF_DISK | CDSTSF_SPIN;
        }
    }

    D(bug("%s: %p = 0x%08x\n", __func__, cu, cu->cu_CDInfo.Status));
    return cu->cu_CDInfo.Status;
}

static VOID CD32_ReadTOC(struct CD32Unit *cu)
{
    UBYTE toc_cmd[12] = { CHCD_MULTI, 0, 0, 0, 0, 0, 0, 0x03, 0x40, 0, 0x00, 0 };
    UBYTE idle_cmd[1] = { CHCD_UNPAUSE };
    UBYTE resp[2];
    ULONG timeout = 5 * 1000;   /* 5 second timeout */
    BOOL  blink = TRUE;

    /* Start the read TOC command */
    if (CD32_Cmd(cu, toc_cmd, 12, resp, 2) != 0)
        return;

    /* Until the TOC is read, blink the LED and unpause */
    D(bug("%s: Waiting for TOC for up to 10 seconds\n", __func__));
    while (!(cu->cu_CDInfo.Status & CDSTSF_TOC) && timeout > 0) {
        CD32_Led(cu, blink);
        blink = !blink;
        cdDelayMS(cu->cu_CDBase, 100);  /* Delay 100ms */
        CD32_Cmd(cu, idle_cmd, 1, resp, 2);
        timeout -= 100;
    }

    if (cu->cu_CDInfo.Status & CDSTSF_CDROM) {
        if (cu->cu_CDTOC[0].Summary.LastTrack == 1) {
            cu->cu_TotalSectors = msf2sec(&cu->cu_CDTOC[0].Summary.LeadOut.MSF) -
                                  msf2sec(&cu->cu_CDTOC[1].Entry.Position.MSF);
        } else {
            cu->cu_TotalSectors = msf2sec(&cu->cu_CDTOC[2].Entry.Position.MSF) -
                                  msf2sec(&cu->cu_CDTOC[1].Entry.Position.MSF);
        }
    } else {
        cu->cu_TotalSectors = 0;
    }

    D(bug("%s: TotalSectors = %d\n", __func__, cu->cu_TotalSectors));
}

static LONG CD32_IsCDROM(struct CD32Unit *cu)
{
    if ((cu->cu_CDInfo.Status & CDSTSF_DISK) == 0) {
        if ((CD32_Status(cu) & CDSTSF_DISK) == 0)
            return CDERR_NoDisk;
    }

    if ((cu->cu_CDInfo.Status & CDSTSF_TOC) == 0) {
        CD32_ReadTOC(cu);
    }

    if ((cu->cu_CDInfo.Status & CDSTSF_CDROM) == 0) {
        return CDERR_SeekError;
    }

    return 0;
}

struct CopyREAD {
    struct IOStdReq *io;
    LONG offset;
    UBYTE *buffer;
    LONG length;
};

static VOID CD32_CopyREAD(APTR frame, APTR priv)
{
    struct CD32Mode1 *mode1 = frame;
    struct CopyREAD *cr = priv;
    LONG tocopy;

    tocopy = 2048 - cr->offset;
    if (tocopy > cr->length)
        tocopy = cr->length;

    D(bug("%s: Copy from 0x%08x to 0x%08x, %d\n", __func__, &mode1->cs_Data[cr->offset], cr->buffer, tocopy));
    CopyMem(&mode1->cs_Data[cr->offset], cr->buffer, tocopy);
    if (0) { int i ; 
    for (i = 0; i < tocopy; i++) {
        int mod = i % 16;
        if (mod == 0) D(bug("%08x:", cr->buffer - (UBYTE *)cr->io->io_Data + i));
        D(bug("%c%02x", (mod==8) ? '-' : ' ', cr->buffer[i]));
        if (mod == 15) D(bug("\n"));
    } 
    D(bug("\n"));
    }

    cr->offset = 0;
    cr->length -= tocopy;
    cr->buffer += tocopy;
    cr->io->io_Actual += tocopy;
}


static LONG CD32_DoIO(struct IOStdReq *io, APTR priv)
{
    struct CD32Unit *cu = priv;
    LONG  err = CDERR_NOCMD;
    UBYTE cmd[16], res[16];
    LONG sect_start, sect_end, origin;
    struct CopyREAD cr;

    D(bug("%s:%p io_Command=%d\n", __func__, io, io->io_Command));

    cu->cu_Task = FindTask(NULL);

    switch (io->io_Command) {
    case CD_CHANGENUM:
        io->io_Actual = cu->cu_ChangeNum;
        err = 0;
        break;
    case CD_CHANGESTATE:
        io->io_Actual = (cu->cu_CDInfo.Status & CDSTSF_TOC) ? 0 : 1;
        D(bug("CD_CHANGESTATE: %d\n", io->io_Actual));
        err = 0;
        break;
    case CD_CONFIG:
        D(bug("CD_CONFIG: Data %p, Length %d\n", io->io_Data, io->io_Length));
        if (io->io_Length != 0) {
            err = CDERR_BADLENGTH;
            break;
        }
        /* Gah. Some bright spark decided to use tags with
         * the same values as TAG_IGNORE, TAG_MORE, and TAG_SKIP,
         * so we have to process the TagItem array manually.
         */
        err = 0;
        if (io->io_Data != NULL) {
            struct TagItem *ti = io->io_Data;
            BOOL done = FALSE;
            while (!done) {
                switch (ti->ti_Tag) {
                case TAGCD_PLAYSPEED:
                    if (((ti->ti_Data % 75) != 0) ||
                        (ti->ti_Data > cu->cu_CDInfo.MaxSpeed)) {
                        err = CDERR_InvalidState;
                        done = TRUE;
                        break;
                    }
                    cu->cu_CDInfo.PlaySpeed = ti->ti_Data;
                    break;
                case TAGCD_READSPEED:
                    if (((ti->ti_Data % 75) != 0) ||
                        (ti->ti_Data > cu->cu_CDInfo.MaxSpeed)) {
                        err = CDERR_InvalidState;
                        done = TRUE;
                        break;
                    }
                    cu->cu_CDInfo.ReadSpeed = ti->ti_Data;
                    break;

                case TAGCD_READXLSPEED:
                    if (((ti->ti_Data % 75) != 0) ||
                        (ti->ti_Data > cu->cu_CDInfo.MaxSpeed)) {
                        err = CDERR_InvalidState;
                        done = TRUE;
                        break;
                    }
                    cu->cu_CDInfo.ReadXLSpeed = ti->ti_Data;
                    break;

                case TAGCD_SECTORSIZE:
                    if (ti->ti_Data != 2048 &&
                        ti->ti_Data != 2328) {
                        err = CDERR_InvalidState;
                        done = TRUE;
                        break;
                    }
                    cu->cu_CDInfo.SectorSize = ti->ti_Data;
                    break;

                case TAGCD_XLECC:
                    cu->cu_CDInfo.XLECC = ti->ti_Data ? 1 : 0;
                    break;
                case TAGCD_EJECTRESET:
                    cu->cu_CDInfo.EjectReset = ti->ti_Data ? 1 : 0;
                    break;
                case TAGCD_DONE:
                    done = TRUE;
                    break;
                default:
                    break;
                }
                ti++;
                D(bug("CD_CONFIG: Tag %d, Item %d, err %d, done %d\n", ti->ti_Tag, ti->ti_Data, err, done));
            }
        }
        break;
    case CD_TOCMSF:
        D(bug("CD_TOCMSF: %d\n", io->io_Length));
        err = CD32_IsCDROM(cu);
        D(bug("CD_TOCMSF: check %d\n", err));
        if (io->io_Data != NULL && (cu->cu_CDInfo.Status & CDSTSF_TOC)) {
            ULONG i, actual = 0;
            ULONG track = io->io_Offset;
            APTR buff = io->io_Data;
            for (i = 0; i < io->io_Length && track <= cu->cu_CDTOC[0].Summary.LastTrack; i++, track++) {
                CopyMem(&cu->cu_CDTOC[track], buff, sizeof(union CDTOC));
                actual++;
                buff+=sizeof(union CDTOC);
            }
            io->io_Actual = actual;
            err = 0;
        }
        D(bug("CD_TOCMSF: err %d\n", err));
        break;
    case CD_EJECT:
        io->io_Actual = 0;
        err = 0;
        break;
    case CD_GETGEOMETRY:
        err = CD32_IsCDROM(cu);
        if (err) {
            D(bug("CD_GETGEOMETRY: Not a data disk\n"));
            break;
        }

        if (io->io_Length >= sizeof(struct DriveGeometry)) {
            struct DriveGeometry *dg = (struct DriveGeometry *)io->io_Data;
            dg->dg_SectorSize = cu->cu_CDInfo.SectorSize;
            dg->dg_TotalSectors = cu->cu_TotalSectors;
            dg->dg_Cylinders = cu->cu_TotalSectors;
            dg->dg_CylSectors = 1;
            dg->dg_Heads = 1;
            dg->dg_TrackSectors = 1;
            dg->dg_BufMemType = MEMF_PUBLIC;
            dg->dg_DeviceType = DG_CDROM;
            dg->dg_Flags = DGF_REMOVABLE;
            dg->dg_Reserved = 0;
            io->io_Actual = sizeof(*dg);
            err = 0;
        } else {
            err = CDERR_BADLENGTH;
        }
        break;
    case CD_INFO:
        if (io->io_Length >= sizeof(struct CDInfo)) {
            CD32_IsCDROM(cu);
            CopyMem(&cu->cu_CDInfo, io->io_Data, sizeof(struct CDInfo));
            io->io_Actual = sizeof(struct CDInfo);
            err = 0;
        }
        break;
    case CD_MOTOR:
        /* FIXME: Should this be a pause/unpause command?
         *        Or just ignored?
         */
        io->io_Actual = 1;
        err = 0;
        break;
    case CD_PLAYTRACK:
        if (io->io_Offset <= cu->cu_CDTOC[0].Summary.LastTrack) {
            ULONG last = io->io_Offset + io->io_Length;
            UBYTE cmd[12], res[2];
            cmd[0] = CHCD_MULTI;
            cmd[1] = cu->cu_CDTOC[io->io_Offset].Entry.Position.MSF.Minute;
            cmd[2] = cu->cu_CDTOC[io->io_Offset].Entry.Position.MSF.Second;
            cmd[3] = cu->cu_CDTOC[io->io_Offset].Entry.Position.MSF.Frame;
            if (last > cu->cu_CDTOC[0].Summary.LastTrack) {
                cmd[4] = cu->cu_CDTOC[0].Summary.LeadOut.MSF.Minute;
                cmd[5] = cu->cu_CDTOC[0].Summary.LeadOut.MSF.Minute;
                cmd[6] = cu->cu_CDTOC[0].Summary.LeadOut.MSF.Minute;
            } else {
                cmd[4] = cu->cu_CDTOC[last].Entry.Position.MSF.Minute;
                cmd[5] = cu->cu_CDTOC[last].Entry.Position.MSF.Second;
                cmd[6] = cu->cu_CDTOC[last].Entry.Position.MSF.Frame;
            }
            cmd[7] = 0x00;
            cmd[8] = (cu->cu_CDInfo.ReadSpeed >= 150) ? 0x40 : 0x00;
            cmd[9] = 0x00;
            cmd[10] = 0x04;
            cmd[11] = 0x00;
            D(bug("CD_PLAYTRACK:"));int i; for(i = 0; i < 12;i++) bug(" %02x", cmd[i]);
            err = CD32_Cmd(cu, cmd, 12, res, 2);
            D(bug("CD_PLAYTRACK: err=%d, res[1]=0x%02x\n", err, res[1]));
            if (!err && (res[1] & 0x80) == 0) {
                cu->cu_CDInfo.Status &= ~(CDSTSF_PLAYING | CDSTSF_PAUSED | CDSTSF_SEARCH | CDSTSF_DIRECTION);
                cu->cu_CDInfo.Status |= CDSTSF_PLAYING;
                D(bug("CD_PLAYTRACK: Playing tracks %d-%d\n", io->io_Offset, last-1));
                err = 0;
            }
        }
        break;
    case CD_READ:
        err = CD32_IsCDROM(cu);
        if (err)
            break;

        /* Convert offset and length to MSF */
        CD32_Led(cu, TRUE);

        io->io_Error = 0;
        io->io_Actual = 0;

        origin = msf2sec(&cu->cu_CDTOC[1].Entry.Position.MSF);
        sect_start = io->io_Offset / cu->cu_CDInfo.SectorSize;
        sect_end = (io->io_Offset + io->io_Length + cu->cu_CDInfo.SectorSize - 1) / cu->cu_CDInfo.SectorSize;

        cr.io = io;
        cr.buffer = io->io_Data;
        cr.offset = io->io_Offset % cu->cu_CDInfo.SectorSize;
        cr.length = io->io_Length;
       
        err = CD32_CmdRead(cu, sect_start + origin, sect_end - sect_start, CD32_CopyREAD, &cr);
        break;
    case CD_RESET:
        cmd[0] = CHCD_RESET;
        err = CD32_Cmd(cu, cmd, 1, res, 1);
    case CD_ATTENUATE:
        io->io_Actual = cu->cu_Muted ? 0 : 0x7fff;
        if (io->io_Offset > 0 && io->io_Offset < 0x7fff) {
            CD32_Mute(cu, io->io_Offset == 0);
        }
        err = 0;
        break;
    default:
        break;
    }

    D(bug("%s:%p err=%d (Actual %d)\n", __func__, io, err, io->io_Actual));
    return err;
}

static VOID CD32_Expunge(APTR priv)
{
    struct CD32Unit *cu = priv;
    RemIntServer(INTB_PORTS, &cu->cu_Interrupt);
    FreeMem(cu->cu_Misc, sizeof(struct CD32Misc));
    FreeMem(cu->cu_Data, sizeof(struct CD32Data) * 16);
    FreeVec(cu);
}

static const struct cdUnitOps CD32Ops = {
    .uo_Name = "CD32 (Akiko)",
    .uo_Expunge = CD32_Expunge,
    .uo_DoIO = CD32_DoIO,
};

static const struct DosEnvec CD32Envec = {
    .de_TableSize = DE_MASK,
    .de_SizeBlock = 2048 >> 2,
    .de_Surfaces  = 1,
    .de_SectorPerBlock = 1,
    .de_BlocksPerTrack = 1,
    .de_Reserved  = 0,
    .de_PreAlloc  = 0,
    .de_Interleave = 0,
    .de_LowCyl = 0,
    .de_HighCyl = 0,
    .de_NumBuffers = 32,
    .de_BufMemType = MEMF_24BITDMA,
    .de_MaxTransfer = 32 * 2048,
    .de_Mask = 0x00fffffe,
};


static int CD32_InitLib(LIBBASETYPE *cb)
{
    LONG unit;
    struct CD32Unit *priv;

    if (readw(AKIKO_ID) != AKIKO_ID_MAGIC) {
        D(bug("%s: No Akiko detected\n", __func__));
        return 1;
    }

    /* Reset the Akiko CD32 interface */
    writeb(readb(AKIKO_CDRESET) | 0x80, AKIKO_CDRESET);
    writeb(readb(AKIKO_CDRESET) & ~0x80, AKIKO_CDRESET);

    /* Disable the CD32 interface for the moment */
    priv = AllocVec(sizeof(*priv), MEMF_ANY | MEMF_CLEAR);
    if (priv) {
        priv->cu_CDBase = cb;
        priv->cu_Misc = LibAllocAligned(sizeof(struct CD32Misc), MEMF_24BITDMA | MEMF_CLEAR, 1024);
        if (priv->cu_Misc) {
            priv->cu_Data = LibAllocAligned(sizeof(struct CD32Data) * 16, MEMF_24BITDMA | MEMF_CLEAR, 4096);
            if (priv->cu_Data) {
                priv->cu_Interrupt.is_Node.ln_Pri = 0;
                priv->cu_Interrupt.is_Node.ln_Name = (STRPTR)CD32Ops.uo_Name;
                priv->cu_Interrupt.is_Node.ln_Type = NT_INTERRUPT;
                priv->cu_Interrupt.is_Data = priv;
                priv->cu_Interrupt.is_Code = (VOID_FUNC)CD32_Interrupt;
                AddIntServer(INTB_PORTS, &priv->cu_Interrupt);

                writel((IPTR)priv->cu_Data, AKIKO_CDADRDATA);
                writel((IPTR)priv->cu_Misc,  AKIKO_CDADRMISC);
                priv->cu_TxHead = readb(AKIKO_CDTXINX);
                priv->cu_RxHead = readb(AKIKO_CDRXINX);
                writeb(priv->cu_TxHead, AKIKO_CDTXCMP);
                writeb(priv->cu_RxHead, AKIKO_CDRXCMP);

                priv->cu_CDInfo.SectorSize = 2048;
                priv->cu_CDInfo.MaxSpeed = 150;
                priv->cu_CDInfo.PlaySpeed = 75;
                priv->cu_CDInfo.ReadSpeed = 150;
                priv->cu_CDInfo.ReadXLSpeed = 150;
                priv->cu_CDInfo.AudioPrecision = 1;
                CD32_IsCDROM(priv);

                unit = cdAddUnit(cb, &CD32Ops, priv, &CD32Envec);
                if (unit >= 0) {
                    D(bug("%s: Akiko as CD Unit %d\n", __func__, unit));
                    return 1;
                }

                /* Unmute the CDROM */
                CD32_Mute(priv, 0);

                RemIntServer(INTB_PORTS, &priv->cu_Interrupt);

                FreeMem(priv->cu_Data, sizeof(struct CD32Data) * 16);
            }
            FreeMem(priv->cu_Misc, sizeof(struct CD32Misc));
        }
        FreeVec(priv);
    }

    return 0;
}

static int CD32_ExpungeLib(LIBBASETYPE *cb)
{
    /* Nothing to do. */
    return 0;
}


ADD2INITLIB(CD32_InitLib, 32);
ADD2EXPUNGELIB(CD32_ExpungeLib, 32);
