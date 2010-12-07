/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Intel HW floppy stuff
    Lang: English
*/

#include <exec/types.h>
#include <devices/trackdisk.h>
#include <devices/timer.h>
#include <hardware/cia.h>
#include <hardware/custom.h>
#include <proto/cia.h>
#include <proto/disk.h>
#include <proto/exec.h>

#include <resources/disk.h>

#include "trackdisk_device.h"
#include "trackdisk_hw.h"

#define DEBUG 0
#include <aros/debug.h>

#define ioStd(x)  ((struct IOStdReq *)x)

static void td_wait_start(struct TrackDiskBase *tdb, UWORD millis)
{
    // do not remove, AbortIO()+WaitIO() does not clear signal bit
    // if AbortIO() finishes before WaitIO() waits.
    SetSignal(0, 1L << tdb->td_TimerMP2->mp_SigBit);
    tdb->td_TimerIO2->tr_node.io_Command = TR_ADDREQUEST;
    tdb->td_TimerIO2->tr_time.tv_secs = millis / 1000;
    tdb->td_TimerIO2->tr_time.tv_micro = (millis % 1000) * 1000;
    SendIO((struct IORequest *)tdb->td_TimerIO2);
}
static void td_wait_end(struct TrackDiskBase *tdb)
{
    WaitIO((struct IORequest*)tdb->td_TimerIO2);
}
static void td_wait(struct TrackDiskBase *tdb, UWORD millis)
{
    td_wait_start(tdb, millis);
    td_wait_end(tdb);
}

static UBYTE drvmask[] = { ~0x08, ~0x10, ~0x20, ~0x40 };
void td_select(struct TDU *tdu, struct TrackDiskBase *tdb)
{
    UBYTE tmp;

    if (tdu->tdu_selected)
    	return;
    tdu->tdu_selected = TRUE;
    tmp = tdb->ciab->ciaprb;
    tmp |= 0x08 | 0x10 | 0x20 | 0x40;
    tdb->ciab->ciaprb = tmp;
    if (tdu->tdu_MotorOn)
       tmp &= ~0x80;
    else
       tmp |= 0x80;
    tdb->ciab->ciaprb = tmp;
    tmp &= drvmask[tdu->tdu_UnitNum];
    tdb->ciab->ciaprb = tmp;
}

void td_deselect(struct TDU *tdu, struct TrackDiskBase *tdb)
{
    UBYTE tmp;
    if (!tdu->tdu_selected)
    	return;
    tdu->tdu_selected = FALSE;
    tmp = tdb->ciab->ciaprb;
    tmp |= 0x08 | 0x10 | 0x20 | 0x40;
    tdb->ciab->ciaprb = tmp;
    tmp |= 0x80;
    tdb->ciab->ciaprb = tmp;
}

static void td_setside(UBYTE side, struct TDU *tdu, struct TrackDiskBase *tdb)
{
    if (!side) {
        tdb->ciab->ciaprb |= 0x4;
        tdu->pub.tdu_CurrTrk |= 1;
    } else {
        tdb->ciab->ciaprb &= ~0x04;
        tdu->pub.tdu_CurrTrk &= ~1;
    }
}

static void td_setdirection(UBYTE dir, struct TDU *tdu, struct TrackDiskBase *tdb)
{
    if (dir)
        tdb->ciab->ciaprb |= 0x02;
    else
        tdb->ciab->ciaprb &= ~0x02;
}

static void td_step(struct TDU *tdu, struct TrackDiskBase *tdb, UBYTE delay)
{
    tdb->ciab->ciaprb &= ~0x01;
    tdb->ciab->ciaprb |= 0x01;
    td_wait(tdb, delay);
}

/* start motor */
void td_motoron(struct TDU *tdu, struct TrackDiskBase *tdb, BOOL wait)
{
    if (tdu->tdu_MotorOn)
	return;
    tdu->tdu_MotorOn = 1;

    td_deselect(tdu, tdb);
    td_select(tdu, tdb);
    if (wait)
	td_wait(tdb, 500);
}

/* stop motor */
void td_motoroff(struct TDU *tdu, struct TrackDiskBase *tdb)
{
    if (!tdu->tdu_MotorOn)
	return;
    tdu->tdu_MotorOn = 0;

    td_deselect(tdu, tdb);
    td_select(tdu, tdb);
}

