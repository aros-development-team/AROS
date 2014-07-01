/* $Id$
 * $Log: device.c $
 * Revision 2.5  1999/09/10  22:14:49  Michiel
 * Bugfixes etc (1.4)
 *
 * Revision 2.4  1999/05/07  16:49:00  Michiel
 * bugfixes etc
 *
 * Revision 2.3  1999/05/04  17:59:09  Michiel
 * check mode, logfile, search rootblock implemented
 * bugfixes
 *
 * Revision 2.2  1999/05/04  04:27:13  Michiel
 * debugged upto buildrext
 *
 * Revision 2.1  1999/04/30  12:17:58  Michiel
 * Accepts OK disks, bitmapfix and hardlink fix works
 *
 * Revision 1.2  1999/04/22  15:26:05  Michiel
 * compiled
 *
 * Revision 1.1  1999/04/19  22:16:53  Michiel
 * Initial revision
 *
 * Device Access functies, inclusief cache etc.
 */

#define __USE_SYSBASE
#include <string.h>
#include <math.h>

#include <exec/memory.h>
#include <exec/errors.h>
#include <devices/trackdisk.h>
#include <devices/scsidisk.h>
#include <dos/filehandler.h>
#include <proto/exec.h>
#include <clib/alib_protos.h>
#include <stdlib.h>
#include "pfs3.h"
#include "doctor.h"

#define BLOCKSHIFT (volume.blockshift)

/**************************************
 * Proto
 **************************************/

static struct cacheline *c_GetCacheLine(uint32 bloknr);

/**************************************
 * Globals
 **************************************/

struct cache cache;

/**************************************
 * Cache
 *
 * Cache module. Implemented as proxy on device. This module has a problem
 * at the end of the disk: it does not take partial cachelines into
 * account. This is no real problem since it is only used for the reserved
 * area which is located at the start of the partition.
 **************************************/

/* create cache
 * cacheline size: 4-8-16
 * cachelines: 8-16-32-64-128
 * size = linesize * nolines * blocksize
 */
error_t InitCache(uint32 linesize, uint32 nolines)
{
	int i;

	/* allocate memory for the cache */
	if (!(cache.cachelines = calloc(nolines, sizeof(struct cacheline))))
		return e_out_of_memory;
	
	cache.linesize = linesize;
	cache.nolines = nolines;
	NewList((struct List *)&cache.LRUqueue);
	NewList((struct List *)&cache.LRUpool);
	for (i=0; i<nolines; i++)
	{
		cache.cachelines[i].blocknr = CL_UNUSED;	/* a bloknr that is never used */
		cache.cachelines[i].data = AllocBufMem(linesize*volume.blocksize);
		if (cache.cachelines[i].data)
		{
			MinAddHead(&cache.LRUpool, &cache.cachelines[i]);
		}
	}
	return e_none;
}

/* get block (of size 'bytes') from cache. Cache is per >device<.
 */
error_t c_GetBlock(uint8 *data, uint32 bloknr, uint32 bytes)
{
	uint32 lineblnr, offset, total;
	struct cacheline *cl = NULL;
	error_t error = e_none;

	__chkabort();
	for (total=0; total<bytes; total+=volume.blocksize, bloknr++, data+=volume.blocksize)
	{
		lineblnr = (bloknr / cache.linesize) * cache.linesize;
		offset = (bloknr % cache.linesize);
		cl = c_GetCacheLine(lineblnr);
		if (!cl)
			return e_read_error;

		memcpy(data, cl->data + offset*volume.blocksize, volume.blocksize);
	}
	return error;
}

/* write block (of size 'bytes') to cache
 */
error_t c_WriteBlock(uint8 *data, uint32 bloknr, uint32 bytes)
{
	uint32 lineblnr, offset, total;
	struct cacheline *cl = NULL;
	error_t error = e_none;

	__chkabort();
	for (total=0; total<bytes; total+=volume.blocksize, bloknr++, data+=volume.blocksize)
	{
		lineblnr = (bloknr / cache.linesize) * cache.linesize;
		offset = (bloknr % cache.linesize);
		cl = c_GetCacheLine(lineblnr);
		if (!cl)
			return e_read_error;

		memcpy(cl->data + offset*volume.blocksize, data, volume.blocksize);
		cl->dirty = true;
	}
	return error;
}

/* (private) locale function to search a cacheline 
 */
