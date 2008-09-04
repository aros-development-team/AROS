/*
 *  Format -- disk formatting and file-system creation program
 *  Copyright (C) 1999 Ben Hutchings <womble@zzumbouk.demon.co.uk>
 *  Copyright (C) 2008 Pavel Fedin <sonic_amiga@rambler.ru>

 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <exec/devices.h>
#include <exec/errors.h>
#include <exec/io.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/resident.h>
#include <graphics/gfxbase.h>
#include <intuition/intuition.h>
#include <workbench/workbench.h>
#include <stdlib.h>
#include <string.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/icon.h>
#include <proto/intuition.h>

#include "format.h"
#include "locale.h"

#ifndef AROS_BSTR_ADDR
#define AROS_BSTR_ADDR(x) (char *)BADDR(x) + 1
#endif

static void WordWrapSz( char * psz );
static BOOL bTransferCylinder(
    APTR pBuffer, ULONG icyl, UWORD cmd64, UWORD cmd32 );

char szDosDevice[MAX_FS_NAME_LEN+2];
char * pchDosDeviceColon;
ULLONG ibyStart, ibyEnd;
IPTR MaxTransfer;
IPTR LowCyl, HighCyl;
ULONG DosType;

static char szVolume[MAX_FS_NAME_LEN+2] __attribute__((aligned (4)));
static char * pszExecDevice;
static ULONG ExecUnit, ExecDeviceFlags;
static ULONG BufMemType;
static ULONG fstCurrent;
static BOOL bFstSet;
static ULONG devfCurrent;
static BOOL bDevfSet;
static struct FileSysStartupMsg * pfssm;
static ULLONG cbyTrack, cbyCylinder;
static struct MsgPort * pmpDiskIO;
static struct IOStdReq * piosDisk;
static BOOL bExecDevOpen;
static BOOL bSuspectIDE = FALSE;
static BOOL bInhibited;
static ULONG * paulWriteBuffer, * paulReadBuffer;
static ULONG cbyTransfer;


const char szVersion[] = "$VER: BHFormat 43.7 (" __DATE__ ")";

int main(void)
{
    int rc = _WBenchMsg ? rcGuiMain() : rcCliMain();
    FreeAll();
    return rc;
}


void ReportErrSz( ErrorType ert, LONG err, const char * pszMessage, ... )
{
    static const char * apszErrorTypes[3];
    apszErrorTypes[0] = _(MSG_ERRORS_WARNING);
    apszErrorTypes[1] = _(MSG_ERRORS_ERROR);
    apszErrorTypes[2] = _(MSG_ERRORS_FAILURE);
    
    char szSysMessage[FAULT_MAX];
    char szFormat[256];
    static char szOutput[1024];
    va_list args;

    va_start(args, pszMessage);
    szSysMessage[0] = 0;
    if( err != -1 )
	(void) Fault( err ? err : IoErr(),
		      (_WBenchMsg && !pszMessage) ? 0 : "",
		      szSysMessage, FAULT_MAX );

    strcpy( szFormat, _(MSG_FORMAT) );
    strcat( szFormat, apszErrorTypes[ert] );

    if(!_WBenchMsg)
    {
	if(pszMessage)
	{
	    strcat( szFormat, " - " );
	    strcat( szFormat, pszMessage );
	}
	strcat( szFormat, szSysMessage );
	strcat( szFormat, "\n" );
	RawDoVFmtSz( szOutput, szFormat, args );
	WordWrapSz(szOutput);
	(void) FPuts( bpfhStdErr, szOutput );
    }
    else if(IntuitionBase)
    {
	struct EasyStruct es;

	char * pszBodyFormat = szFormat + strlen(szFormat) + 1;
	*pszBodyFormat = 0;
	if(pszMessage)
	    strcpy( pszBodyFormat, pszMessage );
	strcat( pszBodyFormat, szSysMessage );

	es.es_StructSize = sizeof(struct EasyStruct);
	es.es_Flags = 0;
	es.es_Title = szFormat;
	es.es_TextFormat = "%s";
	es.es_GadgetFormat = (ert == ertFailure) ? _(MSG_CANCEL) : _(MSG_OK);
	RawDoVFmtSz( szOutput, pszBodyFormat, args );
	WordWrapSz(szOutput);
	(void) EasyRequest( 0, &es, 0, (ULONG)szOutput );
    }
    /* else we can't report the error */
    va_end(args);
}

