/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Hook for handling fonts in FONTS:
    Lang: English.
*/

/****************************************************************************************/

#include <exec/initializers.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/arossupport.h>
//#include <proto/alib.h>
#include <string.h>

#include "diskfont_intern.h"  

/****************************************************************************************/

#ifndef TURN_OFF_DEBUG 
#define DEBUG 0
#endif

#include <aros/debug.h>

/****************************************************************************************/

struct DirEntry;

struct FileEntry
{
    struct MinNode          Node;
    struct DirEntry        *DirEntry;
    STRPTR 		    FileName;
    struct DateStamp        FileChanged;
    UWORD                   ContentsID;
    UBYTE                   SupportedStyles;
    UBYTE                   FontStyle;
    ULONG                   Numentries;
    struct TTextAttr       *Attrs;
};

struct DirEntry
{
    struct MinNode  	   Node;
    BPTR    	    	   DirLock;
    struct DateStamp       DirChanged;
    struct MinList	   FileList;
};

struct DFData /*DiskFontData */
{
#ifdef PROGDIRFONTSDIR
    struct DirEntry       *ProgdirDirEntry;
#endif
    struct DirEntry       *CurrentDirEntry;
    struct FileEntry      *CurrentFileEntry, *PrevFileEntry;
    UWORD                  AttrsIndex;
    struct FileEntry      *RememberFileEntry;
    UWORD                  RememberIndex;
};

/****************************************************************************************/

/*****************/
/* ReadFileEntry */
/*****************/

/****************************************************************************************/

STATIC struct FileEntry *ReadFileEntry(struct ExAllData *ead, struct DiskfontBase_intern *DiskfontBase)
{
    struct FontDescrHeader *fdh;
    struct FileEntry       *retval = NULL;
    ULONG                   size, strsize, tagcount;
    int                     i;
    struct TagItem         *tagitems;
    STRPTR                  filename;
    
    D(bug("ReadFileEntry: ead: 0x%lx\n", ead));
    D(bug("ReadFileEntry: name=\"%s\"\n", ead->ed_Name));
    
    fdh = ReadFontDescr(ead->ed_Name, DiskfontBase);
    if (fdh == NULL)
    {
	D(bug("ReadFileEntry: error in ReadFontDescr\n"));
	return NULL;
    }
    
    strsize = strlen(ead->ed_Name) + 1;
    size = sizeof(struct FileEntry) + strsize + fdh->NumEntries * sizeof(struct TTextAttr);
    if (fdh->ContentsID == OFCH_ID) /* Reserve extra Attr for outline fonts */
         size += sizeof(struct TTextAttr);
    
    for (i = 0; i < fdh->NumEntries; i++)
	if (fdh->TAttrArray[i].tta_Tags != NULL)
	    size += NumTags(fdh->TAttrArray[i].tta_Tags, DiskfontBase) * sizeof(struct TagItem);
    
    retval = AllocVec(size, MEMF_ANY|MEMF_CLEAR);
    if (retval == NULL)
    {
	D(bug("ReadFileEntry: Could not allocate memory\n"));
	FreeFontDescr(fdh, DiskfontBase);
	ReturnPtr("ReadFileEntry", struct FileEntry *, NULL);
    }

    filename = (STRPTR)(retval + 1);
    strcpy(filename, ead->ed_Name);
    retval->FileName = filename;
    retval->FileChanged = *(struct DateStamp *)&ead->ed_Days;
    
    retval->Numentries = fdh->NumEntries;
    retval->ContentsID = fdh->ContentsID;
    retval->Attrs = (struct TTextAttr *)(filename + strsize);
    memcpy(retval->Attrs, fdh->TAttrArray, fdh->NumEntries * sizeof(struct TTextAttr));
    if (retval->ContentsID == OFCH_ID)
    {
         int ind = retval->Numentries;
         
         retval->Numentries++;
         retval->SupportedStyles = OTAG_GetSupportedStyles(fdh->OTagList, DiskfontBase);
         retval->FontStyle = OTAG_GetFontStyle(fdh->OTagList, DiskfontBase);
         retval->Attrs[ind].tta_Name = filename;
         retval->Attrs[ind].tta_Flags = OTAG_GetFontFlags(fdh->OTagList, DiskfontBase);
    }

    tagitems = (struct TagItem *)(retval->Attrs + fdh->NumEntries);
    for (i = 0; i < fdh->NumEntries; i++)
    {
	retval->Attrs[i].tta_Name = retval->FileName;
   
	if (fdh->TAttrArray[i].tta_Tags != NULL)
	{
	    tagcount = NumTags(fdh->TAttrArray[i].tta_Tags, DiskfontBase);
	    CopyTagItems(tagitems, fdh->TAttrArray[i].tta_Tags, DiskfontBase);
	    retval->Attrs[i].tta_Tags = tagitems;
	    tagitems += tagcount;
	}
	else
	    retval->Attrs[i].tta_Tags = NULL;
    }
    
    FreeFontDescr(fdh, DiskfontBase);
    
    ReturnPtr("ReadFileEntry", struct FileEntry *, retval);
}
    
/****************************************************************************************/

/*****************/
/* FreeFileEntry */
/*****************/

/****************************************************************************************/

STATIC VOID FreeFileEntry(struct FileEntry *feptr, struct DiskfontBase_intern *DiskfontBase)
{
    FreeVec(feptr);
}

