/*
 *	Copyright (c) 1994 Michael D. Bayne.
 *	All rights reserved.
 *
 *	Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include <string.h>
#include <stdlib.h>

#include "includes.h"
#include "libraries.h"
#include "protos/protos.h"

struct IBox WinBox = { 0, 16, 250, 200 };
struct IBox OldBox = { 0, 0, 0, 0 };
BYTE OldBlanker[64] = "";

BlankerPrefs PrefsRec = { 1, TRUE, 600, 300, BC_NONE, BC_NONE, "Alt Help",
							  "Alt Delete", "Random", "Blankers", 0L };
STRPTR CornerStrs[] = { "NONE", "UPPERLEFT", "UPPERRIGHT", "LOWERRIGHT",
							"LOWERLEFT" };

VOID BlankerToEnv( BlankerPrefs *Prefs )
{
	if( !Stricmp( OldBlanker, Prefs->bp_Blanker ))
		return;
	strcpy( OldBlanker, Prefs->bp_Blanker );
	SetVar37( "BLANKER", Prefs->bp_Blanker, 64, GVF_GLOBAL_ONLY|GVF_SAVE_VAR );
}

VOID WinBoxToEnv( struct IBox *Box )
{
	if( !memcmp( Box, &OldBox, sizeof( struct IBox )))
		return;
	OldBox = *Box;
	SetVar37( "GBLANKER.win", ( STRPTR )Box, sizeof( struct IBox ),
			 GVF_GLOBAL_ONLY|GVF_BINARY_VAR|GVF_DONT_NULL_TERM|GVF_SAVE_VAR );
}	

BlankerPrefs *LoadDefaultPrefs( VOID )
{
	struct DiskObject *bDO;
	
	GetVar37( "BLANKER", PrefsRec.bp_Blanker, 64, 0L );
	GetVar37( "GBLANKER.win", ( STRPTR )&WinBox, sizeof( struct IBox ),
			 GVF_BINARY_VAR|GVF_DONT_NULL_TERM );
	
	strcpy( OldBlanker, PrefsRec.bp_Blanker );
	OldBox = WinBox;

	if( bDO = GetDiskObject( ProgName ))
	{
		STRPTR tooltype;

		if( tooltype = FindToolType( bDO->do_ToolTypes, "CX_PRIORITY" ))
			PrefsRec.bp_Priority = atoi( tooltype );
		if( tooltype = FindToolType( bDO->do_ToolTypes, "CX_POPUP" ))
			PrefsRec.bp_PopUp = ( LONG )MatchToolValue( tooltype, "YES" );
		if( tooltype = FindToolType( bDO->do_ToolTypes, "CX_POPKEY" ))
			strcpy( PrefsRec.bp_PopKey, tooltype );
		if( tooltype = FindToolType( bDO->do_ToolTypes, "BLANKKEY" ))
			strcpy( PrefsRec.bp_BlankKey, tooltype );
		if( tooltype = FindToolType( bDO->do_ToolTypes, "TIMEOUT" ))
			PrefsRec.bp_Timeout = 10 * atoi( tooltype );
		if( tooltype = FindToolType( bDO->do_ToolTypes, "RANDTIMEOUT" ))
			PrefsRec.bp_RandTimeout = 10 * atoi( tooltype );
		if( tooltype = FindToolType( bDO->do_ToolTypes, "BLANKERDIR" ))
			strcpy( PrefsRec.bp_Dir, tooltype );
		if( tooltype = FindToolType( bDO->do_ToolTypes, "REPLACE" ))
			PrefsRec.bp_Flags |=
				( MatchToolValue( tooltype, "YES" ) ? BF_REPLACE : 0L );
		if( tooltype = FindToolType( bDO->do_ToolTypes, "BLANKCORNER" ))
		{
			LONG i;

			for( i = 1; i < 5; i++ )
			{
				if( MatchToolValue( tooltype, CornerStrs[i] ))
				{
					PrefsRec.bp_BlankCorner = i;
					break;
				}
			}
		}
		if( tooltype = FindToolType( bDO->do_ToolTypes, "DONTCORNER" ))
		{
			LONG i;

			for( i = 1; i < 5; i++ )
			{
				if( MatchToolValue( tooltype, CornerStrs[i] ))
				{
					PrefsRec.bp_DontCorner = i;
					break;
				}
			}
		}
		FreeDiskObject( bDO );
	}
	
	return &PrefsRec;
}

LONG EntriesInList( struct List *List )
{
	BlankerEntry *Ctr = ( BlankerEntry * )List->lh_Head;
	LONG Entries = 0;

	for( ; Ctr->be_Node.ln_Succ; Ctr = ( BlankerEntry * )Ctr->be_Node.ln_Succ )
		if( !Ctr->be_Disabled )
			Entries++;

	return Entries;
}

STRPTR RandomModule( VOID )
{
	BlankerEntry *Ent = ( BlankerEntry * )BlankerEntries->lh_Head;
	LONG i = EntriesInList( BlankerEntries ) - 1;
	LONG Entry = RangeRand( i ) + 1;
	
	if( !i )
		return 0L;
	
	for( ;; )
	{
		if( !Ent->be_Disabled )
			if( !--Entry )
				return Ent->be_Name;
		Ent = ( BlankerEntry * )Ent->be_Node.ln_Succ;
	}
}
