/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Internal utility functions.
*/

#include <aros/debug.h>
#include <aros/system.h>
#include "alib_intern.h"

#ifdef AROS_SLOWSTACKMETHODS
#include <intuition/classusr.h>
#include <proto/exec.h>
/******************************************************************************

    NAME */
	Msg GetMsgFromStack (

/*  SYNOPSIS */
	IPTR	MethodID,
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
	This function fails for structures with more than 32 fields.

    EXAMPLE

    BUGS

    SEE ALSO
	intuition.library/NewObjectA(), intuition.library/SetAttrsA(), intuition.library/GetAttr(),
	intuition.library/DisposeObject(), DoMethodA(),
	DoSuperMethodA(), "Basic Object-Oriented Programming System for
	Intuition" and the "boopsi Class Reference" documentation.

    INTERNALS
	HPPA: Allocate a structure which can contain all IPTRs between
	the first argument of, for example, DoMethod() and its first local
	variable. This will copy a bit too much memory but in the end, it
	saves a lot of work, since it's not neccessary to register every
	structure.

******************************************************************************/
{
    ULONG size;
    Msg   msg;

    size = 33;

    if ((msg = AllocVec (size * sizeof (IPTR), MEMF_CLEAR)))
    {
	IPTR * ulptr = (IPTR *)msg;

	*ulptr ++ = MethodID;

	while (-- size)
	{
	    *ulptr ++ = va_arg (args, IPTR);
	}
    }

    return msg;
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

******************************************************************************/
{
    FreeVec(msg);
} /* FreeMsgFromStack */

#endif /* AROS_SLOWSTACKMETHODS */

#ifdef AROS_SLOWSTACKTAGS
#include <stdarg.h>
#include <utility/tagitem.h>
#include <exec/memory.h>
#include <proto/exec.h>
/******************************************************************************

    NAME */
	struct TagItem * GetTagsFromStack (

/*  SYNOPSIS */
	IPTR	firstTag,
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
	intuition.library/NewObjectA(), intuition.library/SetAttrsA(), intuition.library/GetAttr(),
	intuition.library/DisposeObject(), DoMethodA(),
	DoSuperMethodA(), "Basic Object-Oriented Programming System for
	Intuition" and the "boopsi Class Reference" documentation.

    INTERNALS
	Allocate a structure which can contain all tags until the first
	TAG_END. This takes into account TAG_MORE and the like. The code
	will break if someone makes assumptions about the way taglists
	are built in memory, ie. if he looks for the next TAG_MORE and
	then simply skips it instead of following it.

******************************************************************************/
{
    struct TagItem * ti;
    IPTR	     tag;
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
        (void) va_arg(args, IPTR);
	    size --; /* Don't copy this tag */
	    break;

	case TAG_SKIP: {
	    ULONG skip;

	    skip = va_arg(args, IPTR);

	    while (skip --)
	    {
		(void) va_arg(args, IPTR);
		(void) va_arg(args, IPTR);
	    }

	    break; }

	default:
	    (void) va_arg(args, IPTR);
	}

	tag = va_arg (args, IPTR);
    }

    tag  = firstTag;

    if ((ti = AllocVec (size*sizeof(struct TagItem), MEMF_ANY)))
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
        (void) va_arg(ap, IPTR);
		size --; /* Don't copy this tag */
		break;

	    case TAG_SKIP: {
		ULONG skip;

		skip = va_arg(ap, IPTR);

		while (skip --)
		{
		    (void) va_arg(ap, IPTR);
		    (void) va_arg(ap, IPTR);
		}

		break; }

	    default:
		ti[size].ti_Data = va_arg(ap, IPTR);
	    }

	    tag = va_arg (ap, IPTR);
	}
    }
    va_end(ap);
    return ti;
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

******************************************************************************/
{
    FreeVec(tags);
} /* FreeTagsFromStack */

/******************************************************************************

    NAME */
	APTR GetParamsFromStack (

/*  SYNOPSIS */
	va_list args)