static BOOL td_istrackzero(struct TDU *tdu, struct TrackDiskBase *tdb)
{
    return (tdb->ciaa->ciapra & 0x10) == 0;
}

UBYTE td_getprotstatus(struct TDU *tdu, struct TrackDiskBase *tdb)
{
    UBYTE v;
    v = (tdb->ciaa->ciapra & 0x08) ? 0 : 1;
    return v;
}

int td_recalibrate(struct TDU *tdu, struct TrackDiskBase *tdb)
{
    int steps = 80 + 15;
    td_select(tdu, tdb);
    td_setside(0, tdu, tdb);
    if (td_istrackzero(tdu, tdb)) {
        // step to cyl 1 if current cyl == 0
        td_setdirection(0, tdu, tdb);
        td_wait(tdb, tdu->pub.tdu_SettleDelay);
        td_step(tdu, tdb, tdu->pub.tdu_CalibrateDelay);
    }    
    td_wait(tdb, tdu->pub.tdu_SettleDelay);
    td_setdirection(1, tdu, tdb);
    td_wait(tdb, tdu->pub.tdu_SettleDelay);
    while (!td_istrackzero(tdu, tdb)) {
        if (steps < 0) // drive is broken?
            return 0;
        td_step(tdu, tdb, tdu->pub.tdu_CalibrateDelay);
        steps--;
    }
    td_wait(tdb, tdu->pub.tdu_SettleDelay);
    tdu->pub.tdu_CurrTrk = 0;
    return 1;
}

static int td_seek2(struct TDU *tdu, UBYTE cyl, UBYTE side, struct TrackDiskBase *tdb, int nowait)
{
    int dir;
    D(bug("seek=%d/%d\n", cyl, side));
    td_setside(side, tdu, tdb);
    if (tdu->pub.tdu_CurrTrk / 2 == cyl)
        return 1;
    if (tdu->pub.tdu_CurrTrk / 2 > cyl || cyl == 0xff)
        dir = 1;
    else
        dir = 0;
    td_setdirection(dir, tdu, tdb);
    if (dir != tdu->tdu_lastdir) {
        td_wait(tdb, 18);
        tdu->tdu_lastdir = dir;
    }    
    while (cyl != tdu->pub.tdu_CurrTrk / 2) {
        td_step(tdu, tdb, tdu->pub.tdu_StepDelay);
        if (tdu->pub.tdu_CurrTrk / 2 > cyl && tdu->pub.tdu_CurrTrk >= 2)
            tdu->pub.tdu_CurrTrk -= 2;
        else if (tdu->pub.tdu_CurrTrk / 2 < cyl)
            tdu->pub.tdu_CurrTrk += 2;
        if (cyl == 0xff)
            break;
    }
    td_wait_start(tdb, tdu->pub.tdu_SettleDelay);
    if (!nowait)
        td_wait_end(tdb);
    return 1;
}
int td_seek(struct TDU *tdu, int cyl, int side, struct TrackDiskBase *tdb)
{
    return td_seek2(tdu, cyl, side, tdb, 0);
}
int td_seek_nowait(struct TDU *tdu, int cyl, int side, struct TrackDiskBase *tdb)
{
    return td_seek2(tdu, cyl, side, tdb, 1);
}


// 0 = no disk, 1 = disk inserted
UBYTE td_getDiskChange(struct TDU *tdu, struct TrackDiskBase *tdb)
{
    UBYTE v;
    v = (tdb->ciaa->ciapra & 0x04) ? 1 : 0;
    return v;
}

int td_update(struct TDU *unit, struct TrackDiskBase *tdb)
{
    return 0;
}

static int checkbuffer(struct TDU *tdu, struct TrackDiskBase *tdb)
{
    // allocate HD sized buffer if HD disk inserted
    if ((tdu->tdu_hddisk && !tdb->td_supportHD) || !tdb->td_DMABuffer) {
        FreeMem(tdb->td_DMABuffer, DISK_BUFFERSIZE);
        FreeMem(tdb->td_DataBuffer, 11 * 512);
        tdb->td_DMABuffer = AllocMem(DISK_BUFFERSIZE * 2, MEMF_CHIP);
        tdb->td_DataBuffer = AllocMem(22 * 512, MEMF_ANY);
        if (!tdb->td_DMABuffer || !tdb->td_DataBuffer) {
            FreeMem(tdb->td_DMABuffer, DISK_BUFFERSIZE * 2);
            FreeMem(tdb->td_DataBuffer, 22 * 512);
            return 1;
        }
        tdb->td_supportHD = TRUE;
    }
    return 0;
}

