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


#ifndef __GNUC__
#error "You need to use GNU CC to compile this."
#endif

#ifdef __MORPHOS__
#define HAVE_NEWRAWDOFMT
#endif
#ifdef __AROS__
#define HAVE_NEWRAWDOFMT
#endif

#include <dos/dos.h>
#include <exec/types.h>
#ifdef HAVE_NEWRAWDOFMT
#include <exec/rawfmt.h>
#endif

#include <stdarg.h>

#ifdef __AROS__
#include <aros/debug.h>
#define _WBenchMsg WBenchMsg
#else
#include <clib/debug_protos.h>
#define bug kprintf
#ifdef DEBUG
#define D(x) (x)
#else
#define D(x)
#endif
#endif


/* Maximum length of a name component - I can't find this defined anywhere */
#define MAX_FS_NAME_LEN 30

/* Short name for an unsigned 64-bit integer type */
typedef unsigned long long ULLONG;

typedef enum {
    ertWarning,	/* Just a warning of some odd condition */
    ertError,	/* Arguments were invalid or system configuration is wierd */
    ertFailure,	/* Something failed, and we can't go on */
} ErrorType;

extern struct WBStartup * _WBenchMsg;
extern BPTR bpfhStdErr;
extern char szDosDevice[MAX_FS_NAME_LEN+2];
extern char * pchDosDeviceColon;
extern ULLONG ibyStart, ibyEnd;
extern IPTR MaxTransfer;
extern IPTR LowCyl, HighCyl;
extern ULONG DosType;

void ReportErrSz( ErrorType ert, LONG err, const char * pszMessage, ... );
BOOL bSetSzDosDeviceFromSz( const char * pszDevice );
BOOL bSetSzVolumeFromSz( const char * pszVolume );
BOOL bSetFstFromSz( const char * pszFileSysType );
BOOL bSetDevfFromSz( const char * pszDevFlags );
BOOL bGetDosDevice(struct DosList *pdlList, ULONG flags);
void FreeDosDevice(void);
BOOL bGetExecDevice( BOOL bWillVerify );
void FreeExecDevice(void);
BOOL bFormatCylinder( ULONG icyl );
BOOL bVerifyCylinder( ULONG icyl );
BOOL bMakeFileSys( BOOL bFFS, BOOL bOFS, BOOL bIntl, BOOL bNoIntl,
		   BOOL bDirCache, BOOL bNoDirCache );
BOOL bMakeFiles( BOOL bDiskIcon );
void FreeAll(void);
int rcCliMain(void);
int rcGuiMain(void);

#ifdef HAVE_NEWRAWDOFMT
#define RawDoFmtSz(pszBuffer, pszFormat, args...) NewRawDoFmt(pszFormat, RAWFMTFUNC_STRING, pszBuffer, args)
#define RawDoVFmtSz(pszBuffer, pszFormat, pData) VNewRawDoFmt(pszFormat, RAWFMTFUNC_STRING, pszBuffer, pData)
#else
void RawDoFmtSz( char * pszBuffer, const char * pszFormat, ... );
void RawDoVFmtSz( char * pszBuffer, const char * pszFormat, va_list pData );
#endif
