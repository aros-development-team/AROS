/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: english
*/

#include <exec/types.h>
#include <aros/machine.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include "arp_intern.h"

/*****************************************************************************

    NAME */

      AROS_LH4(ULONG, QSort,

/*  SYNOPSIS */ 
      AROS_LHA(void *   , baseptr            , A0),
      AROS_LHA(ULONG    , region_size        , D0),
      AROS_LHA(ULONG    , byte_size          , D1),
      AROS_LHA(LONG_FUNC, user_function      , A1),

/*  LOCATION */
      struct ArpBase *, ArpBase, 71, Arp)

/*   FUNCTION
 	     QSort -- Quickly sort whatever you want.

     SYNOPSIS
 	     stkerr = QSort( baseptr, region_size, byte_size, user_function)
         d0		  a0	        d0           d1		a1

     FUNCTION
 	     QSort is an implementation of Hoares sorting algorithm.  It
 	     uses a constant amount of stack no matter what the size of the
 	     file.

     INPUTS
       baseptr	- pointer to the start of a memory region to be sorted
       region_size - size of region to be sorted, in number of elements (not bytes!)
       byte_size - size of sorted elements, in bytes.
       user_function - function to be provided by caller, which compares two
       elements from the set to be sorted.  QSort will call the user function
       like so:

       return = user_function(el1, el2)
       d0	   	       a0    a1

       Your function must return the following in D0:

 		           if (el1 < el2)	return < 0
 		           if (el1 > el2)	return > 0
 		           if (el1 == el2) return = 0

       You must save all registers except a0, a1, d0, d1.
       (See below for an example of a C calling sequence)

       QSort will also pass A5 to you unchanged.  You can use this register
       to point to pass information to your comparison routine.

    RETURNS
       -1 if everything is cool, 0 if there was an internal recursion
       stack overflow (not too likely).

    EXAMPLE:
       Here is an example of a calling sequence from C, which is to sort
       an array of pointers to strings:

       char	**names;
       long	numEls;
       extern	Cmp();

       if (QSort(names, numELs, 4L, Cmp))
 	           do whatever
       else
 	           STACK_ERROR

       the Cmp function would look like this:

       Cmp()
       {
         {
            #asm
            public	_geta4
            movem.l	d2-d3/a4/a6,-(sp)	; save important registers
            movem.l	a0/a1,-(sp)		; push args
            bsr	_geta4
            bsr	_cmp			; call real compare function
            addq.l	#8,sp			; clean up args
            movem.l	(sp)+,d2-d3/a4/a6	; restore registers
            #endasm
         }
       }

       The cmp function called by the above is a normal C function, it can
       be whatever you like.  Here is a sample one:

       cmp(s1,s2)
       char **s1, **s2;
       {
         return strcmp(*a, *b);
       }

       BUGS
 	   None known.

       INTERNALS

       HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct ArpBase *, ArpBase)
  
  DoQSort((ULONG)baseptr, 0, region_size-1, byte_size, user_function);
  
  return -1;

  AROS_LIBFUNC_EXIT
} /* QSort */


void DoQSort(ULONG baseptr, 
             ULONG l,
             ULONG r,
             ULONG byte_size,
             LONG  (* user_function)())
{
  ULONG left, right, xindex = l + 1;
  if (l < r)
  {
    /* search for an element that is not the minimum */
    do 
    {
      LONG res = AROS_UFC2(ULONG, user_function,
                   AROS_UFCA(void *, (void *)(baseptr + byte_size * (xindex-1)) , A0),
                   AROS_UFCA(void *, (void *)(baseptr + byte_size *  xindex   ) , A1)
                 );      
      if (0 != res)
      {
        if (res > 0)
          xindex--;
        break;     
      }    
      
    } 
    while (xindex <= r); 
    
    if (xindex > r)
    /* all elements are the same!! */
      return;
    else
    {
      left = l; 
      right = r;
      while (TRUE)
      {
	while (left <= right &&
               AROS_UFC2(ULONG, user_function,
                 AROS_UFCA(void *,(void *)(baseptr + byte_size * xindex), A0),
                 AROS_UFCA(void *,(void *)(baseptr + byte_size * left)  , A1)
               ) > 0)
          left++;
        while (right >= left && 
               AROS_UFC2(ULONG, user_function,
                 AROS_UFCA(void *,(void *)(baseptr + byte_size * xindex), A0),
                 AROS_UFCA(void *,(void *)(baseptr + byte_size * right) , A1)
               ) <= 0)
          right--;
        if (left > right) break;
        Exchange((void *) (baseptr + byte_size * left),
                 (void *) (baseptr + byte_size * right),
                 byte_size);
        if (left == xindex)
          xindex = right;
        else
          if (right == xindex)
            xindex = left;
      }
      DoQSort(baseptr, l   , left-1, byte_size, user_function);
      DoQSort(baseptr, left, r     , byte_size, user_function);
    } /* if */
  }
}

void Exchange(void * Lptr1,
              void * Lptr2,
              ULONG byte_size)
{
  /* 32 bit-copy at the beginning */
  while (byte_size & 0xfffffffc)
  {
    ULONG tmp = *(ULONG *)Lptr1;
    *((ULONG *)Lptr1)++ = *(ULONG *)Lptr2;
    *((ULONG *)Lptr2)++ = tmp;
    byte_size -= 4;
  }
  
  /* one 16-bit copy */
  if (byte_size & 0xfffffffe)
  {
    WORD tmp = *((WORD *)Lptr1);
    *((WORD *)Lptr1)++ = *(WORD *)Lptr2;
    *((WORD *)Lptr2)++ = (WORD)tmp;  
    byte_size -= 2;
  }
  
  /* one 8-bit copy */
  if (0 != byte_size)
  {
    BYTE tmp = *((BYTE *)Lptr1);
    *((BYTE *)Lptr1)++ = *(BYTE *)Lptr2;
    *((BYTE *)Lptr2)++ = (BYTE)tmp;  
  }  
}