static ULONG td_readwritetrack(UBYTE track, UBYTE write, struct TDU *tdu, struct TrackDiskBase *tdb)
{
    ULONG err = 0;
    ULONG sigs;
    UWORD dsklen = 0x8000 | ((DISK_BUFFERSIZE / 2) * (tdu->tdu_hddisk ? 2 : 1)) | (write ? 0x4000 : 0);

    td_motoron(tdu, tdb, TRUE);

    SetSignal(0, 1L << tdb->td_IntBit);

    tdb->custom->intreq = 0x0002; // clear disk interrupt request
    tdb->custom->intena = 0x8002; // enable disk interrupt
    tdb->custom->dmacon = 0x8010; // enable DMA

    tdb->custom->dskpt = tdb->td_DMABuffer;
    tdb->custom->dsklen = dsklen;
    tdb->custom->dsklen = dsklen; // dma started

    td_wait_start(tdb, (tdu->tdu_hddisk ? 2 : 1) * 1000);
    sigs = Wait((1L << tdb->td_TimerMP2->mp_SigBit) | (1L << tdb->td_IntBit));

    tdb->custom->dsklen = 0x4000;
    tdb->custom->intena = 0x0002;

    err = TDERR_BadSecPreamble;
    if (sigs & (1L << tdb->td_IntBit)) {
        // dma finished
        err = 0;
           AbortIO(tdb->td_TimerIO2);
    }
    WaitIO(tdb->td_TimerIO2);

    if (td_getDiskChange(tdu, tdb) == 0)
        err = TDERR_DiskChanged;

    return err;
}

static ULONG getmfmlong (UWORD *mfmbuf)
{
    return ((mfmbuf[0] << 16) | mfmbuf[1]) & 0x55555555;
}

#define QUICKRETRYRCNT 10 // re-read retries before reseeking

