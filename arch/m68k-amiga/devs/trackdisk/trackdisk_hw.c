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

#define QUICKRETRYRCNT 8 // re-read retries before reseeking, must be power of 2

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

static UBYTE const drvmask[] = { ~0x08, ~0x10, ~0x20, ~0x40 };
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
    if (dir != tdb->td_lastdir) {
        td_wait(tdb, 18);
        tdb->td_lastdir = dir;
    }    
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

BOOL td_recalibrate(struct TDU *tdu, struct TrackDiskBase *tdb)
{
    BYTE steps = 80 + 15;
    td_select(tdu, tdb);
    td_setside(0, tdu, tdb);
    td_wait(tdb, tdu->pub.tdu_CalibrateDelay);
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
            return FALSE;
        td_step(tdu, tdb, tdu->pub.tdu_CalibrateDelay);
        steps--;
    }
    td_wait(tdb, tdu->pub.tdu_SettleDelay);
    tdu->pub.tdu_CurrTrk = 0;
    return TRUE;
}

static UBYTE td_seek2(struct TDU *tdu, UBYTE cyl, UBYTE side, struct TrackDiskBase *tdb, BOOL nowait)
{
    UBYTE dir;
    D(bug("seek=%d/%d\n", cyl, side));
    td_setside(side, tdu, tdb);
    if (tdu->pub.tdu_CurrTrk / 2 == cyl)
        return 0;
    if (tdu->pub.tdu_CurrTrk / 2 > cyl || cyl == 0xff)
        dir = 1;
    else
        dir = 0;
    td_setdirection(dir, tdu, tdb);
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
    return 0;
}
UBYTE td_seek(struct TDU *tdu, int cyl, int side, struct TrackDiskBase *tdb)
{
    return td_seek2(tdu, cyl, side, tdb, FALSE);
}
UBYTE td_seek_nowait(struct TDU *tdu, int cyl, int side, struct TrackDiskBase *tdb)
{
    return td_seek2(tdu, cyl, side, tdb, TRUE);
}

// 0 = no disk, 1 = disk inserted
UBYTE td_getDiskChange(struct TDU *tdu, struct TrackDiskBase *tdb)
{
    UBYTE v;
    v = (tdb->ciaa->ciapra & 0x04) ? 1 : 0;
    return v;
}

static BOOL checkbuffer(struct TDU *tdu, struct TrackDiskBase *tdb)
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

static UBYTE td_readwritetrack(UBYTE track, UBYTE write, struct TDU *tdu, struct TrackDiskBase *tdb)
{
    UBYTE err = 0;
    ULONG sigs;
    UWORD dsklen = 0x8000 | ((DISK_BUFFERSIZE / 2) * (tdu->tdu_hddisk ? 2 : 1)) | (write ? 0x4000 : 0);

    td_motoron(tdu, tdb, TRUE);

    SetSignal(0, 1L << tdb->td_IntBit);

    tdb->custom->adkcon = 0x0600;
    tdb->custom->adkcon = write ? 0x9100 : 0x9500;
    tdb->custom->intreq = 0x0002; // clear disk interrupt request
    tdb->custom->intena = 0x8002; // enable disk interrupt
    tdb->custom->dmacon = 0x8010; // enable DMA

    tdb->custom->dskpt = tdb->td_DMABuffer;
    tdb->custom->dsklen = dsklen;
    tdb->custom->dsklen = dsklen; // dma started
    D(bug("td diskdma started, track=%d write=%d len=%d\n", track, write, dsklen & 0x3fff));

    td_wait_start(tdb, (tdu->tdu_hddisk ? 2 : 1) * 1000);
    sigs = Wait((1L << tdb->td_TimerMP2->mp_SigBit) | (1L << tdb->td_IntBit));

    tdb->custom->dsklen = 0x4000;
    tdb->custom->intena = 0x0002;

    err = TDERR_BadSecPreamble;
    if (sigs & (1L << tdb->td_IntBit)) {
        // dma finished
        err = 0;
        AbortIO((struct IORequest *)tdb->td_TimerIO2);
    }
    WaitIO((struct IORequest *)tdb->td_TimerIO2);
    D(bug("td diskdma finished, err=%d\n", err));

    if (td_getDiskChange(tdu, tdb) == 0)
        err = TDERR_DiskChanged;

    return err;
}

