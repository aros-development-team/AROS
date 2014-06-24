/* $Id$ */
/* $Log: disk.c $
 * Revision 15.12  1999/03/25  22:05:00  Michiel
 * fixed deldir related (beta) bug
 *
 * Revision 15.11  1999/02/22  16:25:30  Michiel
 * Changes for increasing deldir capacity
 *
 * Revision 15.10  1998/09/27  11:26:37  Michiel
 * ErrorMsg param
 *
 * Revision 15.9  1998/09/03  07:12:14  Michiel
 * versie 17.4
 * bugfixes 118, 121, 123 and superindexblocks and td64 support
 *
 * Revision 15.8  1998/04/23  22:27:07  Michiel
 * FakeCachedRead toegevoegd om munglist check hits te voorkomen
 * Bug in WriteToFile opgelost: toevoeging CorrectAnodeAc
 *
 * Revision 15.7  1997/03/03  22:04:04  Michiel
 * Release 16.21
 *
 * Revision 15.6  1996/03/14  19:32:56  Michiel
 * Fixed ChangeFileSize bug: had no effect when # blocks remained constant
 *
 * Revision 15.5  1996/01/03  09:58:36  Michiel
 * replaced CopyMem() by memcpy()
 *
 * Revision 15.4  1995/12/21  11:59:31  Michiel
 * bugfixes: ValidateCache() block flushing and
 * WriteToFile expensive last block
 *
 * Revision 15.3  1995/12/21  11:32:44  Michiel
 * code cleanup
 *
 * Revision 15.2  1995/12/14  13:24:08  Michiel
 * Direct SCSI support
 *
 * Revision 15.1  1995/12/05  15:47:27  Michiel
 * Rollover files implemented
 * Restructured: ReadFromObject, WriteToObject etc
 * bugfixes
 *
 * Revision 14.4  1995/11/15  15:44:40  Michiel
 * WriteToFile, ChangeFileSize adapted to postponed op.
 *
 * Revision 14.3  1995/11/07  14:57:31  Michiel
 * support for online directory update in WriteToFile and ChangeFileSize
 * ReservedAreaLock check
 *
 * Revision 14.2  1995/10/11  23:25:24  Michiel
 * UpdateSlot added: improved sequential write
 *
 * Revision 14.1  1995/10/11  22:18:30  Michiel
 * new data-caching algorithm
 *
 * Revision 13.2  1995/10/05  11:01:32  Michiel
 * minor changes
 *
 * Revision 13.1  1995/10/03  11:08:55  Michiel
 * merged with developtree: anodecache
 *
 * Revision 12.15  1995/09/04  09:57:10  Michiel
 * mask check now starts at first whole block and includes last block
 *
 * Revision 12.14  1995/09/01  11:20:15  Michiel
 * RawRead and RawWrite error handling changed:
 * on error a retry|cancel requester appears. Retry and
 * same volumecheck changed. Numsofterrors update added.
 *
 * Revision 12.13  1995/08/24  13:48:26  Michiel
 * TD_DiskChange checking enabled
 *
 * Revision 12.12  1995/08/21  04:25:48  Michiel
 * better checks for out of memory
 *
 * Revision 12.11  1995/08/04  04:13:15  Michiel
 * extra CUTDOWN protection
 *
 * Revision 12.10  1995/07/21  06:48:51  Michiel
 * DELDIR adaptions
 * bugfix: MaxTransfer now doesn't have to be multiple of blocksize
 *
 * Revision 12.9  1995/07/11  17:29:31  Michiel
 * ErrorMsg () calls use messages.c variables now.
 *
 * Revision 12.8  1995/07/07  14:39:17  Michiel
 * AFSLITE stuff
 *
 * Revision 12.7  1995/06/19  09:42:45  Michiel
 * Rawwrite returns error if softprotect is on
 *
 * Revision 12.6  1995/06/16  10:00:15  Michiel
 * using Allec & FreeBufMem
 *
 * Revision 12.5  1995/06/16  04:06:29  Michiel
 * No Touch () in write, just MakeBlockDirty
 *
 * Revision 12.4  1995/05/20  12:12:12  Michiel
 * Updated messages to reflect Ami-FileLock
 * CUTDOWN version
 * protection update
 *
 * Revision 12.3  1995/03/30  11:54:29  Michiel
 * Write & Setfilesize now set the checknotify flag
 *
 * Revision 12.2  1995/02/15  16:43:39  Michiel
 * Release version
 * Using new headers (struct.h & blocks.h)
 *
 * Revision 12.1  1995/02/02  11:26:11  Michiel
 * Version number fix. No changes.
 *
 * Revision 11.8  1995/01/29  07:34:57  Michiel
 * Major update
 * ReadFromFile, WriteToFile completely rewritten
 * CachedRead added
 * DataCaching updated
 *
 * Revision 11.7  1995/01/26  12:20:42  Michiel
 * a minor change
 *
 * Revision 11.6  1995/01/24  17:44:34  Michiel
 * bug fixes
 *
 * Revision 11.5  1995/01/24  15:54:20  Michiel
 * DirectRead, CachedRead etc
 *
 * Revision 11.4  1995/01/23  16:43:35  Michiel
 * Directwrite in WriteToFile added
 *
 * Revision 11.3  1995/01/18  04:29:34  Michiel
 * Bugfixes. Now ready for beta release.
 *
 * Revision 11.2  1995/01/15  05:24:44  Michiel
 * trackdisk specific parts inhibited
 *
 * Revision 11.1  1995/01/08  16:17:32  Michiel
 * Compiled (new MODE_BIG version)
 *
 * Revision 10.4  1994/11/15  18:06:58  Michiel
 * __USE_SYSBASE moved..
 *
 * Revision 10.3  1994/10/28  06:06:40  Michiel
 * uses new listentry field anodenr
 *
 * Revision 10.2  1994/10/27  11:30:12  Michiel
 * *** empty log message ***
 *
 * Revision 10.1  1994/10/24  11:16:28  Michiel
 * first RCS revision
 * */

#define TESTING 1

//#define DEBUG 1
#define __USE_SYSBASE

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <dos/filehandler.h>
//#include <sprof.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "debug.h"

// own includes
#include "blocks.h"
#include "struct.h"
#include "disk_protos.h"
#include "allocation_protos.h"
#include "volume_protos.h"
#include "directory_protos.h"
#include "anodes_protos.h"
#include "update_protos.h"
#include "checkaccess_protos.h"

#define PROFILE_OFF()
#define PROFILE_ON()

/**********************************************************************/
/*                               DEBUG                                */
/**********************************************************************/

#ifdef DEBUG
static UBYTE debugbuf[120];
extern BOOL debug;
#define DebugOn debug++
#define DebugOff debug=0
#define DebugMsg(msg) if(debug) {NormalErrorMsg(msg, NULL);debug=0;}
#define DebugMsgNum(msg, num) sprintf(debugbuf, "%s 0x%08lx.", msg, num); \
			if(debug) {NormalErrorMsg(debugbuf, NULL);debug=0;}
#define DebugMsgName(msg, name) sprintf(debugbuf, "%s >%s<.", msg, name); \
			if(debug) {NormalErrorMsg(debugbuf, NULL);debug=0;}
#else
#define DebugOn
#define DebugOff
#define DebugMsg(m)
#define DebugMsgNum(msg,num)
#define DebugMsgName(msg, name)
#endif

enum vctype {read, write};
static int CheckDataCache(ULONG blocknr, globaldata *g);
static int CachedRead(ULONG blocknr, SIPTR *error, globaldata *g);
static int FakeCachedRead(ULONG blocknr, SIPTR *error, globaldata *g);
static UBYTE *CachedReadD(ULONG blknr, SIPTR *err, globaldata *g);
static int CachedWrite(UBYTE *data, ULONG blocknr, globaldata *g);
static void ValidateCache(ULONG blocknr, ULONG numblocks, enum vctype, globaldata *g);
static void UpdateSlot(int slotnr, globaldata *g);
static ULONG ReadFromRollover(fileentry_t *file, UBYTE *buffer, ULONG size, SIPTR *error, globaldata *g);
static ULONG WriteToRollover(fileentry_t *file, UBYTE *buffer, ULONG size, SIPTR *error, globaldata *g);
static SFSIZE SeekInRollover(fileentry_t *file, SFSIZE offset, LONG mode, SIPTR *error, globaldata *g);
static SFSIZE ChangeRolloverSize(fileentry_t *file, SFSIZE releof, LONG mode, SIPTR *error, globaldata *g);
static ULONG ReadFromFile(fileentry_t *file, UBYTE *buffer, ULONG size, SIPTR *error, globaldata *g);
static ULONG WriteToFile(fileentry_t *file, UBYTE *buffer, ULONG size, SIPTR *error, globaldata *g);

