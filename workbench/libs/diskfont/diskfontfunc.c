/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Hook for getting font descriptions from FONTS:
    Lang: English.
*/


#include "diskfont_intern.h"  

  

/****************/
/* LoadNext     */
/****************/


/* Function for loading the next .font file into memory.
Uses the standard fonthook return values (FALSE is a critical error). */
STATIC ULONG LoadNext(struct DiskFontHook_Data *dfhd, struct DiskfontBase_intern *DiskfontBase)
{
    ULONG retval = FALSE;
    
    BOOL  directory,
          done  = FALSE;
    
    BPTR fh;
        
    do
    {
        /* Use Examine or ExNext ? This test is kind of a kludge. One
        could put Examine in FHC_INIT instead, but that would require
        more code, and I guess this if-test is really not much overhead.
        */
          
        if (!dfhd->dfhd_UseExNext)
        {
            /* Use Examine */
            if (!Examine(dfhd->dfhd_DirLock, dfhd->dfhd_CurrentFIB) )
            {
                retval  = FH_SCANFINISHED;
                done    = TRUE;
            }
            else
                /* If Examine() succeed we should use ExNext() from now on */
                dfhd->dfhd_UseExNext = TRUE;
        }
        else
        {
            if (!ExNext(dfhd->dfhd_DirLock, dfhd->dfhd_CurrentFIB))
            {
                retval  = FH_SCANFINISHED;
                done    = TRUE;
            }
        }
        
        /* Have we found a file or a directory ? */
        directory = (dfhd->dfhd_CurrentFIB->fib_DirEntryType > 0);
        
        /* If a file, try to load it */
        if (!directory)
        {
            
            /* 
              If the file is successfully loaded, then exit. If not
              continue to scan the directory.
            */
            
            if ((fh = Open(dfhd->dfhd_CurrentFIB->fib_FileName, MODE_OLDFILE)) != 0 )
            {
                if ((dfhd->dfhd_CurrentFDH = ReadFontDescr(fh, DFB(DiskfontBase))) != 0 )
                {
                    done    = TRUE;
                    retval  = FH_SUCCESS;
                }
                
                Close(fh);
            }
        } /* if (!directory) */

    /* Do until we find a file */
    }
    while (directory || !done);

    return (retval);
}


STATIC VOID FreeResources(struct DiskFontHook_Data *dfhd, struct DiskfontBase_intern *DiskfontBase )
{
    if ( dfhd->dfhd_CurrentFIB  ) FreeDosObject ( DOS_FIB, dfhd->dfhd_CurrentFIB  );
    if ( dfhd->dfhd_OldDirLock  ) CurrentDir    ( dfhd->dfhd_OldDirLock           );
    if ( dfhd->dfhd_DirLock     ) UnLock        ( dfhd->dfhd_DirLock              );
    
    FreeMem(dfhd, sizeof (struct DiskFontHook_Data));
  
    return;
}

STATIC struct DiskFontHook_Data *AllocResources(struct DiskfontBase_intern *DiskfontBase)
{
    struct DiskFontHook_Data *dfhd;
          
    /* Allocate user data */
    if ((dfhd = AllocMem( sizeof (struct DiskFontHook_Data), MEMF_ANY|MEMF_CLEAR)) != 0  )
    {
          /* Create a lock on the fonts directory */
          if ((dfhd->dfhd_DirLock = Lock("FONTS:", ACCESS_READ)) != 0)
          {
              /* Change the current directory */
              dfhd->dfhd_OldDirLock = CurrentDir(dfhd->dfhd_DirLock);
              
              /* Allocate a FileInfoBlock */  
              if (( dfhd->dfhd_CurrentFIB = AllocDosObject(DOS_FIB,TAG_DONE)) != 0 )
                  return (dfhd);
          }
          
          /* Failure. Free the allocated resources. */
          FreeResources(dfhd, DFB(DiskfontBase) );

    }
    
    return (FALSE);
}



