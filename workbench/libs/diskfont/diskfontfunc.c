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
 
struct FileEntry
{
    struct FileEntry 	*Next;
    UBYTE 		FileName[1];
};	

struct DirEntry
{
    struct MinNode  	Node;
    struct FileEntry	*FileList;
    BPTR    	    	DirLock;
    BOOL		ProgdirFlag;
};

struct DFHData /*DiskFontHookData */
{
    BPTR                    	DirLock;
    BPTR                    	OldDirLock;
    BOOL			DescrLoaded;
    struct FontDescrHeader  	*CurrentFDH;
    /* Index into the current TTextAttr-array */
    UWORD                   	TTextAttrIndex;
	
    struct MinList  	    	DirList;
    struct DirEntry 	    	*CurrentDirEntry;
    struct FileEntry		*CurrentFileEntry;
    STRPTR  	    	    	CurrentFontDescrName;
};

struct FontPath
{
	CONST_STRPTR	Path;
	BOOL		ProgdirFlag;
};

/****************************************************************************************/

/****************/
/* FreeFileList */
/****************/

/****************************************************************************************/

STATIC VOID FreeFileList(struct FileEntry *feptr, struct DiskfontBase_intern *DiskfontBase)
{
    struct FileEntry *nextfeptr;

    D(bug("FreeFileList(flielist=%p)\n", feptr));

    while ( feptr )
    {
	/* Preserve pointer to next element */
	nextfeptr = feptr->Next;

	/* Free FileEntry */
	FreeVec(feptr);

	feptr = nextfeptr;	
    }

    ReturnVoid("FreeFileList");
}

/****************************************************************************************/

/****************/
/* GetFileList	*/
/****************/

/****************************************************************************************/

/* Build the list of .font file names using ExAll() */
STATIC struct FileEntry *GetFileList(BPTR fontslock, struct DiskfontBase_intern *DiskfontBase)
{	
    struct FileInfoBlock *fib;
    struct FileEntry 	 *felist = NULL, *prevfe = NULL, *fe;
    BPTR    	    	 lock;
    
    D(bug("GetFileList(fontslock=%p)\n", fontslock));

    fib = AllocDosObject(DOS_FIB, NULL);
    if (fib)
    {
    	if ((lock = DupLock(fontslock)))
	{
	    if (Examine(lock, fib) == DOSTRUE)
	    {
	    	while(ExNext(lock, fib))
		{
		    LONG namelen;
		    
		    if (fib->fib_DirEntryType >= 0) continue;
		    
		    namelen = strlen(fib->fib_FileName) + 1;
		    
		    fe = AllocVec(sizeof(struct FileEntry) + namelen, MEMF_ANY);
		    if (!fe)
		    {
		    	SetIoErr(ERROR_NO_FREE_STORE);
			break;
		    }
		    
		    strcpy(fe->FileName, fib->fib_FileName);
		    D(bug("Add to list: \"%s\"\n", fe->FileName));
		    
		    fe->Next = NULL;
		    
		    if (prevfe)
		    {
		    	prevfe->Next = fe;
		    }
		    else
		    {
		    	felist = fe;
		    }
		   		 
		    prevfe = fe;
		    
		} /* while(ExNext(lock, fib)) */
		
		if ((IoErr() != ERROR_NO_MORE_ENTRIES) && (IoErr() != 0))
		{
		    if (felist)
		    {
		    	FreeFileList(felist, DiskfontBase);
			felist = NULL;
		    }
		}
		
	    } /* if (Examine(lock, fib) == DOSTRUE) */
	    UnLock(lock);
	    
	} /* if ((lock = DupLock(fontslock))) */
    	FreeDosObject(DOS_FIB, fib);
	
    } /* if (fib) */
    
    ReturnPtr ("GetFileList", struct FileEntry *, felist);
}

/****************************************************************************************/

/****************/
/* LoadNext     */
/****************/

/****************************************************************************************/