static void make_crc_table16(struct TrackDiskBase *tdb)
{
	unsigned short w;
	int n, k;
	
	tdb->crc_table16 = AllocMem(256 * sizeof(UWORD), MEMF_ANY);
	if (!tdb->crc_table16)
		return;
	for (n = 0; n < 256; n++) {
		w = n << 8;
		for (k = 0; k < 8; k++) {
			w = (w << 1) ^ ((w & 0x8000) ? 0x1021 : 0);
		}
		tdb->crc_table16[n] = w;
	}
}

static UWORD get_crc16_next(struct TrackDiskBase *tdb, void *vbuf, UWORD len, UWORD crc)
{
	UBYTE *buf = (UBYTE*)vbuf;
	while (len-- > 0)
		crc = (crc << 8) ^ tdb->crc_table16[((crc >> 8) ^ (*buf++)) & 0xff];
	return crc;
}
static UWORD get_crc16(struct TrackDiskBase *tdb, void *vbuf, UWORD len)
{
	if (!tdb->crc_table16) {
		make_crc_table16(tdb);
		if (!tdb->crc_table16)
			return 0;
	}
	return get_crc16_next(tdb, vbuf, len, 0xffff);
}

/* Following really needs optimization... */
static UBYTE mfmdecode (UWORD **mfmp)
{
	UWORD mfm;
	UBYTE out;
	UBYTE i;

	mfm = **mfmp;
	out = 0;
	(*mfmp)++;
	for (i = 0; i < 8; i++) {
		out >>= 1;
		if (mfm & 1)
			out |= 0x80;
		mfm >>= 2;
	}
	return out;
}

