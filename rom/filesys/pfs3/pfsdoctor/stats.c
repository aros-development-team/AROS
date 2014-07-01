/* $Id$ */
/* $Log: stats.c $
 * Revision 1.4  1999/05/28  05:07:33  Michiel
 * Fixed bug occuring on empty directory blocks
 * Added rbl.always fix; improved rbl.disksize fix
 * Reduced cachesize
 *
 * Revision 1.3  1999/05/04  17:59:09  Michiel
 * check mode, logfile, search rootblock implemented
 * bugfixes
 *
 * Revision 1.2  1999/05/04  04:27:13  Michiel
 * debugged upto buildrext
 *
 * Revision 1.1  1999/04/22  15:24:28  Michiel
 * Initial revision
 * */

#include <exec/types.h>
#include <stdio.h>
#include "pfs3.h"
#include "doctor.h"

static char msgbuffer[512];
static BOOL error = FALSE;
static BOOL fixed = FALSE;

void clearstats(void)
{
	stats.numfiles =
	stats.numdirs =
	stats.numsoftlink =
	stats.numhardlink =
	stats.numrollover =
	stats.fragmentedfiles =
	stats.anodesused = 0;

	volume.updatestats();
}

void adderror(char *message)
{
	if (!error)
	{
		if (stats.blocknr) 
			error = TRUE;
		stats.numerrors++;
	}

	if (message)
	{
		sprintf(msgbuffer, "ERROR: %s\n", message);
		volume.showmsg(msgbuffer);
	}
	volume.updatestats();
} 

void fixederror(char *message)
{
	if (mode == check)
	{
		adderror(message);
		return;
	}

	adderror(NULL);

	if (!fixed)
	{
		if (stats.blocknr)
			fixed = TRUE;
		stats.errorsfixed++;
	}

	if (message)
	{
		sprintf(msgbuffer, "FIXED: %s\n", message);
		volume.showmsg(msgbuffer);
	}
	else
		volume.showmsg("FIXED\n");

	volume.updatestats();
}

void enterblock(uint32 blknr)
{
	if (blknr != stats.blocknr)
	{
		if (stats.blocknr)
			stats.prevblknr = stats.blocknr;
		stats.blocknr = blknr;
		error = fixed = FALSE;
		stats.blockschecked++;
		volume.updatestats();			/* optimise !! */
	}
}

void exitblock(void)
{
	stats.blocknr = stats.prevblknr;
	stats.prevblknr = 0;
	return;
}
