/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: GetNameSpace - Internal utility.library function.
    Lang: english
*/
#include "intern.h"

/* GetNameSpace: Internal function that gets the NameSpace to use.
    Will look at either the supplied NameSpace, or use the Global
    NameSpace.

    History
	11-08-96    iaint   Internal NameSpace function.
	19-10-96    iaint   Changed to more logical NamedObject format.
	06-04-97    iaint   Changed to prevent AllocMem() in libinit.
*/

struct NameSpace *
GetNameSpace(struct NamedObject *nameSpace, struct UtilityBase *UtilityBase)
{
    if(nameSpace)
	return (GetIntNamedObject(nameSpace))->no_NameSpace;
    else
	return &GetIntUtilityBase(UtilityBase)->ub_NameSpace;
}
