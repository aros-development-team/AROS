/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*******************************************************************************************/


#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>

#include "asl_intern.h"

#define CATCOMP_ARRAY
#include "strings.h"

/*******************************************************************************************/

STRPTR GetString(LONG id, struct Catalog *catalog, struct AslBase_intern *AslBase)
{
    STRPTR retval = CatCompArray[id].cca_Str;
 
    if (catalog && LocaleBase)
    {
    	retval = (STRPTR)GetCatalogStr(catalog, id, retval);
    }
    
    return retval;
}

/*******************************************************************************************/

void LocalizeMenus(struct NewMenu *nm, struct Catalog *catalog, struct AslBase_intern *AslBase)
{
    struct NewMenu *actnm = nm;
 
    for(actnm = nm; actnm->nm_Type != NM_END; actnm++)
    {
	if (actnm->nm_Label != NM_BARLABEL)
	{
	    LONG  id   = (LONG)actnm->nm_Label;
	    STRPTR str = GetString(id, catalog, AslBase);
	    
	    switch(actnm->nm_Type)
	    {
	    	case NM_TITLE:
		    actnm->nm_Label = str;
		    break;
		    
		case NM_ITEM:
		case NM_SUB:
		    actnm->nm_Label = str + 2;
		    if (str[0] != ' ') actnm->nm_CommKey = str;
		    break;
	    }
	    
	} /* if (actnm->nm_Label != NM_BARLABEL) */
	
    } /* for(actnm = nm; nm->nm_Type != NM_END; nm++) */

}

/*******************************************************************************************/