/****************************************************************************************/

/****************/
/* FreeFileList */
/****************/

/****************************************************************************************/

STATIC VOID FreeFileList(struct MinList *filelist, struct DiskfontBase_intern *DiskfontBase)
{
    struct FileEntry *feptr, *nextfeptr;

    D(bug("FreeFileList(filelist=%p)\n", filelist));

    ForeachNodeSafe(filelist, feptr, nextfeptr)
    {
	REMOVE(feptr);
	FreeFileEntry(feptr, DiskfontBase);
    }

    ReturnVoid("FreeFileList");
}

/****************************************************************************************/

/****************/
/* GetFileList	*/
/****************/

/****************************************************************************************/

/* Build the list of .font file names using Examine() */
STATIC BOOL GetFileList(struct DirEntry *direntry, struct DiskfontBase_intern *DiskfontBase)
{
    struct MinList       newlist; 
    struct FileEntry 	*fe, *nextfe;
    BOOL                 retval = TRUE, more;
    struct ExAllControl *eac;
    struct ExAllData    *ead, *eadit;
    
    D(bug("GetFileList(direntry=%p)\n", direntry));

    NEWLIST(&newlist);
    
    eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
    eac->eac_LastKey = 0;
    ead = (struct ExAllData *)AllocMem(1024, MEMF_ANY);
    
    do
    { 
	more = ExAll(direntry->DirLock, ead, 1024, ED_DATE, eac);
	if ((!more) && (IoErr() != ERROR_NO_MORE_ENTRIES))
	    retval = FALSE;
	else if (eac->eac_Entries == 0)
	    continue;
	else
	{
	    for (eadit = ead; eadit != NULL; eadit = eadit->ed_Next)
	    {
		ULONG namelen = strlen(eadit->ed_Name);

		D(bug("GetFileList: Scanning file: %s\n", eadit->ed_Name));

		/* Maybe this can be done in a hook function passed in eac */
		if (namelen < 5 || strncmp(eadit->ed_Name+namelen-5, ".font", 5) != 0)
		{
		    D(bug("GetFileList: wrong suffix: %s\n", eadit->ed_Name+namelen-5));
		    continue;
		}
	    
		/* Is a FileEntry for this file already in the list */
		for (fe = (struct FileEntry *)GetHead(&direntry->FileList);
		     fe != NULL;
		     fe = (struct FileEntry *)GetSucc(fe))
		{
		    if (strcmp(eadit->ed_Name, fe->FileName) == 0)
			break;
		}
	    
		if (fe == NULL)
		{
		    D(bug("GetFileList: Filename not yet in memory\n"));
		    
		    fe = ReadFileEntry(eadit, DiskfontBase);
		    if (fe != NULL)
		    {
			fe->DirEntry = direntry;
			
			D(bug("GetFileList: Add to list: 0x%lx\n", fe));
			ADDTAIL(&newlist, fe);
		    }
		}
		else
		{
		    struct DateStamp ds;
		    REMOVE(fe);
		    
		    ds.ds_Days = eadit->ed_Days;
		    ds.ds_Minute = eadit->ed_Mins;
		    ds.ds_Tick = eadit->ed_Ticks;
		    
		    D(bug("GetFileList: Found filename already in memory\n"));
		    
		    if (CompareDates(&ds, &fe->FileChanged) == 0)
		    {
			D(bug("GetFileList: File's date not changed: Add to list 0x%lx\n", fe));
			ADDTAIL(&newlist, fe);
		    }
		    else
		    {
			D(bug("GetFileList: Date changed rereading information\n"));
			
			FreeFileEntry(fe, DiskfontBase);
			
			fe = ReadFileEntry(eadit, DiskfontBase);
			if (fe == NULL)
			{
			    D(bug("GetFileList: Reading information failed\n"));
			    retval = FALSE;
			    break;
			}
			else
			{
			    fe->DirEntry = direntry;
			    D(bug("GetFileList: Reading OK, add to list: 0x%lx\n", fe));
			    ADDTAIL(&newlist, fe);
			}
		    }
		}
	    } /* for (eadit = ... */
	}
    } while (more && retval);
    
    FreeDosObject(DOS_EXALLCONTROL, eac);
    FreeMem(ead, 1024);
    
    if (!retval)
    {
	D(bug("GetFileList: Not OK freeing FileList\n"));
	FreeFileList(&newlist, DiskfontBase);
    }
    else
    {
	/* Remove FileEntries that are still in memory but not anymore
	 * on disk */
	FreeFileList(&direntry->FileList, DiskfontBase);
	
	ForeachNodeSafe(&newlist, fe, nextfe)
	{
	    REMOVE(fe);
	    ADDTAIL(&direntry->FileList, fe);
	    D(bug("GetFileList: Adding fe=%p\n", fe));
	}
    }
    
    ReturnBool ("GetFileList", retval);
}

/****************************************************************************************/

/*********************/
/* StreamOutFileList */
/*********************/

/****************************************************************************************/