/* PC floppy format decoding is very expensive compared to ADOS... */
static UBYTE td_decodebuffer_pcdos(struct TDU *tdu, struct TrackDiskBase *tdb)
{
        UWORD *raw, *rawend;
        UWORD i;
        UBYTE lasterr;
        UBYTE *data = tdb->td_DataBuffer;
        BYTE sector;
	UBYTE tmp[8];
	UWORD datacrc;
	UBYTE current_head, current_cyl;

	tmp[0] = tmp[1] = tmp[2] = 0xa1;
	tmp[3] = 0xfb;
	datacrc = get_crc16(tdb, tmp, 4);
	current_head = tdb->td_buffer_track & 1;
	current_cyl = tdb->td_buffer_track / 2;
	
	lasterr = 0;
	raw = tdb->td_DMABuffer;
	rawend = tdb->td_DMABuffer + DISK_BUFFERSIZE * (tdu->tdu_hddisk ? 2 : 1);
	sector = -1;
	while (tdb->td_sectorbits != (1 << tdu->tdu_sectors) - 1) {
		UBYTE *secdata;
		UWORD crc;
		UBYTE mark;

		if (raw != tdb->td_DMABuffer) {
			while (*raw != 0x4489) {
				if (raw >= rawend) {
					if (lasterr == 0)
						lasterr = TDERR_TooFewSecs;
					goto end;
				}
				raw++;
			}
		}
		while (*raw == 0x4489 && raw < rawend)
			raw++;
		if (raw >= rawend - 8) {
			if (lasterr == 0)
				lasterr = TDERR_TooFewSecs;
			goto end;
		}
		mark = mfmdecode(&raw);
		if (mark == 0xfe) {
			UBYTE cyl, head, size;

			cyl = mfmdecode (&raw);
			head = mfmdecode (&raw);
			sector = mfmdecode (&raw);
			size = mfmdecode (&raw);
			crc = (mfmdecode (&raw) << 8) | mfmdecode (&raw);

			tmp[0] = 0xa1; tmp[1] = 0xa1; tmp[2] = 0xa1; tmp[3] = mark;
			tmp[4] = cyl; tmp[5] = head; tmp[6] = sector; tmp[7] = size;
			if (get_crc16(tdb, tmp, 8) != crc || cyl != current_cyl || head != current_head || size != 2 || sector < 1 || sector > tdu->tdu_sectors) {
				D(bug("PCDOS: corrupted sector header. cyl=%d head=%d sector=%d size=%d crc=%04x\n",
					cyl, head, sector, size, crc));
				sector = -1;
				continue;
			}
			sector--;
			continue;
		}
		if (mark != 0xfb) {
			D(bug("PCDOS: unknown address mark %02X\n", mark));
			continue;
		}
		if (sector < 0)
			continue;
		if (raw >= rawend - 512) {
			if (lasterr == 0)
				lasterr = TDERR_TooFewSecs;
			goto end;
		}
		secdata = data + sector * 512;
		for (i = 0; i < 512; i++)
			secdata[i] = mfmdecode (&raw);
		crc = (mfmdecode (&raw) << 8) | mfmdecode (&raw);
		if (get_crc16_next(tdb, secdata, 512, datacrc) != crc) {
			D(bug("PCDOS: sector %d data checksum error\n", sector + 1));
			continue;
		}
		D(bug("PCDOS: read sector %d\n", sector));
		tdb->td_sectorbits |= 1 << sector;
		sector = -1;
	}
end:
	D(bug("PCDOS: err=%d secmask=%08x\n", lasterr, tdb->td_sectorbits));
	return lasterr;
}


static ULONG getmfmlong (UWORD *mfmbuf)
{
    return ((mfmbuf[0] << 16) | mfmbuf[1]) & 0x55555555;
}

static UBYTE td_decodebuffer_ados(struct TDU *tdu, struct TrackDiskBase *tdb)
{
        UWORD *raw, *rawend;
        UBYTE i;
        UBYTE lasterr;
        UBYTE *data = tdb->td_DataBuffer;

	lasterr = 0;
        raw = tdb->td_DMABuffer;
        rawend = tdb->td_DMABuffer + DISK_BUFFERSIZE * (tdu->tdu_hddisk ? 2 : 1);
        while (tdb->td_sectorbits != (1 << tdu->tdu_sectors) - 1) {
            UWORD *rawnext = raw;
            UBYTE *secdata;
            ULONG odd, even, chksum, id, dlong;
            UBYTE trackoffs;

            if (raw != tdb->td_DMABuffer) {
                while (*raw != 0x4489) {
                    if (raw >= rawend) {
                    	if (lasterr == 0)
                            lasterr = TDERR_TooFewSecs;
                        goto end;
                    }
                    raw++;
                }
            }
            while (*raw == 0x4489 && raw < rawend)
                raw++;
            if (raw + 544 >= rawend) {
            	if (lasterr == 0)
		    lasterr = TDERR_TooFewSecs;
                goto end;
            }

            rawnext = raw + 544 - 3;
            odd = getmfmlong(raw);
            even = getmfmlong(raw + 2);
            raw += 4;
            id = (odd << 1) | even;

            trackoffs = (id & 0xff00) >> 8;
            if (trackoffs >= tdu->tdu_sectors || (id & 0xff000000) != 0xff000000) {
                lasterr = TDERR_BadSecHdr;
                D(bug("td_decodebuffer trackid=%08x\n", id));
                continue; // corrupt sector number
            }
            if (tdb->td_sectorbits & (1 << trackoffs)) {
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
                D(bug("td_decodebuffer sector %d header checksum error\n", trackoffs));
                continue;
            }
            // correct track?
            if (((id & 0x00ff0000) >> 16) != tdb->td_buffer_track) {
                lasterr = TDERR_BadSecHdr;
                D(bug("td_decodebuffer sector %d header track mismatch (%d<>%d)\n",
                	trackoffs, (id & 0x00ff0000) >> 16, tdb->td_buffer_track));
                continue;
            }

            // decode data
            odd = getmfmlong (raw);
            even = getmfmlong (raw + 2);
            raw += 4;
            chksum = (odd << 1) | even;
            secdata = data + trackoffs * 512;
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
           	D(bug("td_decodebuffer sector %d data checksum error\n", trackoffs));
                continue; // data checksum error
            }
            tdb->td_sectorbits |= 1 << trackoffs;
	}
