/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Test of the DOS functions MatchFirst, MatchNext and MatchEnd.
*/

#include <proto/dos.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <exec/types.h>
#include <exec/memory.h>

#include <stdio.h>
#include <string.h>


/* 
   Uncomment the following defines and function prototypes 
   if you want to use the code for the Match*-functions that 
   is in this file. 
*/

/*
#undef MatchFirst
#undef MatchNext
#undef MatchEnd 
#define MatchFirst MyMatchFirst
#define MatchNext  MyMatchNext
#define MatchEnd   MyMatchEnd
  LONG MyMatchFirst(STRPTR pat, struct AnchorPath * AP);
  LONG MyMatchNext(struct AnchorPath *AP);
  void MyMatchEnd(struct AnchorPath *AP);
*/

  BOOL writeFullPath(struct AnchorPath *AP);

int main (void)
{
  BOOL error;
  int strlength=160;
  struct AnchorPath * AP = AllocVec(sizeof(struct AnchorPath) + strlength,MEMF_CLEAR);
  char * Pattern = AllocVec(80,MEMF_ANY);

  //dirlock = Lock("sys:",ACCESS_READ);
  //oldlock = CurrentDir(dirlock);

  AP->ap_BreakBits = SIGBREAKF_CTRL_C;
  AP->ap_Flags  = 0;//APF_DODIR;
  AP->ap_Strlen = strlength;

  printf("Give me a pattern to search for: ");
  /* the following line breaks AROS in MatchEnd() when calling FreeVec() 
     the second time the program is run. I have no idea why, though. */
  scanf("%s",Pattern);
  printf("Pattern to search for: %s\n",Pattern);

  for(error = MatchFirst(Pattern,AP); error == 0;
      error = MatchNext(AP))
  {
    if (AP->ap_Flags & APF_DIDDIR)
    {
      AP->ap_Flags &= ~APF_DIDDIR;
      printf(" leaving \033[32m%s\033[39m\n",AP->ap_Buf);
    }
    else /* if dir then enter it! */
    {
      if (AP->ap_Info.fib_DirEntryType >= 0)
      {
        AP->ap_Flags |=APF_DODIR;
        printf("entering \033[33m%s\033[39m\n",AP->ap_Buf);
      }
      else
      {
        //BPTR fl = CurrentDir(AP->ap_Current->an_Lock);
        //(void)CurrentDir(fl);
        
        printf("         %s\n",AP->ap_Buf);
      }
    }
  }
  printf("error = %i \n",error);
  MatchEnd(AP);
  FreeVec(AP);
  FreeVec(Pattern);
  //CurrentDir(oldlock);

  //UnLock(dirlock);

    return 0;
}

#undef MatchFirst
#undef MatchNext 
#undef MatchEnd   


