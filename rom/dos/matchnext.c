/*
    (C) 1995-96 AROS - The Amiga Replacement OS
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

  BOOL success;
  /*  if the last call to MatchNext detected that there's no more
      memory in the system then we just exit */
  if (AP->ap_Flags & APF_NOMEMERR)
    return ERROR_NO_FREE_STORE;

  AP->ap_Flags &=~APF_DirChanged;

  /* if the last FileInfoBlock passed to the user was a directory and
   * the user tells me to enter the directory I should do so.
   */
  if ((AP->ap_Flags & APF_DODIR)  &&
      (ST_USERDIR == AP->ap_Info.fib_DirEntryType  ))
  {
    /* ok let's enter that dir and get some memory for the new AChain
       first  */
    struct AChain * AC = AllocVec(sizeof(struct AChain)+5, MEMF_CLEAR|MEMF_PUBLIC);
    if (NULL != AC)
    {
      char * dirname;
      int i;
      /* let's connet it to the AnchorPath */
      AC->an_Parent = AP->ap_Last;
      AP->ap_Last   = AC;
      if (NULL != AC->an_Parent)
        AC->an_Parent->an_Child  = AC;

      if (NULL == AP->ap_Base) /* this should never happen */
        AP->ap_Base = AC;

      /* let's write the token for any to the pattern */
      /* The following line seems to be the reason why this program throws an exception,
         so I simply replace it with the direct pattern */
      //success = ParsePattern("#?",(char *)&AC->an_String,(ULONG)3);
      AC->an_String[0] = P_ANY;
      AC->an_String[1] = '\0';
      AC->an_Flags  = APF_DOWILD|APF_ITSWILD;
      AP->ap_Flags |= APF_DirChanged;
      AP->ap_Flags &= ~APF_DODIR;
      /* now we need to create a lock to the specified directory in
         AP->ap_Info where we have the name of that thing and the
         lock to it's directory is in AP->ap_Base->an_Parent */
      /* first we need some space for the name, only temporary space */

      i=1;
      for (;;)
      {
        dirname = AllocMem(256*i, MEMF_PUBLIC );
        if (dirname)
        {
          success = NameFromLock(AP->ap_Last->an_Parent->an_Lock, dirname, 256*i);
          success = AddPart(dirname, AP->ap_Info.fib_FileName ,256*i);
          if (0 == success )
          {
            /* stringlength is too long, try again!  */
            FreeMem(dirname, 256*i);
            i++;
          }
          else
          { /* everything went alright, let's create the Lock then*/
            AC->an_Lock = Lock(dirname, ACCESS_READ);
            /* dispose the dirname */
            FreeMem(dirname, 256*i);
            /* the very first examine in this directory would give
               me the directoryname again. We don't want that, so let's
               skip it! */
            Examine(AC->an_Lock, &AC->an_Info);
            break;
          }
        }
        else
        {
          AP->ap_Flags |= APF_NOMEMERR;
          return ERROR_NO_FREE_STORE;
        }
      } /* for */
    }
    else
    { /* no memory */
      AP->ap_Flags |= APF_NOMEMERR;
      return ERROR_NO_FREE_STORE;
    }
  }

  AP->ap_Flags &= ~APF_DODIR;

  /* let's look for the next matching entry as long as there are entries in the
     directory or until we found a match. */
  for(;;)
  {
    success = ExNext(AP->ap_Current->an_Lock, &AP->ap_Current->an_Info);
    if (DOSFALSE == success)
    {
      struct AChain * AC = AP->ap_Current;
      /* tell the user that we did the directory */
      AP->ap_Flags |= (APF_DIDDIR|APF_DirChanged);

      /* remove the current AChain and free the lock. */
      UnLock(AC->an_Lock);
      AP->ap_Current = AC->an_Parent;
      /* now dispose the AChain. */
      FreeVec(AC);
      if (NULL == AP->ap_Current)
      {
        AP->ap_Base = NULL;
        return ERROR_NO_MORE_ENTRIES;
      }
      /* give the user the previous entry */
      CopyMem(&AP->ap_Current->an_Info, &AP->ap_Info , sizeof(struct FileInfoBlock));
      if (0 != AP->ap_Strlen)
      {
        AP->ap_Buf[0] = '\0';
        strncpy(AP->ap_Buf, AP->ap_Info.fib_FileName, AP->ap_Strlen);
      }
      return 0;
    }
    if (MatchPatternNoCase(AP->ap_Current->an_String,AP->ap_Current->an_Info.fib_FileName))
    {
      CopyMem(&AP->ap_Current->an_Info, &AP->ap_Info , sizeof(struct FileInfoBlock));
      if (0 != AP->ap_Strlen)
      {
        strncpy(AP->ap_Buf, AP->ap_Info.fib_FileName, AP->ap_Strlen);
      }
      return 0;
    }
  }

  return 0; /* no error detected */

  AROS_LIBFUNC_EXIT
} /* MatchNext */