STATIC BOOL StreamOutFileList(struct MinList *filelist, struct FileHandle *fh, struct DiskfontBase_intern *DiskfontBase)
{
    struct FileEntry *fe;
    ULONG i;
    BOOL ok = TRUE;
    
    ForeachNode(filelist, fe)
    {
	D(bug("StreamOutFileList: Writing file %s\n", fe->FileName));
	ok = ok && WriteString(&DiskfontBase->dsh, fe->FileName, fh);
	D(bug("StreamOutFileList: Write days: %d minute: %d, tick: %d\n",
	      fe->FileChanged.ds_Days, fe->FileChanged.ds_Minute,
	      fe->FileChanged.ds_Tick));
	ok = ok && WriteLong(&DiskfontBase->dsh, fe->FileChanged.ds_Days, fh);
	ok = ok && WriteLong(&DiskfontBase->dsh, fe->FileChanged.ds_Minute, fh);
	ok = ok && WriteLong(&DiskfontBase->dsh, fe->FileChanged.ds_Tick, fh);
        ok = ok && WriteWord(&DiskfontBase->dsh, fe->ContentsID, fh);
        ok = ok && WriteByte(&DiskfontBase->dsh, fe->SupportedStyles, fh);
        ok = ok && WriteByte(&DiskfontBase->dsh, fe->FontStyle, fh);
	D(bug("StreamOutFileList: Write numentries=%d\n", fe->Numentries));
	ok = ok && WriteLong(&DiskfontBase->dsh, fe->Numentries, fh);
	
	for (i=0; ok && i<fe->Numentries; i++)
	{
	    ok = ok && WriteWord(&DiskfontBase->dsh, fe->Attrs[i].tta_YSize, fh);
	    ok = ok && WriteByte(&DiskfontBase->dsh, fe->Attrs[i].tta_Style, fh);
	    ok = ok && WriteByte(&DiskfontBase->dsh, fe->Attrs[i].tta_Flags, fh);
	    ok = ok && WriteTagsNum(fh, fe->Attrs[i].tta_Tags, DiskfontBase);
	}
    }
    ok = ok && WriteString(&DiskfontBase->dsh, "", fh);
    
    return ok;
}

/****************************************************************************************/

/********************/
/* StreamInFileList */
/********************/

/****************************************************************************************/

STATIC BOOL StreamInFileList(struct DirEntry *direntry, struct FileHandle *fh, struct DiskfontBase_intern *DiskfontBase)
{
    struct FileEntry *fe, fe2;
    ULONG i, numtags, totnumtags;
    BOOL ok = TRUE;
    struct TTextAttr *attrs;
    struct TagItem *tagptr;
    
    for (ok = ReadString(&DiskfontBase->dsh, &fe2.FileName, fh);
	 ok && strlen(fe2.FileName)>0;
	 ok = ok && ReadString(&DiskfontBase->dsh, &fe2.FileName, fh))
    {
	D(bug("StreamInFileList: reading data for \"%s\"\n", fe2.FileName));
	
	ok = ok && ReadLong(&DiskfontBase->dsh, &fe2.FileChanged.ds_Days, fh);
	ok = ok && ReadLong(&DiskfontBase->dsh, &fe2.FileChanged.ds_Minute, fh);
	ok = ok && ReadLong(&DiskfontBase->dsh, &fe2.FileChanged.ds_Tick, fh);
	D(bug("StreamInFileList: read days: %d minute: %d tick: %d\n",
	      fe2.FileChanged.ds_Days, fe2.FileChanged.ds_Minute,
	      fe2.FileChanged.ds_Tick));
        ok = ok && ReadWord(&DiskfontBase->dsh, &fe2.ContentsID, fh);
        ok = ok && ReadByte(&DiskfontBase->dsh, &fe2.SupportedStyles, fh);
        ok = ok && ReadByte(&DiskfontBase->dsh, &fe2.FontStyle, fh);
	ok = ok && ReadLong(&DiskfontBase->dsh, &fe2.Numentries, fh);

	if (ok)
	{
	    attrs = AllocVec(fe2.Numentries * sizeof(struct TTextAttr), MEMF_ANY|MEMF_CLEAR);

	    ok = ok && attrs != NULL;

	    if (ok)
		for (i = 0; i < fe2.Numentries; i++)
		    attrs[i].tta_Tags = NULL;

	    for (i = 0, totnumtags = 0; ok && i < fe2.Numentries; i++)
	    {
		ok = ok && ReadWord(&DiskfontBase->dsh, &attrs[i].tta_YSize, fh);
		ok = ok && ReadByte(&DiskfontBase->dsh, &attrs[i].tta_Style, fh);
		ok = ok && ReadByte(&DiskfontBase->dsh, &attrs[i].tta_Flags, fh);
		if (ok)
		{
		    attrs[i].tta_Tags = ReadTagsNum(fh, &numtags, DiskfontBase);
		    D(bug("StreamInFileList: read tags %p\n", attrs[i].tta_Tags));
		    totnumtags += numtags;
		    ok = ok && numtags > 0;
		}
	    }
	    D(bug("StreamInFileList: totnumtags == %u\n", totnumtags));

	    if (ok)
	    {
		ULONG size = sizeof(struct FileEntry) +
		    strlen(fe2.FileName)+1 +
		    fe2.Numentries * sizeof(struct TTextAttr) +
		    totnumtags * sizeof(struct TagItem);
		
		fe = (struct FileEntry *)AllocVec(size, MEMF_ANY);
		ok = ok && fe != NULL;
	    }
	    if (ok)
	    {
		fe->DirEntry = direntry;
		fe->FileName = (STRPTR)(fe+1);
		strcpy(fe->FileName, fe2.FileName);
		fe->FileChanged = fe2.FileChanged;
		fe->Numentries = fe2.Numentries;
		fe->Attrs = (struct TTextAttr *)(fe->FileName + strlen(fe2.FileName)+1);

		tagptr = (struct TagItem *)(fe->Attrs + fe2.Numentries);
		for (i = 0; i < fe2.Numentries; i++)
		{
		    fe->Attrs[i].tta_Name = fe->FileName;
		    fe->Attrs[i].tta_YSize = attrs[i].tta_YSize;
		    fe->Attrs[i].tta_Style = attrs[i].tta_Style;
		    fe->Attrs[i].tta_Flags = attrs[i].tta_Flags;
		    numtags = CopyTagItems(tagptr, attrs[i].tta_Tags, DiskfontBase);
		    fe->Attrs[i].tta_Tags = tagptr;
		    tagptr += numtags;
		}
		ADDTAIL(&direntry->FileList, fe);
	    }

	    if (attrs != NULL)
	    {
		for (i = 0; i < fe2.Numentries; i++)
		    if (attrs[i].tta_Tags != NULL)
		    {
			D(bug("StreamInFileList: freeing tags %p\n", attrs[i].tta_Tags));
			FreeVec(attrs[i].tta_Tags);
		    }
		FreeVec(attrs);
	    }
	}
	
	FreeVec(fe2.FileName);
    }
    if (ok)
    {
	D(bug("StreamInFileList: FileName empty\n"));
	FreeVec(fe2.FileName);
    }
    else
    {
	D(bug("StreamInFileList: Error reading chachefile\n"));
	FreeFileList(&direntry->FileList, DiskfontBase);
    }
    
    return ok;
}
    
