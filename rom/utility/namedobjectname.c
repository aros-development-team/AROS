/*
    $Id$
    $Log$
    Revision 1.1  1996/12/18 01:27:36  iaint
    NamedObjects

    Desc: NamedObjectName()
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
        #include <clib/utility_protos.h>

        AROS_LH1(STRPTR, NamedObjectName,

/*  SYNOPSIS */
        AROS_LHA(struct NamedObject *, object, A0),

/*  LOCATION */
        struct UtilityBase *, UtilityBase, 42, Utility)

/*  FUNCTION
        Return the name associated with a NamedObject.

    INPUTS
        object      -   The NamedObject you want the name of.

    RESULT
        The name of the object will be returned.

    NOTES

    EXAMPLE
        struct NamedObject *no;
        STRPTR name;

        \* Some other code here *\

        name = NamedObjectName( no );

    BUGS

    SEE ALSO
        utility/name.h

    INTERNALS
        The name is stored in the ln_Name field of the list node.

    HISTORY
        29-10-95    digulla automatically created from
                            utility_lib.fd and clib/utility_protos.h
        11-08-96    iaint   Reworked for AROS style.
        18-10-96    iaint   New NamedObject style.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Nice'n easy */

    if( object )
        return GetIntNamedObject(object)->no_Node.ln_Name;
    else
        return NULL;

    AROS_LIBFUNC_EXIT

} /* NamedObjectName */