end:
	D(bug("td_decodebuffer err=%d secmask=%08x\n", lasterr, tdb->td_sectorbits));
	return lasterr;
}

static UBYTE td_decodebuffer(struct TDU *tdu, struct TrackDiskBase *tdb)
{
	if (tdu->tdu_disktype == DT_ADOS)
		return td_decodebuffer_ados(tdu, tdb);
	else if (tdu->tdu_disktype == DT_PCDOS)
		return td_decodebuffer_pcdos(tdu, tdb);
	return TDERR_TooFewSecs;
}

static void mfmcode (UWORD *mfm, UWORD words)
{
	ULONG lastword = 0;
	while (words--) {
		ULONG v = *mfm;
		ULONG lv = (lastword << 16) | v;
		ULONG nlv = 0x55555555 & ~lv;
		ULONG mfmbits = (nlv << 1) & (nlv >> 1);
		*mfm++ = v | mfmbits;
		lastword = v;
	}
}

static void td_encodebuffer_ados(struct TDU *tdu, struct TrackDiskBase *tdb)
{
	UWORD i;
	UBYTE sec;
	UBYTE *databuf = tdb->td_DataBuffer;
	UWORD *mfmbuf = (UWORD*)tdb->td_DMABuffer;
	UWORD bufsize = DISK_BUFFERSIZE * (tdu->tdu_hddisk ? 2 : 1);
	UWORD gapsize = bufsize - tdu->tdu_sectors * 2 * 544;

	for (i = 0; i < gapsize / 2 - 2; i++)
		*mfmbuf++ = 0xaaaa;

	for (sec = 0; sec < tdu->tdu_sectors; sec++) {
		UBYTE secbuf[4];
		ULONG deven, dodd;
		ULONG hck = 0, dck = 0;

		secbuf[0] = 0xff;
		secbuf[1] = tdb->td_buffer_track;
		secbuf[2] = sec;
		secbuf[3] = tdu->tdu_sectors - sec;

		mfmbuf[0] = mfmbuf[1] = 0xaaaa;
		mfmbuf[2] = mfmbuf[3] = 0x4489;

		deven = ((secbuf[0] << 24) | (secbuf[1] << 16)
			| (secbuf[2] << 8) | (secbuf[3]));
		dodd = deven >> 1;
		deven &= 0x55555555;
		dodd &= 0x55555555;

		mfmbuf[4] = dodd >> 16;
		mfmbuf[5] = dodd;
		mfmbuf[6] = deven >> 16;
		mfmbuf[7] = deven;

		for (i = 8; i < 24; i++)
			mfmbuf[i] = 0xaaaa;

		for (i = 0; i < 512; i += 4) {
			deven = ((databuf[i + 0] << 24) | (databuf[i + 1] << 16)
				| (databuf[i + 2] << 8) | (databuf[i + 3]));
			dodd = deven >> 1;
			deven &= 0x55555555;
			dodd &= 0x55555555;
			mfmbuf[(i >> 1) + 32] = dodd >> 16;
			mfmbuf[(i >> 1) + 33] = dodd;
			mfmbuf[(i >> 1) + 256 + 32] = deven >> 16;
			mfmbuf[(i >> 1) + 256 + 33] = deven;
		}

		for (i = 4; i < 24; i += 2)
			hck ^= (mfmbuf[i] << 16) | mfmbuf[i + 1];

		deven = dodd = hck;
		dodd >>= 1;
		mfmbuf[24] = dodd >> 16;
		mfmbuf[25] = dodd;
		mfmbuf[26] = deven >> 16;
		mfmbuf[27] = deven;

		for (i = 32; i < 544; i += 2)
			dck ^= (mfmbuf[i] << 16) | mfmbuf[i + 1];

		deven = dodd = dck;
		dodd >>= 1;
		mfmbuf[28] = dodd >> 16;
		mfmbuf[29] = dodd;
		mfmbuf[30] = deven >> 16;
		mfmbuf[31] = deven;
		mfmcode (mfmbuf + 4, 544 - 4);
		
		databuf += 512;
		mfmbuf += 544;
	}
	*mfmbuf++ = 0xaaaa;
	*mfmbuf = 0xaaaa;
}

