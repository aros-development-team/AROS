/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"
#include <proto/exec.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <dos/dos.h>

/*****************************************************************************

    NAME */
#include <dos/dosasl.h>
#include <proto/dos.h>

	AROS_LH2(LONG, MatchFirst,

/*  SYNOPSIS */
	AROS_LHA(STRPTR             , pat, D1),
	AROS_LHA(struct AnchorPath *, AP , D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 137, Dos)

/*  FUNCTION

	Searches for the first file or directory that matches a given pattern.
	MatchFirst() initializes the AnchorPath structure for you but you
	must initilize the following fields: ap_Flags, ap_Strlen, ap_BreakBits
	and ap_FoundBreak. The first call to MatchFirst() also passes you
	the first matching file which you can examine in ap_Info and the directory
	the files is in in ap_Current->an_Lock. After the first call to
        MatchFirst() call MatchNext().
	The search begins whereever the current directory is set to. See
	CurrentDir();
	For more info on patterns see ParsePattern().

    INPUTS
        pat  - pattern to search for
	AP   - pointer to (initilized) AnchorPath structure
	
    RESULT
	0     = success
	other = DOS error code

    NOTES

    EXAMPLE

    BUGS
	Copying of the relative path to ap_Buf is not implemented yet

    SEE ALSO
	MatchNext(), MatchEnd(), ParsePattern(), Examine(), CurrentDir()
	<dos/dosasl.h>

    INTERNALS

    HISTORY
	29-04-97    bergers, initial revision
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

  struct AChain * AC;
  LONG IsWild;
  if (!pat)
    return FALSE;
  /* Get some buffer for the first AChain including some bits for
     the preparsed pattern */
  AC = AllocVec(sizeof(struct AChain) + 2 * strlen(pat) + 2, MEMF_CLEAR);
  if (NULL != AC)
  {
    BOOL success;
    /* put the preparsed string to the end of the AChain */
    IsWild = ParsePatternNoCase(pat, AC->an_String ,2 * strlen(pat) + 2);
    /* if there are any wildcards then leave info */
    if (1 == IsWild)
  	  AP->ap_Flags = (BYTE)APF_ITSWILD;

    /* create a lock to the current directory */
    AC->an_Lock = CurrentDir(NULL);
    /* if there's no lock for whatsoever reason exit*/
    if (NULL == AC->an_Lock)
    {
      FreeVec(AC);
      return ERROR_DIR_NOT_FOUND;
    }
    (void)CurrentDir(AC->an_Lock);

    /* connect the AChain to the AnchorPath  */
    AP->ap_First = AC;
    AP->ap_Last  = AC;
    AC->an_Flags = (AP->ap_Flags & APF_ITSWILD);
    AP->ap_Flags |= APF_DirChanged;
    /* look for the first file that matches the given pattern */
    success = Examine(AC->an_Lock, &AC->an_Info);
    success = ExNext(AC->an_Lock,&AC->an_Info);
    while (DOSTRUE == success &&
           DOSFALSE== MatchPatternNoCase(AC->an_String,AC->an_Info.fib_FileName))
    {
      success = ExNext(AC->an_Lock,&AC->an_Info);
    }

    /* if no matching file was found return */
    if (DOSFALSE == success)
      return ERROR_NO_MORE_ENTRIES;
    /* a matching file was found! */
    CopyMem(&AC->an_Info, &AP->ap_Info, sizeof(struct FileInfoBlock));
    /* if there's a buffer allocated fill it w/ the appropriate data */
    if (0 != AP->ap_Strlen)
    {
      AP->ap_Buf[0] = '\0';
      strncpy(AP->ap_Buf, AP->ap_Info.fib_FileName, AP->ap_Strlen);
    }
    return 0;
  }
  else
    return ERROR_NO_FREE_STORE;

  return 0;


  AROS_LIBFUNC_EXIT
} /* MatchFirst */