IPTR DiskFontFunc
(
    struct Hook             *h,
    struct FontHookCommand  *fhc,
    struct DiskfontBase_intern *DiskfontBase
)
{
    /* Note: TRUE is default */
    ULONG retval = FH_SUCCESS;
    
    ULONG val;

    UWORD index;
    
    struct FontDescrHeader  *fdh;
    struct FontDescr        *fdarray;
    
    struct DiskFontHook_Data    *dfhd;
    struct FontInfoNode         *finode;
    struct TAvailFonts          *taf;
    
    struct DateStamp            *ds;
    struct FileInfoBlock        *fib;
    BPTR lock;
    
    switch (fhc->fhc_Command)
    {
        case FHC_INIT:
            
            if ((fhc->fhc_UserData = dfhd = AllocResources( DFB(DiskfontBase))) != 0)
                retval = TRUE;
            break;
            
            
        case FHC_READFONTINFO:
            
            dfhd = fhc->fhc_UserData;
            
            
            /* Is a font file allready loaded ? If not, load one.*/
            if (!dfhd->dfhd_CurrentFDH)
            {
                /* Try to load next font-file. If a single non-critical error, then
                just load next. */
                do { val = LoadNext(dfhd, DFB(DiskfontBase) ); } while (val == FH_SINGLEERROR);
                

                /* 
                  Examine the return value. May be FALSE, FH_SUCCESS or FH_SCANFINISHED.
                  If finshed or critical error, then exit.
                */
                if (!val || val & FH_SCANFINISHED)
                {
                    retval = val;
                    break;
                }
                
                /* Reset the index to the first FontDescr element */
                dfhd->dfhd_FontDescrIndex = 0;
            }           

            /* Get the current FontDescrHeader */
            fdh     = dfhd->dfhd_CurrentFDH;
            fdarray = fdh->FontDescrArray;
            
            /* Look into the current font file */
            finode  = fhc->fhc_FINode;
            index   = dfhd->dfhd_FontDescrIndex;


            
            /* Insert values into the fontinfonode */
            taf     = &(finode->TAF);
            
            
            taf->taf_Type           = AFF_DISK;
            
            taf->taf_Attr.tta_YSize = fdarray[ index ].YSize;
            taf->taf_Attr.tta_Style = fdarray[ index ].Style;
            taf->taf_Attr.tta_Flags = fdarray[ index ].YSize;
            
            /* If this is the fist node in the file, allocate FontName and evt. FontTags-nodes. */
            
            if (index == 0)
            {
                if (!(finode->FontName = AllocFontNameNode(dfhd->dfhd_CurrentFIB->fib_FileName, DFB(DiskfontBase) )))
                {
                    /* Critical error */
                    retval = FALSE;
                    break;
                }
                
            }
            else
            {
                retval |= FH_REUSENAME;
            }

            if 
            ( 
                fhc->fhc_Flags & AFF_TAGGED 
            && 
                fdarray[ index ].NumTags
            )
            {
                if (!(finode->FontTags = AllocFontTagsNode( fdarray[index].Tags, DFB(DiskfontBase) )))
                {
                    retval = FALSE;
                    break;
                }
            }
            
            /* Increase the index */
            index ++;
            dfhd->dfhd_FontDescrIndex = index;

            /* If we are finished with the last FontDescr in the file, free it */
            if (index == fdh->NumEntries)
            {
                FreeFontDescr(dfhd->dfhd_CurrentFDH, DFB(DiskfontBase));
                
                /* Reset to 0 to tell that there is currently no file loaded */
                dfhd->dfhd_CurrentFDH = 0;
            }
            
            break;
            
        case FHC_CLEANUP:
        
            dfhd = fhc->fhc_UserData;
            
            /* If a .font file has been loaded, free it */
            if (dfhd->dfhd_CurrentFDH)
                FreeFontDescr(dfhd->dfhd_CurrentFDH, DFB(DiskfontBase) );
                
            /* Free all the resources */
            FreeResources(dfhd, DFB(DiskfontBase) );
            
            break;
        
        case FHC_GETDATE:
        
            ds = fhc->fhc_UserData;
            retval = FALSE;
            
            
            if ((lock = Lock("FONTS:",SHARED_LOCK)) != 0)
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
            
    }
    
    return (retval);

}