static void td_encodebuffer(struct TDU *tdu, struct TrackDiskBase *tdb)
{
	if (tdu->tdu_disktype == DT_ADOS)
		td_encodebuffer_ados(tdu, tdb);
}	

static UBYTE td_readbuffer(UBYTE track, struct TDU *tdu, struct TrackDiskBase *tdb)
{
	UBYTE ret;

	if (tdb->td_buffer_unit != tdu->tdu_UnitNum || tdb->td_buffer_track != track)
		tdb->td_sectorbits = 0;
	tdb->td_buffer_unit = tdu->tdu_UnitNum;
	tdb->td_buffer_track = track;
	td_select(tdu, tdb);
	td_seek(tdu, track >> 1, track & 1, tdb);
	ret = td_readwritetrack(track, 0, tdu, tdb);
	if (ret) {
		D(bug("td_readbuffer TRK=%d td_readwritetrack ERR=%d\n", track, ret));
		tdb->td_sectorbits = 0;
		return ret;
	}
	ret = td_decodebuffer(tdu, tdb);
	D(bug("td_readbuffer td_decodebuffer ERR=%d MASK=%08x\n", ret, tdb->td_sectorbits));
	return ret;
}

static void maybe_detect(struct TDU *tdu, struct TrackDiskBase *tdb)
{
	if (tdu->tdu_disktype == DT_UNDETECTED)
		td_detectformat(tdu, tdb);
}

static UBYTE maybe_flush(struct TDU *tdu, struct TrackDiskBase *tdb, int track)
{
	if (tdb->td_buffer_unit != tdu->tdu_UnitNum || tdb->td_buffer_track != track) {
		UBYTE err = 0;
		err = td_flush(tdu, tdb);
		td_clear(tdb);
		return err;
	}
	return 0;
}