static int td_read2(struct IOExtTD *iotd, struct TDU *tdu, struct TrackDiskBase *tdb)
{
    WORD track, oldtrack, i;
    APTR data;
    ULONG len, offset, odd, even;
    ULONG id, trackoffs, chksum, dlong;
    UBYTE sectortable[22];
    BYTE quickretries = QUICKRETRYRCNT;
    BYTE longretries = tdu->pub.tdu_RetryCnt;
    BYTE err = 0;
    BOOL seeking = 0;
    UBYTE lasterr = 0;
 
    if (checkbuffer(tdu, tdb))
        return TDERR_NoMem;

    oldtrack = -1;
    iotd->iotd_Req.io_Actual = 0;
    offset = iotd->iotd_Req.io_Offset;
    len = iotd->iotd_Req.io_Length;
    data = iotd->iotd_Req.io_Data;

    D(bug(" Offset=%d, Len=%d, Data=%p\n", offset, len, data));

    while (len != 0) {
        UBYTE sectorcount = 0;
        UBYTE largestsectorneeded, smallestsectorneeded, totalsectorsneeded;
        UWORD *raw, *rawend;

        track = offset / (512 * tdu->tdu_sectors);

        if (tdb->td_buffer_unit != tdu->tdu_UnitNum || tdb->td_buffer_track != track) {
    	    int ret;
   	    td_select(tdu, tdb);
            if (seeking)
		td_wait_end(tdb);
	    seeking = 0;
    	    td_seek (tdu, track >> 1, track & 1, tdb);
            ret = td_readwritetrack(track, 0, tdu, tdb);
            if (ret) {
                totalsectorsneeded = 0;
	    	lasterr = ret;
	    	goto end;
	    }
	    tdb->td_buffer_unit = tdu->tdu_UnitNum;
            tdb->td_buffer_track = track;
        }

        if (track != oldtrack) {
            // new track, new beginning
            memset(sectortable, 0, sizeof (sectortable));
            oldtrack = track;
            quickretries = QUICKRETRYRCNT;
        }

        smallestsectorneeded = (offset / 512) % tdu->tdu_sectors;
        largestsectorneeded = smallestsectorneeded + len / 512;
        if (largestsectorneeded > tdu->tdu_sectors || len / 512 > tdu->tdu_sectors) {
            UBYTE nexttrack = track + 1;
            if (nexttrack < 160) {
                // start stepping to next track in advance (pointless but..)
                td_seek_nowait(tdu, nexttrack >> 1, nexttrack & 1, tdb);
                seeking = 1;
            }
            largestsectorneeded = tdu->tdu_sectors;
        }
        totalsectorsneeded = largestsectorneeded - smallestsectorneeded;

        raw = tdb->td_DMABuffer;
        rawend = tdb->td_DMABuffer + DISK_BUFFERSIZE * (tdu->tdu_hddisk ? 2 : 1);
        while (len != 0) {
            UWORD *rawnext = raw;
            UBYTE *secdata;
            if (raw != tdb->td_DMABuffer) {
                while (*raw != 0x4489) {
                    if (raw >= rawend) {
                        lasterr = TDERR_TooFewSecs;
                        goto end;
                    }
                    raw++;
                }
            }
            while (*raw == 0x4489 && raw < rawend)
                raw++;
            if (raw + 544 >= rawend) {
                lasterr = TDERR_TooFewSecs;
                break;
            }
            rawnext = raw + 544 - 3;
            odd = getmfmlong(raw);
            even = getmfmlong(raw + 2);
            raw += 4;
            id = (odd << 1) | even;

            trackoffs = (id & 0xff00) >> 8;
            if (trackoffs >= tdu->tdu_sectors || (id & 0xff000000) != 0xff000000) {
                lasterr = TDERR_BadSecHdr;
                continue; // corrupt sector number
            }
            if (trackoffs >= largestsectorneeded || trackoffs < smallestsectorneeded) {
                // skip unneeded sectors
                raw = rawnext;
                continue;
            }
            if (sectortable[trackoffs]) {
                // skip sector if it has already been succesfully decoded and copied
                raw = rawnext;
                continue;
            }
            // decode header
            chksum = odd ^ even;
            for (i = 0; i < 4; i++) {
                odd = getmfmlong (raw);
                even = getmfmlong (raw + 8);
                raw += 2;
                dlong = (odd << 1) | even;
                chksum ^= odd ^ even;
            }
            raw += 8;
            odd = getmfmlong (raw);
            even = getmfmlong (raw + 2);
            raw += 4;
            // header checksum ok?
            if (((odd << 1) | even) != chksum) {
                lasterr = TDERR_BadHdrSum;
                continue;
            }
            // correct track?
            if (((id & 0x00ff0000) >> 16) != track) {
                lasterr = TDERR_BadSecHdr;
                continue;
            }

            // decode data
            odd = getmfmlong (raw);
            even = getmfmlong (raw + 2);
            raw += 4;
            chksum = (odd << 1) | even;
            secdata = data + (trackoffs - smallestsectorneeded) * 512;
            for (i = 0; i < 128; i++) {
                odd = getmfmlong (raw);
                even = getmfmlong (raw + 256);
                raw += 2;
                dlong = (odd << 1) | even;
                *secdata++ = dlong >> 24;
                *secdata++ = dlong >> 16;
                *secdata++ = dlong >> 8;
                *secdata++ = dlong;
                chksum ^= odd ^ even;
            }
            if (chksum) {
                lasterr = TDERR_BadSecSum;
                continue; // data checksum error
            }
            // sector copied succesfully
            len -= 512;
            iotd->iotd_Req.io_Actual += 512;
            sectortable[trackoffs] = 1;
            sectorcount++;
            if (sectorcount == totalsectorsneeded) {
                // all required sectors in this track copied
                // -> next track
                if (len != 0) {
                    data += totalsectorsneeded * 512;
                    offset += totalsectorsneeded * 512;
                }
                break;
            }
            raw = rawnext;
        }
end:;
        if (sectorcount < totalsectorsneeded) {
            // some sector(s) had errors, retry
            tdb->td_buffer_track = -1;
            tdb->td_buffer_unit = -1;

            if (seeking)
                td_wait_end(tdb);
            seeking = 0;
            quickretries--;
            if (quickretries <= 0) {
                longretries--;
                if (longretries <= 0) {
                    err = lasterr;
                    if (!err)
                        err = TDERR_NotSpecified;
                    break;
                }
                if (!td_recalibrate(tdu, tdb)) {
                    err = TDERR_SeekError;
                    break;
                }
                quickretries = QUICKRETRYRCNT;
            }

        }    
    }
    if (seeking)
        td_wait_end(tdb);
    D(bug(" err = %d\n", err));
    return err;
}

