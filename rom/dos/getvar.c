/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: GetVar - Return the value of a local or global variable.
    Lang: english
*/
#include "dos_intern.h"
#include <proto/exec.h>
#include <dos/dos.h>
/*****************************************************************************

    NAME */
#include <dos/var.h>
#include <proto/dos.h>

	AROS_LH4(LONG, GetVar,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, D1),
	AROS_LHA(STRPTR, buffer, D2),
	AROS_LHA(LONG  , size, D3),
	AROS_LHA(LONG  , flags, D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 151, Dos)

/*  FUNCTION
	This function will return the value of a local or environmental
	variable in the supplied buffer.

	It is advised to only use ASCII characters with a variable, but
	this is not required.

	If GVF_BINARY_VAR is not specified, this function will stop putting
	characters into the destination buffer when a '\n' is hit, or the
	end of the buffer is reached. Otherwise it will complete fill the
	buffer.

    INPUTS
	name    -   the name of the variable you want.
	buffer  -   Space to store the returned variable.
	size    -   Length of the buffer in bytes.
	flags   -   A combination of the type of variable to get (lower
		    8 bits) and flags that control the value of this
		    function. Current flags are:

		    GVF_GLOBAL_ONLY    - only tries to get a global variable.
		    GVF_LOCAL_ONLY     - only tries to get a local variable.
		    GVF_BINARY_VAR     - do not stop at a '\n' character.
		    GVF_DONT_NULL_TERM - no NULL termination. This only
					 applies to GVF_BINARY_VAR.

    RESULT
	Will return the number of characters put in the buffer, or -1
	if the variable is not defined. The '\n' character if it exists
	will not be placed in the buffer.

	If the value would overflow the user buffer, then the number of
	characters copied into the buffer will be returned and the buffer
	truncated.The buffer will be NULL terminated unless
	GVF_DONT_NULL_TERM is set.

	IoErr() will contain either:
	  ERROR_OBJECT_NOT_FOUND
	      if the variable is not defined.
	  ERROR_BAD_NUMBER
	      if the size of the buffer is 0.
	  the total length of the variable
	      otherwise.

    NOTES

    EXAMPLE

    BUGS
	LV_VAR is the only type that can be global.

    SEE ALSO
	DeleteVar(), FindVar(), SetVar()

    INTERNALS
	Redo the RESULT documentation.

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    if(name && buffer)
    {
	if( (flags & GVF_GLOBAL_ONLY) == 0)
	{
	    struct LocalVar *lv = NULL;

	    lv = FindVar(name, flags & 0xFF);
	    if(lv)
	    {
		if(size == 0)
		{
		    SetIoErr(ERROR_BAD_NUMBER);
		    return 0;
		}

		if( (flags & (GVF_BINARY_VAR|GVF_DONT_NULL_TERM))
		    == (GVF_BINARY_VAR|GVF_DONT_NULL_TERM) )
		{
		    int i;

		    /* We are not a binary/no null term get() */
		    size--;

		    /* Search for the '\n', and use it as the size. */
		    for(i = 0; lv->lv_Value[i] && (i < size); i++)
			if(lv->lv_Value[i] == '\n')
			    break;

		    size = i + 1;
		}

		/* If we can fit everything into the buffer. */
		if(lv->lv_Len < size)
		    size = lv->lv_Len;

		CopyMem(lv->lv_Value, buffer, size);

		if( (flags & (GVF_BINARY_VAR|GVF_DONT_NULL_TERM))
		    != (GVF_BINARY_VAR|GVF_DONT_NULL_TERM) )
		{
		    buffer[size] = 0;
		}

		SetIoErr(lv->lv_Len);
		return size;

	    } /* Got lv */
	} /* !global only */

	if( (flags & GVF_LOCAL_ONLY) )
	{
	    BPTR file;
	    UBYTE filebuf[256];

	    filebuf[0] = 0;
	    AddPart(filebuf, "ENV:", 256);
	    AddPart(filebuf, name, 256);

	    file = Open(filebuf, MODE_OLDFILE);
	    if(file)
	    {
		ULONG fSize = 0;
		struct FileInfoBlock fib;

		if(size == 0)
		{
		    SetIoErr(ERROR_BAD_NUMBER);
		    return -1;
		}

		if(ExamineFH(file, &fib))
		{
		    /* fSize now contains the size of variable. */
		    fSize = fib.fib_Size;
		}
		else
		    return -1;

		if( (flags & (GVF_BINARY_VAR|GVF_DONT_NULL_TERM))
		    != (GVF_BINARY_VAR|GVF_DONT_NULL_TERM) )
		{
		    /* Space for the NULL terminator */
		    size--;
		}

		if(size > fSize)
		    size = fSize;

		/* We return the number of bytes actually read. */
		size = Read(file, buffer, size);

		if( (flags & (GVF_BINARY_VAR|GVF_DONT_NULL_TERM))
		    != (GVF_BINARY_VAR|GVF_DONT_NULL_TERM) )
		{
		    int i;
		    for(i = 0; i < size; i++)
		    {
			if(buffer[i] == '\n')
			{
			    buffer[i] = 0;
			    break;
			}
		    }
		    buffer[size] = 0;
		}

		Close(file);

		SetIoErr(fSize);
		return size;
	    } /* open(file) */

	    SetIoErr(ERROR_OBJECT_NOT_FOUND);
	} /* ! local file only */

    } /* name and buffer */

    return -1;

    AROS_LIBFUNC_EXIT
} /* GetVar */