/****************************************************************************************/

/****************/
/* FreeDirEntry */
/****************/

/****************************************************************************************/

STATIC VOID FreeDirEntry(struct DirEntry *direntry, struct DiskfontBase_intern *DiskfontBase)
{
    if (direntry!=NULL)
    {
	FreeFileList(&direntry->FileList, DiskfontBase);
	UnLock(direntry->DirLock);
	FreeVec(direntry);
    }
}

/****************************************************************************************/

/****************/
/* ReadDirEntry */
/****************/

/****************************************************************************************/

STATIC struct DirEntry *ReadDirEntry(BPTR dirlock, struct DirEntry *direntry, struct DiskfontBase_intern *DiskfontBase)
{
    struct Process *Self;
    APTR oldwinptr;
    struct FileInfoBlock *fib;
    BPTR olddir;
    struct FileHandle *fh;

    D(bug("ReadDirEntry(dirlock=0x%lx, direntry=0x%lx)\n", dirlock, direntry));
    
    Self               = (struct Process *) FindTask(NULL);
    oldwinptr          = Self->pr_WindowPtr;
    Self->pr_WindowPtr = (APTR) -1;

    fib = AllocDosObject(DOS_FIB, NULL);
    if (fib == NULL)
    {
	D(bug("ReadDirEntry: Could not allocate DosObject\n"));
	Self->pr_WindowPtr = oldwinptr;
	ReturnPtr("ReadDirEntry", struct DirEntry *, NULL);
    }
    Examine(dirlock, fib);
    Self->pr_WindowPtr = oldwinptr;

    olddir = CurrentDir(dirlock);
    
    if (direntry == NULL)
    {
	direntry = AllocVec(sizeof(struct DirEntry), MEMF_ANY | MEMF_CLEAR);
	if (direntry==NULL)
	{
	    D(bug("ReadDirEntry: Could not allocate DirEntry\n"));
	    FreeDosObject(DOS_FIB, fib);
	    CurrentDir(olddir);
	    ReturnPtr("ReadDirEntry", struct DirEntry *, NULL);
	}
	else
	    D(bug("ReadDirEntry: allocated direntry = 0x%lx\n", direntry));

	direntry->DirLock  = dirlock;
	NEWLIST(&direntry->FileList);

	/* Try to read the direntry from the file */
	fh = Open(CACHE_FILE, MODE_OLDFILE);
	if (fh != NULL)
	{
	    BOOL ok;
	    
	    ok = ReadLong(&DiskfontBase->dsh, &direntry->DirChanged.ds_Days, fh);
	    ok = ok && ReadLong(&DiskfontBase->dsh, &direntry->DirChanged.ds_Minute, fh);
	    ok = ok && ReadLong(&DiskfontBase->dsh, &direntry->DirChanged.ds_Tick, fh);

	    if (ok)
		ok = StreamInFileList(direntry, fh, DiskfontBase);

	    Close(fh);

#warning CHECKME
#if 0
	    /* This part is disabled because in emul_handler on UNIX the date
	     * of a parent directory is changed only when a file is added or
	     * removed from the directory
	     */
	    
	    /* If dates are the same return the direntry as is */
	    if (ok && CompareDates(&direntry->DirChanged, &fib->fib_Date) == 0)
	    {
		D(bug("ReadDirEntry: date in cache not changed\n"));
		FreeDosObject(DOS_FIB, fib);
		Self->pr_WindowPtr = oldwinptr;
		CurrentDir(olddir);
		ReturnPtr("ReadDirEntry", struct DirEntry *, direntry);
	    }
#endif
	}
    }
    else
    {
	CurrentDir(direntry->DirLock);
	UnLock(dirlock);
	if (CompareDates(&direntry->DirChanged, &fib->fib_Date) == 0)
	{
	    D(bug("ReadDirEntry: direntry 0x%lx not changed\n", direntry));
	    FreeDosObject(DOS_FIB, fib);
	    Self->pr_WindowPtr = oldwinptr;
	    CurrentDir(olddir);
	    ReturnPtr("ReadDirEntry", struct DirEntry *, direntry);
	}
    }

    direntry->DirChanged = fib->fib_Date;

    if (!GetFileList(direntry, DiskfontBase))
    {
	D(bug("ReadDirEntry: Error reading FileList\n"));
	FreeDosObject(DOS_FIB, fib);
	CurrentDir(olddir);
	UnLock(direntry->DirLock);
	FreeVec(direntry);
	Self->pr_WindowPtr = oldwinptr;
	ReturnPtr("ReadDirEntry", struct DirEntry *, NULL);
    }

    /* If everything went OK Write the cache file */
    fh = Open(CACHE_FILE, MODE_NEWFILE);
    if (fh != NULL)
    {
	BOOL ok;
	
	ok = WriteLong(&DiskfontBase->dsh, fib->fib_Date.ds_Days, fh);
	ok = ok && WriteLong(&DiskfontBase->dsh, fib->fib_Date.ds_Minute, fh);
	ok = ok && WriteLong(&DiskfontBase->dsh, fib->fib_Date.ds_Tick, fh);
	
	if (ok)
	    ok = StreamOutFileList(&direntry->FileList, fh, DiskfontBase);
	
	Close(fh);
	
	if (ok)
	{
	    Examine(direntry->DirLock, fib);
	    direntry->DirChanged = fib->fib_Date;
	    fh = Open(CACHE_FILE, MODE_OLDFILE);
	    if (fh != NULL)
	    {
		Seek(fh, 0, OFFSET_BEGINNING);
		ok = WriteLong(&DiskfontBase->dsh, fib->fib_Date.ds_Days, fh);
		ok = ok && WriteLong(&DiskfontBase->dsh, fib->fib_Date.ds_Minute, fh);
		ok = ok && WriteLong(&DiskfontBase->dsh, fib->fib_Date.ds_Tick, fh);
		Close(fh);
	    }
	}

	if (!ok)
	    DeleteFile(CACHE_FILE);
    }

    FreeDosObject(DOS_FIB, fib);
    CurrentDir(olddir);
    Self->pr_WindowPtr = oldwinptr;
    
    ReturnPtr("ReadDirEntry", struct DirEntry *, direntry);
}

