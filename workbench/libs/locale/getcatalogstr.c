/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <exec/types.h>
#include <proto/exec.h>
#include "locale_intern.h"

/*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH3(STRPTR, GetCatalogStr,

/*  SYNOPSIS */
	AROS_LHA(struct Catalog *, catalog, A0),
	AROS_LHA(LONG            , stringNum, D0),
	AROS_LHA(STRPTR          , defaultString, A1),

/*  LOCATION */
	struct LocaleBase *, LocaleBase, 12, Locale)

/*  FUNCTION
	This function will return the string specified by the 
	stringNum from the given message catalog, or the defaultString
	if the string could not be found.

	If the catalog == NULL, then the defaultString will also be 
	returned.

    INPUTS
	catalog -	Message catalog to search. May be NULL.
	stringNum -	ID of the string to find.
	defaultString - String to return in case catalog is NULL or
			string could not be found.

    RESULT
	A pointer to a READ ONLY NULL terminated string. This string
	pointer is valid as long as the catalog remains open.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenCatalog(), CloseCatalog()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    locale_lib.fd and clib/locale_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,LocaleBase)

    STRPTR str = defaultString;
    if(catalog != NULL)
    {
	struct CatStr *cs = IntCat(catalog)->ic_First;

	while(cs != NULL)
	{
	    if(cs->cs_Id == stringNum)
	    {
		str = &cs->cs_Data[0];
		cs = NULL;
	    }
	    else
	    {
		if(	IntCat(catalog)->ic_Flags & ICF_INORDER
		    &&  cs->cs_Id > stringNum
		  )
		{
		    /* We are guaranteed not to find a match, so lets quit */
		    cs = NULL;
		}
		else
		    cs = cs->cs_Next;
	    }
	}
    }
    return str;

    AROS_LIBFUNC_EXIT
} /* GetCatalogStr */