/**********************************************************************/
/*                            READ & WRITE                            */
/*                            READ & WRITE                            */
/*                            READ & WRITE                            */
/**********************************************************************/

ULONG ReadFromObject(fileentry_t *file, UBYTE *buffer, ULONG size,
	SIPTR *error, globaldata *g)
{
	if (!CheckReadAccess(file,error,g))
		return -1;

	/* check anodechain, make if not there */
	if (!file->anodechain)
	{
		DB(Trace(2,"ReadFromObject","getting anodechain"));
		if (!(file->anodechain = GetAnodeChain(file->le.anodenr, g)))
		{
			*error = ERROR_NO_FREE_STORE;
			return -1;
		}
	}

#if ROLLOVER
	if (IsRollover(file->le.info))
		return ReadFromRollover(file,buffer,size,error,g);
	else
#endif
		return ReadFromFile(file,buffer,size,error,g);
}

ULONG WriteToObject(fileentry_t *file, UBYTE *buffer, ULONG size,
	SIPTR *error, globaldata *g)
{
	/* check write access */
	if (!CheckWriteAccess(file, error, g))
		return -1;

	/* check anodechain, make if not there */
	if (!file->anodechain)
	{
		if (!(file->anodechain = GetAnodeChain(file->le.anodenr, g)))
		{
			*error = ERROR_NO_FREE_STORE;
			return -1;
		}
	}

	/* changing file -> set notify flag */
	file->checknotify = 1;
	g->dirty = 1;

#if ROLLOVER
	if (IsRollover(file->le.info))
		return WriteToRollover(file,buffer,size,error,g);
	else
#endif
		return WriteToFile(file,buffer,size,error,g);
}

SFSIZE SeekInObject(fileentry_t *file, SFSIZE offset, LONG mode, SIPTR *error,
	globaldata *g)
{
	/* check access */
	if (!CheckOperateFile(file,error,g))
		return -1;

	/* check anodechain, make if not there */
	if (!file->anodechain)
	{
		if (!(file->anodechain = GetAnodeChain(file->le.anodenr, g)))
		{
			*error = ERROR_NO_FREE_STORE;
			return -1;
		}
	}

#if ROLLOVER
	if (IsRollover(file->le.info))
		return SeekInRollover(file,offset,mode,error,g);
	else
#endif
		return SeekInFile(file,offset,mode,error,g);
}

SFSIZE ChangeObjectSize(fileentry_t *file, SFSIZE releof, LONG mode,
	SIPTR *error, globaldata *g)
{
	/* check access */
	if (!CheckChangeAccess(file, error, g))
		return -1;

	/* Changing file -> set notify flag */
	file->checknotify = 1;
	*error = 0;

	/* check anodechain, make if not there */
	if (!file->anodechain)
	{
		if (!(file->anodechain = GetAnodeChain(file->le.anodenr, g)))
		{
			*error = ERROR_NO_FREE_STORE;
			return -1;
		}
	}

#if ROLLOVER
	if (IsRollover(file->le.info))
		return ChangeRolloverSize(file,releof,mode,error,g);
	else
#endif
		return ChangeFileSize(file,releof,mode,error,g);
}



/**********************************************************************
 *
 **********************************************************************/

#if ROLLOVER

/* Read from rollover: at end of file,
 * goto start
 */
static ULONG ReadFromRollover(fileentry_t *file, UBYTE *buffer, ULONG size,
	SIPTR *error, globaldata *g)
{
#define direntry_m file->le.info.file.direntry
#define filesize_m GetDEFileSize(file->le.info.file.direntry, g)

	struct extrafields extrafields;
	ULONG read = 0;
	LONG q; // quantity
	LONG end, virtualoffset, virtualend, t;

	DB(Trace(1,"ReadFromRollover","size = %lx offset = %lx\n",size,file->offset));
	if (!size) return 0;
	GetExtraFields(direntry_m,&extrafields);

	/* limit access to end of file */
	virtualoffset = file->offset - extrafields.rollpointer;
	if (virtualoffset < 0) virtualoffset += filesize_m;
	virtualend = virtualoffset + size;
	virtualend = min(virtualend, extrafields.virtualsize);
	end = virtualend - virtualoffset + file->offset;

	if (end > filesize_m)
	{
		q = filesize_m - file->offset;
		if ((read = ReadFromFile(file, buffer, q, error, g)) != q)
			return read;

		end -= filesize_m;
		buffer += q;
		SeekInFile(file, 0, OFFSET_BEGINNING, error, g);
	}

	q = end - file->offset;
	t = ReadFromFile(file, buffer, q, error, g);
	if (t == -1)
		return (ULONG)t;
	else
		read += t;

	return read;

#undef filesize_m
#undef direntry_m
}

/* Write to rollover file. First write upto end of rollover. Then
 * flip to start.
 * Max virtualsize = filesize-1
 */
static ULONG WriteToRollover(fileentry_t *file, UBYTE *buffer, ULONG size,
	SIPTR *error, globaldata *g)
{
#define direntry_m file->le.info.file.direntry
#define filesize_m GetDEFileSize(file->le.info.file.direntry, g)

	struct extrafields extrafields;
	struct direntry *destentry;
	union objectinfo directory;
	struct fileinfo fi;
	UBYTE entrybuffer[MAX_ENTRYSIZE];
	LONG written = 0;
	LONG q; // quantity
	LONG end, virtualend, virtualoffset, t;
	BOOL extend = FALSE;

	DB(Trace(1,"WriteToRollover","size = %lx offset=%lx, file=%lx\n",size,file->offset,file));
	GetExtraFields(direntry_m,&extrafields);
	end = file->offset + size;

	/* new virtual size */
	virtualoffset = file->offset - extrafields.rollpointer;
	if (virtualoffset < 0) virtualoffset += filesize_m;
	virtualend = virtualoffset + size;
	if (virtualend >= extrafields.virtualsize)
	{
		extrafields.virtualsize = min(filesize_m-1, virtualend);
		extend = TRUE;
	}

	while (end > filesize_m)
	{
		q = filesize_m - file->offset;
		t = WriteToFile(file, buffer, q, error, g);
		if (t == -1) return (ULONG)t;
		written += t;
		if (t != q) return (ULONG)written;
		end -= filesize_m;
		buffer += q;
		SeekInFile(file, 0, OFFSET_BEGINNING, error, g);
	}

	q = end - file->offset;
	t = WriteToFile(file, buffer, q, error, g);
	if (t == -1)
		return (ULONG)t;
	else
		written += t;

	/* change rollpointer etc */
	if (extend && extrafields.virtualsize == filesize_m - 1)
		extrafields.rollpointer = end + 1;  /* byte PAST eof is offset 0 */
	destentry = (struct direntry *)entrybuffer;
	memcpy(destentry, direntry_m, direntry_m->next);
	AddExtraFields(destentry, &extrafields);

	/* commit changes */
	if (!GetParent(&file->le.info, &directory, error, g))
		return DOSFALSE;
	else
		ChangeDirEntry(file->le.info.file, destentry, &directory, &fi, g);

	return (ULONG)written;

#undef direntry_m
#undef filesize_m
}

static SFSIZE SeekInRollover(fileentry_t *file, SFSIZE offset, LONG mode, SIPTR *error, globaldata *g)
{
#define filesize_m GetDEFileSize(file->le.info.file.direntry, g)
#define direntry_m file->le.info.file.direntry

	struct extrafields extrafields;
	LONG oldvirtualoffset, virtualoffset;
	ULONG anodeoffset, blockoffset;

	DB(Trace(1,"SeekInRollover","offset = %ld mode=%ld\n",offset,mode));
	GetExtraFields(direntry_m,&extrafields);

	/* do the seeking */
	oldvirtualoffset = file->offset - extrafields.rollpointer;
	if (oldvirtualoffset < 0) oldvirtualoffset += filesize_m;

	switch (mode)
	{
		case OFFSET_BEGINNING:
			virtualoffset = offset;
			break;

		case OFFSET_END:
			virtualoffset = extrafields.virtualsize + offset;
			break;
		
		case OFFSET_CURRENT:
			virtualoffset = oldvirtualoffset + offset;
			break;
		
		default:
			*error = ERROR_SEEK_ERROR;
			return -1;
	}

	if ((virtualoffset > extrafields.virtualsize) || virtualoffset < 0)
	{
		*error = ERROR_SEEK_ERROR;
		return -1;
	}

	/* calculate real offset */
	file->offset = virtualoffset + extrafields.rollpointer;
	if (file->offset > filesize_m)
		file->offset -= filesize_m;

	/* calculate new values */
	anodeoffset = file->offset >> BLOCKSHIFT;
	blockoffset = file->offset & (BLOCKSIZE-1);
	file->currnode = &file->anodechain->head;
	CorrectAnodeAC(&file->currnode, &anodeoffset, g);
	
	file->anodeoffset  = anodeoffset;
	file->blockoffset  = blockoffset;

	return oldvirtualoffset;

#undef filesize_m
#undef direntry_m
}


