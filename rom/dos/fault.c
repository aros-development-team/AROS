/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: dos.library function Fault()
    Lang: english
*/
#include <aros/options.h>
#include "dos_intern.h"
#if PassThroughErrnos
#   include <errno.h>
#endif
#include <string.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH4(BOOL, Fault,

/*  SYNOPSIS */
	AROS_LHA(LONG,         code,   D1),
	AROS_LHA(CONST_STRPTR, header, D2),
	AROS_LHA(STRPTR,       buffer, D3),
	AROS_LHA(LONG,         len,    D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 78, Dos)

/*  FUNCTION
	Fault will obtain the error message string for the given error
	code. First the header string is copied to the buffer, followed
	by a ":" (colon), then the NULL terminated string for the error
	message into the buffer.

	By convention, error messages are ALWAYS less than 80 (plus 1 for
	NULL termination), and ideally less than 60 characters.

	If the error code is not know, then the string "Unknown error"
	followed by the error number will be added to the string.

    INPUTS
	code	-   The error code.
	header	-   The string to prepend to the buffer before the error
		    text. This may be NULL in which case nothing is prepended.
	buffer	-   The destination buffer.
	len	-   Length of the buffer.

    RESULT
	Number of characters placed in the buffer, may be 0.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    LONG index = 0;
    STRPTR theString;

    if (code == 0)
    {
	*buffer = '\0';
	return 0;
    }

    /* Do this to make sure there is room for a NULL terminator */
    len--;

    if (header)
    {
	while((index < len) && *header)
	{
	    buffer[index++] = *header++;
	}

	buffer[index++] = ':';
	buffer[index++] = ' ';
    }

    theString = DosGetString(code);
#if PassThroughErrnos
    if ((!theString) && (code & PassThroughErrnos))
    {
	theString = strerror (code ^ PassThroughErrnos);
    }
#endif
    if(theString)
    {
	while((index < len) && *theString)
	{
	    buffer[index++] = *theString++;
	}
    }
    else
    {
	/* String buffer/index for long 2 string */
	UBYTE l2str[12], l2idx = 11;

	theString = "Unknown error ";
	while((index < len) && *theString)
	{
	    buffer[index++] = *theString++;
	}

	/* If the number is negative, whack in a - sign. */
	if(code < 0)
	{
	    code = -code;
	    buffer[index++] = '-';
	}

	/* Convert the number to a string, I work backwards, its easier */
	l2str[l2idx--] = '\0';
	while(code != 0)
	{
	    l2str[l2idx--] = (code % 10) + '0';
	    code /= 10;
	}

	l2str[l2idx] = ' ';

	/* Copy the number onto the fault string */
	while((index < len) && l2str[l2idx])
	{
	    buffer[index++] = l2str[l2idx++];
	}
    }
    buffer[index] = '\0';
    return (len - index + 1);

    AROS_LIBFUNC_EXIT
} /* Fault */