/****************************************************************************************/

/****************/
/* FreeResouces */
/****************/

/****************************************************************************************/

STATIC VOID FreeResources(struct DFData *dfdata, struct DiskfontBase_intern *DiskfontBase)
{
    D(bug("FreeResources(dfdata=%p)\n", dfdata));

    ReleaseSemaphore(&DiskfontBase->fontssemaphore);

#ifdef PROGDIRFONTSDIR
    FreeDirEntry(dfdata->ProgdirDirEntry, DiskfontBase);
#endif
    
    FreeMem(dfdata, sizeof (struct DFData));
  
    ReturnVoid("FreeResources");
}

/****************************************************************************************/

/****************************/
/* CleanUpFontsDirEntryList */
/****************************/

/****************************************************************************************/

VOID CleanUpFontsDirEntryList(struct DiskfontBase_intern *DiskfontBase)
{
    struct DirEntry *direntry, *direntry2;
    
    ForeachNodeSafe(&DiskfontBase->fontsdirentrylist, direntry, direntry2)
	FreeDirEntry(direntry, DiskfontBase);

    ReturnVoid("CleanUpFontsDirEntryList");
}

/****************************************************************************************/

/******************/
/* AllocResources */
/******************/

/****************************************************************************************/

STATIC struct DFData *AllocResources(struct DiskfontBase_intern *DiskfontBase)
{
    struct DFData *dfdata;
    
    D(bug("AllocResources(void)\n"));

    ObtainSemaphore(&DiskfontBase->fontssemaphore);
    
    /* Allocate user data */
    if ((dfdata = AllocMem( sizeof (struct DFData), MEMF_ANY | MEMF_CLEAR)))
    {
	struct DevProc *dp = NULL;
	struct MinList newdirlist;

#ifdef PROGDIRFONTSDIR
	do
	{
	    struct Process *Self;
	    APTR oldwinptr;
	    BPTR lock;
	    
	    dfdata->ProgdirDirEntry = NULL;
	    
	    if (!GetProgramDir())
		break;

	    Self               = (struct Process *) FindTask(NULL);
	    oldwinptr          = Self->pr_WindowPtr;
	    Self->pr_WindowPtr = (APTR) -1;
	    lock               = Lock(PROGDIRFONTSDIR, ACCESS_READ);
	    Self->pr_WindowPtr = oldwinptr;

	    D(bug("AllocResources: PROGDIR:Fonts DirLock = 0x%lx\n", lock));
	    
	    if (!lock)
		break;

	    dfdata->ProgdirDirEntry = ReadDirEntry(lock, NULL, DiskfontBase);
	    D(bug("AllocResources: PROGDIR:Fonts direntry 0x%lx\n", dfdata->ProgdirDirEntry));
	} while (0);
#endif

	NEWLIST(&newdirlist);
	while((dp = GetDeviceProc(FONTSDIR, dp))!=NULL)
	{
	    struct DirEntry *direntry, *direntry2;
	    BPTR lock;
	    
	    D(bug("AllocResources: FONTS: lock = 0x%lx\n", dp->dvp_Lock));
	    
	    lock = DupLock(dp->dvp_Lock);
	    if (lock==NULL)
	    {
		D(bug("AllocResources: Could not duplicate lock\n"));
		continue;
	    }

	    /* See if direntry is already in memory */
	    for (direntry = (struct DirEntry *)GetHead(&DiskfontBase->fontsdirentrylist);
		 direntry != NULL;
		 direntry = (struct DirEntry *)GetSucc(direntry))
	    {
		if (SameLock(direntry->DirLock, lock) == LOCK_SAME)
		{
		    REMOVE(direntry);
		    break;
		}
	    }

	    /* Read or update fonts information in this directory */
	    direntry = ReadDirEntry(lock, direntry, DiskfontBase);
	    if (direntry!=NULL)
	    {
		D(bug("AllocResources: addtail direntry 0x%lx\n", direntry));
		D(bug("AllocResources: first FileEntry: %p\n", GetHead(&direntry->FileList)));
		ADDTAIL(&newdirlist, direntry);
	    }
	    else
		D(bug("AllocResources: Error reading DirEntry\n"));
	    
	    /* Clean up directory lists that are in memory but not in the
	     * FONTS: assign anymore */
	    ForeachNodeSafe(&DiskfontBase->fontsdirentrylist, direntry, direntry2)
	    {
		REMOVE(direntry);
		FreeDirEntry(direntry, DiskfontBase);
	    }
	    
	    ForeachNodeSafe(&newdirlist, direntry, direntry2)
	    {
		REMOVE(direntry);
		ADDTAIL(&DiskfontBase->fontsdirentrylist, direntry);
	    }
	}
    }
	
    ReturnPtr("AllocResources", struct DFData *, dfdata);
}