static void WordWrapSz( char * psz )
{
    char * pszLine, * pchSpace, * pchLastSpace;
    pszLine = psz;
    pchLastSpace = pszLine-1;
    while( (pchSpace = strchr( pchLastSpace+1, ' ' )) != 0 )
    {
	size_t cch = pchSpace - pszLine;

	if( cch >= 60 )
	{
	    if( cch == 60 || pchLastSpace < pszLine )
		pchLastSpace = pchSpace;

	    *pchLastSpace = '\n';
	    pszLine = pchLastSpace+1;
	}

	pchLastSpace = pchSpace;
    }
}


BOOL bSetSzDosDeviceFromSz( const char * pszDevice )
{
    /* Check the length of the device name - it may be followed by a
       colon, and possibly some junk which we must ignore */
    char * pszEnd = strchr( pszDevice, ':' );
    size_t cch = pszEnd ? (pszEnd - pszDevice) : strlen(pszDevice);

    if( cch != 0 && cch <= MAX_FS_NAME_LEN )
    {
	/* Make a copy, without the colon */
	strncpy( szDosDevice, pszDevice, cch );
	pchDosDeviceColon = &szDosDevice[cch]; /* need to put it back later */
	*pchDosDeviceColon = 0;
	*(pchDosDeviceColon+1) = 0;
	return TRUE;
    }

    ReportErrSz( ertError, ERROR_INVALID_COMPONENT_NAME, 0 );
    return FALSE;
}


BOOL bSetSzVolumeFromSz( const char * pszVolume )
{
    /* Check the length and validity of the volume name */
    size_t cch = strlen(pszVolume);

    if( cch != 0 && cch <= MAX_FS_NAME_LEN && strpbrk( pszVolume, ":/" ) == 0 )
    {
	/* Make a copy with a length prefix because v36 Format function
	   incorrectly expects a BSTR */
	szVolume[0] = cch;
	strcpy( &szVolume[1], pszVolume );
	return TRUE;
    }

    ReportErrSz( ertError, ERROR_INVALID_COMPONENT_NAME, 0 );
    return FALSE;
}


BOOL bSetFstFromSz( const char * pszFileSysType )
{
    char * pszEnd;
    ULONG ub;
    size_t cub;

    if( *pszFileSysType == 0 )
    {
	bFstSet = FALSE;
	return TRUE;
    }

    /* Look for a C-style number as used in Mountlists. */
    fstCurrent = strtoul( pszFileSysType, &pszEnd, 0 );
    if( *pszEnd == 0 )
    {
	bFstSet = TRUE;
	return TRUE;
    }

    /* Look for a type in the standard-ish format of text with
       \-escaped numbers. These numbers normally have only one
       digit, so it's a bit hard to decide what base they are
       in.  I have specified 8 here because that's what C uses
       for \-escaped character numbers. */
    cub = 0;
    fstCurrent = 0;
    while( (ub = (unsigned char)*pszFileSysType++) != 0 )
    {
	if( ub == (unsigned char)'\\' )
	{
	    ub = strtoul( pszFileSysType, (char **)&pszFileSysType, 8 );
	    if( ub >= 0x100 )
	    {
		cub = 0;
		break;
	    }
	}
	fstCurrent = (fstCurrent << 8) + ub;
	++cub;
    }
    if( cub == 4 )
    {
	bFstSet = TRUE;
	return TRUE;
    }

    ReportErrSz( ertError, -1, _(MSG_ERROR_INVALID_DOSTYPE) );
    return FALSE;
}