/* Function for loading the next .font file into memory.
Uses the standard fonthook return values (FALSE is a critical error). */
STATIC BOOL LoadNext(struct DFHData *dfhd, struct DiskfontBase_intern *DiskfontBase)
{
    BOOL retval = FALSE;
 
    BOOL done = FALSE;
    
    struct FileEntry *curfile;

    D(bug("LoadNext(dfhdt=%p)\n", dfhd));
        
    curfile = dfhd->CurrentFileEntry;
    
    do
    {
    	if (curfile)
	{
	    CurrentDir(dfhd->CurrentDirEntry->DirLock);
	    	    
     	    if ((dfhd->CurrentFDH = ReadFontDescr(curfile->FileName, dfhd->CurrentDirEntry->ProgdirFlag, DFB(DiskfontBase))) != 0 )
       	    {
	    	dfhd->CurrentFontDescrName = curfile->FileName;
		
       		done    = TRUE;
        	retval  = TRUE;
       	    }
        
            curfile = curfile->Next;
	}

	if (!done) while (!curfile)
	{
    	    dfhd->CurrentDirEntry = (struct DirEntry *)dfhd->CurrentDirEntry->Node.mln_Succ;

	    /* Paranoia check */
	    if (!dfhd->CurrentDirEntry)
	    {
		ReturnInt ("LoadNext", ULONG, FALSE);
	    }

	    /* Needed check */
	    if (!dfhd->CurrentDirEntry->Node.mln_Succ)
	    {
		ReturnInt ("LoadNext", ULONG, FALSE);	
	    }

    	    curfile  = dfhd->CurrentDirEntry->FileList;
	}
	
    } while (curfile && !done);
    
    dfhd->CurrentFileEntry = curfile;
    
    ReturnInt ("LoadNext", ULONG, retval);
}

/****************************************************************************************/

/****************/
/* FreeResouces */
/****************/

/****************************************************************************************/

STATIC VOID FreeResources(struct DFHData *dfhd, struct DiskfontBase_intern *DiskfontBase )
{
    struct DirEntry *direntry, *direntry2;
    
    D(bug("FreeResources(dfhd=%p)\n", dfhd));

    CurrentDir(dfhd->OldDirLock);
    
    ForeachNodeSafe(&dfhd->DirList, direntry, direntry2)
    {
    	FreeFileList(direntry->FileList, DFB(DiskfontBase));
	UnLock(direntry->DirLock);
	FreeVec(direntry);
    }
    
    FreeMem(dfhd, sizeof (struct DFHData));
  
    ReturnVoid("FreeResources");
}

/****************************************************************************************/

/******************/
/* AllocResources */
/******************/

/****************************************************************************************/

