/*
    $Id$
    $Log$
    Revision 1.1  1996/12/18 01:27:35  iaint
    NamedObjects

    Desc: FindNamedObject() - find a NamedObject in a given NameSpace.
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
        #include <utility/name.h>
        #include <clib/utility_protos.h>

        AROS_LH3(struct NamedObject *, FindNamedObject,

/*  SYNOPSIS */
        AROS_LHA(struct NamedObject *, nameSpace, A0),
        AROS_LHA(STRPTR              , name, A1),
        AROS_LHA(struct NamedObject *, lastObject, A2),

/*  LOCATION */
        struct UtilityBase *, UtilityBase, 40, Utility)

/*  FUNCTION
        This function will search through a given NameSpace, or the
        system global NameSpace to find a NamedObject with the name
        requested. Optionally you can have the search start from a
        specific NamedObject. This way you can look for each occurence
        of a specifically named NamedObject in a NameSpace that allows
        for duplicates.

    INPUTS
        nameSpace   -   The NameSpace to search through. If NULL will use
                        the system default NameSpace.
        name        -   The name of the object to search for.
        lastObject  -   The (optional) last NamedObject to start the search
                        from.

    RESULT
        If a NamedObject with the name supplied exists, it will be returned.
        Otherwise will return NULL.

        When you have finised with this NamedObject, you should call
        ReleaseNamedObject( NamedObject ).

    NOTES
        If you are going to use a returned NamedObject to be the starting
        point for another search you must call ReleaseNamedObject() AFTER
        searching, as the ReleaseNamedObject() call can cause the NamedObject
        to be freed, leaving you with an invalid pointer.

    EXAMPLE

    BUGS

    SEE ALSO
        utility/name.h ReleaseNamedObject() "Utility: Named Objects"

    INTERNALS
        Could we implement named objects with hash chains perhaps?

    HISTORY
        29-10-95    digulla automatically created from
                            utility_lib.fd and clib/utility_protos.h
        11-08-96    iaint   Wrote based on AmigaOS 3.0 function.


*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NamedObject      *foundObj = NULL;
    struct IntNamedObject   *no;
    struct Node             *StartObj;
    struct NameSpace        *ns;

    /* It is a bit stupid to do something with a NULL name */
    if( name )
    {
        ns = GetNameSpace( nameSpace, UtilityBase);
        ObtainSemaphore( &ns->ns_Lock );

        /*
            if the user supplied a lastObject, then we shall use that
            to get the index of the starting object. Otherwise we shall
            extract the address of the first node in the NameSpace.
        */
        if(lastObject)
            StartObj = (GetIntNamedObject(lastObject))->no_Node.ln_Succ;
        else
            StartObj = (struct Node *)ns->ns_List.mlh_Head;

        if((no = IntFindNamedObj(ns, StartObj, name, UtilityBase)))
        {
            no->no_UseCount++;
            foundObj = GetNamedObject(no);
        }
        ReleaseSemaphore( &ns->ns_Lock );

    } /* if( name ) */
    return foundObj;

    AROS_LIBFUNC_EXIT

} /* FindNamedObject */