static SFSIZE ChangeRolloverSize(fileentry_t *file, SFSIZE releof, LONG mode,
	SIPTR *error, globaldata *g)
{
#define filesize_m GetDEFileSize(file->le.info.file.direntry, g)
#define direntry_m file->le.info.file.direntry

	struct extrafields extrafields;
	SFSIZE virtualeof, virtualoffset;
	union objectinfo directory;
	struct fileinfo fi;
	struct direntry *destentry;
	UBYTE entrybuffer[MAX_ENTRYSIZE];

	DB(Trace(1,"ChangeRolloverSize","offset = %ld mode=%ld\n",releof,mode));
	GetExtraFields(direntry_m,&extrafields);

	switch (mode)
	{
		case OFFSET_BEGINNING:
			virtualeof = releof;
			break;

		case OFFSET_END:
			virtualeof = extrafields.virtualsize + releof;
			break;

		case OFFSET_CURRENT:
			virtualoffset = file->offset - extrafields.rollpointer;
			if (virtualoffset < 0) virtualoffset += filesize_m;
			virtualeof = virtualoffset + releof;
			break;
		default: /* bogus parameter -> ERROR_SEEK_ERROR */
			virtualeof = -1;
			break;
	}
  
	if (virtualeof < 0)
	{
		*error = ERROR_SEEK_ERROR;
		return -1;
	}

	/* change virtual size */
	if (virtualeof >= filesize_m)
		extrafields.virtualsize = filesize_m - 1;
	else
		extrafields.virtualsize = virtualeof;

	/* we don't update other filehandles or current offset here */

	/* commit directoryentry changes */
	destentry = (struct direntry *)entrybuffer;
	memcpy(destentry, direntry_m, direntry_m->next);
	AddExtraFields(destentry, &extrafields);

	/* commit changes */
	if (!GetParent(&file->le.info, &directory, error, g))
		return DOSFALSE;
	else
		ChangeDirEntry(file->le.info.file, destentry, &directory, &fi, g);

	return virtualeof;

#undef filesize_m
#undef direntry_m
}

#endif /* ROLLOVER */

/* <ReadFromFile>
**
** Specification:
**
** Reads 'size' bytes from file to buffer (if not readprotected)
** result: #bytes read; -1 = error; 0 = eof
*/
static ULONG ReadFromFile(fileentry_t *file, UBYTE *buffer, ULONG size,
				SIPTR *error, globaldata *g)
{
	ULONG anodeoffset, blockoffset, blockstoread;
	ULONG fullblks, bytesleft;
	ULONG t;
	FSIZE tfs;
	UBYTE *data = NULL, *dataptr;
	BOOL directread =  FALSE;
	struct anodechainnode *chnode;
#if DELDIR
	struct deldirentry *dde;
#endif

	DB(Trace(1,"ReadFromFile","size = %lx offset = %lx\n",size,file->offset));
	if (!CheckReadAccess(file, error, g))
		return -1;

	/* correct size and check if zero */
#if DELDIR
	if (IsDelFile(file->le.info)) {
		if (!(dde = GetDeldirEntryQuick(file->le.info.delfile.slotnr, g)))
			return -1;
		tfs = GetDDFileSize(dde, g) - file->offset;
	}
	else
#endif
		tfs = GetDEFileSize(file->le.info.file.direntry, g) - file->offset;

	if (!(size = min(tfs, size)))
		return 0;

	/* initialize */
	anodeoffset = file->anodeoffset;
	blockoffset = file->blockoffset;
	chnode = file->currnode;
	t = blockoffset + size;
	fullblks = t>>BLOCKSHIFT;       /* # full blocks */
	bytesleft = t&(BLOCKSIZE-1);    /* # bytes in last incomplete block */

	/* check mask, both at start and end */
	t = (((IPTR)(buffer-blockoffset+BLOCKSIZE))&~g->dosenvec->de_Mask) ||
		(((IPTR)(buffer+size-bytesleft))&~g->dosenvec->de_Mask);
	t = !t;

	/* read indirect if
	 * - mask failure
	 * - too small
	 * - larger than one block (use 'direct' cached read for just one)
	 */
	if (!t || (fullblks<2*DIRECTSIZE && (blockoffset+size>BLOCKSIZE) &&
			  (blockoffset || (bytesleft&&fullblks<DIRECTSIZE))))
	{
		/* full indirect read */
		blockstoread = fullblks + (bytesleft>0);
		if (!(data = AllocBufmem (blockstoread<<BLOCKSHIFT, g)))
		{
			*error = ERROR_NO_FREE_STORE;
			return -1;
		}
		dataptr = data;
	}
	else
	{
		/* direct read */
		directread = TRUE;
		blockstoread = fullblks;
		dataptr = buffer;

		/* read first blockpart */
		if (blockoffset)
		{
			data = CachedReadD(chnode->an.blocknr + anodeoffset, error, g);
			if (data)
			{
				NextBlockAC(&chnode, &anodeoffset, g);

				/* calc numbytes */
				t = BLOCKSIZE-blockoffset;
				t = min(t, size);
				memcpy(dataptr, data+blockoffset, t);
				dataptr+=t;
				if (blockstoread)
					blockstoread--;
				else
					bytesleft = 0;      /* single block access */
			}
		}
	}

	/* read middle part */
	while (blockstoread && !*error) 
	{
		if ((blockstoread + anodeoffset) >= chnode->an.clustersize)
			t = chnode->an.clustersize - anodeoffset;   /* read length */
		else
			t = blockstoread;

		*error = DiskRead(dataptr, t, chnode->an.blocknr + anodeoffset, g);
		if (!*error)
		{
			blockstoread -= t;
			dataptr      += t<<BLOCKSHIFT;
			anodeoffset  += t;
			CorrectAnodeAC(&chnode, &anodeoffset, g);
		}
	}
	
	/* read last block part/ copy read data to buffer */
	if (!*error)
	{
		if (!directread)
			memcpy(buffer, data+blockoffset, size);
		else if (bytesleft)
		{
			data = CachedReadD(chnode->an.blocknr+anodeoffset, error, g);
			if (data)
				memcpy(dataptr, data, bytesleft);
		}
	}

	if (!directread)
		FreeBufmem(data, g);
	if (!*error)
	{
		file->anodeoffset += fullblks;
		file->blockoffset = (file->blockoffset + size)&(BLOCKSIZE-1);   // not bytesleft!!
		CorrectAnodeAC(&file->currnode, &file->anodeoffset, g);
		file->offset += size;
		return size;
	}
	else
	{
		DB(Trace(1,"Read","failed\n"));
		return -1;
	}
}



