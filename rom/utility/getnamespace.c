/*
    (C) 1996 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/12/18 01:27:35  iaint
    NamedObjects


    Desc: GetNameSpace - Internal utility.library function.
*/
#include "utility_intern.h"

/* GetNameSpace: Internal function that gets the NameSpace to use.
    Will look at either the supplied NameSpace, or use the Global
    NameSpace.

    History
        11-08-96    iaint   Internal NameSpace function.
        19-10-96    iaint   Changed to more logical NamedObject format.
*/

struct NameSpace *
GetNameSpace(struct NamedObject *nameSpace, struct UtilityBase *UtilityBase)
{
    struct IntNamedObject *no;

    no = GetIntNamedObject((nameSpace 
		? nameSpace 
		: GetIntUtilityBase(UtilityBase)->ub_GlobalNameSpace));

    return no->no_NameSpace;
}