static struct cacheline *c_GetCacheLine(uint32 bloknr)
{
	uint32 i;
	struct cacheline *cl = NULL;
	error_t error;

	for (i=0; i<cache.nolines; i++)
	{
		if (cache.cachelines[i].blocknr == bloknr)
		{
			cl = &cache.cachelines[i];
			break;
		}
	}

	if (!cl)
	{
		if (IsMinListEmpty(&cache.LRUpool))
		{
			cl = (struct cacheline *)cache.LRUqueue.mlh_TailPred;
			if (cl->dirty)
				error = volume.writerawblocks(cl->data, cache.linesize, cl->blocknr);
		}
		else
		{
			cl = HeadOf(&cache.LRUpool);
		}

		error = volume.getrawblocks(cl->data, cache.linesize, bloknr);
		if (error)
			return NULL;
		cl->blocknr = bloknr;
		cl->dirty = false;
	}

	MinRemove(cl);
	MinAddHead(&cache.LRUqueue, cl);
	return cl;
}

/* update alle dirty blocks in the cache
 */
void UpdateCache(void)
{
	struct cacheline *cl;
	error_t error;

	for (cl = HeadOf(&cache.LRUqueue); cl->next; cl=cl->next)
	{
		if (cl->dirty)
		{
			error = volume.writerawblocks(cl->data, cache.linesize, cl->blocknr);
			if (error)
				adderror("write error");
		}
	}
}

/* destroy cache
 */
void FreeCache(void)
{
	int i;

	if (cache.cachelines)
	{
		UpdateCache();
		for (i = 0; i < cache.nolines; i++)
			FreeBufMem(cache.cachelines[i].data);

		free(cache.cachelines);
		cache.cachelines = NULL;
	}
}


/**************************************
 * Device functions
 **************************************/

/* OpenDiskDevice
 *
 * Open the diskdevice passed in 'startup' and generate the
 * messageport and request structure, returned in 'port'
 * and 'request' resp.
 *
 * in startup
 * out port, request, trackdisk
 */
BOOL OpenDiskDevice(struct FileSysStartupMsg *startup, struct MsgPort **port,
		struct IOExtTD **request, BOOL *trackdisk)
{
  UBYTE name[FNSIZE];

	*port = CreateMsgPort();
	if(*port)
	{
		*request = (struct IOExtTD *)CreateIORequest(*port, sizeof(struct IOExtTD));
		if(*request)
		{
			BCPLtoCString(name, (UBYTE *)BADDR(startup->fssm_Device));
			*trackdisk = (strcmp(name, "trackdisk.device") == 0) || (strcmp(name, "diskspare.device") == 0);
			if(OpenDevice(name, startup->fssm_Unit, (struct IORequest *)*request,
				startup->fssm_Flags) == 0)
				return TRUE;
		}
	}
	return FALSE;
}

/*
 * Get blocks from device. Blocknrs are disk (not partition!) based
 */
error_t dev_GetBlocks(uint8 *buffer, int32 blocks, uint32 blocknr)
{
	struct IOExtTD *request;
	uint32 io_length, io_transfer, io_offset, io_actual = 0, io_startblock = 0;
	uint8 *io_buffer;
	int retry = 4;

retry_read:
	if(blocknr == -1)   // blocknr of uninitialised anode
		return 1;

	io_length = blocks << volume.blockshift;
	io_offset = blocknr << volume.blockshift;
	io_buffer = buffer;
	if (volume.td64mode || volume.nsdmode) {
		// upper 32 bit of offset
		io_actual = blocknr >> (32-volume.blockshift);
		io_startblock = blocknr;
	}

	while (io_length > 0)
	{
		io_transfer = min(io_length, volume.dosenvec->de_MaxTransfer);
		io_transfer &= ~(volume.blocksize-1);
		request = volume.request;
		request->iotd_Req.io_Command = CMD_READ;
		request->iotd_Req.io_Length  = io_transfer;
		request->iotd_Req.io_Data    = io_buffer;       // bufmemtype ??
		request->iotd_Req.io_Offset  = io_offset;
		if (volume.td64mode) {
			request->iotd_Req.io_Actual  = io_actual;
			request->iotd_Req.io_Command = TD_READ64;
		} else if (volume.nsdmode) {
			request->iotd_Req.io_Actual  = io_actual;
			request->iotd_Req.io_Command = NSCMD_TD_READ64;
		}			

		if (DoIO((struct IORequest*)request))
		{
			volume.showmsg("READ ERROR\n");
			if (!retry--)
				return e_read_error;
			goto retry_read;
		}
		io_buffer += io_transfer;
		io_length -= io_transfer;
		if (volume.td64mode || volume.nsdmode) {
			io_startblock += (io_transfer >> volume.blockshift);
			io_offset = io_startblock << volume.blockshift;
			io_actual = io_startblock >> (32-volume.blockshift);
		} else {
			io_offset += io_transfer;
		}
	}

	return e_none;
}


