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

struct DF_FontsData /*DiskFontData */
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

struct DF_FileData
{
    struct FontDescrHeader *FDH;
    STRPTR                  FilePart;
    STRPTR                  OrigName;
    struct TTextAttr       *LastAttr;
    UWORD                   AttrsIndex;
    UWORD                   RememberIndex;
    struct TTextAttr        ExtraAttr;
};

typedef enum {DF_FONTSDATA, DF_FILEDATA} DF_DataType;

struct DF_Data
{
    DF_DataType       Type;
    struct TTextAttr *ReqAttr;
    union
    {
	struct DF_FontsData FontsData;
	struct DF_FileData  FileData;
    } u;
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
		fe->ContentsID = fe2.ContentsID;
		fe->SupportedStyles = fe2.SupportedStyles;
		fe->FontStyle = fe2.FontStyle;
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

STATIC VOID FreeResources(struct DF_Data *df_data, struct DiskfontBase_intern *DiskfontBase)
{
    D(bug("FreeResources(df_data=%p)\n", df_data));

    ReleaseSemaphore(&DiskfontBase->fontssemaphore);

    switch (df_data->Type)
    {
    case DF_FONTSDATA:
#ifdef PROGDIRFONTSDIR
	FreeDirEntry(df_data->u.FontsData.ProgdirDirEntry, DiskfontBase);
#endif
	break;
	
    case DF_FILEDATA:
	if (df_data->u.FileData.FDH != NULL)
	    FreeFontDescr(df_data->u.FileData.FDH, DiskfontBase);
	break;
    }
    
    FreeMem(df_data, sizeof (struct DF_Data));
  
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

STATIC struct DF_Data *AllocResources(struct TTextAttr *reqattr, struct DiskfontBase_intern *DiskfontBase)
{
    struct DF_Data *df_data;
    
    D(bug("AllocResources(void)\n"));

    ObtainSemaphore(&DiskfontBase->fontssemaphore);
    
    /* Allocate user data */
    if ((df_data = AllocMem( sizeof (struct DF_Data), MEMF_ANY | MEMF_CLEAR)))
    {
	df_data->ReqAttr = reqattr;
	
	if (reqattr==NULL || FilePart(reqattr->tta_Name)==reqattr->tta_Name)
	{
	    struct DevProc *dp = NULL, *dp2;
	    struct MinList newdirlist;

	    df_data->Type = DF_FONTSDATA;
	    
#ifdef PROGDIRFONTSDIR
	    do
	    {
		struct Process *Self;
		APTR oldwinptr;
		BPTR lock;
	    
		df_data->u.FontsData.ProgdirDirEntry = NULL;
	    
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

		df_data->u.FontsData.ProgdirDirEntry = ReadDirEntry(lock, NULL, DiskfontBase);
		D(bug("AllocResources: PROGDIR:Fonts direntry 0x%lx\n", df_data->u.FontsData.ProgdirDirEntry));
	    } while (0);
#endif

	    NEWLIST(&newdirlist);
	    while((dp2 = GetDeviceProc(FONTSDIR, dp))!=NULL)
	    {
		struct DirEntry *direntry, *direntry2;
		BPTR lock;
	    
		D(bug("AllocResources: FONTS: lock = 0x%lx\n", dp2->dvp_Lock));
	    
		lock = DupLock(dp2->dvp_Lock);
		if (lock==NULL)
		{
		    D(bug("AllocResources: Could not duplicate lock\n"));
		    dp = dp2;
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
		dp = dp2;
	    }
	    FreeDeviceProc(dp);
	}
	else
	{
	    struct FontDescrHeader *fdh;
	    
	    df_data->Type = DF_FILEDATA;
	    fdh = ReadFontDescr(reqattr->tta_Name, DiskfontBase);
	    df_data->u.FileData.FDH = fdh;
	    
	    if (fdh != NULL);
	    {
		if (fdh->ContentsID==OFCH_ID)
		{
		    UBYTE SupportedStyles = OTAG_GetSupportedStyles(fdh->OTagList, DiskfontBase);

		    df_data->u.FileData.ExtraAttr.tta_Name = reqattr->tta_Name;
		    df_data->u.FileData.ExtraAttr.tta_YSize = reqattr->tta_YSize;
		    df_data->u.FileData.ExtraAttr.tta_Style = OTAG_GetFontStyle(fdh->OTagList, DiskfontBase);
		    df_data->u.FileData.ExtraAttr.tta_Flags = OTAG_GetFontFlags(fdh->OTagList, DiskfontBase);

		    df_data->u.FileData.ExtraAttr.tta_Tags = NULL;
		    if (reqattr->tta_Style & FSF_TAGGED)
		    {
			df_data->u.FileData.ExtraAttr.tta_Style |= FSF_TAGGED;
			df_data->u.FileData.ExtraAttr.tta_Tags = reqattr->tta_Tags;
		    }
			
		    if ((reqattr->tta_Style & FSF_BOLD)
			&& !(df_data->u.FileData.ExtraAttr.tta_Style & FSF_BOLD)
			&& (SupportedStyles & FSF_BOLD))
		    {
			df_data->u.FileData.ExtraAttr.tta_Style |= FSF_BOLD;
		    }
			
		    if ((reqattr->tta_Style & FSF_ITALIC)
			&& !(df_data->u.FileData.ExtraAttr.tta_Style & FSF_ITALIC)
			&& (SupportedStyles & FSF_ITALIC))
		    {
			df_data->u.FileData.ExtraAttr.tta_Style |= FSF_ITALIC;
		    }
		}
	    }
	}
    }
    
    ReturnPtr("AllocResources", struct DF_Data *, df_data);
}

/****************************************************************************************/

/*******************/
/* DF_IteratorInit */
/*******************/

/****************************************************************************************/

APTR DF_IteratorInit(struct TTextAttr *reqattr, struct DiskfontBase_intern *DiskfontBase)
{
    struct DF_Data *df_data;

    D(bug("DF_IteratorInit(reqattr=0x%lx)\n", reqattr));
    
    df_data = AllocResources(reqattr, DiskfontBase);
    if (df_data == NULL)
    {
	D(bug("DF_IteratorInit: Error executing Allocresources\n"));
	ReturnPtr("DF_IteratorInit", APTR, NULL);
    }

    switch (df_data->Type)
    {
    case DF_FONTSDATA:
	df_data->u.FontsData.CurrentDirEntry = NULL;
	df_data->u.FontsData.CurrentFileEntry = NULL;
    
#ifdef PROGDIRFONTSDIR
	if (df_data->u.FontsData.ProgdirDirEntry != NULL)
	{
	    D(bug("DF_IteratorInit: ProgdirDirEntry found\n"));
	    df_data->u.FontsData.CurrentDirEntry = df_data->u.FontsData.ProgdirDirEntry;
	    df_data->u.FontsData.CurrentFileEntry = (struct FileEntry *)GetHead(df_data->u.FontsData.ProgdirDirEntry);
	}
	else
	    D(bug("DF_IteratorInit: No ProgdirEntry found\n"));
#endif
	if (df_data->u.FontsData.CurrentDirEntry == NULL || df_data->u.FontsData.CurrentFileEntry == NULL)
	{
	    df_data->u.FontsData.CurrentDirEntry = (struct DirEntry *)GetHead(&DiskfontBase->fontsdirentrylist);
	    if (df_data->u.FontsData.CurrentDirEntry != NULL)
		df_data->u.FontsData.CurrentFileEntry = (struct FileEntry *)GetHead(&df_data->u.FontsData.CurrentDirEntry->FileList);
	}

	D(bug("DF_IteratorInit: CurrentDirEntry: %p CurrentFileEntry: %p\n",
	      df_data->u.FontsData.CurrentDirEntry, df_data->u.FontsData.CurrentFileEntry));
    
	/* If DirEntry was empty search for one that is not empty */
	while (df_data->u.FontsData.CurrentDirEntry != NULL && df_data->u.FontsData.CurrentFileEntry == NULL)
	{
	    df_data->u.FontsData.CurrentDirEntry = (struct DirEntry *)GetSucc(df_data->u.FontsData.CurrentDirEntry);
	    if (df_data->u.FontsData.CurrentDirEntry != NULL)
		df_data->u.FontsData.CurrentFileEntry = (struct FileEntry *)GetHead(df_data->u.FontsData.CurrentDirEntry);

	    D(bug("DF_IteratorInit: CurrentDirEntry: %p CurrentFileEntry: %p\n",
		  df_data->u.FontsData.CurrentDirEntry, df_data->u.FontsData.CurrentFileEntry));
	}
	df_data->u.FontsData.AttrsIndex = 0;
	
	break;

    case DF_FILEDATA:
	df_data->u.FileData.AttrsIndex = 0;
	df_data->u.FileData.FilePart = FilePart(df_data->ReqAttr->tta_Name);
	df_data->u.FileData.LastAttr = NULL;
	break;
    }
    
    ReturnPtr("DF_IteratorInit", APTR, df_data);
}

/****************************************************************************************/

/**********************/
/* DF_IteratorGetNext */
/**********************/

/****************************************************************************************/

struct TTextAttr *DF_IteratorGetNext(APTR iterator, struct DiskfontBase_intern *DiskfontBase)
{
    struct TTextAttr *retval;
    struct DF_Data *df_data = (struct DF_Data *)iterator;

    D(bug("DF_IteratorGetNext(iterator=0x%lx)\n", iterator));
    
    if (df_data==NULL)
	ReturnPtr("DF_IteratorGetNext", struct TTextAttr *, NULL);
    
    switch (df_data->Type)
    {
    case DF_FONTSDATA:
	if (df_data->u.FontsData.CurrentDirEntry==NULL)
	    ReturnPtr("DF_IteratorGetNext", struct TTextAttr *, NULL);
	    
	retval = df_data->u.FontsData.CurrentFileEntry->Attrs + df_data->u.FontsData.AttrsIndex;
	D(bug("DF_IteratorGetNext:\n"
	      "  ContentsID: 0x%x == 0x%x\n"
	      "  ReqAttr: %p\n"
	      "  AttrIndex: %d, Numentries: %d\n",
	      df_data->u.FontsData.CurrentFileEntry->ContentsID, OFCH_ID,
	      df_data->ReqAttr,
	      df_data->u.FontsData.AttrsIndex, df_data->u.FontsData.CurrentFileEntry->Numentries));
	
	if (df_data->u.FontsData.CurrentFileEntry->ContentsID == OFCH_ID
	    && df_data->ReqAttr != NULL
	    && df_data->u.FontsData.AttrsIndex == df_data->u.FontsData.CurrentFileEntry->Numentries-1)
	{
	    D(bug("DF_IteratorGetNext: Setting last outline element\n"));
	    
	    /* The last attr for a outline font is filled with values matching
	     * as close as possible the reqattr */
	    retval->tta_YSize = df_data->ReqAttr->tta_YSize;
	    retval->tta_Style = df_data->u.FontsData.CurrentFileEntry->FontStyle;
	    
	    retval->tta_Tags  = NULL;
       
	    if (df_data->ReqAttr->tta_Style & FSF_TAGGED)
	    {
		retval->tta_Style |= FSF_TAGGED;
		retval->tta_Tags  = df_data->ReqAttr->tta_Tags;
	    }

	    if ((df_data->ReqAttr->tta_Style & FSF_BOLD)
		&& !(retval->tta_Style & FSF_BOLD)
		&& (df_data->u.FontsData.CurrentFileEntry->SupportedStyles & FSF_BOLD))
	    {
		retval->tta_Style |= FSF_BOLD;
	    }
       
	    if ((df_data->ReqAttr->tta_Style & FSF_ITALIC)
		&& !(retval->tta_Style & FSF_ITALIC)
		&& (df_data->u.FontsData.CurrentFileEntry->SupportedStyles & FSF_ITALIC))
	    {
		retval->tta_Style |= FSF_ITALIC;
	    }
	}
       
	/* Let the iterator point to the next attr */
	if ((df_data->u.FontsData.AttrsIndex == df_data->u.FontsData.CurrentFileEntry->Numentries-1)
	    || (df_data->u.FontsData.CurrentFileEntry->ContentsID == OFCH_ID
		&& df_data->ReqAttr==NULL &&
		df_data->u.FontsData.AttrsIndex == df_data->u.FontsData.CurrentFileEntry->Numentries-2
	       )
	   )
	{
	    df_data->u.FontsData.PrevFileEntry = df_data->u.FontsData.CurrentFileEntry;
	    df_data->u.FontsData.CurrentFileEntry = (struct FileEntry *)GetSucc(df_data->u.FontsData.CurrentFileEntry);
	    df_data->u.FontsData.AttrsIndex = 0;
	    if (df_data->u.FontsData.CurrentFileEntry == NULL)
	    {
#ifdef PROGDIRFONTSDIR
		if (df_data->u.FontsData.CurrentDirEntry == df_data->u.FontsData.ProgdirDirEntry)
		    df_data->u.FontsData.CurrentDirEntry = (struct DirEntry *)GetHead(&DiskfontBase->fontsdirentrylist);
		else
		    df_data->u.FontsData.CurrentDirEntry = (struct DirEntry *)GetSucc(df_data->u.FontsData.CurrentDirEntry);
#else
		df_data->u.FontsData.CurrentDirEntry = (struct DirEntry *)GetSucc(df_data->u.FontsData.CurrentDirEntry);
#endif
		if (df_data->u.FontsData.CurrentDirEntry != NULL)
		    df_data->u.FontsData.CurrentFileEntry = (struct FileEntry *)GetHead(&df_data->u.FontsData.CurrentDirEntry->FileList);
	    }
	}
	else
	    df_data->u.FontsData.AttrsIndex++;
	break;
	
    case DF_FILEDATA:
	if (df_data->u.FileData.LastAttr != NULL)
	{
	    df_data->u.FileData.LastAttr->tta_Name = df_data->u.FileData.OrigName;
	    df_data->u.FileData.LastAttr = NULL;
	}
	
	if (df_data->u.FileData.FDH==NULL
	    || df_data->u.FileData.AttrsIndex > df_data->u.FileData.FDH->NumEntries)
	    ReturnPtr("DF_IteratorGetNext", struct TTextAttr *, NULL);

	/* Get the TextAttr the iterator is pointing to
	 * If it points to the element after the last element return
	 * the Extra Attr that matches the outline font as close as possible
	 */
	if (df_data->u.FileData.AttrsIndex == df_data->u.FileData.FDH->NumEntries)
	    retval = &df_data->u.FileData.ExtraAttr;
	else
	{
	    retval = &df_data->u.FileData.FDH->TAttrArray[df_data->u.FileData.AttrsIndex];
	    df_data->u.FileData.OrigName = retval->tta_Name;
	    retval->tta_Name = df_data->u.FileData.FilePart;
	    df_data->u.FileData.LastAttr = retval;
	}
	
	/* Let the iterator point to the next element.
	 */
	df_data->u.FileData.AttrsIndex++;

	/* Do not return the best-match attribute if it is not an outline font,
	 * this is done by increasing the AttrsIndex once more
	 */
	if (df_data->u.FileData.FDH->ContentsID != OFCH_ID
	    && df_data->u.FileData.AttrsIndex == df_data->u.FileData.FDH->NumEntries)
	{
	    df_data->u.FileData.AttrsIndex++;
	}
	break;
    }

    ReturnPtr("DF_IteratorGetNext", struct TTextAttr *, retval);
}

/****************************************************************************************/

/***********************/
/* DF_IteratorRemember */
/***********************/

/****************************************************************************************/

VOID DF_IteratorRemember(APTR iterator, struct DiskfontBase_intern *DiskfontBase)
{
    struct DF_Data *df_data = (struct DF_Data *)iterator;

    D(bug("DF_IteratorRemember(iterator=0x%lx)\n", iterator));

    switch (df_data->Type)
    {
    case DF_FONTSDATA:
	if (df_data->u.FontsData.AttrsIndex > 0)
	{
	    df_data->u.FontsData.RememberIndex = df_data->u.FontsData.AttrsIndex-1;
	    df_data->u.FontsData.RememberFileEntry = df_data->u.FontsData.CurrentFileEntry;
	}
	else
	{
	    df_data->u.FontsData.RememberFileEntry = df_data->u.FontsData.PrevFileEntry;
	    df_data->u.FontsData.RememberIndex = df_data->u.FontsData.RememberFileEntry->Numentries-1;
	    if (df_data->u.FontsData.RememberFileEntry->ContentsID == OFCH_ID
		&& df_data->ReqAttr == NULL
	       )
	    {
		df_data->u.FontsData.RememberIndex--;
	    }
	}
    
	D(bug("DF_IteratorRemember: Remembered font: %s/%d\n",
	      df_data->u.FontsData.RememberFileEntry->Attrs[df_data->u.FontsData.RememberIndex].tta_Name,
	      df_data->u.FontsData.RememberFileEntry->Attrs[df_data->u.FontsData.RememberIndex].tta_YSize));
	break;

    case DF_FILEDATA:
	df_data->u.FileData.RememberIndex = df_data->u.FileData.AttrsIndex-1;

	D(bug("DF_IteratorRemember: Remembered font: %s(%d)\n",
	      df_data->ReqAttr->tta_Name, df_data->u.FileData.RememberIndex));
	break;
    }
}

/****************************************************************************************/

/***************************/
/* DF_IteratorRememberOpen */
/***************************/

/****************************************************************************************/

struct TextFont *DF_IteratorRememberOpen(APTR iterator, struct DiskfontBase_intern *DiskfontBase)
{
    struct DF_Data *df_data = (struct DF_Data *)iterator;
    struct FontDescrHeader *fdh;
    struct TTextAttr *RememberAttr;
    struct TextFont *tf = NULL;
    BPTR olddir, lock, dirlock;

    D(bug("DF_IteratorRememberOpen(iterator=0x%lx)\n", iterator));

    /* Set current dir and get the Remember TextAttr */
    switch (df_data->Type)
    {
    case DF_FONTSDATA:
	{
	    struct FileEntry *rementry = df_data->u.FontsData.RememberFileEntry;
	    
	    olddir = CurrentDir(rementry->DirEntry->DirLock);
	    fdh = ReadFontDescr(rementry->FileName, DiskfontBase);
	    if (rementry->ContentsID == OFCH_ID
		&& df_data->u.FontsData.RememberIndex == rementry->Numentries - 1)
	    {
		/* It is the TAttr generated for best matching */
		RememberAttr = &rementry->Attrs[df_data->u.FontsData.RememberIndex];
	    }
	    else
		RememberAttr = &fdh->TAttrArray[df_data->u.FontsData.RememberIndex];
	}
	break;
	
    case DF_FILEDATA:
	if (df_data->u.FileData.LastAttr != NULL)
	{
	    df_data->u.FileData.LastAttr->tta_Name = df_data->u.FileData.OrigName;
	    df_data->u.FileData.LastAttr = NULL;
	}
	
	RememberAttr = NULL;
	lock = Lock(df_data->ReqAttr->tta_Name, ACCESS_READ);
	if (lock == NULL)
	{
	    D(bug("DF_IteratorRememberOpen: Could not lock file\n"));
	    break;
	}
	
	dirlock = ParentDir(lock);
	UnLock(lock);
	if (dirlock == NULL)
	{
	    D(bug("DF_IteratorRememberOpen: Could not get ParentDir\n"));
	    break;
	}
	olddir = CurrentDir(dirlock);
	
	fdh = df_data->u.FileData.FDH;
	if (df_data->u.FileData.RememberIndex == fdh->NumEntries)
	    RememberAttr = &df_data->u.FileData.ExtraAttr;
	else
	    RememberAttr = &fdh->TAttrArray[df_data->u.FileData.RememberIndex];
	break;
    }

    if (RememberAttr == NULL)
	return NULL;
    
    if (fdh != NULL)
    {
	D(bug("DF_IteratorRememberOpen: Font Description read\n"));
	
	if (IS_OUTLINE_FONT(RememberAttr))
	{
	    D(bug("DF_IteratorRememberOpen: loading outline font\n"));
	
	    tf = OTAG_ReadOutlineFont(RememberAttr,
				      df_data->ReqAttr,
				      fdh->OTagList,
				      DiskfontBase);
	    D(bug("DF_IteratorRememberOpen: tf=0x%lx\n", tf));
	
	}
	else
	{
	    D(bug("DF_IteratorRememberOpen: loading bitmap font\n"));
	
	    tf = ReadDiskFont(RememberAttr,
			      FilePart(df_data->ReqAttr->tta_Name),
			      DiskfontBase);

	    D(bug("DF_IteratorRememberOpen: tf=0x%lx\n", tf));
	}
    }
    else
	D(bug("DF_IteratorRememberOpen: Font Description read failed\n"));

    dirlock = CurrentDir(olddir);
    switch (df_data->Type)
    {
    case DF_FONTSDATA:
	FreeFontDescr(fdh, DiskfontBase);
	break;
	
    case DF_FILEDATA:
	UnLock(dirlock);
	break;
    }

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
    struct DF_Data *df_data = (struct DF_Data *)iterator;

    if (df_data->Type == DF_FILEDATA && df_data->u.FileData.LastAttr != NULL)
	df_data->u.FileData.LastAttr->tta_Name = df_data->u.FileData.OrigName;
    
    FreeResources(df_data, DiskfontBase);
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
    BPTR olddir, lock, dirlock;
    
    D(bug("DF_OpenFontPath(reqattr=0x%lx)\n", reqattr));
    
    lock = Lock(reqattr->ta_Name, ACCESS_READ);
    if (lock == NULL)
    {
	D(bug("DF_OpenFontPath: Could not lock file\n"));
	return NULL;
    }
    
    dirlock = ParentDir(lock);
    UnLock(lock);
    if (dirlock == NULL)
    {
	D(bug("DF_OpenFontPath: Could not get ParentDir\n"));
	return NULL;
    }
    
    olddir = CurrentDir(dirlock);
    
    fdh = ReadFontDescr(reqattr->ta_Name, DiskfontBase);
    
    if (fdh != NULL)
    {
        WORD max_match_weight = 0, match_weight;
        LONG match_index = -1;
        int i;
        
	D(bug("DF_OpenFontPath: Font Description read\n"));
       
        for (i=0; i<fdh->NumEntries; i++)
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
	else
	    D(bug("DF_OpenFontPath: No matching font found\n"));
	    
        FreeFontDescr(fdh, DiskfontBase);
    }
    else
        D(bug("DF_OpenFontPath: Font Description read failed\n"));

    CurrentDir(olddir);
    UnLock(dirlock);
    
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
