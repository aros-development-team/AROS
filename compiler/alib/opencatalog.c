/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of OpenCatalog()
    Lang: english
*/

#define AROS_TAGRETURNTYPE  struct Catalog *
#include <utility/tagitem.h>


/*****************************************************************************

    NAME */
#include <libraries/locale.h>
#include <proto/locale.h>
extern struct LocaleBase *LocaleBase;
#undef OpenCatalog /* Get rid of the macro from inline/ */

	struct Catalog * OpenCatalog (

/*  SYNOPSIS */
	struct Locale * locale,
	STRPTR          name,
	Tag             tag1,
	...             )

/*  FUNCTION
	This is the varargs version of the locale.library OpenCatalogA().
	For information see locale.library/OpenCatalog()

    INPUTS
	locale      -   The locale describing the language the users
			wants.
	name        -   Name of the catalog file.
	tag1        -   TagList of extra arguments.

    RESULT
	Either a pointer to a Catalog, or NULL.

	Although the function may have returned NULL, that does not
	necessarily meant there is an error. If dos/IoErr() returns
	0, then there was no error, but the language of the built in
	strings is the same as that of a catalog.

	If IoErr() != 0, then there was an error however.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	locale/OpenCatalogA(), locale/CloseCatalog(),
	locale/GetCatalogStr()

    INTERNALS

    HISTORY
	15-02-1997  iaint   Wrote.

*****************************************************************************/
{
    AROS_LIBBASE_EXT_DECL(struct LocaleBase *,LocaleBase)

    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = OpenCatalogA(locale, name, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
} /* OpenCatalog */