UBYTE td_read(struct IOExtTD *iotd, struct TDU *tdu, struct TrackDiskBase *tdb)
{
	UBYTE err;
	APTR data;
	ULONG len, offset;
	WORD totalretries;
	BYTE seeking;
  
	if (tdu->tdu_DiskIn == TDU_NODISK)
		return TDERR_DiskChanged;
	if (checkbuffer(tdu, tdb))
		return TDERR_NoMem;
	maybe_detect(tdu, tdb);

	iotd->iotd_Req.io_Actual = 0;
	offset = iotd->iotd_Req.io_Offset;
	len = iotd->iotd_Req.io_Length;
	data = iotd->iotd_Req.io_Data;

	D(bug("td_read: DATA=%x OFFSET=%x (TRK=%d) LEN=%d\n", data, offset, offset / (512 * tdu->tdu_sectors), len));

	seeking = 0;
	err = 0;
	totalretries = (tdu->pub.tdu_RetryCnt + 1) * QUICKRETRYRCNT - 1;

	while (len > 0 && totalretries >= 0) {

		UBYTE largestsectorneeded, smallestsectorneeded, totalsectorsneeded;
		UBYTE track;
		UBYTE sec, sectorsdone;
 		
		track = offset / (512 * tdu->tdu_sectors);

		if (seeking)
			td_wait_end(tdb);

		if ((totalretries % QUICKRETRYRCNT) == 0) {
			if (!td_recalibrate(tdu, tdb)) {
                    		err = TDERR_SeekError;
                    		break;
                	}
                }

		err = maybe_flush(tdu, tdb, track);
		if (err)
			break;
		if (tdb->td_buffer_unit != tdu->tdu_UnitNum || tdb->td_buffer_track != track)
			err = td_readbuffer(track, tdu, tdb);
		
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

		sectorsdone = 0;
		for (sec = smallestsectorneeded; sec < largestsectorneeded; sec++) {
			if (tdb->td_sectorbits & (1 << sec)) {
				CopyMem(tdb->td_DataBuffer + sec * 512, data + (sec - smallestsectorneeded) * 512, 512);
				sectorsdone++;
			}
		}
		
		D(bug("td_read2 TRK=%d MIN=%d MAX=%d DONE=%d\n", track, smallestsectorneeded, largestsectorneeded, sectorsdone));
		
		if (sectorsdone < totalsectorsneeded) {
			// errors, force re-read
			tdb->td_buffer_unit = -1;
			// couldn't decode any sectors = reseek immediately
			if (tdb->td_sectorbits == 0) 
				totalretries = (totalretries - 1) & ~(QUICKRETRYRCNT - 1);
			else
				totalretries--;
			continue;
		}
		
		data += sectorsdone * 512;
		offset += sectorsdone * 512;
		len -= sectorsdone * 512;
		iotd->iotd_Req.io_Actual += sectorsdone * 512;
		
		err = 0;
	}

	if (seeking)
		td_wait_end(tdb);
	D(bug("td_read2 ERR=%d io_Actual=%d\n", err, iotd->iotd_Req.io_Actual));
	return err;
}

static UBYTE td_write2(struct IOExtTD *iotd, struct TDU *tdu, struct TrackDiskBase *tdb)
{
	APTR data;
	ULONG len, offset;
	UBYTE err;
 
	if (checkbuffer(tdu, tdb))
		return TDERR_NoMem;

	err = 0;
	iotd->iotd_Req.io_Actual = 0;
	offset = iotd->iotd_Req.io_Offset;
	len = iotd->iotd_Req.io_Length;
	data = iotd->iotd_Req.io_Data;

	D(bug("TD_WRITE: DATA=%x OFFSET=%x (TRK=%d) LEN=%d\n", data, offset, offset / (512 * tdu->tdu_sectors), len));

	while (len > 0) {
		UBYTE track, sec, totalsectorsneeded;
		UBYTE smallestsectorneeded, largestsectorneeded;

		track = offset / (512 * tdu->tdu_sectors);
		err = maybe_flush(tdu, tdb, track);
		if (err)
			break;
		tdb->td_buffer_unit = tdu->tdu_UnitNum;
		tdb->td_buffer_track = track;

		smallestsectorneeded = (offset / 512) % tdu->tdu_sectors;
		largestsectorneeded = smallestsectorneeded + len / 512;
		if (largestsectorneeded > tdu->tdu_sectors || len / 512 > tdu->tdu_sectors)
			largestsectorneeded = tdu->tdu_sectors;
		totalsectorsneeded = largestsectorneeded - smallestsectorneeded;

		D(bug("TD_WRITE: TRK=%d MIN=%d MAX=%d MASK=%08x\n",
			track, smallestsectorneeded, largestsectorneeded, tdb->td_sectorbits));

		// fill buffer with new data
		for (sec = smallestsectorneeded; sec < largestsectorneeded; sec++) {
			CopyMem(data + (sec - smallestsectorneeded) * 512, tdb->td_DataBuffer + sec * 512, 512);
			tdb->td_sectorbits |= 1 << sec;
			tdb->td_dirty = TRUE;
		}

		data += totalsectorsneeded * 512;
		offset += totalsectorsneeded * 512;
		len -= totalsectorsneeded * 512;
		iotd->iotd_Req.io_Actual += totalsectorsneeded * 512;
	}
	return err;
}