/****************************************************************************************/

/*******************/
/* DF_IteratorInit */
/*******************/

/****************************************************************************************/

APTR DF_IteratorInit(struct DiskfontBase_intern *DiskfontBase)
{
    struct DFData *dfdata;

    D(bug("DF_IteratorInit()\n"));
    
    dfdata = AllocResources(DiskfontBase);
    if (dfdata == NULL)
    {
	D(bug("DF_IteratorInit: Error executing Allocresources\n"));
	ReturnPtr("DF_IteratorInit", APTR, NULL);
    }

    dfdata->CurrentDirEntry = NULL;
    dfdata->CurrentFileEntry = NULL;
    
#ifdef PROGDIRFONTSDIR
    if (dfdata->ProgdirDirEntry != NULL)
    {
	D(bug("DF_IteratorInit: ProgdirDirEntry found\n"));
	dfdata->CurrentDirEntry = dfdata->ProgdirDirEntry;
	dfdata->CurrentFileEntry = (struct FileEntry *)GetHead(dfdata->ProgdirDirEntry);
    }
    else
	D(bug("DF_IteratorInit: No ProgdirEntry found\n"));
#endif
    if (dfdata->CurrentDirEntry == NULL || dfdata->CurrentFileEntry == NULL)
    {
	dfdata->CurrentDirEntry = (struct DirEntry *)GetHead(&DiskfontBase->fontsdirentrylist);
	if (dfdata->CurrentDirEntry != NULL)
	    dfdata->CurrentFileEntry = (struct FileEntry *)GetHead(&dfdata->CurrentDirEntry->FileList);
    }

    D(bug("DF_IteratorInit: CurrentDirEntry: %p CurrentFileEntry: %p\n",
	  dfdata->CurrentDirEntry, dfdata->CurrentFileEntry));
    
    /* If DirEntry was empty search for one that is not empty */
    while (dfdata->CurrentDirEntry != NULL && dfdata->CurrentFileEntry == NULL)
    {
	dfdata->CurrentDirEntry = (struct DirEntry *)GetSucc(dfdata->CurrentDirEntry);
	if (dfdata->CurrentDirEntry != NULL)
	    dfdata->CurrentFileEntry = (struct FileEntry *)GetHead(dfdata->CurrentDirEntry);

	D(bug("DF_IteratorInit: CurrentDirEntry: %p CurrentFileEntry: %p\n",
	      dfdata->CurrentDirEntry, dfdata->CurrentFileEntry));
    }
    dfdata->AttrsIndex = 0;

    ReturnPtr("DF_IteratorInit", APTR, dfdata);
}

/****************************************************************************************/

/**********************/
/* DF_IteratorGetNext */
/**********************/

/****************************************************************************************/

struct TTextAttr *DF_IteratorGetNext(APTR iterator, struct TTextAttr *reqattr, struct DiskfontBase_intern *DiskfontBase)
{
    struct TTextAttr *retval;
    struct DFData *dfdata = (struct DFData *)iterator;

    D(bug("DF_IteratorGetNext(iterator=0x%lx)\n", iterator));
    
    if (dfdata==NULL || dfdata->CurrentDirEntry==NULL)
	ReturnPtr("DF_IteratorGetNext", struct TTextAttr *, NULL);
    