BOOL bSetDevfFromSz( const char * pszDevFlags )
{
    if( *pszDevFlags == 0 )
    {
	bDevfSet = FALSE;
	return TRUE;
    }

    devfCurrent = strtoul( pszDevFlags, (char **)&pszDevFlags, 0 );
    if( *pszDevFlags == 0 )
    {
	bDevfSet = TRUE;
	return TRUE;
    }

    ReportErrSz( ertError, ERROR_BAD_NUMBER, 0 );
    return FALSE;
}


BOOL bGetDosDevice(struct DosList *pdlDevice, ULONG flags)
{
    struct DosList *pdlList;
    struct DosEnvec * pdenDevice;

    if (!pdlDevice) {
	flags = LDF_DEVICES|LDF_READ;
	pdlList = LockDosList(flags);
	D(Printf( "LockDosList( LDF_DEVICES | LDF_READ ) = 0x%08lx\n", (ULONG)pdlList ));
	*pchDosDeviceColon = 0;
	pdlDevice = FindDosEntry( pdlList, szDosDevice, LDF_DEVICES );
	D(Printf("FindDosEntry( 0x%08lx, \"%s\", LDF_DEVICES ) = 0x%08lx\n",
		 (ULONG)pdlList, (ULONG)szDosDevice, (ULONG)pdlDevice ));
	if( pdlDevice == 0 )
	{
	    UnLockDosList(flags);
	    ReportErrSz( ertError, ERROR_DEVICE_NOT_MOUNTED, 0 );
	    return FALSE;
	}
    }

    /* Find startup message and verify file-system settings. Use
       TypeOfMem to protect against devices that use integer or string
       startup values. */
    if( (pfssm = (struct FileSysStartupMsg *)
	 BADDR(pdlDevice->dol_misc.dol_handler.dol_Startup)) == 0
	|| TypeOfMem(pfssm) == 0
	|| pfssm->fssm_Device == 0
	|| (pdenDevice = (struct DosEnvec *)BADDR(pfssm->fssm_Environ)) == 0
	|| TypeOfMem(pdenDevice) == 0
	|| pdenDevice->de_TableSize < DE_DOSTYPE
	/* Check that parameters that should always be 0, are */
	|| pdenDevice->de_SecOrg != 0
	|| pdenDevice->de_Interleave != 0 )
    {
        UnLockDosList(flags);
	ReportErrSz( ertError, ERROR_OBJECT_WRONG_TYPE, 0 );
	return FALSE;
    }

    /* Get the device name with the original correct case */
    RawDoFmtSz( szDosDevice, "%b", pdlDevice->dol_Name );

    /* Unlike most BCPL strings, this one is guaranteed to be null-
       terminated. */
    pszExecDevice = AROS_BSTR_ADDR(pfssm->fssm_Device);
    ExecUnit = pfssm->fssm_Unit;
    ExecDeviceFlags = pfssm->fssm_Flags;
    MaxTransfer = pdenDevice->de_MaxTransfer;
    BufMemType = pdenDevice->de_BufMemType;
    LowCyl = pdenDevice->de_LowCyl;
    HighCyl = pdenDevice->de_HighCyl;
    DosType = pdenDevice->de_DosType;

    cbyTrack = (ULLONG)pdenDevice->de_BlocksPerTrack * (ULLONG)(pdenDevice->de_SizeBlock * sizeof(LONG));
    cbyCylinder = cbyTrack * pdenDevice->de_Surfaces;

    ibyStart = pdenDevice->de_LowCyl * cbyCylinder;
    ibyEnd = (pdenDevice->de_HighCyl + 1) * cbyCylinder;

#ifdef __mc68000
    /* If the device has a native Amiga file-system, we can check for
       various limitations and also apply the various command-line
       flags that specify which variant to use for the new volume. */
       
    if( pdenDevice->de_DosType >= 0x444F5300
	&& pdenDevice->de_DosType <= 0x444F5305 )
    {
	const struct Resident * prt;
	UWORD verFS;

	/* I'd prefer to do this properly by reading the segment size
	   and going through the segment list, but the fake ROM
	   "segment lists" don't have valid segment sizes set so that
	   would be a bit pointless. Perhaps I should use TypeOfMem
	   to decide whether the segment is in ROM or RAM first? */

	prt = (struct Resident *)((char *)BADDR(
	    pdlDevice->dol_misc.dol_handler.dol_SegList) + 4);
	while( prt->rt_MatchWord != RTC_MATCHWORD
	       || prt->rt_MatchTag != prt )
	    prt = (struct Resident *)((UWORD *)prt + 1);
	verFS = prt->rt_Version;
	D(Printf( "found RomTag at 0x%08lx; rt_Version = %lu\n",
		  (ULONG)prt, (ULONG)verFS ));
	    
	/* check that the fs can handle this device correctly */

	if( ibyEnd - ibyStart > 0x100000000ULL
	    && verFS <= 43 ) /* perhaps v44 will change this? ;-) */
	{
            UnLockDosList(flags);
	    ReportErrSz(
		ertError,
		-1,
		_(MSG_ERROR_TOO_LARGE_1) );
	    return FALSE;
	}
	if( ibyEnd > 0x100000000ULL && verFS < 43 )
	{
            UnLockDosList(flags);
	    ReportErrSz(
		ertError,
		-1,
		_(MSG_ERROR_TOO_LARGE_2) );
	    return FALSE;
	}

	if( ibyEnd - ibyStart > 0x80000000ULL && verFS < 40 )
	{
            UnLockDosList( LDF_DEVICES | LDF_READ );
	    ReportErrSz(
		ertError,
		-1,
		_(MSG_ERROR_TOO_LARGE_3) );
	    return FALSE;
	}
    }
#endif

    UnLockDosList(flags);

#ifndef __MORPHOS__
    /* Certain documentation says MEMF_PUBLIC doesn't matter, and
       certain progams don't bother setting it by default.  They
       are wrong and should be fixed!
       Unfortunately MorphOS mounts builtin filesystems without
       this flag and this can't be changed. So we disable this check
       under MorphOS */
    if( !(BufMemType & MEMF_PUBLIC) )
    {
	ReportErrSz(
	    ertWarning,
	    -1,
	    _(MSG_ERROR_TOO_LARGE_4) );
	BufMemType |= MEMF_PUBLIC;
    }
#endif

    return TRUE;
}