/* <WriteToFile> 
**
** Specification:
**
** - Copy data in file at current position;
** - Automatic fileextension;
** - Error = bytecount <> opdracht
** - On error no position update
**
** - Clear Archivebit -> done by Touch()
**V- directory protection (amigados does not do this)
**
** result: num bytes written; DOPUS wants -1 = error;
**
** Implementation parts
**
** - Test on writeprotection; yes -> error;
** - Initialisation
** - Extend filesize
** - Write firstblockpart
** - Write all whole blocks
** - Write last block
** - | Update directory (if no errors)
**   | Deextent filesize (if error)
*/
static ULONG WriteToFile(fileentry_t *file, UBYTE *buffer, ULONG size,
			SIPTR *error, globaldata *g)
{
	ULONG maskok, t;
	ULONG totalblocks, oldblocksinfile;
	FSIZE oldfilesize, newfileoffset;
	ULONG newblocksinfile, bytestowrite, blockstofill;
	ULONG anodeoffset, blockoffset;
	UBYTE *data = NULL, *dataptr;
	BOOL directwrite = FALSE;
	struct anodechainnode *chnode;
	int slotnr;

	DB(Trace(1,"WriteToFile","size = %lx offset=%lx, file=%lx\n",size,file->offset,file));
	/* initialization values */
	chnode = file->currnode;
	anodeoffset = file->anodeoffset;
	blockoffset = file->blockoffset;
	totalblocks = (blockoffset + size + BLOCKSIZE-1)>>BLOCKSHIFT;   /* total # changed blocks */
	if (!(bytestowrite = size))                                     /* # bytes to be done */
		return 0;

	/* filesize extend */
	oldfilesize = GetDEFileSize(file->le.info.file.direntry, g);
	newfileoffset = file->offset + size;

	/* Check if too large (QUAD) or overflowed (ULONG)? */
	if (newfileoffset > MAX_FILE_SIZE || newfileoffset < file->offset) {
		*error = ERROR_DISK_FULL;
		return -1;
	}

	oldblocksinfile = (oldfilesize + BLOCKSIZE-1)>>BLOCKSHIFT;
	newblocksinfile = (newfileoffset + BLOCKSIZE-1)>>BLOCKSHIFT;
	if (newblocksinfile > oldblocksinfile)
	{
		t = newblocksinfile - oldblocksinfile;
		if (!AllocateBlocksAC(file->anodechain, t, &file->le.info.file, g))
		{
			SetDEFileSize(file->le.info.file.direntry, oldfilesize, g);
			*error = ERROR_DISK_FULL;
			return -1;
		}
	}
	/* BUG 980422: this CorrectAnodeAC mode because of AllocateBlockAC!! AND
	 * because anodeoffset can be outside last block! (filepointer is
	 * byte 0 new block
	 */
	CorrectAnodeAC(&chnode,&anodeoffset,g);

	/* check mask */
	maskok = (((IPTR)(buffer-blockoffset+BLOCKSIZE))&~g->dosenvec->de_Mask) ||
			 (((IPTR)(buffer-blockoffset+(totalblocks<<BLOCKSHIFT)))&~g->dosenvec->de_Mask);
	maskok = !maskok;

	/* write indirect if
	 * - mask failure
	 * - too small
	 */
	if (!maskok || (totalblocks<2*DIRECTSIZE && (blockoffset+size>BLOCKSIZE*2) &&
			  (blockoffset || totalblocks<DIRECTSIZE)))
	{
		/* indirect */
		/* allocate temporary data buffer */
		if (!(dataptr = data = AllocBufmem(totalblocks<<BLOCKSHIFT, g)))
		{
			*error = ERROR_NO_FREE_STORE;
			goto wtf_error;
		}

		/* first blockpart */
		if (blockoffset)
		{
			*error = DiskRead(dataptr, 1, chnode->an.blocknr + anodeoffset, g);
			bytestowrite += blockoffset;
			if (bytestowrite<BLOCKSIZE)
				bytestowrite = BLOCKSIZE;   /* the first could also be the last block */
		}

		/* copy all 'to be written' to databuffer */
		memcpy(dataptr+blockoffset, buffer, size);
	}
	else
	{
		/* direct */
		dataptr = buffer;
		directwrite = TRUE;

		/* first blockpart */
		if (blockoffset || (totalblocks==1 && newfileoffset > oldfilesize))
		{
			ULONG fbp;  /* first block part */
			UBYTE *firstblock;

			if (blockoffset) 
			{
				slotnr = CachedRead(chnode->an.blocknr + anodeoffset, error, g);
				if (*error)
					goto wtf_error;
			}
			else
			{
				/* for one block no offset growing file */
				slotnr = FakeCachedRead(chnode->an.blocknr + anodeoffset, error, g);
			}

			/* copy data to cache and mark block as dirty */
			firstblock = &g->dc.data[slotnr<<BLOCKSHIFT];
			fbp = BLOCKSIZE-blockoffset;
			fbp = min(bytestowrite, fbp);       /* the first could also be the last block */
			memcpy(firstblock+blockoffset, buffer, fbp);
			MarkDataDirty(slotnr);

			NextBlockAC(&chnode, &anodeoffset, g);
			bytestowrite -= fbp;
			dataptr += fbp;
			totalblocks--;
		}
	}

	/* write following blocks. If done, then blockoffset always 0 */
	if (newfileoffset > oldfilesize)
	{
		blockstofill = totalblocks;
		bytestowrite = totalblocks<<BLOCKSHIFT;
	}
	else
		blockstofill = bytestowrite>>BLOCKSHIFT;

	while (blockstofill && !*error)
	{
		if (blockstofill + anodeoffset >= chnode->an.clustersize)
			t = chnode->an.clustersize - anodeoffset;   /* t is # blocks to write now */
		else
			t = blockstofill;

		*error = DiskWrite(dataptr, t, chnode->an.blocknr + anodeoffset, g);
		if (!*error)
		{
			blockstofill  -= t;
			dataptr       += t<<BLOCKSHIFT;
			bytestowrite  -= t<<BLOCKSHIFT;
			anodeoffset   += t;
			CorrectAnodeAC(&chnode, &anodeoffset, g);
		}
	}   

	/* write last block (RAW because cache direct) */
	if (bytestowrite && !*error)
	{
	  UBYTE *lastblock;

		slotnr = CachedRead(chnode->an.blocknr + anodeoffset, error, g);
		if (!*error)
		{
			lastblock = &g->dc.data[slotnr<<BLOCKSHIFT];
			memcpy(lastblock, dataptr, bytestowrite);
			MarkDataDirty(slotnr);
		}
	}

	/* free mem for indirect write */
	if (!directwrite)
		FreeBufmem(data, g);
	if (!*error)
	{
		file->anodeoffset += (blockoffset + size)>>BLOCKSHIFT; 
		file->blockoffset  = (blockoffset + size)&(BLOCKSIZE-1);
		CorrectAnodeAC(&file->currnode, &file->anodeoffset, g);
		file->offset      += size;
		SetDEFileSize(file->le.info.file.direntry, max(oldfilesize, file->offset), g);
		MakeBlockDirty((struct cachedblock *)file->le.info.file.dirblock, g);
		return size;
	}

wtf_error:
	if (newblocksinfile>oldblocksinfile)
	{
		/* restore old state of file */
#if VERSION23
		SetDEFileSize(file->le.info.file.direntry, oldfilesize, g);
		MakeBlockDirty((struct cachedblock *)file->le.info.file.dirblock, g);
		FreeBlocksAC(file->anodechain, newblocksinfile-oldblocksinfile, freeanodes, g);
#else
		FreeBlocksAC(file->anodechain, newblocksinfile-oldblocksinfile, freeanodes, g);
		SetDEFileSize(file->le.info.file.direntry, oldfilesize, g);
		MakeBlockDirty((struct cachedblock *)file->le.info.file.dirblock, g);
#endif
	}

	DB(Trace(1,"WriteToFile","failed\n"));
	return -1;
}


/* SeekInFile
**
** Specification:
**
** - set fileposition
** - if wrong position, resultposition unknown and error
** - result = old position to start of file, -1 = error
**
** - the end of the file is 0 from end
**
*/
SFSIZE SeekInFile(fileentry_t *file, SFSIZE offset, LONG mode, SIPTR *error, globaldata *g)
{
	SFSIZE oldoffset, newoffset;
	ULONG anodeoffset, blockoffset;
#if DELDIR
	struct deldirentry *delfile = NULL;

	DB(Trace(1,"SeekInFile","offset = %ld mode=%ld\n",offset,mode));
	if (IsDelFile(file->le.info))
		if (!(delfile = GetDeldirEntryQuick(file->le.info.delfile.slotnr, g)))
			return -1;
#endif

	/* do the seeking */
	oldoffset = file->offset;
	newoffset = -1;

	/* TODO: 32-bit wraparound checks */

	switch (mode)
	{
		case OFFSET_BEGINNING:
			newoffset = offset;
			break;
		
		case OFFSET_END:
#if DELDIR
			if (delfile)
				newoffset = GetDDFileSize(delfile, g) + offset;
			else
#endif
				newoffset = GetDEFileSize(file->le.info.file.direntry, g) + offset;
			break;
		
		case OFFSET_CURRENT:
			newoffset = oldoffset + offset;
			break;
		
		default:
			*error = ERROR_SEEK_ERROR;
			return -1;
	}

#if DELDIR
	if ((newoffset > (delfile ? GetDDFileSize(delfile, g) :
		GetDEFileSize(file->le.info.file.direntry, g))) || (newoffset < 0))
#else
	if ((newoffset > GetDEFileSize(file->le.info.file.direntry)) || (newoffset < 0))
#endif
	{
		*error = ERROR_SEEK_ERROR;
		return -1;
	}

	/* calculate new values */
	anodeoffset = newoffset >> BLOCKSHIFT;
	blockoffset = newoffset & (BLOCKSIZE-1);
	file->currnode = &file->anodechain->head;
	CorrectAnodeAC(&file->currnode, &anodeoffset, g);
	/* DiskSeek(anode.blocknr + anodeoffset, g); */
	
	file->anodeoffset  = anodeoffset;
	file->blockoffset  = blockoffset;
	file->offset       = newoffset;
	return oldoffset;
}


