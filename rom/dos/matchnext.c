/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/dosasl.h>
#include <proto/dos.h>

	AROS_LH1(LONG, MatchNext,

/*  SYNOPSIS */
	AROS_LHA(struct AnchorPath *, AP, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 138, Dos)

/*  FUNCTION
	Find next file or directory that matches a given pattern.
	See <dos/dosasl.h> for more docs and how to control MatchNext().


    INPUTS
	AP  - pointer to Anchor Path structure which had been passed to
              MatchFirst() before.

    RESULT

    NOTES

    EXAMPLE

    BUGS
	Copying of the relative path to ap_Buf is not implemented yet


    SEE ALSO
	MatchFirst() MatchEnd() CurrentDir() Examine() ExNext()
	ParsePattern() <dos/dosasl.h>

    INTERNALS

    HISTORY
	29-04-97    bergers, initial revision
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

  /* 
  ** If the user says I am supposed to enter the directory then I first check
  ** whether it is a directory... 
  */
  
  struct AChain * AC = AP->ap_Current;
  BOOL success;
  struct Task * task = FindTask(NULL);
  
  AP->ap_BreakBits &= (SIGBREAKF_CTRL_C|
                       SIGBREAKF_CTRL_D|
                       SIGBREAKF_CTRL_E|
                       SIGBREAKF_CTRL_F);
  
  if (0 != (AP->ap_Flags & APF_DODIR ))
  {
    if (AC->an_Info.fib_DirEntryType >= 0 /* &&
        AC->an_Info.fib_DirEntryType != ST_SOFTLINK */)
    { 

      /* Ok, it seems to be a directory so I will enter it. */
      /* See whether there's a AnchorChain for that dir... */
      if (NULL == AC->an_Child)
      {
        AC->an_Child = (struct AChain *)
           AllocVec(sizeof(struct AChain)+1, MEMF_CLEAR);
        
        AC->an_Child->an_Parent = AC;
        
        AC->an_Child->an_String[0] = P_ANY;
        AC->an_Child->an_Flags = DDF_PatternBit;
        
//kprintf("Created temporary AChain structure!\n");
      }
      
      if (NULL != AC->an_Child)
      {
        BPTR newdir;
        /* Ok, we're all set. */
        /* Lock the director by it's name. */
        AP->ap_Current = AC->an_Child;
        
        newdir = Lock(AC->an_Info.fib_FileName, ACCESS_READ);
//kprintf("Locking dir %s\n",AC->an_Info.fib_FileName);
        (void)CurrentDir(newdir);

        AC = AC->an_Child;
        AC->an_Lock = newdir;
        Examine(AC->an_Lock, &AC->an_Info);
      }
      else
        return ERROR_NO_FREE_STORE;
    }
  }
  
  AP->ap_Flags &= ~(BYTE)(APF_DODIR|APF_DIDDIR);


  /* 
  ** AC points to the current AnchorChain 
  */
  while (TRUE)
  {
    do
    {
      ULONG breakbits;
      /*
      ** Check for a break signal CTRL C/D/E/F
      */
      breakbits = (AP->ap_BreakBits & task->tc_SigRecvd);
      
      if (0 != breakbits)
      {
        /*
        ** Finish right here... there might be a problem when/if the
        ** algorithm is resumed the next time... Gotta test that.
        */
        AP->ap_FoundBreak = breakbits;
        return ERROR_BREAK;
      }

      success = ExNext (AC->an_Lock, &AC->an_Info);
    }
    while (DOSTRUE == success &&
           DOSFALSE == MatchPatternNoCase(AC->an_String,
                                          AC->an_Info.fib_FileName));


    if (DOSFALSE == success)
    {
      /* 
      ** No more entries in this dir that match. So I might have to
      ** step back one directory. Unlock the current dir first.
      */

//kprintf("Couldn't find a matching file.!\n");

      if (AP->ap_Base == AC)
      {
        UnLock(AC->an_Lock);
        AP->ap_Current = AC->an_Parent;
        return ERROR_NO_MORE_ENTRIES;
      }

      /* 
      ** Are there any previous directories??? 
      */
      if (NULL != AC->an_Parent && NULL != AC)
      {
        LONG retval = 0;

        UnLock(AC->an_Lock);
        AC->an_Lock = NULL;

        AC             = AC->an_Parent;
        AP->ap_Current = AC;
        
        if (AC->an_Child->an_Flags & DDF_PatternBit)
        {
          FreeVec(AC->an_Child);
          AC->an_Child = NULL;
        }
        else
          if (0 == (AC->an_Flags & DDF_PatternBit))
          {
            /*
            ** In this case I must silently follow the pattern again...
            */
            return followpattern(AP, AC, DOSBase);
          }
        
        AP->ap_Flags |= APF_DIDDIR;
        /* 
        ** I show this dir again as I come back from searching it 
        */
        
        retval = createresult(AP, AC, DOSBase);

        /* 
        ** Step back to this directory and go on searching here 
        */
        
        CurrentDir(AC->an_Lock);
        
        if (NULL == AC->an_Parent)
          retval = ERROR_NO_MORE_ENTRIES;

        return retval;
      }
      
      /* 
      ** No previous directory, so I am done here... 
      */
      return ERROR_NO_MORE_ENTRIES;
    }
    else
    {
      /* Alright, I found a match... */
      return createresult(AP, AC, DOSBase);
    }
  } /* while (TRUE) */
  return 0;

  AROS_LIBFUNC_EXIT
} /* MatchNext */