void FreeDosDevice(void)
{
    if(bInhibited)
    {
	*pchDosDeviceColon = ':';
	D(Printf( "Inhibit( \"%s\", DOSFALSE );\n", (ULONG)szDosDevice ));
	(void) Inhibit( szDosDevice, DOSFALSE );
	bInhibited = FALSE;
    }
}


BOOL bGetExecDevice( BOOL bWillVerify )
{
    *pchDosDeviceColon = ':';
    D(Printf( "Inhibit( \"%s\", DOSTRUE );\n", (ULONG)szDosDevice ));
    if(!Inhibit( szDosDevice, DOSTRUE ))
    {
	/* This is a bit stupid, but compatible with v40 Format */
	ReportErrSz( ertFailure, ERROR_OBJECT_WRONG_TYPE, 0 );
	return FALSE;
    }
    bInhibited = TRUE;

    if( (pmpDiskIO = CreateMsgPort()) == 0
	|| (piosDisk = (struct IOStdReq *)
	    CreateIORequest( pmpDiskIO, sizeof(struct IOStdReq) )) == 0 )
    {
	D(Printf("pmpDiskIO = 0x%08lX, piosDisk = 0x%08lX\n", pmpDiskIO, piosDisk));
	ReportErrSz( ertFailure, ERROR_NO_FREE_STORE, 0 );
	return FALSE;
    }

    {
	BYTE derr = OpenDevice( pszExecDevice,
				ExecUnit,
				(struct IORequest *)piosDisk,
				bDevfSet ? devfCurrent : ExecDeviceFlags );
	if( derr != 0 )
	{
	    ReportErrSz(
		ertFailure, -1, _(MSG_ERROR_DEVICE), (ULONG)derr );
	    return FALSE;
	}
	bExecDevOpen = TRUE;
    }

#ifdef __mc68000
    {
	/* This is an attempt to spot a scsi.device that is really an
	   IDE driver. If we have a card slot (A600/A1200) or AGA
	   (A1200/A4000/A4000T/CD32/clones) then it probably is.  If
	   the version number is lower than 40 then it does not limit
	   transfers to 128 blocks as required for correct operation
	   of some drives. */
	struct GfxBase * GfxBase = (struct GfxBase *)
	    OpenLibrary( "graphics.library", 39 );
	if( !strcmp( pszExecDevice, "scsi.device" )
	    && piosDisk->io_Device->dd_Library.lib_Version < 40
	    && (OpenResource("card.resource")
		|| (GfxBase && (GfxBase->ChipRevBits0 & GFXF_AA_ALICE)) ))
	{
	    bSuspectIDE = TRUE;
	    if( MaxTransfer > 0x10000 )
	    {
		MaxTransfer = 0x10000;
		ReportErrSz(
		    ertWarning,
		    -1,
		    _(MSG_ERROR_IDE) );
	    }
	}
	if(GfxBase)
	    CloseLibrary((struct Library *)GfxBase);
    }
#endif

    if( ibyEnd > 0x100000000ULL )
    {
	static struct NSDeviceQueryResult nsdqr; /* must be in public memory */
	BYTE derr;
	BOOL bDoes64Bit = FALSE;

	piosDisk->io_Command	= NSCMD_DEVICEQUERY;
	piosDisk->io_Data	= &nsdqr;
	piosDisk->io_Length	= sizeof(nsdqr);
	nsdqr.DevQueryFormat	= 0;
	nsdqr.SizeAvailable 	= 0;

	switch( derr = DoIO((struct IORequest *)piosDisk) )
	{
	case 0:
	    if( piosDisk->io_Actual == sizeof(nsdqr) )
	    {
		if( nsdqr.DeviceType == NSDEVTYPE_TRACKDISK )
		{
		    /* Look for 64-bit trackdisk commands - any one will
		       do, as drivers must implement all or none of them. */
		    UWORD * pcmd;
		    for( pcmd = nsdqr.SupportedCommands; *pcmd != 0; ++pcmd )
			if( *pcmd == NSCMD_TD_READ64 )
			    bDoes64Bit = TRUE;
		}
		else
		{
		    /* How odd... it's not trackdisk-like */
		    ReportErrSz( ertError, ERROR_OBJECT_WRONG_TYPE, 0 );
		    return FALSE;
		}
	    }
	    break;

	case IOERR_NOCMD:
	    break;

	default:
	    ReportErrSz(
		ertFailure,
		-1,
		_(MSG_ERROR_DEVICE_QUERY),
		(ULONG)derr );
	    return FALSE;
	}

	if(!bDoes64Bit)
	{
	    ReportErrSz(
		ertError,
		-1,
		_(MSG_ERROR_64BIT) );
	    return FALSE;
	}
    }

    /* We need two buffers - one for writing and one for reading back to
       verify the write */
    D(Printf("Allocating buffers, size %lu bytes\n", cbyCylinder));
    if( (paulWriteBuffer = AllocMem( cbyCylinder,
				     BufMemType )) == 0
	|| (bWillVerify
	    && (paulReadBuffer = AllocMem( cbyCylinder, BufMemType )) == 0) )
    {
	ReportErrSz( ertFailure, ERROR_NO_FREE_STORE, 0 );
	return FALSE;
    }

    {
	/* Fill a buffer with junk, just like the official Format
	   does.  Perhaps this is meant to make the verification a
	   harder test. */
	ULONG * pulFill = paulWriteBuffer;
	ULONG * pulLimit = (ULONG *)((char *)paulWriteBuffer + cbyCylinder);
	while( pulFill < pulLimit )
	{
	    int i, j;
	    for( i = 0; i < 16 && pulFill < pulLimit; ++i )
	    {
		ULONG ul = 0x444F5300 + ((i >> 2) << 10);
		for( j = 0; j < 256 && pulFill < pulLimit; ++j )
		    *pulFill++ = ul++;
	    }
	}
    }

    /* For most drives, the geometry is totally fake and the drive can
       never be formatted by the user.  The driver will really treat
       TD_FORMAT the same as CMD_READ.  However, for some drives, we
       will be doing a real format using the real geometry.  In that
       case we *must* write a whole track at a time.  So what if
       MaxTransfer is smaller than one track?

       * MaxTransfer happens to be useful for working round firmware
         bugs in some IDE drives which scsi.device versions < 40
         didn't work around, but that's not its purpose.  We've
         already spotted problematic drivers and we know that they
         don't really format the disk, so we should apply the limit
	 in that case.

       * As far as I can see, MaxTransfer is really there to limit the
         time that the I/O bus may be tied up with DMA transfers.  If
	 we have to write more than that to format the disk, so be it.

       Ideally, we want to write whole cylinders at once, since longer
       transfers are fastest.  We do this if the length of a cylinder
       is less than or equal to MaxTransfer.
       Otherwise, we write a track each time, unless we have a suspect
       IDE drive and the length of a track is greater than MaxTransfer
       - in which case we write at most MaxTransfer each time.  */

    cbyTransfer = (cbyCylinder <= MaxTransfer) ?
	cbyCylinder :
	((!bSuspectIDE || cbyTrack <= MaxTransfer) ?
	 cbyTrack : MaxTransfer);

    return TRUE;
}