STATIC struct DFHData *AllocResources(struct DiskfontBase_intern *DiskfontBase)
{
    struct DFHData *dfhd;

    D(bug("AllocResources(void)\n"));
          
    /* Allocate user data */
    if ((dfhd = AllocMem( sizeof (struct DFHData), MEMF_ANY | MEMF_CLEAR)))
    {
    	struct DevProc *dp = NULL;
	
    	NEWLIST((struct List *)&dfhd->DirList);
	
	dfhd->OldDirLock = CurrentDir(0);
	CurrentDir(dfhd->OldDirLock);
	
#ifdef PROGDIRFONTSDIR
	do
	{
	    struct Process *Self;
	    APTR oldwinptr;
	    struct DirEntry *direntry;

	    if (!GetProgramDir())
		break;

	    direntry = AllocVec(sizeof(struct DirEntry), MEMF_ANY | MEMF_CLEAR);
	    if (!direntry) break;

	    Self = (struct Process *) FindTask(NULL);
	    oldwinptr          = Self->pr_WindowPtr;
	    Self->pr_WindowPtr = (APTR) -1;
	    direntry->DirLock  = Lock(PROGDIRFONTSDIR, ACCESS_READ);
	    Self->pr_WindowPtr = oldwinptr;

	    D(bug("AllocResources: PROGDIR:Fonts DirLock = 0x%lx\n", direntry->DirLock));

	    if (!direntry->DirLock)
	    {
		FreeVec(direntry);
		break;
	    }

	    if (!(direntry->FileList = GetFileList(direntry->DirLock, DFB(DiskfontBase))))
	    {
		UnLock(direntry->DirLock);
		FreeVec(direntry);
		break;
	    }

	    direntry->ProgdirFlag = TRUE;

	    D(bug("AllocResources: addtail direntry 0x%lx\n", direntry));
	    ADDTAIL((struct List *)&dfhd->DirList, (struct Node *)direntry);

	} while (0);
#endif

	for(;;)
	{
	    dp = GetDeviceProc(FONTSDIR, dp);
	    if (dp)
	    {
	    	struct DirEntry *direntry;
		
		direntry = AllocVec(sizeof(struct DirEntry), MEMF_ANY | MEMF_CLEAR);
		if (!direntry) break;
		
		if (!(direntry->DirLock = DupLock(dp->dvp_Lock)))
		{
		    FreeVec(direntry);
		    break;
		}
		
		if (!(direntry->FileList = GetFileList(direntry->DirLock, DFB(DiskfontBase))))
		{
		    UnLock(direntry->DirLock);
		    FreeVec(direntry);
		    break;
		}
		
		//direntry->ProgdirFlag = FALSE;

		ADDTAIL((struct List *)&dfhd->DirList, (struct Node *)direntry);
		
	    } /* if (dp) */
	    else
	    {
	    	break;
	    }
	    
	} /* for(;;) */
	
	if (dp) FreeDeviceProc(dp);
	
        if (!(IsListEmpty((struct List *)&dfhd->DirList)))
        {
	    ReturnPtr("AllocResources", struct DFHData *, dfhd); 
        }
          
        /* Failure. Free the allocated resources. */
        FreeResources(dfhd, DFB(DiskfontBase) );

    }
    
    ReturnPtr ("AllocResources", struct DFHData*, FALSE);
}

/****************************************************************************************/

