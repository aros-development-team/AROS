/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Parse a set of attributes in a single interface
    Lang: english
*/
#include <exec/lists.h>
#include <proto/exec.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <proto/oop.h>

	AROS_LH5(LONG, OOP_ParseAttrs,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, tags, A0),
	AROS_LHA(IPTR *, storage, A1),
	AROS_LHA(ULONG , numattrs, D0),
	AROS_LHA(OOP_AttrCheck *, attrcheck, A2),
	AROS_LHA(OOP_AttrBase  , attrbase, D1),

/*  LOCATION */
	struct Library *, OOPBase, 20, OOP)

/*  FUNCTION
	Parse a taglist of attributes and put the result in an array.
	It will only parse the attr from a single interface
	which is indicated by the 'attrbase' parameter.

    INPUTS
    	tags - tags to be parsed.
	storage - array where the tag values will be stored.
		  To get the value for a certain tag just use
		  ao#? attribute offset as an index into the array.
		  The array must be of size 'numattrs', ie. the number
		  of attributes in the interface.
		  
	numattrs - number of attributes in the interface.
	attrcheck - will is a flag that where flags will be set according
	            to the attributes' offset. Since this is only 32
		    bytes you can only parse interfaces
		    with <= 32 attributes with this function.
		    If you try with more, you will get a
		    ooperr_ParseAttrs_TooManyAttrs error.
		    The flags will be set like this if an attr is found:
		    
		    attrcheck |= 1L << attribute_offset
		    
	attrbase - attrbase for the interface whise attrs we should look for.
		    

    RESULT
    	0 for success, and an error otherwise.
	Possible values are:
		ooperr_ParseAttrs_TooManyAttrs.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    LONG err = 0;
    BOOL done = FALSE;
    
    if (numattrs > 32)
    	return ooperr_ParseAttrs_TooManyAttrs;

    /* Parse the taglist. Since we reimplement NexTagItem() instead
       of calling it this hould be really fast. */
    while (!done) {
    	register ULONG tag = tags->ti_Tag;
	
	/* Instead of having a default: in switch we have an if test
	   here because tag > TAG_SKIP is the most-often case.
	   
	   With the way gcc generates code for switc() when
	   there are low number of case: (no jumptable,
	   just "binarysearch" jumping) this is much faster.
	*/
    	if (tag > TAG_SKIP) {
	    /* Get the attribute offset */
	    /* Look for same attrbase */
	    if ((tag & (~METHOD_MASK)) == attrbase) {
	    	register ULONG offset;
		
		offset = tag & METHOD_MASK;
		if (offset >= numattrs)
		    continue;
		   
// bug("---PARSERATTRS: OFFSET %d\n", offset);
	    	storage[offset] = tags->ti_Data;
		/* Mark it as found */
// bug("--- ADDING %p TO ATTRCHECK\n", 1L << offset);
		*attrcheck |= 1L << offset;
	    }
	} else {
	    switch (tag) {
	    	case TAG_DONE:
	    	    return err;
		
	    	case TAG_IGNORE:
		    break;
		
	    	case TAG_MORE:
	    	    tags = (struct TagItem *)tags->ti_Data;
		    if (NULL == tags)
		    	return err;
		    continue;
		
	    	case TAG_SKIP:
	    	    tags += tags->ti_Data + 1;
		    break;
	    } /* swithc() */
	} /* if (tag <= TAG_SKIP) */
	
	tags ++;
    }
    
    return err;
    
    AROS_LIBFUNC_EXIT
} /* OOP_ParseAttrs */