LONG MyMatchFirst(STRPTR pat, struct AnchorPath * AP)
{
  struct AChain * AC;
  struct AChain * AC_Prev = NULL;
  LONG PatLength;
  STRPTR ParsedPattern;
  BPTR firstlock; 
  
  if (!pat)
    return FALSE;
  
  PatLength = 2*strlen(pat)+2;
  
  ParsedPattern = AllocMem(PatLength, MEMF_ANY);
  if (NULL != ParsedPattern)
  {
    LONG PatStart = 0;
    LONG PatEnd   = 0;
    BOOL AllDone  = FALSE;
    LONG index;
    BOOL success = FALSE;
    /* put the preparsed string to some memory */
    /* If there are any wildcards then leave info */
    if (1 == ParsePatternNoCase(pat, ParsedPattern, PatLength))
      AP->ap_Flags = (BYTE)APF_ITSWILD;

    /* First I search for the very first ':'. If a '/' comes along
       before that I quit. The string before and including the ':' 
       is assumed to be an assigned director, for example 'libs:'.
       So I will start looking for the pattern in that directory.
     */
    while (TRUE)
    {
      if (ParsedPattern[PatEnd] == ':')
      {
        success = TRUE;
        break;
      }
      else
      {
        if ( ParsedPattern[PatEnd]         == '/'  ||
             ParsedPattern[PatEnd]         == '\0' ||
            (ParsedPattern[PatEnd] & 0x80) != 0 
                           /* a token or nonprintable letter */)
        {
          PatEnd = 0;
          break;
        }
      } 
      PatEnd++;
    }

    /* Only if success == TRUE an assigned dir was found. */
    if (TRUE == success)
    {
      /* try to create a lock to that assigned dir. */
      char Remember = ParsedPattern[PatEnd+1];
      PatEnd++;
      ParsedPattern[PatEnd] = '\0';
      
      firstlock = Lock(ParsedPattern, ACCESS_READ);
      /* check whether an error occurred */
      if (BNULL == firstlock)
      {
        FreeMem(ParsedPattern, PatLength);
        return ERROR_DIR_NOT_FOUND; /* hope that's the right one... */
      }
      
      /* I have the correct lock. */
      ParsedPattern[PatEnd] = Remember;
      PatStart=PatEnd;
    }
    else
    {
      /* Create a lock to the current dir. */
      firstlock = CurrentDir(BNULL);
      firstlock = DupLock(firstlock);      
      (void)CurrentDir(firstlock);
    }
    
    /* Build the Anchor Chain. For every subdirector I allocate
       a AChain structure and link them all together */   
    while (FALSE == AllDone)
    {


      /* 
         Search for the next '/' in the pattern and everything behind
         the previous '/' and before this '/' will go to an_String 
       */
      while (TRUE)
      {
        if (ParsedPattern[PatEnd] == '\0')
        {
          AllDone = TRUE;
          PatEnd--;
          break;
        }
        if (ParsedPattern[PatEnd] == '/')
        {
          PatEnd--;
          break;
        }
        PatEnd++;
      }
      
      AC = AllocMem(sizeof(struct AChain)+(PatEnd-PatStart+2), MEMF_CLEAR);
      if (NULL == AC)
      {
        /* not so bad if this was not the very first AC. */
        if (NULL == AP->ap_Base)
        {
          /* oops, it was the very first one. I really cannot do anything for 
             you. - sorry */
          FreeMem(ParsedPattern, PatLength);
          return ERROR_NO_FREE_STORE;
        }
        
        /* let me out of here. I will at least try to do something for you.
           I can check the first few subdirs but that's gonna be it. 
         */
        AP->ap_Flags |= APF_NOMEMERR;
        break;
      }
      
      if (NULL == AP->ap_Base)
      {
        AP->ap_Base = AC;
        AP->ap_Current = AC;
      }
      
      if (NULL != AC_Prev)
      {
        AC_Prev->an_Child = AC;
      }
      AC->an_Parent = AC_Prev;
      AC_Prev       = AC;
      
      /* copy the part of the pattern to the end of the AChain. */
      index = 0;
      while (PatStart <= PatEnd)
      {
        AC->an_String[index] = ParsedPattern[PatStart];
        index++;
        PatStart++;
      }
      /* Put PatStart and PetEnd behind the '/' that was found. */
      PatStart   = PatEnd + 2;
      PatEnd    += 2;
      /* 
         the trailing '\0' is there automatically as I allocated enough store
         with MEMF_CLEAR
      */

    } /* while () */

    /* The AnchorChain to work with is the very first one. */
    AC = AP->ap_Base;
    AC->an_Lock = firstlock;
    
    /* look for the first file that matches the given pattern */
    success = Examine(AC->an_Lock, &AC->an_Info);
    success = ExNext (AC->an_Lock, &AC->an_Info);
    while (DOSTRUE == success &&
           DOSFALSE == MatchPatternNoCase(AC->an_String,
                                          AC->an_Info.fib_FileName))
    {
      /* I still haven't found what I've been looking for ... */
      success = ExNext(AC->an_Lock, &AC->an_Info); 
    }  
    
    if (DOSFALSE == success)
    {
      return ERROR_NO_MORE_ENTRIES;
    }
    
    /* Hooray! A matching file was found. Also show the data in AP */
    CopyMem(&AC->an_Info, &AP->ap_Info, sizeof(struct FileInfoBlock));
    if (0 != AP->ap_Strlen)
    {
       if (FALSE == writeFullPath(AP))
          return ERROR_BUFFER_OVERFLOW;
    }
    return 0;
     
  }
  else
  {
    return ERROR_NO_FREE_STORE;
  }

  return 0;
}


