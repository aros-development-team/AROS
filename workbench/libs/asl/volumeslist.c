/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <dos/dosextens.h>
#include <string.h>
#include <gadgets/aroslist.h>
#include "asl_intern.h"
#include "filereqhooks.h"

#ifndef TURN_OFF_DEBUG
#define DEBUG 1
#endif

#include <aros/debug.h>

/********************
**  VLDisplayHook  **
*********************/
AROS_UFH3(VOID, VLDisplayHook,
    AROS_UFHA(struct Hook *,		hook,		A0),
    AROS_UFHA(STRPTR *,			dharray,	A2),
    AROS_UFHA(struct VolumeInfo *, 	vi,		A1)
)
{
    
    *dharray = vi->vi_Name;

D(bug("DispHook: str=%s\n", vi->vi_Name));

    return;
}

/**********************
**  VLConstructHook  **
**********************/

AROS_UFH3(APTR, VLConstructHook,
    AROS_UFHA(struct Hook *,		hook,		A0),
    AROS_UFHA(APTR,			pool,		A2),
    AROS_UFHA(struct DosList *,		dlist,		A1)
)
{
    struct VolumeInfo *vi;
    
    D(bug("VLConstructHook: %s\n", dlist->dol_DevName));
    
    vi = AllocPooled(pool, sizeof (struct VolumeInfo));
    if (!vi)
    	goto failure;

    vi->vi_Name = AllocPooled(pool, strlen(dlist->dol_DevName) + 1 + 1); /* ":" and "\0" */
    if (!vi->vi_Name)
    	goto failure;
			
    strcpy(vi->vi_Name, dlist->dol_DevName);
    strcat(vi->vi_Name, ":");

    vi->vi_Type = dlist->dol_Type;

    return (vi);
    
failure:
    if (vi)
    {
    	if (vi->vi_Name)
    	    FreePooled(pool, vi->vi_Name, strlen(vi->vi_Name) + 1);
    
    	FreePooled(pool, vi, sizeof (struct VolumeInfo));
    }
    return (NULL);
}

/*********************
**  VLDestructHook  **
*********************/

AROS_UFH3(VOID, VLDestructHook,
    AROS_UFHA(struct Hook *,		hook,		A0),
    AROS_UFHA(APTR,			pool,		A2),
    AROS_UFHA(struct VolumeInfo *,	vi,		A1)
)
{
        
    FreePooled(pool, vi->vi_Name, strlen(vi->vi_Name) + 1);
    
    FreePooled(pool, vi, sizeof (struct VolumeInfo));
    
    return;

}

/*****************
**  GetVolumes  **
*****************/

BOOL GetVolumes(Object *list, struct AslBase_intern *AslBase)
{
    struct DosList *dlist;

    BOOL success = TRUE;
    
    dlist = LockDosList(LDF_ALL);
    
    while ((dlist = NextDosEntry(dlist, LDF_ALL)) != NULL)
    {
    	D(bug("\tGetVolumes: dlist=%p, devname=%s\n", dlist, dlist->dol_DevName));
    	if (!DoMethod(	list, 
    			AROSM_List_InsertSingle,
    			dlist,
    			AROSV_List_Insert_Bottom))

    	{
    	    success = FALSE;
    	    break;
    	}
    }
    
    UnLockDosList(LDF_ALL);
    return (success);
}
