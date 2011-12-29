/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Close a message catalog.
    Lang: english
*/
#include <exec/types.h>
#include <proto/exec.h>
#include "locale_intern.h"

#define        DEBUG_CLOSECATALOG(x)        ;

/*****************************************************************************

    NAME */
#include <proto/locale.h>

        AROS_LH1(void, CloseCatalog,

/*  SYNOPSIS */
        AROS_LHA(struct Catalog *, catalog, A0),

/*  LOCATION */
        struct LocaleBase *, LocaleBase, 6, Locale)

/*  FUNCTION
        Conclude access to a message catalog, and decrement the use count.
        If this use count is 0, the catalog can be expunged when the
        system memory is running low.

    INPUTS
        catalog        -        the message catalog to close, note that NULL is
                        a valid catalog.

    RESULT
        The catalog is closed, and should no longer be used by the
        application.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        GetCatalogStr(), OpenCatalogA() 

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    DEBUG_CLOSECATALOG(dprintf("CloseCatalog: catalog 0x%lx\n",
        catalog));

    if (catalog != NULL)
    {
        ObtainSemaphore(&IntLB(LocaleBase)->lb_CatalogLock);

        /* Decrement the use counter. */
        IntCat(catalog)->ic_UseCount--;

        if (0 == IntCat(catalog)->ic_UseCount)
        {
            Remove(&catalog->cat_Link);

            dispose_catalog((struct IntCatalog *)catalog, LocaleBase);
            FreeVec(catalog);
        }

        ReleaseSemaphore(&IntLB(LocaleBase)->lb_CatalogLock);

    }

    DEBUG_CLOSECATALOG(dprintf("CloseCatalog: done\n"));

    AROS_LIBFUNC_EXIT
}
