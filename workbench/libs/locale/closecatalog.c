/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Close a message catalog.
    Lang: english
*/
#include <exec/types.h>
#include <proto/exec.h>
#include "locale_intern.h"

/*****************************************************************************

    NAME */
#include <proto/locale.h>

	AROS_LH1(void, CloseCatalog,

/*  SYNOPSIS */
	AROS_LHA(struct Catalog *, catalog, A0),

/*  LOCATION */
	struct Locale *, LocaleBase, 6, Locale)

/*  FUNCTION
	Conclude access to a message catalog, and decrement the use count.
	If this use count is 0, the catalog can be expunged when the
	system memory is running low.

    INPUTS
	catalog	-	the message catalog to close, note that NULL is
			a valid catalog.

    RESULT
	The catalog is closed, and should no longer be used by the
	application.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetCatalogStr(), OpenCatalog() 

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    locale_lib.fd and clib/locale_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,LocaleBase)

    if(catalog != NULL)
    {
	/* Decrement the use counter. */
	IntCat(catalog)->ic_UseCount--;

        if (0 == IntCat(catalog)->ic_UseCount)
        {
          ObtainSemaphore (&IntLB(LocaleBase)->lb_CatalogLock);
          Remove(&catalog->cat_Link);
          ReleaseSemaphore(&IntLB(LocaleBase)->lb_CatalogLock);

          dispose_catalog((struct IntCatalog *)catalog, (struct Library *)LocaleBase);
          FreeVec(IntCat(catalog)->ic_Name);
          FreeVec(catalog->cat_Language);
          FreeMem(catalog, sizeof(struct IntCatalog));
        }
    }

    AROS_LIBFUNC_EXIT
} /* CloseCatalog */