AROS_UFH3(IPTR, DiskFontFunc,
    AROS_UFHA(struct Hook *,			h, 		A0),
    AROS_UFHA(struct FontHookCommand *,		fhc,		A2),
    AROS_UFHA(struct DiskfontBase_intern *,	DiskfontBase,	A1)
)
{
    AROS_USERFUNC_INIT

    /* Note: TRUE is default */
    ULONG retval = FH_SUCCESS;
    struct FontDescrHeader  *fdh;
    struct DFHData    *dfhd;
    BPTR lock;
    UWORD index;
    struct DateStamp *ds;
    struct FileInfoBlock *fib;

    STRPTR filename;

    (void)h;

    D(bug("DiskFontFunc(hook=%p, fhc=%p)\n", h, fhc));

    switch (fhc->fhc_Command)
    {
        case FHC_AF_INIT:
	    D(bug("DiskFontFunc: FHC_AF_INIT\n"));
            
            if (!(fhc->fhc_UserData = dfhd = AllocResources( DFB(DiskfontBase) )))
	    {
		D(bug("DiskFontFunc: can't alloc resources\n"));
            	retval = FALSE;
	    }
            else
	    {
	    	dfhd->CurrentDirEntry = (struct DirEntry *)dfhd->DirList.mlh_Head;
		dfhd->CurrentFileEntry = dfhd->CurrentDirEntry->FileList;
    	    }
            break;
        
        case FHC_AF_READFONTINFO:
	    D(bug("DiskFontFunc: FHC_AF_READFONTINFO\n"));
            
            dfhd = fhc->fhc_UserData;
            
            
            /* Is a font file allready loaded ? If not, load one.*/
            if (!dfhd->DescrLoaded)
            {
            	/* If a FontDescr has been read earlier, then free it */
            	if (dfhd->CurrentFDH)
            	{
            		FreeFontDescr(dfhd->CurrentFDH, DFB(DiskfontBase));
            		dfhd->CurrentFDH = 0;
            	}	
                /* Try to load next font-file. */
                retval = LoadNext(dfhd, DFB(DiskfontBase) );                
               
               	/* No more files ? */
                if (!retval)
                	{retval = FH_SCANFINISHED; break;}
                
                
                /* Reset the index to the first FontDescr element */
                dfhd->TTextAttrIndex = 0;
                
                dfhd->DescrLoaded = TRUE;
            }           

            /* Get the current FontDescrHeader */
            fdh     = dfhd->CurrentFDH;
            
            /* Look into the current TTextAttr entry */
            index   = dfhd->TTextAttrIndex;
            
            /* Copy the read TTextAttr */
            memcpy(
            	&(fhc->fhc_DestTAttr),
            	&(fdh->TAttrArray[ index ]),
            	sizeof (struct TTextAttr) );
            
	    fhc->fhc_DestTAttr.tta_Name = dfhd->CurrentFontDescrName;
	    
            /* 
            	Since the fontname is the same for all entries in a .font file,
             	we can reuse it
            */
            if (index != 0)
            	retval |= FH_REUSENAME;

            /* Go on with next entry in the TextAttrArray */
            index ++;
            dfhd->TTextAttrIndex = index;

            /* Finished with last entry in the TTextAttrArray ? */
            if (index == fdh->NumEntries)
            	dfhd->DescrLoaded = FALSE;

            break;
            
        case FHC_AF_CLEANUP:
       	    D(bug("DiskFontFunc: FHC_AF_CLEANUP\n"));

            dfhd = fhc->fhc_UserData;
            
            /* If a .font file has been loaded, free it */
            if (dfhd->CurrentFDH)
                FreeFontDescr(dfhd->CurrentFDH, DFB(DiskfontBase) );
                
            /* Free all the resources */

            FreeResources(dfhd, DFB(DiskfontBase) );
            
            break;

        case FHC_AF_GETDATE:
	    D(bug("DiskFontFunc: FHC_AF_GETDATE\n"));
            ds = fhc->fhc_UserData;
            retval = FALSE;
            
#if 1
	    if ((fib = AllocDosObject(DOS_FIB,NULL)))
	    {
		struct DevProc *devproc = NULL, *dp = NULL;
		struct DateStamp date = {0};

		do
		{
		    dp = GetDeviceProc(FONTSDIR, dp);
		    if (!dp)
			break;

		    if (!devproc)
			devproc = dp;

		    if (Examine(dp->dvp_Lock, fib))
		    {
			if (CompareDates(&date, &fib->fib_Date) > 0)
			{
			    date = fib->fib_Date;

			    retval = TRUE;
			}
		    }
            
		}
		while (dp->dvp_Flags & DVPF_ASSIGN);

		FreeDeviceProc(devproc);
	    
		FreeDosObject(DOS_FIB, fib);

		if (retval)
            {
		    *ds = date;
		}
	    }
#else
#warning "FIXME: scan all parts of multiassign and find the one with latest changedate"
            if ((lock = Lock(FONTSDIR, SHARED_LOCK)))
            {
                if ((fib = AllocDosObject(DOS_FIB,NULL)))
                {
                    if (Examine(lock,fib))
                    {
                        *ds = fib->fib_Date;
                        
                        retval = TRUE;
                    }
          
                   FreeDosObject(DOS_FIB,fib);
                }
                UnLock(lock);
            }
#endif
            break;
		
	case FHC_ODF_INIT:
	    D(bug("DiskFontFunc: FHC_ODF_INIT\n"));
	    dfhd = AllocMem(sizeof (struct DFHData), MEMF_ANY|MEMF_CLEAR);
	    if (dfhd)
	    {
		fhc->fhc_UserData = dfhd;

		D(bug("DiskFontFunc: FilePart %s fontname %s\n",
			FilePart(fhc->fhc_ReqAttr->tta_Name),
			fhc->fhc_ReqAttr->tta_Name));

		if (FilePart(fhc->fhc_ReqAttr->tta_Name) != fhc->fhc_ReqAttr->tta_Name)
		{
		    D(bug("DiskFontFunc: wrong names\n"));
		    dfhd->CurrentFDH = ReadFontDescr(fhc->fhc_ReqAttr->tta_Name, FALSE, DFB(DiskfontBase));
		    D(bug("DiskFontFunc: FDH 0x%lx\n",dfhd->CurrentFDH));
		}
		else
		{
#ifdef PROGDIRFONTSDIR
		    filename = AllocVec(
			sizeof (PROGDIRFONTSDIR) + strlen(fhc->fhc_ReqAttr->tta_Name) + 1,
			MEMF_ANY);
#else
		    filename = AllocVec(
		        sizeof (FONTSDIR) + strlen(fhc->fhc_ReqAttr->tta_Name) + 1,
			MEMF_ANY);
#endif
		    if (filename)
		    {

			const struct FontPath fontpaths[] =
			{
#ifdef PROGDIRFONTSDIR
			    {PROGDIRFONTSDIR, TRUE},		/* MUST BE THE FIRST ONE ! */
#endif
			    {FONTSDIR,        FALSE}
			};
#ifdef PROGDIRFONTSDIR
			int index = GetProgramDir() ? 0 : 1;
#else
			int index = 0;
#endif
			for (;
			     index < (sizeof(fontpaths) / sizeof(fontpaths[0]));
			     index++)
			{
			    strcpy(filename, fontpaths[index].Path);
			    strcat(filename, fhc->fhc_ReqAttr->tta_Name);

			    D(bug("DiskFontFunc: try reading font \"%s\"\n", filename));

			    dfhd->CurrentFDH = ReadFontDescr(filename, fontpaths[index].ProgdirFlag, DFB(DiskfontBase));

			    if (dfhd->CurrentFDH)
			    {
				D(bug("DiskFontFunc: FDH 0x%lx found\n",dfhd->CurrentFDH));
				break;
			    }
			}

			FreeVec(filename);
		    }
		    else
		    {
			D(bug("DiskFontFunc: can't alloc filename\n"));
		    }
		}

		if (dfhd->CurrentFDH)
		{
		    D(bug("DiskFontFunc: success\n"));
		    dfhd->TTextAttrIndex = 0;
		    retval = FH_SUCCESS;
		    break;
		}

		FreeMem(dfhd, sizeof (struct DFHData));
	    }
	    retval = FALSE;
	    break;

	case FHC_ODF_GETMATCHINFO:
	    D(bug("DiskFontFunc: FHC_ODF_GETMATCHINFO\n"));
	    dfhd = (struct DFHData*)fhc->fhc_UserData;

	    fdh = dfhd->CurrentFDH;
	    index = dfhd->TTextAttrIndex;

	    /* Finished scanning ? */
	    if (index >= fdh->NumEntries)
	    {
	    	retval = FH_SCANFINISHED;

	    	/*
		** In case of an outline font, return also a perfect
		** matching outline entry.
		*/
		   
	    	if ((index == fdh->NumEntries) && 
		    (fdh->ContentsID == OFCH_ID) &&
		    (fdh->OTagList))
		{
		    UBYTE supportedstyles;
		    
		    fhc->fhc_DestTAttr.tta_Name  = fdh->OTagList->filename;
		    fhc->fhc_DestTAttr.tta_YSize = fhc->fhc_ReqAttr->tta_YSize;
		    fhc->fhc_DestTAttr.tta_Flags = OTAG_GetFontFlags(fdh->OTagList, DiskfontBase);
		    fhc->fhc_DestTAttr.tta_Style = OTAG_GetFontStyle(fdh->OTagList, DiskfontBase);
		    fhc->fhc_DestTAttr.tta_Tags  = NULL;

    	    	    if (fhc->fhc_ReqAttr->tta_Style & FSF_TAGGED)
		    {
		    	fhc->fhc_DestTAttr.tta_Style |= FSF_TAGGED;
			fhc->fhc_DestTAttr.tta_Tags  = fhc->fhc_ReqAttr->tta_Tags;
		    }
		    
		    supportedstyles = OTAG_GetSupportedStyles(fdh->OTagList, DiskfontBase);

		    if ((fhc->fhc_ReqAttr->tta_Style & FSF_BOLD) &&
		        !(fhc->fhc_DestTAttr.tta_Style & FSF_BOLD) &&
			(supportedstyles & FSF_BOLD))
		    {
		    	fhc->fhc_DestTAttr.tta_Style |= FSF_BOLD;
		    }
		    
		    if ((fhc->fhc_ReqAttr->tta_Style & FSF_ITALIC) &&
		        !(fhc->fhc_DestTAttr.tta_Style & FSF_ITALIC) &&
			(supportedstyles & FSF_ITALIC))
		    {
		    	fhc->fhc_DestTAttr.tta_Style |= FSF_ITALIC;
		    }
		    
		    retval = FH_SUCCESS;
		    index++;
		}
		
    	    } /* if (index >= fdh->NumEntries) */
	    else
	    {
	    	fhc->fhc_DestTAttr = fdh->TAttrArray[index ++];
	    }
	    dfhd->TTextAttrIndex = index;

	    break;

	case FHC_ODF_CLEANUP:
	    D(bug("DiskFontFunc: FHC_ODF_CLEANUP\n"));
	    dfhd = (struct DFHData*)fhc->fhc_UserData;

	    if (dfhd)
	    {
		    if (dfhd->CurrentFDH) 
			    FreeFontDescr(dfhd->CurrentFDH, DFB(DiskfontBase) );

		    FreeMem(dfhd, sizeof (struct DFHData));
	    }
	    break;

	case FHC_ODF_OPENFONT:
	    D(bug("DiskFontFunc: FHC_ODF_OPENFONT\n"));
	    dfhd = (struct DFHData*)fhc->fhc_UserData;
	    if ((fhc->fhc_DestTAttr.tta_Flags & FONTTYPE_FLAGMASK) == FONTTYPE_OUTLINEFONT)
	    {
	    	fhc->fhc_TextFont = OTAG_ReadOutlineFont(&fhc->fhc_DestTAttr,
		    	    	    	    	    	 fhc->fhc_ReqAttr,
							 dfhd->CurrentFDH->OTagList,
							 DiskfontBase);
	    }
	    else
	    {
		fhc->fhc_TextFont = ReadDiskFont(&fhc->fhc_DestTAttr,
	    	    	    	    		 fhc->fhc_ReqAttr->tta_Name,
						 DiskfontBase);
	    }
	    
	    if (fhc->fhc_TextFont)
	    {
	    	/* PPaint's personal.font/8 has not set FPF_DISKFONT,
		   (FPF_ROMFONT neither), but AmigaOS diskfont.library
		   still shows FPF_DISKFONT set when opening this font */
		   
	    	fhc->fhc_TextFont->tf_Flags &= ~FPF_ROMFONT;
		fhc->fhc_TextFont->tf_Flags |= FPF_DISKFONT;
		
	    	D(bug("Adding font: %p\n", fhc->fhc_TextFont));
		
		/* Forbid() must be called before AddFont, because AddFont clears
		   tf_Accessors and in the worst case it could happen to us that
		   after the AddFont() another task opens and closes/frees the
		   diskfont, before we manage to increase tf_Accessors. */
		   
		Forbid();

		AddFont(fhc->fhc_TextFont);				
		fhc->fhc_TextFont->tf_Accessors++;

    	    	{
		    struct DiskFontHeader *dfh;
		    
		    dfh = (struct DiskFontHeader *)(((IPTR)fhc->fhc_TextFont) - (IPTR)OFFSET(DiskFontHeader, dfh_TF));

    	    	    /* Paranoia check */
    	    	    if (dfh->dfh_FileID == DFH_ID)
		    {
		    	ADDTAIL(&DiskfontBase->diskfontlist, &dfh->dfh_DF);
		    }
		}
		Permit();
				
		D(bug("Font added\n"));

		retval = FH_SUCCESS;
	    }	
	    break;
            
    }
    
    ReturnInt ("DiskFontFunc", ULONG, retval);

    AROS_USERFUNC_EXIT
}

/****************************************************************************************/
