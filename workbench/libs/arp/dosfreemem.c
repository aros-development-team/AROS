/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

/*****************************************************************************

    NAME */

      AROS_LH1(void, DosFreeMem,

/*  SYNOPSIS */ 
      AROS_LHA(void *, memBlock, A1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 58, Arp)

/*  NAME
        DosFreeMem -- AmigaDOS compatible memory freer.
 
    SYNOPSIS
        DosFreeMem( memBlock )
                        A1
 
    FUNCTION
        This function frees a memory block which was allocated using
        DosAllocMem or was allocated by AmigaDOS.
 
    INPUTS
        memBlock -- the pointer to the memblock returned by DosAllocMem.
                    This pointer may also be NULL, in which case no
                    memory will be freed.
 
    ADDITIONAL CONSIDERATIONS
        memBlock is not a BPTR - if you are passing a value obtained
        from AmigaDOS, make sure you convert it from a BPTR to a real
        pointer first!
 
    RESULT
        None.
 
    BUGS
        None known.
 
    SEE ALSO
        DosAllocMem()
 
    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)

  if (NULL != memBlock)
  {
    ULONG * Block = (ULONG *) memBlock;

    FreeMem(&Block[-1], Block[-1]); 
  }
  AROS_LIBFUNC_EXIT
} /* DosFreeMem */
