#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dosasl.h>
#include "dos_intern.h"
#include <string.h>

LONG followpattern(struct AnchorPath * AP,
                   struct AChain * AC,
                   struct DosLibrary * DOSBase)
{
    LONG success;
  
    /* 
    ** If the user specified the pattern libs:#?/s#/
    ** then I must enter that pattern before I return to the user.
    */
    while (TRUE)
    {
      do
      {
        success = ExNext (AC->an_Lock, &AC->an_Info);
      }
      while (DOSTRUE == success &&
             DOSFALSE == MatchPatternNoCase(AC->an_String,
                                            AC->an_Info.fib_FileName));

      /*
      ** If no matching entry could be found here then I
      ** step back one directory, unless this is the very
      ** first directory already.
      */
      if (DOSFALSE == success)
      {
        UnLock(AC->an_Lock);
        AC->an_Lock = NULL;
          
        AC = AC->an_Parent;
        AP->ap_Current = AC;
        
        CurrentDir(AC->an_Lock);

        if (NULL == AC->an_Parent) 
          return ERROR_NO_MORE_ENTRIES;
      }
      else
      {
        if (AC->an_Info.fib_DirEntryType >= 0 /* &&
            AC->an_Info.fib_DirEntryType != ST_SOFTLINK */)
        {
          /*
          ** I must silently enter this directory if there
          ** are further patterns left, otherwise I return to the
          ** user. 
          */
          if (NULL != AC->an_Child)
          {
            /*
            ** Silently entering this dir according to the 
            ** pattern.
            */
            BPTR newdir;
            AP->ap_Current = AC->an_Child;
            
            newdir = Lock(AC->an_Info.fib_FileName, ACCESS_READ);
            (void)CurrentDir(newdir);
            
            AC = AC->an_Child;
            AC->an_Lock = newdir;
            Examine(AC->an_Lock, &AC->an_Info);
          }
          else
          {
            /*
            ** Ask the user whether to enter this dir or not
            */
            return createresult(AP, AC, DOSBase);
          }
        }
        else
        {
          /*
          ** This is a file. If there is pattern left to match then
          ** I must not show this file because I must fulfill the pattern
          ** first.
          */
          if (NULL == AC->an_Child)
          {
            /*
            ** There's no pattern left!
            */
            return createresult(AP, AC, DOSBase);
          }
#if 0
          else
            kprintf("Silently skipping file %s!\n",AC->an_Info.fib_FileName);
#endif
        }
      } /* if (DOSFALSE == success) */
    }

#if 0
    /* 
    ** Hooray! A matching file was found. Also show the data in AP 
    */
    createresult(AP, AC, DOSBase);
#endif      

}


/*
** A file/directory has been found and now it must be written
** into the relevant structures.
*/
LONG createresult(struct AnchorPath * AP,
                  struct AChain * AC,
                  struct DosLibrary * DOSBase)
{
  CopyMem(&AC->an_Info, &AP->ap_Info, sizeof(struct FileInfoBlock));
  if (0 != AP->ap_Strlen)
  {
      if (FALSE == writeFullPath(AP))
         return ERROR_BUFFER_OVERFLOW;
  }
  return 0;
}


/* Function needed by MatchFirst/Next */

BOOL writeFullPath(struct AnchorPath * AP)
{
  struct AChain * AC = AP->ap_Base;
  BOOL end = FALSE;
  char * LastPos = (char *)&AP->ap_Buf;
  int copied = 0;
  
  while (FALSE == end)
  {
    int len = strlen(AC->an_Info.fib_FileName);
    if (copied+len > AP->ap_Strlen)
    {
      return FALSE;
    } 
    strcpy(&LastPos[copied], AC->an_Info.fib_FileName);
    copied += len;
    
    if (AC != AP->ap_Current)
    {
      /* also add a '/' */
      if (copied+1 > AP->ap_Strlen)
      {
        return FALSE;
      }
      LastPos[copied]='/';
      copied++;
    }
    else
    {
      if (copied+1 > AP->ap_Strlen)
      {
        return FALSE;
      }
      LastPos[copied]='\0';
      end = TRUE;
    }
      
    AC = AC->an_Child;
  }
  return TRUE;
}