UBYTE td_write(struct IOExtTD *iotd, struct TDU *tdu, struct TrackDiskBase *tdb)
{
    UBYTE err;
    if (tdu->tdu_DiskIn == TDU_NODISK)
        return TDERR_DiskChanged;
    if (tdu->tdu_disktype != DT_ADOS)
    	return TDERR_WriteProt;
    if (!td_getprotstatus(tdu, tdb)) {
        err = td_write2(iotd, tdu, tdb);
    } else {
        err = TDERR_WriteProt;
    }
    return err;
}

void td_clear(struct TrackDiskBase *tdb)
{
	tdb->td_buffer_unit = -1;
	tdb->td_sectorbits = 0;
	tdb->td_dirty = 0;
}

UBYTE td_flush(struct TDU *tdu, struct TrackDiskBase *tdb)
{
	WORD totalretries;
	UBYTE lasterr, err;

	if (tdb->td_buffer_unit != tdu->tdu_UnitNum)
		return 0;
	if (!tdb->td_dirty)
		return 0;

	err = 0;
	td_select(tdu, tdb);
	D(bug("td_flush, writing unit %d track %d wmask=%08x\n",
		tdb->td_buffer_unit, tdb->td_buffer_track, tdb->td_sectorbits));

	totalretries = (tdu->pub.tdu_RetryCnt + 1) * QUICKRETRYRCNT - 1;
	// read all non-modified sectors from disk (if needed)
	lasterr = 0;
	while (tdb->td_sectorbits != (1 << tdu->tdu_sectors) - 1) {
		if ((totalretries % QUICKRETRYRCNT) == 0) {
			if (!td_recalibrate(tdu, tdb)) {
				tdb->td_dirty = 0;
				tdb->td_buffer_unit = -1;
				return TDERR_SeekError;
               		}
                }
		err = td_readbuffer(tdb->td_buffer_track, tdu, tdb);
		if (err)
			lasterr = err;
		D(bug("TD_FLUSH READBUF ERR=%d MASK=%08x\n", err, tdb->td_sectorbits));
		if (totalretries-- <= 0) {
			bug("TD_FLUSH disk error track %d, written data was lost!\n", tdb->td_buffer_track);
			tdb->td_dirty = 0;
			tdb->td_buffer_unit = -1;
			return lasterr ? lasterr : TDERR_NotSpecified;
		}
	}
	// MFM encode buffer
	td_encodebuffer(tdu, tdb);
	// write buffer
	err = td_readwritetrack(tdb->td_buffer_track, 1, tdu, tdb);
       	td_wait(tdb, 2);
	tdb->td_dirty = 0;
	return err;
}