/* Changes the length of a file
** Returns new length; -1 if failure
**
** Check if allowed
** 'Seek with extend'
** change other locks
** change direntry
*/
SFSIZE ChangeFileSize(fileentry_t *file, SFSIZE releof, LONG mode, SIPTR *error,
	globaldata *g)
{
	listentry_t *fe;
	SFSIZE abseof;
	LONG t;
	ULONG myanode, oldblocksinfile, newblocksinfile;
	FSIZE oldfilesize;

	/* check access */
	DB(Trace(1,"ChangeFileSize","offset = %ld mode=%ld\n",releof,mode));
	if (!CheckChangeAccess(file, error, g))
		return -1;

	/* Changing file -> set notify flag */
	file->checknotify = 1;
	*error = 0;

	/* TODO: 32-bit wraparound checks */

	/* calculate new eof (ala 'Seek') */
	switch (mode)
	{
		case OFFSET_BEGINNING:
			abseof = releof;
			break;
		
		case OFFSET_END:
			abseof = GetDEFileSize(file->le.info.file.direntry, g) + releof;
			break;
		
		case OFFSET_CURRENT:
			abseof = file->offset + releof;
			break;
		
		default:
			*error = ERROR_SEEK_ERROR;
			return DOSFALSE;
	}

	/* < 0 check still needed because QUAD is signed */
	if (abseof < 0 || abseof > MAX_FILE_SIZE)
	{
		*error = ERROR_SEEK_ERROR;
		return -1;
	}

	/* change allocation (ala WriteToFile) */
	oldfilesize = GetDEFileSize(file->le.info.file.direntry, g);
	oldblocksinfile = (GetDEFileSize(file->le.info.file.direntry, g) + BLOCKSIZE-1)>>BLOCKSHIFT;
	newblocksinfile = (abseof+BLOCKSIZE-1)>>BLOCKSHIFT;

	if (newblocksinfile > oldblocksinfile)
	{
		/* new blocks, 4*allocated anode, dirblock */
		t = newblocksinfile - oldblocksinfile; 
		if (!AllocateBlocksAC(file->anodechain, t, &file->le.info.file, g))
		{
			SetDEFileSize(file->le.info.file.direntry, oldfilesize, g);
			*error = ERROR_DISK_FULL;
			return -1;
		}

		/* change directory: in case of allocate this has to be done
		 * afterwards
		 */
		SetDEFileSize(file->le.info.file.direntry, abseof, g);
		MakeBlockDirty((struct cachedblock *)file->le.info.file.dirblock, g);
	}
	else if (oldblocksinfile > newblocksinfile)
	{
		/* change directoryentry beforehand (needed for postponed delete but not
		 * allowed for online updating allocate).
		 */
		SetDEFileSize(file->le.info.file.direntry, abseof, g);
		MakeBlockDirty((struct cachedblock *)file->le.info.file.dirblock, g);

		t = oldblocksinfile - newblocksinfile;
		FreeBlocksAC(file->anodechain, t, freeanodes, g);

		/* PS: there will always be an anode left, since FreeBlocksAC
		 * doesn't delete the last anode
		 */
	}
	else
	{
		/* no change in number of blocks, just change directory entry */
		SetDEFileSize(file->le.info.file.direntry, abseof, g);
		MakeBlockDirty((struct cachedblock *)file->le.info.file.dirblock, g);
	}

	/* change filehandles (including own) */
	myanode = file->le.anodenr;
	for (fe = HeadOf(&g->currentvolume->fileentries); fe->next; fe=fe->next)
	{
		if (fe->anodenr == myanode)
		{
			if (IsFileEntry(fe) && ((fileentry_t *)fe)->offset >= abseof)
				SeekInFile((fileentry_t *)fe, abseof, OFFSET_BEGINNING, error, g);
		}
	}

	return abseof;
}


/**********************************************************************/
/*                           CACHE STUFF                              */
/**********************************************************************/

/* check datacache. return cache slotnr or -1
 * if not found
 */
static int CheckDataCache(ULONG blocknr, globaldata *g)
{
	int i;

	for (i = 0; i < g->dc.size; i++)
	{
		if (g->dc.ref[i].blocknr == blocknr)
			return i;
	}

	return -1;
}

/* get block from cache or put it in cache if it wasn't
 * there already. return cache slotnr. errors are indicated by 'error'
 * (null = ok)
 */
static int CachedRead(ULONG blocknr, SIPTR *error, globaldata *g)
{
	int i;

	*error = 0;
	i = CheckDataCache(blocknr, g);
	if (i != -1) return i;
	i = g->dc.roving;
	if (g->dc.ref[i].dirty && g->dc.ref[i].blocknr)
		UpdateSlot(i, g);

	*error = RawRead(&g->dc.data[i<<BLOCKSHIFT], 1, blocknr, g);
	g->dc.roving = (g->dc.roving+1)&g->dc.mask;
	g->dc.ref[i].dirty = 0;
	g->dc.ref[i].blocknr = blocknr;
	return i;
}

static int FakeCachedRead(ULONG blocknr, SIPTR *error, globaldata *g)
{
	int i;

	*error = 0;
	i = CheckDataCache(blocknr, g);
	if (i != -1) return i;
	i = g->dc.roving;
	if (g->dc.ref[i].dirty && g->dc.ref[i].blocknr)
		UpdateSlot(i, g);

	memset(&g->dc.data[i<<BLOCKSHIFT], 0xAA, BLOCKSIZE);
	g->dc.roving = (g->dc.roving+1)&g->dc.mask;
	g->dc.ref[i].dirty = 0;
	g->dc.ref[i].blocknr = blocknr;
	return i;
}

static UBYTE *CachedReadD(ULONG blknr, SIPTR *err, globaldata *g)
{ 
	int i;

	i = CachedRead(blknr,err,g);
	if (*err)   
		return NULL;
	else
		return &g->dc.data[i<<BLOCKSHIFT];
}

/* write block in cache. if block was already cached,
 * overwrite it. return slotnr (never fails).
 */
static int CachedWrite(UBYTE *data, ULONG blocknr, globaldata *g)
{
	int i;

	i = CheckDataCache(blocknr, g);
	if (i == -1)
	{
		i = g->dc.roving;
		g->dc.roving = (g->dc.roving+1)&g->dc.mask;
		if (g->dc.ref[i].dirty && g->dc.ref[i].blocknr)
			UpdateSlot(i, g);
	}
	memcpy(&g->dc.data[i<<BLOCKSHIFT], data, BLOCKSIZE);
	g->dc.ref[i].dirty = 1;
	g->dc.ref[i].blocknr = blocknr;
	return i;
}


/* flush all blocks in datacache (without updating them first).
 */
void FlushDataCache(globaldata *g)
{
	int i;

	for (i=0; i<g->dc.size; i++)
		g->dc.ref[i].blocknr = 0;
}

/* write all dirty blocks to disk
 */
void UpdateDataCache(globaldata *g)
{
	int i;

	for (i=0; i<g->dc.size; i++)
		if (g->dc.ref[i].dirty && g->dc.ref[i].blocknr)
			UpdateSlot (i, g);
}


/* update a data cache slot, and any adjacent blocks
 */
static void UpdateSlot(int slotnr, globaldata *g)
{
	ULONG blocknr;
	int i;
	
	blocknr = g->dc.ref[slotnr].blocknr;

	/* find out how many adjacent blocks can be written */
	for (i=slotnr; i<g->dc.size; i++)
	{
		if (g->dc.ref[i].blocknr != blocknr++)
			break;
		g->dc.ref[i].dirty = 0;
	}

	/* write them */
	RawWrite(&g->dc.data[slotnr<<BLOCKSHIFT], i-slotnr, g->dc.ref[slotnr].blocknr, g);
}

/* update cache to reflect blocks read to or written
 * from disk. to be called before disk is accessed.
 */
static void ValidateCache(ULONG blocknr, ULONG numblocks, enum vctype vctype, globaldata *g)
{
	int i;

	ENTER("ValidateCache");
	for (i=0; i<g->dc.size; i++)
	{
		if (g->dc.ref[i].blocknr >= blocknr && 
			g->dc.ref[i].blocknr < blocknr + numblocks)
		{
			if (vctype == read)
			{
				if (g->dc.ref[i].dirty)
					UpdateSlot(i, g);
			}
			else    // flush
				g->dc.ref[i].blocknr = 0;
		}
	}
}


/**********************************************************************/
/*                           DEVICECOMMANDS                           */
/*                           DEVICECOMMANDS                           */
/*                           DEVICECOMMANDS                           */
/**********************************************************************/


/* DiskRead
**
** Reads 'blocks' complete blocks in a caller supplied buffer.
**
** input : - buffer: buffer for data
**         - blockstoread: number of blocks to read
**         - blocknr: starting block
**       
** global: - disk is used to get request struct
**
** result: errornr, 0=ok
*/
ULONG DiskRead(UBYTE *buffer, ULONG blockstoread, ULONG blocknr, globaldata *g)
{
	SIPTR error;
	int slotnr;

	DB(Trace(1, "DiskRead", "%ld blocks from %ld firstblock %ld\n",
		 (ULONG)blockstoread, (ULONG)blocknr, g->firstblock));

	if (blocknr == (ULONG)-1)  // blocknr of uninitialised anode
		return 1;
	if (!blockstoread)
		return 0;

	if (blockstoread == 1)
	{
		slotnr = CachedRead(blocknr, &error, g);
		memcpy(buffer, &g->dc.data[slotnr<<BLOCKSHIFT], BLOCKSIZE);
		return error;
	}
	ValidateCache(blocknr, blockstoread, read, g);
	return RawRead(buffer, blockstoread, blocknr, g);
}


