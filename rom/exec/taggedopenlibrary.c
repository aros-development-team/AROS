/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Shortcut OpenLibrary call for system modules (private!)
    Lang: english
*/

#include <aros/libcall.h>
#include <proto/exec.h>

#include LC_LIBDEFS_FILE

static const char * const libnames[] =
{
    "graphics.library",
    "layers.library",
    "intuition.library",
    "dos.library",
    "icon.library",
    "expansion.library",
    "utility.library",
    "keymap.library",
    "gadtools.library",
    "workbench.library"
};

static const char * const copyrights[] =
{
    "AROS Research Operating System (AROS)",
    "Copyright © 1995-2020, ",
    "The AROS Development Team.",
    "Other parts © by respective owners.",
    "ALPHA ",
    MOD_NAME_STRING,
    "exec " MOD_VERS_STRING " (" MOD_DATE_STRING ")\r\n"
};

/*****i* exec.library/TaggedOpenLibrary **************************************

	TaggedOpenLibrary -- open a library by tag (V39)

    NAME */
	AROS_LH1(APTR, TaggedOpenLibrary,

/*  SYNOPSIS */
	AROS_LHA(LONG, tag, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 135, Exec)

/*  FUNCTION
	Opens a library given by tag.

	All libraries will be opened with version number 0.

	If the library cannot be opened the first try, this function calls
	FindResident and InitResident on the library, and tries again.

    INPUTS
	tag -  Which library or text string to return.

    RESULT
	Pointer to library or pointer to text string.

    NOTES
	THIS FUNCTION IS PRIVATE/INTERNAL.
	TaggedOpenLibrary provides a means for  system ROM modules
	to open the standard system libraries without having to include
	the library name in string form.
	TaggedOpenLibrary also provides the standard system copyright
	notice.

    EXAMPLE

    BUGS

    SEE ALSO
	OpenLibrary(), FindResident(), InitResident()

    INTERNALS
	No checks are made on the validity of the tag.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Library *lib;
    struct Resident *res;

    if(tag > 0)
    {
	/*
	    Try to open the library. If it opened, return.
	*/
	if((lib = OpenLibrary(libnames[tag-1], 0))) return (APTR)lib;
#if (0)
	/*
	    If it didn't open, FindResident(), InitResident(), and then
	    try to open it again.
	*/
	if(!(res = FindResident(libnames[tag-1]))) return NULL;
	InitResident(res, BNULL);
	if((lib = OpenLibrary(libnames[tag-1], 0))) return (APTR)lib;
#endif
    }
    else 
        if(tag < 0) return( (APTR)copyrights[(-tag)-1] );

    /*
	If we get here, tag must be 0, or the lib didn't open.
    */
    return NULL;

    AROS_LIBFUNC_EXIT
} /* TaggedOpenLibrary */