int td_read(struct IOExtTD *iotd, struct TDU *tdu, struct TrackDiskBase *tdb)
{
    ULONG err;
    if (tdu->tdu_DiskIn == TDU_NODISK)
        return TDERR_DiskChanged;
    err = td_read2(iotd, tdu, tdb);
    return err;
}

static int td_write2(struct IOExtTD *iotd, struct TDU *tdu, struct TrackDiskBase *tdb)
{
    int track, oldtrack;
    APTR data;
    ULONG len, offset;
 
    if (checkbuffer(tdu, tdb))
        return TDERR_NoMem;
    oldtrack = -1;
    iotd->iotd_Req.io_Actual = 0;
    offset = iotd->iotd_Req.io_Offset;
    len = iotd->iotd_Req.io_Length;
    data = iotd->iotd_Req.io_Data;
    track = offset / (512 * tdu->tdu_sectors);

    while (len > 0) {
        int smallestsectorneeded, largestsectorneeded, totalsectorsneeded;

        smallestsectorneeded = (offset / 512) % tdu->tdu_sectors;
        largestsectorneeded = smallestsectorneeded + len / 512;
        if (largestsectorneeded > tdu->tdu_sectors)
            largestsectorneeded = tdu->tdu_sectors;
        totalsectorsneeded = largestsectorneeded - smallestsectorneeded;
        if (totalsectorsneeded != tdu->tdu_sectors) {
            // partial write, need to read complete sector first
            
            
        }
        len -= totalsectorsneeded * 512;
        offset += totalsectorsneeded * 512;
        data += totalsectorsneeded * 512;
    }
    return TDERR_WriteProt;
}

int td_write(struct IOExtTD *iotd, struct TDU *tdu, struct TrackDiskBase *tdb)
{
    ULONG err;
    if (tdu->tdu_DiskIn == TDU_NODISK)
        return TDERR_DiskChanged;
    if (!td_getprotstatus(tdu, tdb)) {
        if (0 /* FIXME: NO WRITE SUPPORT YET */) {
            err = td_write2(iotd, tdu, tdb);
        } else {
            err = TDERR_SeekError;
        }
    } else {
        err = TDERR_WriteProt;
    }
    return err;
}

static int td_format2(struct IOExtTD *iotd, struct TDU *tdu, struct TrackDiskBase *tdb)
{
    APTR data;
    ULONG len, offset;

    if (checkbuffer(tdu, tdb))
        return TDERR_NoMem;
    iotd->iotd_Req.io_Actual = 0;
    offset = iotd->iotd_Req.io_Offset;
    len = iotd->iotd_Req.io_Length;
    data = iotd->iotd_Req.io_Data;
    while (len >= tdu->tdu_sectors * 512) {
        ULONG err;
        int track = offset / (512 * tdu->tdu_sectors);
        td_seek(tdu, track >> 1, track & 1, tdb);
        err = td_readwritetrack(track, 1, tdu, tdb);
        if (err)
            return err;
        data += tdu->tdu_sectors * 512;
        offset += tdu->tdu_sectors * 512;
        iotd->iotd_Req.io_Actual += tdu->tdu_sectors * 512;
        len -= tdu->tdu_sectors * 512;
        td_wait(tdb, 1);
    }
    return TDERR_WriteProt;
}

int td_format(struct IOExtTD *iotd, struct TDU *tdu, struct TrackDiskBase *tdb)
{
    ULONG err;
    if (tdu->tdu_DiskIn == TDU_NODISK)
        return TDERR_DiskChanged;
    if (!td_getprotstatus(tdu, tdb)) {
        err = td_format2(iotd, tdu, tdb);
    } else {
        err = TDERR_WriteProt;
    }
    return err;
}