error_t dev_GetBlocksDS(uint8 *buffer, int32 blocks, uint32 blocknr)
{
	uint8  cmdbuf[10];
	uint32 transfer, maxtransfer;
	int retry = 4;

retry_read:
	if(blocknr == -1)   // blocknr of uninitialised anode
		return 1;

	/* chop in maxtransfer chunks */
	maxtransfer = volume.dosenvec->de_MaxTransfer >> volume.blockshift;
	while (blocks > 0)
	{
		transfer = min(blocks,maxtransfer);
		*((UWORD *)&cmdbuf[0]) = 0x2800;
		*((ULONG *)&cmdbuf[2]) = blocknr;
		*((ULONG *)&cmdbuf[6]) = transfer<<8;
		if (!DoSCSICommand(buffer,transfer<<BLOCKSHIFT,cmdbuf,10,SCSIF_READ))
		{
			volume.showmsg("READ ERROR\n");
			if (!retry--)
				return e_read_error;

			goto retry_read;
		}
		buffer += transfer<<BLOCKSHIFT;
		blocks -= transfer;
		blocknr += transfer;
	}

	return e_none;
}

error_t dev_WriteBlocksDummy(uint8 *buffer, int32 blocks, uint32 blocknr)
{
	return e_none;
}

error_t dev_WriteBlocksDS(uint8 *buffer, int32 blocks, uint32 blocknr)
{
	UBYTE cmdbuf[10];
	ULONG transfer, maxtransfer;
	int retry = 4;

retry_write:
	if(blocknr == -1)
		return 1;

	/* chop in maxtransfer chunks */
	maxtransfer = volume.dosenvec->de_MaxTransfer >> volume.blockshift;
	while (blocks > 0)
	{
		transfer = min(blocks,maxtransfer);
		*((UWORD *)&cmdbuf[0]) = 0x2a00;
		*((ULONG *)&cmdbuf[2]) = blocknr;
		*((ULONG *)&cmdbuf[6]) = transfer<<8;
		if (!DoSCSICommand(buffer,blocks<<BLOCKSHIFT,cmdbuf,10,SCSIF_WRITE))
		{
			volume.showmsg("WRITE ERROR\n");
			if (!retry--)
				return e_write_error;

			goto retry_write;
		}
		buffer += transfer<<BLOCKSHIFT;
		blocks -= transfer;
		blocknr += transfer;
	}

	return e_none;
}

error_t dev_WriteBlocks(uint8 *buffer, int32 blocks, uint32 blocknr)
{
	struct IOExtTD *request;
	uint32 io_length, io_transfer, io_offset, io_actual = 0, io_startblock = 0;
	uint8 *io_buffer;
	int	retry = 4;

retry_write:
	if(blocknr == -1)   // blocknr of uninitialised anode
		return e_write_error;

	io_length = blocks << volume.blockshift;
	io_offset = blocknr << volume.blockshift;
	io_buffer = buffer;
	if (volume.td64mode || volume.nsdmode) {
		// upper 32 bit of offset
		io_actual = blocknr >> (32 - BLOCKSHIFT);
		io_startblock = blocknr;
	}

	while(io_length > 0)
	{
		io_transfer = min(io_length, volume.dosenvec->de_MaxTransfer);
		io_transfer &= ~(volume.blocksize-1);
		request = volume.request;
		request->iotd_Req.io_Command = CMD_WRITE;
		request->iotd_Req.io_Length  = io_transfer;
		request->iotd_Req.io_Data    = io_buffer;       // bufmemtype ??
		request->iotd_Req.io_Offset  = io_offset;
		if (volume.td64mode) {
			request->iotd_Req.io_Actual = io_actual;
			request->iotd_Req.io_Command = TD_WRITE64;
		} else if (volume.nsdmode) {
			request->iotd_Req.io_Actual = io_actual;
			request->iotd_Req.io_Command = NSCMD_TD_WRITE64;
		}

		if (DoIO((struct IORequest*)request))
		{
			volume.showmsg("WRITE ERROR\n");
			if (!retry--)
				return e_write_error;
			goto retry_write;
		}
		io_buffer += io_transfer;
		io_length -= io_transfer;
		if (volume.td64mode || volume.nsdmode) {
			io_startblock += (io_transfer >> volume.blockshift);
			io_offset = io_startblock << volume.blockshift;
			io_actual = io_startblock >> (32-volume.blockshift);
		} else {
			io_offset += io_transfer;
		}
	}

	return e_none;
}

