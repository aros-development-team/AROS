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
#include <proto/alib.h>
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
	    	    
     	    if ((dfhd->CurrentFDH = ReadFontDescr(curfile->FileName, DFB(DiskfontBase))) != 0 )
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
	
    	NewList((struct List *)&dfhd->DirList);
	
	dfhd->OldDirLock = CurrentDir(0);
	CurrentDir(dfhd->OldDirLock);
	
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
		
		AddTail((struct List *)&dfhd->DirList, (struct Node *)direntry);
		
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
            
    D(bug("DiskFontFunc(hook=%p, fhc=%p)\n", h, fhc));

    switch (fhc->fhc_Command)
    {
        case FHC_AF_INIT:
            
            if (!(fhc->fhc_UserData = dfhd = AllocResources( DFB(DiskfontBase) )))
	    {
            	retval = FALSE;
	    }
            else
	    {
	    	dfhd->CurrentDirEntry = (struct DirEntry *)dfhd->DirList.mlh_Head;
		dfhd->CurrentFileEntry = dfhd->CurrentDirEntry->FileList;
    	    }
            break;
        
        case FHC_AF_READFONTINFO:
            
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
        
            dfhd = fhc->fhc_UserData;
            
            /* If a .font file has been loaded, free it */
            if (dfhd->CurrentFDH)
                FreeFontDescr(dfhd->CurrentFDH, DFB(DiskfontBase) );
                
            /* Free all the resources */

            FreeResources(dfhd, DFB(DiskfontBase) );
            
            break;

        case FHC_AF_GETDATE:

            ds = fhc->fhc_UserData;
            retval = FALSE;
            
            
            if ((lock = Lock(FONTSDIR, SHARED_LOCK)) != 0)
            {
                if ((fib = AllocDosObject(DOS_FIB,NULL)) != 0)
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
            break;
		
	case FHC_ODF_INIT:
	    dfhd = AllocMem(sizeof (struct DFHData), MEMF_ANY|MEMF_CLEAR);
	    if (dfhd)
	    {
		fhc->fhc_UserData = dfhd;

		filename = AllocVec(
		    sizeof (FONTSDIR) + strlen(fhc->fhc_ReqAttr->tta_Name) + 1,
		    MEMF_ANY);
		if (filename)
		{
		    if (FilePart(fhc->fhc_ReqAttr->tta_Name) == fhc->fhc_ReqAttr->tta_Name)
		    {
		    	strcpy(filename, FONTSDIR);
		    }
		    else
		    {
		    	filename[0] = 0;
		    }
		    strcat(filename, fhc->fhc_ReqAttr->tta_Name);


		    dfhd->CurrentFDH = ReadFontDescr(filename, DFB(DiskfontBase));

		    FreeVec(filename);
		    if (dfhd->CurrentFDH)
		    {
			dfhd->TTextAttrIndex = 0;
			retval = FH_SUCCESS;
			break;
		    }
		}
		FreeMem(dfhd, sizeof (struct DFHData));
	    }
	    retval = FALSE;
	    break;

	case FHC_ODF_GETMATCHINFO:
	    dfhd = (struct DFHData*)fhc->fhc_UserData;

	    fdh = dfhd->CurrentFDH;
	    index = dfhd->TTextAttrIndex;

	    /* Finished scanning ? */
	    if (index == fdh->NumEntries)
		    {retval = FH_SCANFINISHED; break; }

	    fhc->fhc_DestTAttr = fdh->TAttrArray[index ++];
	    dfhd->TTextAttrIndex = index;

	    break;

	case FHC_ODF_CLEANUP:
	    dfhd = (struct DFHData*)fhc->fhc_UserData;

	    if (dfhd)
	    {
		    if (dfhd->CurrentFDH) 
			    FreeFontDescr(dfhd->CurrentFDH, DFB(DiskfontBase) );

		    FreeMem(dfhd, sizeof (struct DFHData));
	    }
	    break;

	case FHC_ODF_OPENFONT:
	    fhc->fhc_TextFont = ReadDiskFont(fhc->fhc_ReqAttr,
	    	    	    	    	     fhc->fhc_DestTAttr.tta_Name,
					     DiskfontBase);
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
		    	AddTail(&DiskfontBase->diskfontlist, &dfh->dfh_DF);
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