static UBYTE td_format2(struct IOExtTD *iotd, struct TDU *tdu, struct TrackDiskBase *tdb)
{
    APTR data;
    ULONG len, offset;
    UBYTE err = 0;

    if (checkbuffer(tdu, tdb))
        return TDERR_NoMem;
    if (tdu->tdu_disktype != DT_ADOS)
    	return TDERR_WriteProt;
    iotd->iotd_Req.io_Actual = 0;
    offset = iotd->iotd_Req.io_Offset;
    len = iotd->iotd_Req.io_Length;
    data = iotd->iotd_Req.io_Data;
    D(bug("TD_FORMAT: DATA=%x OFFSET=%x (TRK=%d) LEN=%d\n", data, offset, offset / (512 * tdu->tdu_sectors), len));
    while (len >= tdu->tdu_sectors * 512) {
        int track = offset / (512 * tdu->tdu_sectors);
        td_select(tdu, tdb);
        td_wait(tdb, 2);
        td_seek(tdu, track >> 1, track & 1, tdb);
	CopyMem(data, tdb->td_DataBuffer, tdu->tdu_sectors * 512);
	tdb->td_sectorbits = (1 << tdu->tdu_sectors) - 1;
	tdb->td_buffer_unit = tdu->tdu_UnitNum;
	tdb->td_buffer_track = track;
	td_encodebuffer(tdu, tdb);
        err = td_readwritetrack(tdb->td_buffer_track, 1, tdu, tdb);
        td_clear(tdb);
        if (err)
            return err;
        data += tdu->tdu_sectors * 512;
        offset += tdu->tdu_sectors * 512;
        iotd->iotd_Req.io_Actual += tdu->tdu_sectors * 512;
        len -= tdu->tdu_sectors * 512;
    }
    td_wait(tdb, 2);
    return err;
}

UBYTE td_format(struct IOExtTD *iotd, struct TDU *tdu, struct TrackDiskBase *tdb)
{
    UBYTE err;
    if (tdu->tdu_DiskIn == TDU_NODISK)
	return TDERR_DiskChanged;
    if (!td_getprotstatus(tdu, tdb)) {
	td_flush(tdu, tdb);
	err = td_format2(iotd, tdu, tdb);
    } else {
	err = TDERR_WriteProt;
    }
    return err;
}

static UBYTE countbits(ULONG mask)
{
    UBYTE cnt = 0;
    while (mask) {
    	cnt++;
    	mask >>= 1;
    }
    return cnt;
}

void td_detectformat(struct TDU *tdu, struct TrackDiskBase *tdb)
{
    UBYTE err;
    UBYTE track = 0;
    UBYTE cnt;

    D(bug("detectformat HD=%d\n", tdu->tdu_hddisk ? 1 : 0)); 
    if (checkbuffer(tdu, tdb)) {
	tdu->tdu_disktype = DT_ADOS;
	tdu->tdu_sectors = tdu->tdu_hddisk ? 22 : 11;
	return;
    }
    tdu->tdu_disktype = DT_UNDETECTED;
    tdb->td_sectorbits = 0;
    tdb->td_buffer_unit = tdu->tdu_UnitNum;
    tdb->td_buffer_track = track;
    td_select(tdu, tdb);
    td_seek(tdu, track >> 1, track & 1, tdb);
    tdu->tdu_sectors = tdu->tdu_hddisk ? 22 : 11;
    err = td_readwritetrack(track, 0, tdu, tdb);
    if (!err) {
	err = td_decodebuffer_ados(tdu, tdb);
	/* Did all sectors fail to decode? It could be non-ADOS disk */
	if (err && tdb->td_sectorbits == 0) {
	    tdu->tdu_sectors = tdu->tdu_hddisk ? 18 : 9;
	    err = td_decodebuffer_pcdos(tdu, tdb);
	    cnt = countbits(tdb->td_sectorbits);
	    if (cnt > tdu->tdu_sectors / 2 + 1) {
	    	/* enough sectors decoded fine, assume PCDOS */
	    	tdu->tdu_disktype = DT_PCDOS;
	    }
	} else {
	    tdu->tdu_disktype = DT_ADOS;
	}   
    }
    if (tdu->tdu_disktype == DT_UNDETECTED) {
	tdu->tdu_disktype = DT_ADOS;
	tdu->tdu_sectors = tdu->tdu_hddisk ? 22 : 11;
	tdb->td_sectorbits = 0;
    }
    D(bug("detectformat=%d sectors=%d sectormask=%08x\n", tdu->tdu_disktype, tdu->tdu_sectors, tdb->td_sectorbits));
 }