/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <gadgets/aroslist.h>
#include <gadgets/aroslistview.h>
#include <stdio.h>
#include <string.h>
#include "asl_intern.h"
#include "dirlist.h"

#ifndef TURN_OFF_DEBUG
#define DEBUG 1
#endif

#include <aros/debug.h>

/**********************
**  FileDisplayHook  **
***********************/
AROS_UFH3(VOID, FileDisplayHook,
    AROS_UFHA(struct Hook *,		hook,		A0),
    AROS_UFHA(STRPTR *,			dharray,	A2),
    AROS_UFHA(struct ExAllData *, 	ead,		A1)
)
{
    
    STRPTR bufstrptr;
    

    bufstrptr = (STRPTR)hook->h_Data;
    
    *dharray ++ = ead->ed_Name;
    snprintf(bufstrptr, DISPHOOKBUFSIZE, "%d", ead->ed_Size);
    *dharray = bufstrptr;

    return;
}


/************************
**  FileConstructHook  **
************************/

AROS_UFH3(APTR, FileConstructHook,
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

/***********************
**  FileDestructHook  **
***********************/

AROS_UFH3(VOID, FileDestructHook,
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

/****************
**  AddToPath  **
*****************/
BOOL AddToPath(		STRPTR			path2add,
			STRPTR			*bufptr,
			ULONG			*lptr,
			struct AslBase_intern	*AslBase)
{
#undef SIZEINCREASE
#define SIZEINCREASE 100

    if (!*bufptr)
    {
    	*bufptr = AllocMem(SIZEINCREASE, MEMF_ANY);
    	if (!*bufptr)
    	    return (FALSE);
    	    
    	**bufptr = 0; /* null-terminate */
    	*lptr = SIZEINCREASE;
    }
    
    while (!AddPart(*bufptr, path2add, *lptr))
    {
    	STRPTR newbuf;
    	
    	newbuf = AllocMem(*lptr + SIZEINCREASE, MEMF_ANY);
    	if (!newbuf)
    	    return (FALSE);
    	  
    	strcpy(newbuf, *bufptr);
    	FreeMem(*bufptr, *lptr);
    	
    	*lptr += SIZEINCREASE;
    }
    
    return (TRUE);
    
}

/*********************
**  UpdateFileList  **
*********************/
BOOL UpdateFileList(	Class			*cl,
			Object			*dirlist,
		  	struct GadgetInfo	*ginfo,
		    	STRPTR			pathadd,
		   	struct AslBase_intern	*AslBase)
{
    struct opSet set_msg;
    
    struct TagItem tags[] =
    {
    	{ AROSA_Listview_List,		NULL		},
    	{ AROSA_Listview_Format, 	(IPTR)"P=l, P=l"},
    	{ AROSA_Listview_DisplayHook,	NULL		},
    	{ TAG_END }
    };
    struct DirListData *data = (struct DirListData *)INST_DATA(cl, dirlist);
    if (!AddToPath(pathadd, 
		&(data->dld_CurPath),
		&(data->dld_PathBufSize),
		ASLB(AslBase)))
	return (FALSE);

    DoMethod(data->dld_FileList, AROSM_List_Clear);

    if (!GetDir(data->dld_CurPath, data->dld_FileList, ASLB(AslBase)))
	return (FALSE);

    set_msg.MethodID = OM_SET;
    set_msg.ops_GInfo 	= ginfo;
    set_msg.ops_AttrList = tags;
    
    tags[0].ti_Data = (IPTR)data->dld_FileList;
    tags[2].ti_Data = (IPTR)&(GetSDLD(cl)->sd_FileDisplayHook);
    
    DoSuperMethodA(cl, dirlist, (Msg)&set_msg);
    
    /* Rerender dirlist */
    if (ginfo)
    {
    	struct RastPort *rp;
    	DoSuperMethod(cl, dirlist, GM_LAYOUT, ginfo, TRUE);
    	if ((rp = ObtainGIRPort(ginfo)) != NULL)
    	{
    	    DoMethod(dirlist, GM_RENDER, ginfo, rp, GREDRAW_REDRAW);
    	    
    	    ReleaseGIRPort(rp);
    	}
    }

    data->dld_Flags |= DLFLG_FILELIST;
    
    return (TRUE);
}		    


/*********************
**  VolDisplayHook  **
**********************/
AROS_UFH3(VOID, VolDisplayHook,
    AROS_UFHA(struct Hook *,		hook,		A0),
    AROS_UFHA(STRPTR *,			dharray,	A2),
    AROS_UFHA(struct VolumeInfo *, 	vi,		A1)
)
{
    *dharray = vi->vi_Name;

    return;
}

/***********************
**  VolConstructHook  **
***********************/

AROS_UFH3(APTR, VolConstructHook,
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

/**********************
**  VolDestructHook  **
**********************/

AROS_UFH3(VOID, VolDestructHook,
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