/* DiskWrite
**
** Writes 'blocks' complete blocks from a buffer.
**
** input : - buffer: the data
**         - blockstowrite: number of blocks to write
**         - blocknr: starting block
**       
** global: - disk is used to get request struct
**
** result: errornr, 0=ok
*/
ULONG DiskWrite(UBYTE *buffer, ULONG blockstowrite, ULONG blocknr, globaldata *g)
{
	ULONG slotnr;
	ULONG error = 0;

	DB(Trace(1, "DiskWrite", "%ld blocks from %ld + %ld\n", blockstowrite, blocknr,
			g->firstblock));

	if (blocknr == (ULONG)-1)  // blocknr of uninitialised anode
		return 1;
	if (!blockstowrite)
		return 0;

	if (blockstowrite == 1)
	{
		CachedWrite(buffer, blocknr, g);
		return 0;
	}
	ValidateCache(blocknr, blockstowrite, write, g);
	error = RawWrite(buffer, blockstowrite, blocknr, g);

	/* cache last block written */
	if (!error)
	{
		buffer += ((blockstowrite-1)<<BLOCKSHIFT);
		slotnr = CachedWrite(buffer, blocknr+blockstowrite-1, g);
		g->dc.ref[slotnr].dirty = 0;    // we just wrote it
	}
	return error;
}

/*
 * SCSI direct functions
 */

#if SCSIDIRECT

static int DoSCSICommand(UBYTE *data, ULONG datalen, UBYTE *command,
	UWORD commandlen, UBYTE direction, globaldata *g)
{
	g->scsicmd.scsi_Data = (UWORD *)data;
	g->scsicmd.scsi_Length = datalen;
	g->scsicmd.scsi_Command = command;
	g->scsicmd.scsi_CmdLength = commandlen;
	g->scsicmd.scsi_Flags = SCSIF_AUTOSENSE | direction; /* SCSIF_READ or SCSIF_WRITE */
	g->scsicmd.scsi_SenseData = g->sense;
	g->scsicmd.scsi_SenseLength = 18;
	g->scsicmd.scsi_SenseActual = 0;
	g->scsicmd.scsi_Status = 1;

	g->request->iotd_Req.io_Length = sizeof(struct SCSICmd);
	g->request->iotd_Req.io_Data = (APTR)&g->scsicmd;
	g->request->iotd_Req.io_Command = HD_SCSICMD;
	if (DoIO((struct IORequest *)g->request) != 0)
		return 0;
#if 0
	DebugPutHex("doio", err);
	DebugPutHex("status", g->scsicmd.scsi_Status);
#endif
	if (g->scsicmd.scsi_Status)
		return 0;
	else 
		return 1;
}

static ULONG RawRead_DS(UBYTE *buffer, ULONG blocks, ULONG blocknr, globaldata *g)
{
	UBYTE cmdbuf[10];
	ULONG transfer, maxtransfer;

	if(blocknr == (ULONG)-1)   // blocknr of uninitialised anode
		return 1;

	blocknr += g->firstblock;
retry_read:
	if(!(InPartition(blocknr) && InPartition(blocknr+blocks-1)))
	{
		ErrorMsg(AFS_ERROR_READ_OUTSIDE, NULL, g);
		return ERROR_SEEK_ERROR;
	}

	/* chop in maxtransfer chunks */
	maxtransfer = g->maxtransfer >> BLOCKSHIFT;
	while (blocks > 0)
	{
		transfer = min(blocks,maxtransfer);
		*((UWORD *)&cmdbuf[0]) = 0x2800;
		*((ULONG *)&cmdbuf[2]) = blocknr;
		*((ULONG *)&cmdbuf[6]) = transfer<<8;
		PROFILE_OFF();
		if (!DoSCSICommand(buffer,transfer<<BLOCKSHIFT,cmdbuf,10,SCSIF_READ,g))
		{
			ULONG args[2];
			PROFILE_ON();
			args[0] = g->sense[2];
			args[1] = blocknr - g->firstblock;
			while ((g->ErrorMsg)(AFS_ERROR_READ_ERROR, args, 2, g))
			{
				if (CheckCurrentVolumeBack(g))
					goto retry_read;
			}
			if (!g->softprotect)
			{
				g->softprotect = 1;
				g->protectkey = ~0;
			}
			if (g->currentvolume)
				g->currentvolume->numsofterrors++;
		}
		PROFILE_ON();
		buffer += transfer<<BLOCKSHIFT;
		blocks -= transfer;
		blocknr += transfer;
	}

	return 0;
}

/*
 * VVV Todo: SCSI ErrorNumber in requester
 */

static ULONG RawWrite_DS(UBYTE *buffer, ULONG blocks, ULONG blocknr, globaldata *g)
{
	UBYTE cmdbuf[10];
	ULONG transfer, maxtransfer;

	if(blocknr == (ULONG)-1)
		return 1;

	blocknr += g->firstblock;
retry_write:
	if(g->softprotect)
		return ERROR_DISK_WRITE_PROTECTED;

	if (!(InPartition(blocknr) && InPartition(blocknr+blocks-1)))
	{
		ErrorMsg (AFS_ERROR_WRITE_OUTSIDE, NULL, g);
		return ERROR_SEEK_ERROR;
	}

	/* chop in maxtransfer chunks */
	maxtransfer = g->maxtransfer >> BLOCKSHIFT;
	while (blocks > 0)
	{
		transfer = min(blocks,maxtransfer);
		*((UWORD *)&cmdbuf[0]) = 0x2a00;
		*((ULONG *)&cmdbuf[2]) = blocknr;
		*((ULONG *)&cmdbuf[6]) = transfer<<8;
		PROFILE_OFF();
		if (!DoSCSICommand(buffer,blocks<<BLOCKSHIFT,cmdbuf,10,SCSIF_WRITE,g))
		{
			ULONG args[2];
			PROFILE_ON();
			args[0] = g->sense[2];
			args[1] = blocknr - g->firstblock;
			while ((g->ErrorMsg)(AFS_ERROR_WRITE_ERROR, args, 2, g))
			{
				if (CheckCurrentVolumeBack(g))
					goto retry_write;
			}
			if (!g->softprotect)
			{
				g->softprotect = 1;
				g->protectkey = ~0;
			}
			if (g->currentvolume)
				g->currentvolume->numsofterrors++;
		}
		PROFILE_ON();
		buffer += transfer<<BLOCKSHIFT;
		blocks -= transfer;
		blocknr += transfer;
	}

	return 0;
}

#endif /* SCSI Direct */

#if TD64

/*
 * Normal commands
 */
	
/* Geometry MUST be loaded!!
*/
static ULONG RawRead_TD(UBYTE *buffer, ULONG blocks, ULONG blocknr, globaldata *g)
{
	struct IOExtTD *request;
	ULONG realblocknr;
	ULONG io_length, io_transfer, io_offset, io_actual = 0, io_startblock = 0;
	UBYTE *io_buffer;

	DB(Trace(1, "RawRead", "%ld blocks from %ld firstblock %ld\n",
		 (ULONG)blocks, (ULONG)blocknr, g->firstblock));

retry_read:
	if(blocknr == (ULONG)-1)   // blocknr of uninitialised anode
		return 1;

	realblocknr = blocknr + g->firstblock;
	if(!(InPartition(realblocknr) && InPartition(realblocknr+blocks-1)))
	{
		ErrorMsg (AFS_ERROR_READ_OUTSIDE, NULL, g);
		return ERROR_SEEK_ERROR;
	}

	io_length = blocks << BLOCKSHIFT;
	io_offset = realblocknr << BLOCKSHIFT;
	io_buffer = buffer;
	if (g->tdmode >= ACCESS_TD64) {
		// upper 32 bit of offset
		io_actual = realblocknr >> (32-BLOCKSHIFT);
		io_startblock = realblocknr;
	}

	while (io_length > 0)
	{
		io_transfer = min(io_length, g->maxtransfer);
		io_transfer &= ~(BLOCKSIZE-1);
		request = g->request;
		request->iotd_Req.io_Command = CMD_READ;
		request->iotd_Req.io_Length  = io_transfer;
		request->iotd_Req.io_Data    = io_buffer;       // bufmemtype ??
		request->iotd_Req.io_Offset  = io_offset;
		if (g->tdmode >= ACCESS_TD64) {
			request->iotd_Req.io_Actual  = io_actual;
			request->iotd_Req.io_Command = g->tdmode == ACCESS_NSD ? NSCMD_TD_READ64 : TD_READ64;
		}

		PROFILE_OFF();
		if (DoIO((struct IORequest*)request) != 0)
		{
			ULONG args[2];
			PROFILE_ON();
			args[0] = request->iotd_Req.io_Error;
			args[1] = blocknr;  /* should be realblocknr ?? */
			while ((g->ErrorMsg)(AFS_ERROR_READ_ERROR, args, 2, g))
			{
				if (CheckCurrentVolumeBack(g))
					goto retry_read;
			}
			if (!g->softprotect)
			{
				g->softprotect = 1;
				g->protectkey = ~0;
			}
			if (g->currentvolume)
				g->currentvolume->numsofterrors++;
			DB(Trace(1,"RawRead","readerror nr %ld\n", args[0], args[1]));
			return ERROR_NOT_A_DOS_DISK;
		}
		PROFILE_ON();
		io_buffer += io_transfer;
		io_length -= io_transfer;
		if (g->tdmode >= ACCESS_TD64) {
			io_startblock += (io_transfer >> BLOCKSHIFT);
			io_offset = io_startblock << BLOCKSHIFT;
			io_actual = io_startblock >> (32-BLOCKSHIFT);
		} else {
			io_offset += io_transfer;
		}
	}

	return 0;
}