    retval = dfdata->CurrentFileEntry->Attrs + dfdata->AttrsIndex;
    if (dfdata->CurrentFileEntry->ContentsID == OFCH_ID
	&& reqattr != NULL
	&& dfdata->AttrsIndex == dfdata->CurrentFileEntry->Numentries-1)
    {
        /* The last attr for a outline font is filled with values matching
	 * as close as possible the reqattr */
        retval->tta_YSize = reqattr->tta_YSize;
        retval->tta_Style = dfdata->CurrentFileEntry->FontStyle;
        
        retval->tta_Tags  = NULL;
       
        if (reqattr->tta_Style & FSF_TAGGED)
	{
	     retval->tta_Style |= FSF_TAGGED;
	     retval->tta_Tags  = reqattr->tta_Tags;
	}

        if ((reqattr->tta_Style & FSF_BOLD)
	    && !(retval->tta_Style & FSF_BOLD)
	    && (dfdata->CurrentFileEntry->SupportedStyles & FSF_BOLD))
	{
	    retval->tta_Style |= FSF_BOLD;
	}
       
        if ((reqattr->tta_Style & FSF_ITALIC)
	    && !(retval->tta_Style & FSF_ITALIC)
	    && (dfdata->CurrentFileEntry->SupportedStyles & FSF_ITALIC))
	{
	    retval->tta_Style |= FSF_ITALIC;
	}
    }
       
    /* Let the iterator point to the next attr */
    if ((dfdata->AttrsIndex == dfdata->CurrentFileEntry->Numentries-1)
	|| (dfdata->CurrentFileEntry->ContentsID == OFCH_ID && reqattr==NULL && dfdata->AttrsIndex == dfdata->CurrentFileEntry->Numentries-1))
    {
	dfdata->PrevFileEntry = dfdata->CurrentFileEntry;
	dfdata->CurrentFileEntry = (struct FileEntry *)GetSucc(dfdata->CurrentFileEntry);
	dfdata->AttrsIndex = 0;
	if (dfdata->CurrentFileEntry == NULL)
	{
#ifdef PROGDIRFONTSDIR
	    if (dfdata->CurrentDirEntry == dfdata->ProgdirDirEntry)
		dfdata->CurrentDirEntry = (struct DirEntry *)GetHead(&DiskfontBase->fontsdirentrylist);
	    else
		dfdata->CurrentDirEntry = (struct DirEntry *)GetSucc(dfdata->CurrentDirEntry);
#else
	    dfdata->CurrentDirEntry = (struct DirEntry *)GetSucc(dfdata->CurrentDirEntry);
#endif
	    if (dfdata->CurrentDirEntry != NULL)
		dfdata->CurrentFileEntry = (struct FileEntry *)GetHead(&dfdata->CurrentDirEntry->FileList);
	}
    }
    else
	dfdata->AttrsIndex++;

    ReturnPtr("DF_IteratorGetNext", struct TTextAttr *, retval);
}

/****************************************************************************************/

/***********************/
/* DF_IteratorRemember */
/***********************/

/****************************************************************************************/

VOID DF_IteratorRemember(APTR iterator, struct DiskfontBase_intern *DiskfontBase)
{
    struct DFData *dfdata = (struct DFData *)iterator;

    D(bug("DF_IteratorRemember(iterator=0x%lx)\n", iterator));
    
    if (dfdata->AttrsIndex > 0)
    {
	dfdata->RememberIndex = dfdata->AttrsIndex-1;
	dfdata->RememberFileEntry = dfdata->CurrentFileEntry;
    }
    else
    {
	dfdata->RememberFileEntry = dfdata->PrevFileEntry;
	dfdata->RememberIndex = dfdata->RememberFileEntry->Numentries-1;
    }
    
    D(bug("DF_IteratorRemember: Remembered font: %s\n", dfdata->RememberFileEntry->Attrs[dfdata->RememberIndex].tta_Name));
}

/****************************************************************************************/

/***************************/
/* DF_IteratorRememberOpen */
/***************************/

/****************************************************************************************/

struct TextFont *DF_IteratorRememberOpen(APTR iterator, struct TTextAttr *tattr, struct DiskfontBase_intern *DiskfontBase)
{
    struct DFData *dfdata = (struct DFData *)iterator;
    struct TextFont *tf = NULL;
    BPTR olddir;
    struct FontDescrHeader *fdh;

    D(bug("DF_IteratorRememberOpen(iterator=0x%lx, tattr=0x%lx)\n", iterator, tattr));
    
    olddir = CurrentDir(dfdata->RememberFileEntry->DirEntry->DirLock);

    fdh = ReadFontDescr(dfdata->RememberFileEntry->FileName, DiskfontBase);
    
    if (fdh != NULL)
    {
	D(bug("DF_IteratorRememberOpen: Font Description read\n"));
	
	if (IS_OUTLINE_FONT(&dfdata->RememberFileEntry->Attrs[dfdata->RememberIndex]))
	{
	    D(bug("DF_IteratorRememberOpen: loading outline font\n"));
	
	    tf = OTAG_ReadOutlineFont(&fdh->TAttrArray[dfdata->RememberIndex],
				      tattr,
				      fdh->OTagList,
				      DiskfontBase);
	    D(bug("DF_IteratorRememberOpen: tf=0x%lx\n", tf));
	}
	else
	{
	    D(bug("DF_IteratorRememberOpen: loading bitmap font\n"));
	    
	    tf = ReadDiskFont(&fdh->TAttrArray[dfdata->RememberIndex],
			      tattr->tta_Name,
			      DiskfontBase);

	    D(bug("DF_IteratorRememberOpen: tf=0x%lx\n", tf));
	}

        FreeFontDescr(fdh, DiskfontBase);
    }
    else
	D(bug("DF_IteratorRememberOpen: Font Description read failed\n"));
    