void FreeExecDevice(void)
{
    if(paulWriteBuffer)
    {
	FreeMem( paulWriteBuffer, cbyCylinder );
	paulWriteBuffer = 0;
    }
    if(paulReadBuffer)
    {
	FreeMem( paulReadBuffer, cbyCylinder );
	paulReadBuffer = 0;
    }
    if(bExecDevOpen)
    {
	CloseDevice((struct IORequest *)piosDisk);
	bExecDevOpen = FALSE;
    }
    if(piosDisk)
    {
	DeleteIORequest((struct IORequest *)piosDisk);
	piosDisk = 0;
    }
    if(pmpDiskIO)
    {
	DeleteMsgPort(pmpDiskIO);
	pmpDiskIO = 0;
    }
}


BOOL bFormatCylinder( ULONG icyl )
{
    BYTE derr;

    /* put a "BAD\0" marker at the beginning in case format is aborted */
    /* put normal "DOS\0" marker at the beginning of other cylinders */
    paulWriteBuffer[0] =
	(icyl == LowCyl) ? 0x42414400 : 0x444F5300;

    if( !bTransferCylinder( paulWriteBuffer, icyl,
			    NSCMD_TD_FORMAT64, TD_FORMAT ) )
	return FALSE;

    /* Write out and invalidate the track buffer */
    piosDisk->io_Command = CMD_UPDATE;
    derr = DoIO((struct IORequest *)piosDisk);
    if( derr == 0 )
    {
	piosDisk->io_Command = CMD_CLEAR;
	derr = DoIO((struct IORequest *)piosDisk);
	if( derr == 0 )
	    return TRUE;
    }

    ReportErrSz( ertFailure, -1,
		 _(MSG_ERROR_DEVICE_RETURN),
		 (piosDisk->io_Command == CMD_UPDATE) ?
		 _(MSG_ERROR_FLUSH) : _(MSG_ERROR_INVALIDATE),
		 (ULONG)derr );
    return FALSE;
}


