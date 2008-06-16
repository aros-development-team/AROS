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


#include <dos/dos.h>
#include <dos/dosasl.h> /* for ERROR_BREAK definition */
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <dos/rdargs.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "format.h"


BPTR bpfhStdErr;

static struct {
    char   *pszDevice;
    char   *pszName;
    LONG    bOFS;
    LONG    bFFS;
    LONG    bIntl;
    LONG    bNoIntl;
#ifndef __AROS__
    LONG    bDirCache;
    LONG    bNoDirCache;
#endif
    LONG    bNoIcons;
    LONG    bDiskIcon;
    LONG    bQuick;
    LONG    bNoVerify;
    char   *pszType;
    char   *pszFlags;
} args;

#ifdef __AROS__
#define ARGS_TEMPLATE "DEVICE=DRIVE/K/A,NAME/K/A,OFS/S,FFS/S," \
		      "INTL=INTERNATIONAL/S,NOINTL=NOINTERNATIONAL/S," \
		      "NOICONS/S,DISKICON/S," \
		      "QUICK/S,NOVERIFY/S,TYPE=DOSTYPE/K,FLAGS/K"
#else
#define ARGS_TEMPLATE "DEVICE=DRIVE/K/A,NAME/K/A,OFS/S,FFS/S," \
		      "INTL=INTERNATIONAL/S,NOINTL=NOINTERNATIONAL/S," \
		      "DIRCACHE/S,NODIRCACHE/S,NOICONS/S,DISKICON/S," \
		      "QUICK/S,NOVERIFY/S,TYPE=DOSTYPE/K,FLAGS/K"
#endif

int rcCliMain(void)
{
    struct RDArgs * prda;
    BOOL bCloseStdErr = FALSE;
    LONG rc = RETURN_FAIL;
    BPTR bpfhStdIn, bpfhStdOut;
    BOOL DirCache = FALSE;
    BOOL NoDirCache = TRUE;

    prda = ReadArgs(ARGS_TEMPLATE, (LONG *)&args, 0 );
    if( prda == 0 )
    {
	PrintFault( IoErr(), 0 );
	return RETURN_ERROR;
    }

    {
	struct Process * pprSelf = (struct Process *)FindTask(0);
	bpfhStdIn  = pprSelf->pr_CIS;
	bpfhStdOut = pprSelf->pr_COS;
	bpfhStdErr = pprSelf->pr_CES;
	/* CES is allowed to be 0; in that case the best behaviour is to
	   try opening the console, or as a last resort sending errors to
	   COS instead */
	if( bpfhStdErr == 0 )
	{
	    if( (bpfhStdErr = Open( "*", MODE_NEWFILE )) == 0 )
		bpfhStdErr = bpfhStdOut;
	    else
		bCloseStdErr = TRUE;
	}
    }

    if( !bSetSzDosDeviceFromSz(args.pszDevice)
	|| !bSetSzVolumeFromSz(args.pszName)
	|| (args.pszType && !bSetFstFromSz(args.pszType))
	|| (args.pszFlags && !bSetDevfFromSz(args.pszFlags))
	|| !bGetDosDevice(NULL) )
    {
	rc = RETURN_ERROR;
	goto cleanup;
    }

    /* Get confirmation before we start the process */
    *pchDosDeviceColon = 0;
    Printf( "Insert disk to be formatted in device %s\n"
	    "Press RETURN to begin formatting or CTRL-C to abort: ",
	    (ULONG)szDosDevice );
    Flush(bpfhStdOut);
    SetMode( bpfhStdIn, 1 ); /* raw input */
    {
	LONG cch;
	static char ch; /* this must be in public memory! */
	do
	    cch = Read( bpfhStdIn, &ch, 1 );
	while( cch == 1 && ch != 3 && ch != 13 );
    }
    SetMode( bpfhStdIn, 0 ); /* cooked input */
    PutStr("\n");
    if(CheckSignal(SIGBREAKF_CTRL_C))
    {
	PrintFault( ERROR_BREAK, 0 );
	goto cleanup;
    }
    PutStr("\n");
	
    /* Do (low-level) format unless the "QUICK" switch was used */
    if(!args.bQuick)
    {
	ULONG icyl;
	
	if(!bGetExecDevice(!args.bNoVerify))
	    goto cleanup;

	PutStr("\033[0 p"); /* turn cursor off */
	/* TODO: arrange for cursor to be turned back on in case of error */

	for( icyl = LowCyl;
	     icyl <= HighCyl;
	     ++icyl )
	{
	    /* Allow the user to break */
	    if(CheckSignal(SIGBREAKF_CTRL_C))
	    {
		PrintFault( ERROR_BREAK, 0 );
		goto cleanup;
	    }

	    Printf( "\rFormatting cylinder %lu, %lu to go ", icyl, HighCyl-icyl );
	    D(PutStr("\n"));
	    Flush(bpfhStdOut);
	    if(!bFormatCylinder(icyl))
		goto cleanup;

	    if(!args.bNoVerify)
	    {
		PutStr("\rVerifying ");
		D(PutStr("\n"));
		Flush(bpfhStdOut);
		if(!bVerifyCylinder(icyl))
		    goto cleanup;
	    }
	}

	PutStr("\033[ p\n"); /* turn cursor on and go to next line */
    }

    FreeExecDevice();

    PutStr( "Initializing disk...\n" );

#ifndef __AROS__
    DirCache = args.bDirCache;
    NoDirCache = args.bNoDirCache;
#endif
    if( bMakeFileSys( args.bFFS, args.bOFS, args.bIntl, args.bNoIntl,
		      DirCache, NoDirCache )
	&& (args.bNoIcons || bMakeFiles(args.bDiskIcon)) )
	rc = RETURN_OK;

cleanup:
    if(bCloseStdErr)
	(void) Close(bpfhStdErr);
    FreeArgs(prda);

    return rc;
}