LONG MyMatchNext(struct AnchorPath * AP)
{
  /* If the user says I am supposed to enter the directory then I first check
     whether it is a directory... */
  struct AChain * AC = AP->ap_Current;
  BOOL success;
  if (0 != (AP->ap_Flags & APF_DODIR ))
  {
    if (AC->an_Info.fib_DirEntryType >= 0 /* &&
        AC->an_Info.fib_DirEntryType != ST_SOFTLINK */)
    { 
      /* Ok, it seems to be a directory so I will enter it. */
      /* See whether there's a AnchorChain for that dir... */
      if (NULL != AC->an_Child)
      {
        /* Ok, we're all set. */
        /* Lock the director by it's name. */
        AP->ap_Current = AC->an_Child;
        AC->an_Child->an_Lock = Lock(AC->an_Info.fib_FileName, ACCESS_READ);
        AC = AC->an_Child;
        Examine(AC->an_Lock, &AC->an_Info);
      }
    }
  }
  AP->ap_Flags &= ~(BYTE)(APF_DODIR|APF_DIDDIR);

  /* AC points to the current AnchorChain */
  while (TRUE)
  {
    success = ExNext (AC->an_Lock, &AC->an_Info);
    while (DOSTRUE == success &&
           DOSFALSE == MatchPatternNoCase(AC->an_String,
                                          AC->an_Info.fib_FileName))
    {
      success = ExNext(AC->an_Lock, &AC->an_Info);
    }

    if (DOSFALSE == success)
    {
      /* No more entries in this dir that match. So I might have to
         step back one directory. Unlock the current dir first, 
         !!!!!???? but only if it is not the one from where I started
         Otherwise AROS crashes... 
      */

      if (NULL != AC->an_Parent)
        UnLock(AC->an_Lock);


      AC->an_Lock = BNULL;
      /* Are there any previous directories??? */
      if (NULL != AC->an_Parent)
      {
        /* Step back to this directory and go on searching here */
        AC = AC->an_Parent;
        AP->ap_Current = AC;
        CurrentDir(AC->an_Lock);
        /* I show this dir again as I come back from searching it */
        CopyMem(&AC->an_Info, &AP->ap_Info, sizeof(struct FileInfoBlock));
        AP->ap_Flags |= APF_DIDDIR;
        if (0 != AP->ap_Strlen)
        {
          if (FALSE == writeFullPath(AP))
            return ERROR_BUFFER_OVERFLOW;
        }
        return 0;      
      }
      else
      {
        /* No previous directory, so I am done here... */
        return ERROR_NO_MORE_ENTRIES;
      }
    }
    else
    {
      /* Alright, I found a match... */
      CopyMem(&AC->an_Info, &AP->ap_Info, sizeof(struct FileInfoBlock));
      if (0 != AP->ap_Strlen)
      {
        if (FALSE == writeFullPath(AP))
          return ERROR_BUFFER_OVERFLOW;
      }
      return 0;
    }
  } /* while (TRUE) */
  return 0;
}

void MyMatchEnd(struct AnchorPath * AP)
{
  /* Free the AChain and unlock all locks that are still there */
  struct AChain * AC = AP->ap_Current;
  struct AChain * AC_tmp;
  /* Unlock everything */
  if (NULL == AC)
    
  while (AC != AP->ap_Base)
  {
    UnLock(AC->an_Lock);
    AC = AC->an_Parent;
  }
  /* AC points to the very first AChain obj. in the list */
  
  /* Free the AChain List */
  while (NULL != AC)
  {
    AC_tmp = AC->an_Child;
    FreeVec(AC);
    AC = AC_tmp;
  }
  /* Cleanup AP */
  AP->ap_Base = NULL;
  AP->ap_Current = NULL;
}

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
