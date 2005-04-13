/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Internal utility functions.
*/

#include <aros/system.h>
#include "alib_intern.h"

#ifdef AROS_SLOWSTACKMETHODS
/******************************************************************************

    NAME */
	Msg GetMsgFromStack (

/*  SYNOPSIS */
	ULONG	MethodID,
	va_list args)

/*  FUNCTION
	Builds a message structure with the parameters which are passed on
	the stack. This function is used on machines which have compilers
	which don't pass the arguments to a varargs function unlike the
	Amiga ones.

    INPUTS
	MethodID - This is the ID of the message
	args - This has to be initialized by va_start()
	firstlocal - The address of the first local function of the
		function which wants to call GetMsgFromStack()

    RESULT
	A message which can be passed to any function which expects the
	structure which is defined for this MethodID or NULL if something
	failed. This call may fail for different reasons on different
	systems. On some systems, NULL indicates that there was not enough
	memory, on others that the MethodID is unknown.

    NOTES
	This function fails for structures with more than 20 fields.

    EXAMPLE

    BUGS

    SEE ALSO
	NewObject(), SetAttrs(), GetAttr(), DisposeObject(), DoMethod(),
	DoSuperMethod(), "Basic Object-Oriented Programming System for
	Intuition" and the "boopsi Class Reference" Dokument.

    INTERNALS
	HPPA: Allocate a structure which can contain all ULONGs between
	the first argument of, for example, DoMethod() and its first local
	variable. This will copy a bit too much memory but in the end, it
	saves a lot of work, since it's not neccessary to register every
	structure.

    HISTORY
	22.11.96 digulla created

******************************************************************************/
{
#if !AROS_STACK_GROWS_DOWNWARDS
    ULONG size;
    Msg   msg;

    size = 21;

    if ((msg = AllocVec (size * sizeof (ULONG), MEMF_CLEAR)))
    {
	ULONG * ulptr = (ULONG * msg);

	*ulptr ++ = MethodID;

	while (-- size)
	{
	    *ulptr ++ = va_arg (args, ULONG);
	}
    }

    return msg;
#else
    return NULL;
#endif
} /* GetMsgFromStack */

/******************************************************************************

    NAME */
	void FreeMsgFromStack (

/*  SYNOPSIS */
	Msg msg)

/*  FUNCTION
	Frees the memory occupied by the message which was created by
	GetMsgFromStack().

    INPUTS
	msg - The return value of GetMsgFromStack(). May be NULL.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetMsgFromStack()

    HISTORY
	22.11.96 digulla created

******************************************************************************/
{
    if (msg)
	FreeVec (msg);
} /* FreeMsgFromStack */

#endif /* AROS_SLOWSTACKMETHODS */

#ifdef AROS_SLOWSTACKTAGS
/******************************************************************************

    NAME */
	struct TagItem * GetTagsFromStack (

/*  SYNOPSIS */
	ULONG	firstTag,
	va_list args)

/*  FUNCTION
	Builds a tagitem array with the tags on the stack. This function is
	used on machines which have compilers which don't pass the
	arguments to a varargs function unlike the Amiga ones.

    INPUTS
	firstTag - This is the first tag passed to the function
	args - This has to be initialized by va_start()

    RESULT
	A TagItem array which can be passed to any function which expects
	such an array or NULL if something failed. This call may fail for
	different reasons on different systems. On some systems, NULL
	indicates that there was not enough memory, on others that the
	MethodID is unknown.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	NewObject(), SetAttrs(), GetAttr(), DisposeObject(), DoMethod(),
	DoSuperMethod(), "Basic Object-Oriented Programming System for
	Intuition" and the "boopsi Class Reference" Dokument.

    INTERNALS
	Allocate a structure which can contain all tags until the first
	TAG_END. This takes into account TAG_MORE and the like. The code
	will break if someone makes assumptions about the way taglists
	are built in memory, ie. if he looks for the next TAG_MORE and
	then simply skips it instead of following it.

    HISTORY
	22.11.96 digulla created

******************************************************************************/
{
    struct TagItem * ti;
    ULONG	     tag;
    ULONG	     size;
    va_list	     ap;

    va_copy(ap, args);
    tag = firstTag;

    for (size=0;;size++)
    {
	if (tag == TAG_END || tag == TAG_MORE)
	{
	    size ++; /* Copy this tag, too */
	    break;
	}

	switch (tag)
	{
	case TAG_IGNORE:
	    size --; /* Don't copy this tag */
	    break;

	case TAG_SKIP: {
	    ULONG skip;

	    skip = va_arg(args, IPTR);

	    while (skip --)
	    {
		(void) va_arg(args, ULONG);
		(void) va_arg(args, IPTR);
	    }

	    break; }

	default:
	    (void) va_arg(args, IPTR);
	}

	tag = va_arg (args, ULONG);
    }

    tag  = firstTag;

    if ((msg = AllocVec (size*sizeof(TagItem), MEMF_ANY)))
    {
	for (size=0;;size++)
	{
	    ti[size].ti_Tag = tag;

	    if (tag == TAG_END)
		break;
	    else if (tag == TAG_MORE)
	    {
		ti[size].ti_Data = (IPTR) va_arg (ap, struct TagItem *);
		break;
	    }

	    switch (tag)
	    {
	    case TAG_IGNORE:
		size --; /* Don't copy this tag */
		break;

	    case TAG_SKIP: {
		ULONG skip;

		skip = va_arg(ap, IPTR);

		while (skip --)
		{
		    (void) va_arg(ap, ULONG);
		    (void) va_arg(ap, IPTR);
		}

		break; }

	    default:
		ti[size].ti_Data = va_arg(ap, IPTR);
	    }

	    tag = va_arg (ap, ULONG);
	}
    }
    va_end(ap);
    return msg;
} /* GetTagsFromStack */

/******************************************************************************

    NAME */
	void FreeTagsFromStack (

/*  SYNOPSIS */
	struct TagItem * tags)

/*  FUNCTION
	Frees the memory occupied by the tagitems which were created by
	GetTagsFromStack().

    INPUTS
	tags - The return value of GetTagsFromStack(). May be NULL.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetTagsFromStack()

    HISTORY
	22.11.96 digulla created

******************************************************************************/
{
    if (tags)
	FreeVec (tags);
} /* FreeTagsFromStack */

#endif /* AROS_SLOWSTACKTAGS */