BOOL bVerifyCylinder( ULONG icyl )
{
    if( !bTransferCylinder( paulReadBuffer, icyl,
			    NSCMD_TD_READ64, CMD_READ ) )
	return FALSE;

    if( memcmp( paulWriteBuffer, paulReadBuffer, cbyCylinder ) != 0 )
    {
	ReportErrSz( ertFailure, -1, _(MSG_ERROR_VERIFY) );
	return FALSE;
    }

    return TRUE;
}


static BOOL bTransferCylinder(
    APTR pBuffer, ULONG icyl, UWORD cmd64, UWORD cmd32 )
{
    ULLONG ibyOffset = (ULLONG)icyl * (ULLONG)cbyCylinder;
    ULLONG cbyLeft = cbyCylinder;

    do
    {
	ULONG cbyLength = (cbyTransfer <= cbyLeft) ? cbyTransfer : cbyLeft;

	if( ibyOffset + cbyLength >= 0x100000000ULL )
	{
	    piosDisk->io_Command = cmd64;
	    piosDisk->io_Actual  = ibyOffset >> 32;
	}
	else
	    piosDisk->io_Command = cmd32;

	piosDisk->io_Offset      = ibyOffset;
	piosDisk->io_Length      = cbyLength;
	piosDisk->io_Data        = pBuffer;

	D(Printf( "DoIO() Cmd=%2lu Act=0x%08lx Len=0x%08lx "
		  "Data=0x%08lx Off=0x%08lx\n",
		  piosDisk->io_Command,
		  piosDisk->io_Actual,
		  piosDisk->io_Length,
		  (ULONG)piosDisk->io_Data,
		  piosDisk->io_Offset ));

	if( DoIO((struct IORequest *)piosDisk) != 0 )
	{
	    const char * pszCommand;
	    switch(piosDisk->io_Command)
	    {
	    case NSCMD_TD_FORMAT64:
		pszCommand = _(MSG_COMMAND_FORMAT64); break;
	    case TD_FORMAT:
		pszCommand = _(MSG_COMMAND_FORMAT); break;
	    case NSCMD_TD_READ64:
		pszCommand = _(MSG_COMMAND_READ64); break;
	    case CMD_READ:
		pszCommand = _(MSG_COMMAND_READ); break;
	    }
	    ReportErrSz( ertFailure, -1,
			 _(MSG_ERROR_DEVICE_RETURN),
			 pszCommand, (ULONG)piosDisk->io_Error );
	    return FALSE;
	}

	ibyOffset += cbyLength;
	pBuffer    = (char *)pBuffer + cbyLength;
	cbyLeft   -= cbyLength;
    }
    while( cbyLeft != 0 );

    return TRUE;
}