    CurrentDir(olddir);

    if (tf != NULL)
    {
	struct DiskFontHeader *dfh;
	
	/* PPaint's personal.font/8 has not set FPF_DISKFONT,
	 (FPF_ROMFONT neither), but AmigaOS diskfont.library
	 still shows FPF_DISKFONT set when opening this font */
		   
	tf->tf_Flags &= ~FPF_ROMFONT;
	tf->tf_Flags |= FPF_DISKFONT;
		
	D(bug("Adding font: %p\n", tf));
		
	/* Forbid() must be called before AddFont, because AddFont clears
	 tf_Accessors and in the worst case it could happen to us that
	 after the AddFont() another task opens and closes/frees the
	 diskfont, before we manage to increase tf_Accessors. */
		   
	Forbid();

	AddFont(tf);				
	tf->tf_Accessors++;

	dfh = (struct DiskFontHeader *)((UBYTE *)(tf) - (LONG)OFFSET(DiskFontHeader, dfh_TF));

	/* Paranoia check */
	if (dfh->dfh_FileID == DFH_ID)
	    ADDTAIL(&DiskfontBase->diskfontlist, &dfh->dfh_DF);

	Permit();
	
	D(bug("Font added\n"));
    }

    return tf;
}

/****************************************************************************************/

/*******************/
/* DF_IteratorFree */
/*******************/

/****************************************************************************************/

VOID DF_IteratorFree(APTR iterator, struct DiskfontBase_intern *DiskfontBase)
{
    struct DFData *dfdata = (struct DFData *)iterator;
    
    FreeResources(dfdata, DiskfontBase);
}

/****************************************************************************************/

/*******************/
/* DF_OpenFontPath */
/*******************/

/****************************************************************************************/

struct TextFont *DF_OpenFontPath(struct TextAttr *reqattr, struct DiskfontBase_intern *DiskfontBase)
{
    struct TextFont *tf = NULL;
    struct FontDescrHeader *fdh;

    D(bug("DF_OpenFontPath(reqattr=0x%lx)\n", reqattr));
    
    fdh = ReadFontDescr(reqattr->ta_Name, DiskfontBase);
    
    if (fdh != NULL)
    {
        WORD max_match_weight = 0, match_weight;
        LONG match_index = -1;
        int i;
        
	D(bug("DF_OpenFontPath: Font Description read\n"));
       
        for (i=0; i>fdh->NumEntries; i++)
	{
	    match_weight = WeighTAMatch((struct TextAttr *)reqattr,
					(struct TextAttr *)&fdh->TAttrArray[i],
					fdh->TAttrArray[i].tta_Tags);
	   
	    if (match_weight > max_match_weight)
	    {
	        max_match_weight = match_weight;
	        match_index = i;
	    }
	}
       
        if (match_index >= 0)
	{
	   if (IS_OUTLINE_FONT(&fdh->TAttrArray[match_index]))
	   {
	       D(bug("DF_OpenFontPath: loading outline font\n"));
	
	       tf = OTAG_ReadOutlineFont(&fdh->TAttrArray[match_index],
					 (struct TTextAttr *)reqattr,
					 fdh->OTagList,
					 DiskfontBase);
	       D(bug("DF_OpenFontPath: tf=0x%lx\n", tf));

	   }
	   else
	   {
	       D(bug("DF_OpenFontPath: loading bitmap font\n"));
	    
	       tf = ReadDiskFont(&fdh->TAttrArray[match_index],
				 reqattr->ta_Name,
				 DiskfontBase);

	       D(bug("DF_OpenFontPath: tf=0x%lx\n", tf));
	   }
	}
       
        FreeFontDescr(fdh, DiskfontBase);
    }
    else
        D(bug("DF_OpenFontPath: Font Description read failed\n"));
    
    if (tf != NULL)
    {
	struct DiskFontHeader *dfh;
	
	/* PPaint's personal.font/8 has not set FPF_DISKFONT,
	 (FPF_ROMFONT neither), but AmigaOS diskfont.library
	 still shows FPF_DISKFONT set when opening this font */
		   
	tf->tf_Flags &= ~FPF_ROMFONT;
	tf->tf_Flags |= FPF_DISKFONT;
		
	D(bug("Adding font: %p\n", tf));
		
	/* Forbid() must be called before AddFont, because AddFont clears
	 tf_Accessors and in the worst case it could happen to us that
	 after the AddFont() another task opens and closes/frees the
	 diskfont, before we manage to increase tf_Accessors. */
		   
	Forbid();

	AddFont(tf);				
	tf->tf_Accessors++;

	dfh = (struct DiskFontHeader *)((UBYTE *)(tf) - (LONG)OFFSET(DiskFontHeader, dfh_TF));

	/* Paranoia check */
	if (dfh->dfh_FileID == DFH_ID)
	    ADDTAIL(&DiskfontBase->diskfontlist, &dfh->dfh_DF);

	Permit();
	
	D(bug("Font added\n"));
    }

    return tf;
}

