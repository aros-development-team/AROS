/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <dos/dostags.h>
#include <string.h>
#include "/includes.h"

#include "Executor_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

extern CheckCPUDisabled;

VOID Defaults( PrefObject *Prefs )
{
	strcpy( Prefs[0].po_Value, "DisplayInfo Executor.hlp" );
	strcpy( Prefs[2].po_Value, "Kill Command" );
}

LONG Blank( PrefObject *Prefs )
{
	BPTR InFile, OutFile;
	LONG RetVal;
	
	/* Since whatever we execute is likely to take up all the CPU we really
	   can't know whether our executed process or some other is taking up all
	   the CPU time so we bite the bullet and disable CPU checking entirely. */
	CheckCPUDisabled = TRUE;

	InFile = Open( "NIL:", MODE_OLDFILE );
	OutFile = Open( "NIL:", MODE_OLDFILE );
	SystemTags( Prefs[0].po_Value, NP_Name, "!* StealthProcess *!",
			   SYS_Asynch, TRUE, SYS_Input, InFile, SYS_Output, OutFile,
			   TAG_END );

	do
	{
		Delay( 5 );
		RetVal = ContinueBlanking();
	}
	while( RetVal == OK );

	InFile = Open( "NIL:", MODE_OLDFILE );
	OutFile = Open( "NIL:", MODE_OLDFILE );
	SystemTags( Prefs[2].po_Value, SYS_Asynch, TRUE, SYS_Input, InFile,
			   SYS_Output, OutFile, TAG_END );

	return RetVal;
}