/*
 * Allocation of memory for buffers
 */

void *AllocBufMem(uint32 size)
{
	void *buffer;

	buffer = AllocVec(size, volume.dosenvec->de_BufMemType|MEMF_CLEAR);

	if (((uint32)buffer) & ~volume.dosenvec->de_Mask)
	{
		volume.showmsg ("Memory mask fails - aborting\n");
		FreeVec(buffer);
		return NULL;
	}

	return buffer;
}

void FreeBufMem (void *mem)
{
	FreeVec (mem);
}

/*
 * Do direct scsi command
 */

static uint8 sense[20];
static struct SCSICmd scsicmd;

int DoSCSICommand(UBYTE *data, ULONG datalen, UBYTE *command,
	UWORD commandlen, UBYTE direction)
{
	scsicmd.scsi_Data = (UWORD *)data;
	scsicmd.scsi_Length = datalen;
	scsicmd.scsi_Command = command;
	scsicmd.scsi_CmdLength = commandlen;
	scsicmd.scsi_Flags = SCSIF_AUTOSENSE | direction; /* SCSIF_READ or SCSIF_WRITE */
	scsicmd.scsi_SenseData = sense;
	scsicmd.scsi_SenseLength = 18;
	scsicmd.scsi_SenseActual = 0;

	volume.request->iotd_Req.io_Length = sizeof(struct SCSICmd);
	volume.request->iotd_Req.io_Data = (APTR)&scsicmd;
	volume.request->iotd_Req.io_Command = HD_SCSICMD;
	DoIO((struct IORequest *)volume.request);
	if (scsicmd.scsi_Status)
		return 0;
	else 
		return 1;
}

/* Convert BCPL/Pascal string to a CString. 
 * dest: resulting c string
 * src: source pascal string (no BPTR!)
 */
UBYTE *BCPLtoCString(STRPTR dest, UBYTE *src)
{
  UBYTE len, *to;

	len  = *(src++);
	len	 = min (len, PATHSIZE);
	to	 = dest;

	while (len--)
		*(dest++) = *(src++);
	*dest = 0x0;

	return to;
}

/* ACCESS MODE AUTODETECT */

#define BLOCKSHIFT (volume.blockshift)
#define BLOCKSIZE (volume.blocksize)

static void fillbuffer(UBYTE *buffer, UBYTE data)
{
	memset (buffer, data + 1, BLOCKSIZE);
}
/* Check if at least one byte has changed */
static BOOL testbuffer(UBYTE *buffer, UBYTE data)
{
	ULONG cnt;
	
	for (cnt = 0; cnt < BLOCKSIZE; cnt++) {
		if (buffer[cnt] != data + 1)
			return TRUE;
	}
	return FALSE;
}

static BOOL testread_ds(UBYTE *buffer)
{
	UBYTE cmdbuf[10];
	UBYTE cnt;

	for (cnt = 0; cnt < 2; cnt++) {
		ULONG capacity;

		fillbuffer(buffer, 0xfe);
		/* Read Capacity */
		*((UWORD *)&cmdbuf[0]) = 0x2500;
		*((ULONG *)&cmdbuf[2]) = 0;
		*((ULONG *)&cmdbuf[6]) = 0;
		if (!DoSCSICommand(buffer, 8, cmdbuf, 10, SCSIF_READ)) {
			return FALSE;
		}
		capacity = *((ULONG*)buffer);

		if (volume.lastblock > capacity) {
			return FALSE;
		}
		fillbuffer(buffer, cnt);
		/* Read(10) */
		*((UWORD *)&cmdbuf[0]) = 0x2800;
		*((ULONG *)&cmdbuf[2]) = volume.lastblock;
		*((ULONG *)&cmdbuf[6]) = 1 << 8;
		if (!DoSCSICommand(buffer, 1 << BLOCKSHIFT, cmdbuf, 10, SCSIF_READ)) {
			return FALSE;
		}
		if (testbuffer(buffer, cnt)) {
			return TRUE;
		}
	}
	return FALSE;
}