#if 0
static ULONG TD_Format(UBYTE *buffer, ULONG blocks, ULONG blocknr, globaldata *g)
{
	struct IOExtTD *request;
	ULONG realblocknr;

	DB(Trace(1, "TD_Format", "%ld blocks from %ld + %ld\n", blocks, blocknr,
			g->firstblock));

retry_format:
	if (blocknr == (ULONG)-1)  // blocknr of uninitialised anode
		return(1);

	realblocknr = blocknr + g->firstblock;
	if(!InPartition(realblocknr))
	{
		ErrorMsg (AFS_ERROR_WRITE_OUTSIDE, NULL, g);
		return ERROR_SEEK_ERROR;
	}

	request = g->request;
	request->iotd_Req.io_Command = TD_FORMAT;
	request->iotd_Req.io_Length  = blocks*BLOCKSIZE;
	request->iotd_Req.io_Data    = buffer;      // bufmemtype ??
	request->iotd_Req.io_Offset  = realblocknr*BLOCKSIZE;
	PROFILE_OFF();
	if(DoIO((struct IORequest*)request) != NULL)
	{
		ULONG args[2];
		PROFILE_ON();
		args[0] = request->iotd_Req.io_Error;
		args[1] = blocknr;      /* should be realblocknr?? */
		while ((g->ErrorMsg)(AFS_ERROR_WRITE_ERROR, args, 2, g))
		{
			if (CheckCurrentVolumeBack(g))
				goto retry_format;
		}
		if (!g->softprotect)
		{
			g->softprotect = 1;
			g->protectkey = ~0;
		}
		if (g->currentvolume)
			g->currentvolume->numsofterrors++;
		DB(Trace(1,"TD_Format","writeerror nr %d\n", args[0], args[1]));
		return ERROR_NOT_A_DOS_DISK;
	} 
	PROFILE_ON();
	return NULL;
}

#endif /* TRACKDISK */


static ULONG RawWrite_TD(UBYTE *buffer, ULONG blocks, ULONG blocknr, globaldata *g)
{
	struct IOExtTD *request;
	ULONG realblocknr;
	ULONG io_length, io_transfer, io_offset, io_actual = 0, io_startblock = 0;
	UBYTE *io_buffer;

	DB(Trace(1, "RawWrite", "%ld blocks from %ld + %ld\n", blocks, blocknr,
			g->firstblock));

retry_write:
	if(blocknr == (ULONG)-1)   // blocknr of uninitialised anode
		return 1;

	if (g->softprotect)
		return ERROR_DISK_WRITE_PROTECTED;

	realblocknr = blocknr + g->firstblock;
	if (!(InPartition(realblocknr) && InPartition(realblocknr+blocks-1)))
	{
		ErrorMsg (AFS_ERROR_WRITE_OUTSIDE, NULL, g);
		return ERROR_SEEK_ERROR;
	}

	io_length = blocks << BLOCKSHIFT;
	io_offset = realblocknr << BLOCKSHIFT;
	io_buffer = buffer;
	if (g->tdmode >= ACCESS_TD64) {
		// upper 32 bit of offset
		io_actual = realblocknr >> (32 - BLOCKSHIFT);
		io_startblock = realblocknr;
	}

	while(io_length > 0)
	{
		io_transfer = min(io_length, g->maxtransfer);
		io_transfer &= ~(BLOCKSIZE-1);
		request = g->request;
		request->iotd_Req.io_Command = CMD_WRITE;
		request->iotd_Req.io_Length  = io_transfer;
		request->iotd_Req.io_Data    = io_buffer;       // bufmemtype ??
		request->iotd_Req.io_Offset  = io_offset;
		if (g->tdmode >= ACCESS_TD64) {
			request->iotd_Req.io_Actual = io_actual;
			request->iotd_Req.io_Command = g->tdmode == ACCESS_NSD ? NSCMD_TD_WRITE64 : TD_WRITE64;
		}

		PROFILE_OFF();
		if (DoIO((struct IORequest*)request) != 0)
		{
			ULONG args[2];
			PROFILE_ON();
			args[0] = request->iotd_Req.io_Error;
			args[1] = blocknr;      /* should be realblocknr?? */
			while ((g->ErrorMsg)(AFS_ERROR_WRITE_ERROR, args, 2, g))
			{
				if (CheckCurrentVolumeBack(g))
					goto retry_write;
			}
			if (!g->softprotect)
			{
				g->softprotect = 1;
				g->protectkey = ~0;
			}
			if (g->currentvolume)
				g->currentvolume->numsofterrors++;
			DB(Trace(1,"RawWrite","writeerror nr %d\n", args[0], args[1]));
			return ERROR_NOT_A_DOS_DISK;
		}
		PROFILE_ON();
		io_buffer += io_transfer;
		io_length -= io_transfer;
		if (g->tdmode >= ACCESS_TD64) {
			io_startblock += (io_transfer >> BLOCKSHIFT);
			io_offset = io_startblock << BLOCKSHIFT;
			io_actual = io_startblock >> (32-BLOCKSHIFT);
		} else {
			io_offset += io_transfer;
		}
	}

	return 0;
}

#endif /* TD64 */

ULONG RawRead2(UBYTE *buffer, ULONG blocks, ULONG blocknr, globaldata *g)
{
#if (TRACKDISK || TD64 || NSD) && SCSIDIRECT
	if (g->tdmode == ACCESS_DS)
		return RawRead_DS(buffer, blocks, blocknr, g);
	else
		return RawRead_TD(buffer, blocks, blocknr, g);
#elif SCSIDIRECT
	return RawRead_DS(buffer, blocks, blocknr, g);
#else
	return RawRead_TD(buffer, blocks, blocknr, g);
#endif
}

ULONG RawRead(UBYTE *buffer, ULONG blocks, ULONG blocknr, globaldata *g)
{
	ULONG err = RawRead2(buffer, blocks, blocknr, g);
	if (err) {
		ULONG args[4] = { (ULONG)buffer, blocks, blocknr, err };
		ErrorMsg ("RawRead(%lX, %ld, %ld) failed with error %ld", args, g);
	}
	return err;
}

ULONG RawWrite2(UBYTE *buffer, ULONG blocks, ULONG blocknr, globaldata *g)
{
#if (TRACKDISK || TD64 || NSD) && SCSIDIRECT
	if (g->tdmode == ACCESS_DS)
		return RawWrite_DS(buffer, blocks, blocknr, g);
	else
		return RawWrite_TD(buffer, blocks, blocknr, g);
#elif SCSIDIRECT
	return RawWrite_DS(buffer, blocks, blocknr, g);
#else
	return RawWrite_TD(buffer, blocks, blocknr, g);
#endif
}

ULONG RawWrite(UBYTE *buffer, ULONG blocks, ULONG blocknr, globaldata *g)
{
	ULONG err = RawWrite2(buffer, blocks, blocknr, g);
	if (err) {
		ULONG args[4] = { (ULONG)buffer, blocks, blocknr, err };
		ErrorMsg ("RawWrite(%lX, %ld, %ld) failed with error %ld", args, g);
	}
	return err;
}

#if ACCESS_DETECT

