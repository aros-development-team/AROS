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
#include <clib/macros.h>

#include "asl_intern.h"
#include "dirlist.h"

#define SDEBUG 0
#define DEBUG 0

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
    if (ead->ed_Type < 0)
    {
    	/* file */
  	snprintf(bufstrptr, DISPHOOKBUFSIZE, "%d", ead->ed_Size);
  	*dharray = bufstrptr;
	
    } else {
    	/* dir */
	*dharray = "Directory";
    }
    
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
    
    EnterFunc(bug("ConstructHook: %s\n", ead->ed_Name));
    
    new_ead = AllocPooled(pool, sizeof (struct ExAllData));
    if (!new_ead)
    	goto failure;

    new_ead->ed_Name	= NULL;
    new_ead->ed_Comment	= NULL;
    
    /* We allways have a file name ...*/
    if (!(new_ead->ed_Name = AllocPooled(pool, strlen(ead->ed_Name) + 1)))
    	goto failure;

    
    /* ... but not allways a file comment */
    if (ead->ed_Comment)
    {
	if (!(new_ead->ed_Comment = AllocPooled(pool, strlen(ead->ed_Comment) + 1)))
	    goto failure;
    	strcpy(new_ead->ed_Comment,	ead->ed_Comment);
    }
    
			
    strcpy(new_ead->ed_Name,	ead->ed_Name);

						
    /* Set the other fields */
    new_ead->ed_Type	= ead->ed_Type;
    new_ead->ed_Size	= ead->ed_Size;
    new_ead->ed_Prot	= ead->ed_Prot;
    new_ead->ed_Days	= ead->ed_Days;
    new_ead->ed_Mins	= ead->ed_Mins;
    new_ead->ed_Ticks	= ead->ed_Ticks;

    ReturnPtr ("ConstructHook", struct ExAllData *, new_ead);
    
failure:
D(bug("failure\n"));
    if (new_ead)
    {
    	if (new_ead->ed_Name);
    	    FreePooled(pool, new_ead->ed_Name, strlen(new_ead->ed_Name) + 1);
    
    	if (new_ead->ed_Comment);
    	    FreePooled(pool, new_ead->ed_Comment, strlen(new_ead->ed_Comment) + 1);
    
    	FreePooled(pool, new_ead, sizeof (struct ExAllData));
    }
    ReturnPtr ("ConstructHook", struct ExAllData *, NULL);
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
    if (ead->ed_Comment) FreePooled(pool, ead->ed_Comment, strlen(ead->ed_Comment) + 1);
    
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
    
    EnterFunc(bug("GetDir(path=%s, AslBase=%p\n", path, AslBase));
	
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


/*********************
**  UpdateFileList  **
*********************/
BOOL UpdateFileList(	Class			*cl,
			Object			*dirlist,
		  	struct GadgetInfo	*ginfo,
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
    
    EnterFunc(bug("UpdateFileList(dirlist=%p, ginfo=%p, AslBase=%p)\n"
    		, dirlist, ginfo, AslBase));


    DoMethod(data->dld_FileList, AROSM_List_Clear);

    if (!GetDir(path_string(data->dld_CurPath), data->dld_FileList, ASLB(AslBase)))
	ReturnBool("UpdateFileList", FALSE);

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
    
    ReturnBool ("UpdateFileList", TRUE);
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


/**************** Path handling functions *********************/

#define SIZEINCREASE 100

struct path *path_init(STRPTR initval, struct AslBase_intern *AslBase)
{
    struct path *path;
    if (!initval)
    	initval = "SYS:"; /* default value */
    
    path = AllocMem( sizeof (*path) , MEMF_ANY|MEMF_CLEAR);
    if (path)
    {
	if (path_set(path, initval, AslBase))
	{
	        return path;
	}
	
	FreeMem(path, sizeof (*path));
    }
    return NULL;
}

BOOL path_set(struct path *path, STRPTR val, struct AslBase_intern *AslBase)
{
    ULONG len;
    ULONG alloclen = 0;
    
    BOOL ok;
    
    len = strlen(val) + 1;
    if (!path->buf)
    	alloclen = MAX(len, SIZEINCREASE);
    else
    {
        if (len > path->buflen)
	    alloclen = len;
    }
    
    if (alloclen) /* Allocate more mem ? */
    {
        /* Allocate new space first, if it fails
           we can keep th old one 
	*/
	STRPTR newbuf;
	
	newbuf = AllocMem(alloclen, MEMF_ANY);
	if (newbuf)
	{
	    if (path->buf)
		FreeMem(path->buf, path->buflen);
	    
	    path->buf = newbuf;
	    path->buflen = alloclen;
	    ok = TRUE;
	}
	else
	    ok = FALSE;
    }
    else
        ok = TRUE;
    
    if (ok)
    	strcpy(path->buf, val);
    
    return ok;
}

BOOL path_add(struct path *path, STRPTR toadd, struct AslBase_intern *AslBase)
{
    struct path *backup;
    STRPTR newbuf = NULL;
    
    backup = path_init(path->buf, AslBase);
    if (!backup)
    	return FALSE;
    

    while (!AddPart(path->buf, toadd, path->buflen))
    {
        ULONG oldlen = path->buflen;
	path->buflen += SIZEINCREASE;
	
    	newbuf = AllocMem(path->buflen, MEMF_ANY);
    	if (!newbuf)
	{
	    /* Ran out of mem */
	    path->buf	 = backup->buf;
	    path->buflen = backup->buflen;
	    
	    return FALSE;
	}
	
        FreeMem(path->buf, oldlen);
	path->buf = newbuf;
    	strcpy(path->buf, backup->buf);
	
    }
    
    return TRUE;
    
}

VOID path_cleanup(struct path *path, struct AslBase_intern *AslBase)
{
    if (path->buf)
    	FreeMem(path->buf, path->buflen);
	
    FreeMem(path, sizeof (*path));
    
    return;
    
}
STRPTR path_string(struct path *path)
{
    return path->buf;
}