static BOOL testread_td(UBYTE *buffer)
{
	struct IOExtTD *io = volume.request;
	UBYTE cnt;

	if (volume.accessmode == ACCESS_NSD) {
    	struct NSDeviceQueryResult nsdqr;
    	UWORD *cmdcheck;
		nsdqr.SizeAvailable  = 0;
		nsdqr.DevQueryFormat = 0;
		io->iotd_Req.io_Command = NSCMD_DEVICEQUERY;
		io->iotd_Req.io_Length  = sizeof(nsdqr);
		io->iotd_Req.io_Data    = (APTR)&nsdqr;
		if (DoIO((struct IORequest*)io) != 0)
			return FALSE;
		if (!((io->iotd_Req.io_Actual >= 16) && (io->iotd_Req.io_Actual <= sizeof(nsdqr)) && (nsdqr.SizeAvailable == io->iotd_Req.io_Actual)))
    		return FALSE;
		if(nsdqr.DeviceType != NSDEVTYPE_TRACKDISK)
			return FALSE;
		for(cmdcheck = nsdqr.SupportedCommands; *cmdcheck; cmdcheck++) {
			if(*cmdcheck == NSCMD_TD_READ64)
				break;
		}
		if (*cmdcheck == 0)
			return FALSE;
	}
	if (volume.accessmode == ACCESS_TD64) {
		UBYTE err;
		io->iotd_Req.io_Command = TD_READ64;
		io->iotd_Req.io_Length = 0;
		io->iotd_Req.io_Data = 0;
		io->iotd_Req.io_Offset = 0;
		io->iotd_Req.io_Actual = 0;
		err = DoIO((struct IORequest*)io);
		if (err != 0 && err != IOERR_BADLENGTH && err != IOERR_BADADDRESS)
			return FALSE;
	}
	for (cnt = 0; cnt < 2; cnt++) {
		fillbuffer(buffer, cnt);
		io->iotd_Req.io_Command = CMD_READ;
		io->iotd_Req.io_Length  = BLOCKSIZE;
		io->iotd_Req.io_Data    = buffer;
		io->iotd_Req.io_Offset  = volume.lastblock << BLOCKSHIFT;
		if (volume.accessmode >= ACCESS_TD64) {
			io->iotd_Req.io_Actual  = volume.lastblock >> (32 - BLOCKSHIFT);
			io->iotd_Req.io_Command = volume.accessmode == ACCESS_NSD ? NSCMD_TD_READ64 : TD_READ64;
		}
		if (DoIO((struct IORequest*)io) != 0)
			return FALSE;
		if (testbuffer(buffer, cnt))
			return TRUE;
	}
	return TRUE;
}

BOOL DetectAccessmode(UBYTE *buffer, BOOL scsidirectfirst)
{
	BOOL inside4G = volume.lastblock < (0x80000000ul >> (BLOCKSHIFT - 1));

	if (scsidirectfirst) {
		volume.accessmode = ACCESS_DS;
		if (testread_ds(buffer))
			return TRUE;
	}

	if (volume.lastblock < 262144) {
		/* Don't bother with tests if small enough (<128M) */
		volume.accessmode = ACCESS_STD;
		return TRUE;
	}

	if (inside4G) {
		/* inside first 4G? Try standard CMD_READ first. */
		volume.accessmode = ACCESS_STD;
		if (testread_td(buffer))
			return TRUE;
		volume.accessmode = ACCESS_DS;
		/* It failed, we may have 1G limit A590 pre-7.0 ROM or CDTV SCSI, try DS */
		if (testread_ds(buffer))
			return TRUE;
		volume.accessmode = ACCESS_STD;
		return FALSE;
	}
	/* outside of first 4G, must use TD64, NSD or DS */
	volume.accessmode = ACCESS_NSD;
	if (testread_td(buffer))
		return TRUE;
	volume.accessmode = ACCESS_TD64;
	if (testread_td(buffer))
		return TRUE;
	volume.accessmode = ACCESS_DS;
	if (testread_ds(buffer))
		return TRUE;

	volume.accessmode = ACCESS_STD;
	return FALSE;
}