#if DETECTDEBUG
static UBYTE ACCESS_DEBUG1[] = "%s:%ld\nfirstblock=%ld\nlastblock=%ld\nblockshift=%ld\nblocksize=%ld\ninside4G=%ld";
static UBYTE ACCESS_DEBUG2[] = "Test %ld = %ld";
static UBYTE ACCESS_DEBUG3[] = "SCSI Read Capacity = %ld, Lastblock = %ld";
#endif

static void fillbuffer(UBYTE *buffer, UBYTE data, globaldata *g)
{
	memset (buffer, data + 1, BLOCKSIZE);
}
/* Check if at least one byte has changed */
static BOOL testbuffer(UBYTE *buffer, UBYTE data, globaldata *g)
{
	ULONG cnt;
	
	for (cnt = 0; cnt < BLOCKSIZE; cnt++) {
		if (buffer[cnt] != data + 1)
			return TRUE;
	}
	return FALSE;
}

#if SCSIDIRECT
static BOOL testread_ds2(UBYTE *buffer, globaldata *g)
{
	UBYTE cmdbuf[10];
	UBYTE cnt;

#if DETECTDEBUG
	DebugPutStr("testread_ds\n");
#endif

	for (cnt = 0; cnt < 2; cnt++) {
		ULONG capacity;

		fillbuffer(buffer, 0xfe, g);
		/* Read Capacity */
		*((UWORD *)&cmdbuf[0]) = 0x2500;
		*((ULONG *)&cmdbuf[2]) = 0;
		*((ULONG *)&cmdbuf[6]) = 0;
		if (!DoSCSICommand(buffer, 8, cmdbuf, 10, SCSIF_READ, g)) {
#if DETECTDEBUG
			DebugPutStr("DoSCSICommand Read Capacity failed\n");
#endif
			return FALSE;
		}
		capacity = *((ULONG*)buffer);

#if DETECTDEBUG
		ULONG args[2];
		args[0] = capacity;
		args[1] = g->lastblock;
		g->ErrorMsg = _NormalErrorMsg;
		(g->ErrorMsg)(ACCESS_DEBUG3, args, 1, g);
#endif

		if (g->lastblock > capacity) {
#if DETECTDEBUG
			DebugPutStr("DoSCSICommand capacity smaller than last block\n");
#endif
			return FALSE;
		}
		fillbuffer(buffer, cnt, g);
		/* Read(10) */
		*((UWORD *)&cmdbuf[0]) = 0x2800;
		*((ULONG *)&cmdbuf[2]) = g->lastblock;
		*((ULONG *)&cmdbuf[6]) = 1 << 8;
		if (!DoSCSICommand(buffer, 1 << BLOCKSHIFT, cmdbuf, 10, SCSIF_READ, g)) {
#if DETECTDEBUG
			DebugPutStr("DoSCSICommand Read(10) failed\n");
#endif
			return FALSE;
		}
		if (testbuffer(buffer, cnt, g)) {
#ifdef DETECTDEBUG
			DebugPutStr("ok\n");
#endif
			return TRUE;
		}
	}
#if DETECTDEBUG
	DebugPutStr("testbuffer fail\n");
#endif
	return FALSE;
}
#endif

static BOOL testread_ds(UBYTE *buffer, globaldata *g)
{
	BOOL ok;
	
	ok = testread_ds2(buffer, g);
#if DETECTDEBUG
	ULONG args[2];
	args[0] = g->tdmode;
	args[1] = ok;
	g->ErrorMsg = _NormalErrorMsg;
	(g->ErrorMsg)(ACCESS_DEBUG2, args, 1, g);
#endif
	return ok;
}

static BOOL testread_td2(UBYTE *buffer, globaldata *g)
{
	struct IOExtTD *io = g->request;
	UBYTE cnt;

#if NSD
	if (g->tdmode == ACCESS_NSD) {
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
#endif
#if TD64
	if (g->tdmode == ACCESS_TD64) {
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
#endif
	for (cnt = 0; cnt < 2; cnt++) {
		fillbuffer(buffer, cnt, g);
		io->iotd_Req.io_Command = CMD_READ;
		io->iotd_Req.io_Length  = BLOCKSIZE;
		io->iotd_Req.io_Data    = buffer;
		io->iotd_Req.io_Offset  = g->lastblock << BLOCKSHIFT;
		if (g->tdmode >= ACCESS_TD64) {
			io->iotd_Req.io_Actual  = g->lastblock >> (32 - BLOCKSHIFT);
			io->iotd_Req.io_Command = g->tdmode == ACCESS_NSD ? NSCMD_TD_READ64 : TD_READ64;
		}
		if (DoIO((struct IORequest*)io) != 0)
			return FALSE;
		if (testbuffer(buffer, cnt, g))
			return TRUE;
	}
	return TRUE;
}

static BOOL testread_td(UBYTE *buffer, globaldata *g)
{
	BOOL ok;

#if DETECTDEBUG
	DebugPutHex("testread_td mode=", g->tdmode);
#endif
	ok = testread_td2(buffer, g);

#if DETECTDEBUG
	ULONG args[3];
	args[0] = g->tdmode;
	args[1] = ok;
	g->ErrorMsg = _NormalErrorMsg;
	(g->ErrorMsg)(ACCESS_DEBUG2, args, 1, g);
#endif

#if DETECTDEBUG
	DebugPutHex("testread_td ret=", ok);
#endif
	return ok;
}
	
BOOL detectaccessmode(UBYTE *buffer, globaldata *g)
{
	UBYTE name[FNSIZE];
	BOOL inside4G = g->lastblock < (0x80000000ul >> (BLOCKSHIFT - 1));

	BCPLtoCString(name, (UBYTE *)BADDR(g->startup->fssm_Device));

#if DETECTDEBUG
	ULONG args[8];
	args[0] = (ULONG)name;
	args[1] = g->startup->fssm_Unit;
	args[2] = g->firstblock;
	args[3] = g->lastblock;
	args[4] = BLOCKSHIFT;
	args[5] = BLOCKSIZE;
	args[6] = inside4G;
	g->ErrorMsg = _NormalErrorMsg;
	(g->ErrorMsg)(ACCESS_DEBUG1, args, 1, g);

	DebugPutHex("firstblock", g->firstblock);
	DebugPutHex("lastblock", g->lastblock);
	DebugPutHex("inside4G", inside4G);
	DebugPutHex("maxtransfer", g->maxtransfer);
#endif

#if SCSIDIRECT
	/* if dostype = PDSx, test Direct SCSI first and always use it if test succeeded */
	if ((g->dosenvec->de_DosType & 0xffffff00) == 0x50445300) {
		g->tdmode = ACCESS_DS;
		if (testread_ds(buffer, g))
			return TRUE;
	}
#endif

	if (g->lastblock < 262144) {
		/* Don't bother with tests if small enough (<128M) */
		g->tdmode = ACCESS_STD;
		return TRUE;
	}

	if (inside4G) {
		ULONG args[3];
		/* inside first 4G? Try standard CMD_READ first. */
		g->tdmode = ACCESS_STD;
		if (testread_td(buffer, g))
			return TRUE;
#if SCSIDIRECT
		g->tdmode = ACCESS_DS;
		/* It failed, we may have 1G limit A590 pre-7.0 ROM or CDTV SCSI, try DS */
		if (testread_ds(buffer, g))
			return TRUE;
#endif
		g->tdmode = ACCESS_STD;
		/* Both failed. Panic! */
		args[0] = g->lastblock;
		args[1] = (ULONG)name;
		args[2] = g->startup->fssm_Unit;
		g->ErrorMsg = _NormalErrorMsg;
		(g->ErrorMsg)(AFS_ERROR_32BIT_ACCESS_ERROR, args, 1, g);
		return FALSE;
	}
	/* outside of first 4G, must use TD64, NSD or DS */
#if NSD
	g->tdmode = ACCESS_NSD;
	if (testread_td(buffer, g))
		return TRUE;
#endif
#if TD64
	g->tdmode = ACCESS_TD64;
	if (testread_td(buffer, g))
		return TRUE;
#endif
#if SCSIDIRECT
	g->tdmode = ACCESS_DS;
	if (testread_ds(buffer, g))
		return TRUE;
#endif

#if DETECTDEBUG
	args[0] = -1;
	args[1] = 0;
	g->ErrorMsg = _NormalErrorMsg;
	(g->ErrorMsg)(ACCESS_DEBUG2, args, 1, g);
#endif

	g->tdmode = ACCESS_STD;
	return FALSE;
}
#endif

/*************************************************************************/

/* turn drivemotor off */
void MotorOff(globaldata *g)
{
	struct IOExtTD *request = g->request;

	if(g->removable)
	{
		request->iotd_Req.io_Command = TD_MOTOR;
		request->iotd_Req.io_Length  = 0;
		request->iotd_Count = g->changecount;

		DoIO((struct IORequest*)request);
	}
}
