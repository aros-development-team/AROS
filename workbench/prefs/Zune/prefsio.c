/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <string.h>
#include <stdlib.h>

#include <libraries/iffparse.h>

#include <proto/exec.h>
#include <proto/iffparse.h>

#include "prefsio.h"

/****************************************************************

*****************************************************************/
STRPTR ReadStringChunkPooled( APTR pool, struct StoredProperty *sp)
{
	STRPTR buf;

	if (!sp) return NULL;
	if (sp->sp_Size==0) return NULL;

	if ((buf = (STRPTR)AllocVec(sp->sp_Size+1,0)))
	{
		strncpy(buf, (STRPTR)sp->sp_Data,sp->sp_Size);
		buf[sp->sp_Size]=0;
	}
	return buf;
}

/****************************************************************

*****************************************************************/
BOOL PushStringChunk( struct IFFHandle *iff, LONG id, STRPTR txt)
{
	if(txt)
	{
		if(!PushChunk(iff,0,id,IFFSIZE_UNKNOWN))
		{
			WriteChunkBytes(iff,txt,strlen(txt));
			PopChunk(iff);
		}
	}
	return TRUE;
}

/****************************************************************
 Save zune prefs
*****************************************************************/
void zune_prefs_save_as_iff(char *filename, struct ZunePrefs *prefs)
{
	if (!prefs) return;
}

/****************************************************************
 Load zune prefs from given filename
*****************************************************************/
struct ZunePrefs *zune_prefs_load(char *filename)
{
	return NULL;
}

/****************************************************************
 Free prefs allocated by zune_prefs_load
*****************************************************************/
void zune_prefs_free(struct ZunePrefs *prefs)
{
	if (!prefs) return;
}