/*  FUNCTION
	Builds an array of parameters which are passed on the stack.
	This function is used on machines which have compilers which
	don't pass the arguments to a varargs function unlike the
	Amiga ones.

    INPUTS
	args - This has to be initialized by va_start()

    RESULT
	An array which can be passed to any function which expects the
	structure or NULL if something failed. This call may fail for
	different reasons on different systems. On some systems, NULL 
	indicates that there was not enough	memory.

    NOTES
	This function fails for structures with more than 20 fields.

    EXAMPLE

    BUGS

    SEE ALSO
	CallHook()

    INTERNALS
	HPPA: Allocate a structure which can contain all APTRs between the
	first variadic parameter of, for example, CallHook() and its first
	local variable. This will copy a bit too much memory but in the end,
	it saves a lot of work, since it's not neccessary to register every
	structure.

******************************************************************************/
{
    ULONG size;
    APTR params;

    size = 21;

    if ((params = AllocVec (size * sizeof (IPTR), MEMF_CLEAR)))
    {
	IPTR * ulptr = (IPTR *) params;

	while (-- size)
	{
	    *ulptr ++ = va_arg (args, IPTR);
	}
    }

    return params;
} /* GetParamsFromStack */

/******************************************************************************

    NAME */
	void FreeParamsFromStack (

/*  SYNOPSIS */
	APTR params)

/*  FUNCTION
	Frees the memory occupied by the parameters array which was
	created by GetParamsFromStack().

    INPUTS
	params - The return value of GetParamsFromStack(). May be NULL.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetParamsFromStack()

******************************************************************************/
{
    FreeVec(params);
} /* FreeParamsFromStack */
#endif /* AROS_SLOWSTACKTAGS */

/******************************************************************************

    NAME */
	VOID GetDataStreamFromFormat (

/*  SYNOPSIS */
	CONST_STRPTR format,
	va_list args,
	APTR dataStream, ULONG *dataSize,
	ULONG *indexStream, ULONG *indexSize)