BOOL bMakeFileSys( BOOL bFFS, BOOL bOFS, BOOL bIntl, BOOL bNoIntl,
		   BOOL bDirCache, BOOL bNoDirCache )
{
    if(!bFstSet)
    {
	fstCurrent = DosType;

	if( fstCurrent >= 0x444F5300 && fstCurrent <= 0x444F5305 )
	{
	    /* Adjust the file-system type according to command-line
	       switches or check-boxes (this exactly matches the logic of the
	       official version 40 Format command). */
	
	    if(bFFS)		fstCurrent |= 1;
	    if(bOFS)		fstCurrent &= ~1;
	    if( !(fstCurrent & 2) )
	    {
		if(bDirCache)	fstCurrent |= 4;
		if(bNoDirCache)	fstCurrent &= ~4;
	    }
	    if( !(fstCurrent & 4) )
	    {
		if(bIntl)	fstCurrent |= 2;
		if(bNoIntl)	fstCurrent &= ~2;
	    }
	} /* if( fstCurrent >= 0x444F5300 && fstCurrent <= 0x444F5305 ) */
    } /* if(!bFstSet) */

    if(!bInhibited)
    {
	*pchDosDeviceColon = ':';
	D(Printf( "Inhibit( \"%s\", DOSTRUE );\n", (ULONG)szDosDevice ));
	if(!Inhibit( szDosDevice, DOSTRUE ))
	{
	    /* This is a bit stupid, but compatible with v40 Format */
	    ReportErrSz( ertFailure, ERROR_OBJECT_WRONG_TYPE, 0 );
	    return FALSE;
	}
	bInhibited = TRUE;
    }

    D(Printf( "Format( \"%s\", \"%s\", 0x%08lx );\n",
	      (ULONG)szDosDevice, (ULONG)(szVolume + 1), fstCurrent ));
    if( !Format( szDosDevice,
		 (DOSBase->dl_lib.lib_Version == 36) ?
		 (char *)MKBADDR(szVolume) : szVolume + 1,
		 fstCurrent ) )
    {
	ReportErrSz( ertFailure, 0, 0 );
	return FALSE;
    }

    return TRUE;
}


