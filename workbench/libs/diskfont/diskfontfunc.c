/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Hook for handling fonts in FONTS:
    Lang: English.
*/

/****************************************************************************************/

#include <proto/dos.h>
#include <proto/graphics.h>
#include <dos/exall.h>
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
    STRPTR 		FileName;
};	

struct DFHData /*DiskFontHookData */
{
    BPTR                    	DirLock;
    BPTR                    	OldDirLock;
    BOOL			DescrLoaded;
    struct FontDescrHeader  	*CurrentFDH;
    /* Index into the current TTextAttr-array */
    UWORD                   	TTextAttrIndex;
	
    struct FileEntry	    	*FileList;
    struct FileEntry		*CurrentFileEntry;
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

	/* Free filename */
	if (feptr->FileName)
	    FreeVec(feptr->FileName);

	/* Free FileEntry */
	FreeMem(feptr, sizeof (struct FileEntry));

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
	
    struct FileEntry	felisthead = {0, 0},	/* Dummy list head */
			*feptr,
			*felist = NULL;

    BOOL success = TRUE;

    struct ExAllControl *eac;
    struct ExAllData *ead;

    /* Buffer to put ExAll() file in */
    static UBYTE buffer[4096];

    D(bug("GetFileList(fontslock=%p)\n", fontslock));

    if ( !(eac = AllocDosObject(DOS_EXALLCONTROL, NULL)) )
	success = FALSE;
    else
    {
	/* Important to clear this field */
	eac->eac_LastKey = 0;

	feptr = &felisthead;

	while (ExAll(fontslock, (struct ExAllData *)buffer, sizeof (buffer), ED_TYPE, eac) )
	{

	    /* Get first ExallData element */
	    ead = (struct ExAllData *)buffer;

	    /* We should continue to ExAll() even if memoryalloc failed */
	    while (ead && success && eac->eac_Entries != 0)
	    {
		/* Is this a file ? */
		if (ead->ed_Type < 0)
		{

		    /* Allocate a list entry */
		    if ( !( feptr->Next = AllocMem(sizeof (struct FileEntry), MEMF_ANY|MEMF_CLEAR)))
		    {
		        success = FALSE;
			break;
		    }

		    feptr = feptr->Next;

		    /* Allocate space for filename string */
		    if ( !(feptr->FileName = AllocVec( strlen(ead->ed_Name) + 1, MEMF_ANY)))
		    {
		        success = FALSE;
			break;
		    }

		    strcpy(feptr->FileName, ead->ed_Name);
		}
		ead = ead->ed_Next;

	    } /* while (ead && success && eac->eac_Entries) */

	} /* While ExAll() */

	/* Skip the dummy filelist head */
	felist = felisthead.Next;

	/* Was the whole directory scanned ? */
	if (IoErr() != ERROR_NO_MORE_ENTRIES)
	    success = FALSE;

	FreeDosObject(DOS_EXALLCONTROL, eac);
    }

    if (!success) 
	FreeFileList(felist, DFB(DiskfontBase)); 


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

	
    if (!curfile)
	ReturnInt ("LoadNext", ULONG, FALSE);

    while (curfile && !done)
    {
     	if ((dfhd->CurrentFDH = ReadFontDescr(curfile->FileName, DFB(DiskfontBase))) != 0 )
       	{
       	    done    = TRUE;
            retval  = TRUE;
       	}
        
        curfile = curfile->Next;
    }
    
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
    D(bug("FreeResources(dfhd=%p)\n", dfhd));

    CurrentDir( dfhd->OldDirLock);
    if ( dfhd->FileList		) FreeFileList	( dfhd->FileList, DFB(DiskfontBase)	);		
    if ( dfhd->DirLock		) UnLock        ( dfhd->DirLock    			);
    
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
    if ((dfhd = AllocMem( sizeof (struct DFHData), MEMF_ANY|MEMF_CLEAR)) != 0  )
    {
    	/* Create a lock on the fonts directory */
        if ((dfhd->DirLock = Lock(FONTSDIR, ACCESS_READ)) != 0)
        {
         	/* Change the current directory */
             dfhd->OldDirLock = CurrentDir(dfhd->DirLock);
        	
	     /* Get a list of the .font files */
	     if ((dfhd->FileList = GetFileList(dfhd->DirLock, DFB(DiskfontBase) )) != 0)
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
            	retval = FALSE;
            else
              	dfhd->CurrentFileEntry = dfhd->FileList;

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
		    strcpy(filename, FONTSDIR);
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
	    fhc->fhc_TextFont = ReadDiskFont(fhc->fhc_ReqAttr, DFB(DiskfontBase));
	    if (fhc->fhc_TextFont)
	    {
	    	D(bug("Adding font: %p\n", fhc->fhc_TextFont));
		
		/* Forbid() must be called before AddFont, because AddFont clears
		   tf_Accessors and in the worst case it could happen to us that
		   after the AddFont() another task opens and closes/frees the
		   diskfont, before we manage to increase tf_Accessors. */
		   
		Forbid();

		AddFont(fhc->fhc_TextFont);				
		fhc->fhc_TextFont->tf_Accessors++;

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