/*  FUNCTION
	Builds an array of parameters which are passed on the stack.
	This function is used on machines which have compilers which
	don't pass the arguments to a varargs function unlike the
	Amiga ones.

    INPUTS
        format - Exec/RawDoFmt or Locale/FormatString format string
	args   - This has to be initialized by va_start()
	         (not used if dataStream is NULL)
	dataStream - data buffer to write to
	             (can be NULL for sizing)
	dataSize  - size of the buffer
	             (can be NULL, or pointer to 0 for sizing)
	             updated to the actual size required at exit.
	indexStream- array of offsets to the Nth element in the dataStream
	             (can be NULL for sizing)
	indexSize  - size of the index, in bytes
	             (can be NULL, or pointer to 0 for sizing)
	             updated to the actual size required at exit.

    RESULT
	An array which can be passed to any function which expects the
	structure or NULL if something failed. This call may fail for
	different reasons on different systems. On some systems, NULL
	indicates that there was not enough memory.

    NOTES
        This structure converts from format types to the following
        dataStream values in the returned structure. The dataStream
        is suitable for use by Exec/RawDoFmt or Locale/FormatString,
        and is packed on WORD alignment. The indexStream is used
        internally by Locale/FormatString for format repositioning.

        Format code     GET_ARG() type           Datastream type
        -----------     -------------           ---------------
        %c              char (really int)       WORD
        %d/%D           int                     WORD
        %u/%U           unsigned int            UWORD
        %x/%X           unsigned int            UWORD
        %b              BPTR                    IPTR
        %s              const char *            IPTR
        %p/%P           void *                  IPTR
        %l<cdDuUxX>     LONG                    LONG
        %ll<cdDuUxX>    QUAD                    QUAD

    EXAMPLE

    BUGS

    SEE ALSO
	exec.library/RawDoFmt(), locale.library/FormatString()

    INTERNALS

******************************************************************************/
{
    size_t len = dataSize ? *dataSize : 0;
    int    ilen = (indexSize ? *indexSize : 0) / sizeof(indexStream[0]);
    int    imax = 0;
    size_t size = 0;
    size_t isize = 0;
    size_t size_force = 0;
    BOOL in_format = FALSE;
    ULONG iflags = 0;
    size_t vsize = 0, dsize = 0;
    unsigned int argpos = 0;

#define IFLAG_SIGNED    (1UL << 31)
#define IFLAG_VSIZE(x)  ((x) << 16)
#define IFLAG_VSIZE_of(x)  (((x) >> 16) & 0xf)
#define IFLAG_DSIZE(x)  ((x) << 0)
#define IFLAG_DSIZE_of(x)  (((x) >> 0) & 0xf)
#define GET_ARG(args, type) (dataStream ? va_arg(args, type) : 0)

    for (size = 0; format && *format; format++) {
        if (!in_format) {
            if (*format == '%') {
                in_format = TRUE;
                vsize = dsize = 0;
                size_force = 0;
                argpos = 0;
                iflags = 0;
            }
        } else {
            /* Ignore non-argument characters */
            if ((*format >= '0' && *format <= '9')) {
                argpos = (argpos * 10) + (*format - '0');
                continue;
            }

            switch (*format) {
            case '.':
                break;
            case '-':
                break;
            case '$':
                isize = argpos - 1;
                argpos = 0;
                break;
            case 'l':
                if (size_force >= sizeof(LONG))
                    size_force = sizeof(QUAD);
                else
                    size_force = sizeof(LONG);
                break;
            case 'd':
            case 'D':
                vsize = sizeof(int);
                dsize = sizeof(WORD);
                iflags |= IFLAG_SIGNED;
                in_format = FALSE;
                break;
            case 'c':   /* char is promoted to int through (fmt, ...) */
            case 'u':
            case 'U':
            case 'x':
            case 'X':
                vsize = sizeof(unsigned int);
                dsize = sizeof(WORD);
                in_format = FALSE;
                break;
            case 'p':
            case 'P':
            case 's':
                vsize = sizeof(void *);
                dsize = sizeof(IPTR);
                in_format = FALSE;
                break;
            case 'b':
            case 'B':
                vsize = sizeof(BPTR);
                dsize = sizeof(BPTR);
                in_format = FALSE;
                break;
            default:
                vsize = sizeof(int);
                dsize = sizeof(WORD);
                in_format = FALSE;
                break;
            }

            if (in_format == FALSE) {
                D(bug("%s: '%c' l=%d (v=%d, d=%d)\n", __func__, *format, size_force, vsize, dsize));
                if (size_force) {
                    vsize = size_force;
                    dsize = size_force;
                }

                iflags |= IFLAG_VSIZE(vsize);
                iflags |= IFLAG_DSIZE(dsize);

                if (ilen > isize)
                    indexStream[isize] = iflags;

                isize++;
                if (isize > imax)
                    imax = isize;
            }
        }
    }

    /* Convert indexStream flags into offsets
     * into the datastream. If the datastream
     * is present, assume we have to pull its
     * data from the va_list.
     */
    if (indexStream) {
        int i;

        for (i = 0; i < imax && i < ilen; i++) {
            IPTR arg_val;
            CONST_APTR buff = (CONST_APTR)((IPTR)dataStream + size);
            ULONG iflags = indexStream[i];

            D(bug("%s: indexStream[%d] = %d\n", __func__, i, (int)size));
            indexStream[i] = size;
            size += IFLAG_DSIZE_of(iflags);

            if (!dataStream)
                continue;

            /* dataStream present - pull its data from the va_list.
             */
            switch (IFLAG_VSIZE_of(iflags)) {
            case sizeof(LONG):
                if (iflags & IFLAG_SIGNED)
                    arg_val = (SIPTR)va_arg(args, LONG);
                else
                    arg_val = (IPTR)va_arg(args, ULONG);
                break;
           case sizeof(QUAD):
                if (iflags & IFLAG_SIGNED)
                    arg_val = (SIPTR)va_arg(args, QUAD);
                else
                    arg_val = (IPTR)va_arg(args, UQUAD);
                break;
           default:
                if (iflags & IFLAG_SIGNED)
                    arg_val = (SIPTR)va_arg(args, int);
                else
                    arg_val = (IPTR)va_arg(args, int);
                break;
            }

            D(bug("%s: dataStream len = 0x%x (%d)\n", __func__, (unsigned int)len, dataSize ? (int)*dataSize : -1));
            D(bug("%s: dataStream + 0x%x (%d) = 0x%p\n", __func__, (int)((IPTR)buff - (IPTR)dataStream), IFLAG_DSIZE_of(iflags), (void *)arg_val));
            if (len >= size) {
                switch (IFLAG_DSIZE_of(iflags)) {
                case sizeof(UWORD):
                    *((UWORD *)buff) = (UWORD)arg_val;
                    break;
                case sizeof(ULONG):
                    *((ULONG *)buff) = (ULONG)arg_val;
                    break;
                case sizeof(UQUAD):
                    *((UQUAD *)buff) = (UQUAD)arg_val;
                    break;
                }
            }
        }
    }

    if (dataSize)
        *dataSize = size;

    if (indexSize)
        *indexSize = imax * sizeof(indexStream[0]);
} /* GetDataStreamFromFormat */