BOOL bMakeFiles( BOOL bDiskIcon )
{
    struct Library * IconBase;
    BOOL bSuccess = FALSE;

    /* We have to unlock the DOS list and uninhibit the device before we
       can write files to the new volume! */
    FreeDosDevice();

    if( (IconBase = OpenLibrary( "icon.library", 36 )) != 0 )
    {
	BPTR bpflRoot;
	*pchDosDeviceColon = ':';
	if( (bpflRoot = Lock( szDosDevice, SHARED_LOCK )) != 0 )
	{
	    BPTR bpflOldCD = CurrentDir(bpflRoot);
	    BPTR bpflTrash = CreateDir("Trashcan");

	    if(bpflTrash)
	    {
		struct DiskObject * pdo;
		UnLock(bpflTrash);
		pdo = GetDefDiskObject(WBGARBAGE);

		if(pdo)
		{
		    if( PutDiskObject( "Trashcan", pdo ) )
			bSuccess = TRUE;
		    FreeDiskObject(pdo);
		}
	    }

	    if(!bSuccess)
		ReportErrSz( ertFailure,
			     -1,
			     _(MSG_ERROR_TRASHCAN),
			     (ULONG)(szVolume + 1) );
	    else if( bDiskIcon )
	    {
		struct DiskObject * pdo;
		char szDefDiskIcon[12+MAX_FS_NAME_LEN+4+1];

		bSuccess = FALSE; /* always assume the worst */

		/* Try to get the icon that DefIcons would use -
		   first, look for a device-specific one; if that
		   fails look for a file-system-specific one; if that
		   fails then get the standard default disk icon. */
		*pchDosDeviceColon = 0;
		RawDoFmtSz( szDefDiskIcon, "ENV:sys/def_%sdisk", szDosDevice );
		if( (pdo = GetDiskObject(szDefDiskIcon)) == 0 )
		{
		    int i;
		    strcpy( szDefDiskIcon, "ENV:sys/def_    disk" );
		    for( i = 0; i < 4; ++i )
		    {
			char chFSType = ((char *)&fstCurrent)[i];
			szDefDiskIcon[12+i] =
			    (chFSType < 10) ? ('0' + chFSType) : chFSType;
		    }
		    if( (pdo = GetDiskObject(szDefDiskIcon)) == 0 )
			pdo = GetDefDiskObject(WBDISK);
		}
		if(pdo)
		{
		    if( PutDiskObject( "Disk", pdo ) )
			bSuccess = TRUE;
		    FreeDiskObject(pdo);
		}

		if(!bSuccess)
		    ReportErrSz( ertFailure,
				 -1,
				 _(MSG_ERROR_ICON),
				 (ULONG)(szVolume + 1) );
	    }

	    CurrentDir(bpflOldCD);
	    UnLock(bpflRoot);
	}
	CloseLibrary(IconBase);
    }
    else /* IconBase == 0 */
	ReportErrSz( ertFailure, ERROR_INVALID_RESIDENT_LIBRARY, 0 );

    return bSuccess;
}


void FreeAll(void)
{
    FreeExecDevice();
    FreeDosDevice();
}

#ifndef HAVE_NEWRAWDOFMT
#ifdef __mc68000
static const UWORD AddChSz[] = {0x16C0, 0x4E75}; /* move.l d0,(a3)+ : rts */

void RawDoFmtSz( char * pszBuffer, const char * pszFormat, ... )
{
    RawDoFmt( (char *)pszFormat, (APTR)(&pszFormat+1),
	      (void (*)())AddChSz, pszBuffer );
}

void RawDoVFmtSz( char * pszBuffer, const char * pszFormat, APTR pData )
{
    RawDoFmt( (char *)pszFormat, pData, (void (*)())AddChSz, pszBuffer );
}
#else
#error CPU is not supported
#endif
#endif
