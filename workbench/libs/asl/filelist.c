/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <dos/dos.h>
#include <gadgets/aroslist.h>
#include <stdio.h>
#include "asl_intern.h"
#include "filereqhooks.h"

#ifndef TURN_OFF_DEBUG
#define DEBUG 1
#endif

#include <aros/debug.h>

/********************
**  FLDisplayHook  **
*********************/
AROS_UFH3(VOID, FLDisplayHook,
    AROS_UFHA(struct Hook *,		hook,		A0),
    AROS_UFHA(STRPTR *,			dharray,	A2),
    AROS_UFHA(struct ExAllData *, 	ead,		A1)
)
{
    struct FRUserData *udata = (struct FRUserData *)hook->h_Data;
    
    STRPTR bufstrptr;
    
    bufstrptr = udata->DispHookBuf;
    
    *dharray ++ = ead->ed_Name;
    snprintf(bufstrptr, FILENAMEBUFSIZE, "%d", ead->ed_Size);
    *dharray = bufstrptr;
    
    D(bug("File disphook: name=%s, size=%s\n", ead->ed_Name, bufstrptr));
    
    return;
}


	

/**********************
**  FLConstructHook  **
**********************/

AROS_UFH3(APTR, FLConstructHook,
    AROS_UFHA(struct Hook *,		hook,		A0),
    AROS_UFHA(APTR,			pool,		A2),
    AROS_UFHA(struct ExAllData *,	ead,		A1)
)
{
    struct ExAllData *new_ead;
    
    D(bug("ConstructHook: %s\n", ead->ed_Name));
    
    new_ead = AllocPooled(pool, sizeof (struct ExAllData));
    if (!new_ead)
    	goto failure;

    new_ead->ed_Name	= NULL;
    ead->ed_Comment	= NULL;
    
    if (!(new_ead->ed_Name = AllocPooled(pool, strlen(ead->ed_Name) + 1)))
    	goto failure;
    	
    if (!(new_ead->ed_Comment = AllocPooled(pool, strlen(ead->ed_Comment) + 1)))
    	goto failure;
    
			
    strcpy(new_ead->ed_Name,	ead->ed_Name);
    strcpy(new_ead->ed_Comment,	ead->ed_Comment);

						
    /* Set the other fields */
    new_ead->ed_Type	= ead->ed_Type;
    new_ead->ed_Size	= ead->ed_Size;
    new_ead->ed_Prot	= ead->ed_Prot;
    new_ead->ed_Days	= ead->ed_Days;
    new_ead->ed_Mins	= ead->ed_Mins;
    new_ead->ed_Ticks	= ead->ed_Ticks;

    return (new_ead);
    
failure:
    if (new_ead)
    {
    	if (new_ead->ed_Name);
    	    FreePooled(pool, new_ead->ed_Name, strlen(new_ead->ed_Name) + 1);
    
    	if (new_ead->ed_Comment);
    	    FreePooled(pool, new_ead->ed_Comment, strlen(new_ead->ed_Comment) + 1);
    
    	FreePooled(pool, new_ead, sizeof (struct ExAllData));
    }
    return (NULL);
}

/*********************
**  FLDestructHook  **
*********************/

AROS_UFH3(VOID, FLDestructHook,
    AROS_UFHA(struct Hook *,		hook,		A0),
    AROS_UFHA(APTR,			pool,		A2),
    AROS_UFHA(struct ExAllData *,	ead,		A1)
)
{
        
    FreePooled(pool, ead->ed_Name, strlen(ead->ed_Name) + 1);
    FreePooled(pool, ead->ed_Comment, strlen(ead->ed_Comment) + 1);
    
    FreePooled(pool, ead, sizeof (struct ExAllData));
    
    return;

}



/**************
**  GetDir   **
**************/

/* Get a list object of files for use in a directory */

/* Build the list of .font file names using ExAll() */
BOOL GetDir(STRPTR path, Object *list, struct AslBase_intern *AslBase)
{
	
    BOOL success = TRUE;

    struct ExAllControl *eac;
    struct ExAllData *ead;
    BPTR lock;

    /* Buffer to put ExAll() file in */
    UBYTE buffer[4096];
	
    lock = Lock(path, ACCESS_READ);
    if (!lock)
    {
    	success = FALSE;
    	D(bug("Could not lock directory\n"));
    }
    else
    {
    
    	eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
	if (!eac)
	    success = FALSE;
    	else
    	{
	    BOOL more;

	    /* Important to clear this field */
	    eac->eac_LastKey = 0;
	    do
	    {
	    	more = ExAll(lock, (struct ExAllData *)buffer, 4096, ED_COMMENT, eac);
			
	    	if ( (!more) && ( IoErr() != ERROR_NO_MORE_ENTRIES) )
	    	{
		    success = FALSE;
		    break;
	   	}
			
	    	if (eac->eac_Entries == 0)
	    	{
		    continue;
	    	}
			
	    	/* Get first ExallData element */
	    	ead = (struct ExAllData *)buffer;
			
	    	/* We should continue to ExAll() even if memoryalloc failed */
	    	while (ead && success)
	    	{
		    /*  Insert a list entry */
		    if (!DoMethod(list, 
		    		AROSM_List_InsertSingle, 
		    		ead, 
		    		AROSV_List_Insert_Bottom))
		    {
		    	success = FALSE;
		    }
		    ead = ead->ed_Next;

	    	} /* while (ead && success) */
	    }
	    while (more);
		
		
	FreeDosObject(DOS_EXALLCONTROL, eac);
    	} /* if (DosObject allocated) */
    	
    	UnLock(lock);
    } /* if (directory locked) */
    ReturnBool ("GetDir", success);		
}

