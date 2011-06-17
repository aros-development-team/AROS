/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include "version.h"

#include <string.h>

/*********************************************************************************************/

struct NewMenu nm[] =
{
    {NM_TITLE, (STRPTR)MSG_MEN_PROJECT      },
    {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_QUIT  },
    {NM_TITLE, (STRPTR)MSG_MEN_EDIT         },
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_RESTORE },
    {NM_END 	    	    	    	    }
};

/*********************************************************************************************/

void InitMenus(void)
{
    struct NewMenu *actnm = nm;
    
    for(actnm = nm; actnm->nm_Type != NM_END; actnm++)
    {
	if (actnm->nm_Label != NM_BARLABEL)
	{
	    IPTR   id = (IPTR)actnm->nm_Label;
	    STRPTR str = MSG(id);
	    
	    if (actnm->nm_Type == NM_TITLE)
	    {
		actnm->nm_Label = str;
	    } else {
		actnm->nm_Label = str + 2;
		if (str[0] != ' ') actnm->nm_CommKey = str;
	    }
	    actnm->nm_UserData = (APTR)id;
	    
	} /* if (actnm->nm_Label != NM_BARLABEL) */
	
    } /* for(actnm = nm; nm->nm_Type != NM_END; nm++) */

}

/*********************************************************************************************/

LONG NumMonthDays(struct ClockData *cd)
{
    struct ClockData 	cd2;
    ULONG   	    	secs;
    LONG    	    	monthday = 28;
    
    cd2 = *cd;
    
    while(monthday < 32)
    {
    	cd2.mday = monthday;
    
    	secs = Date2Amiga(&cd2);
	secs += 24 * 60 * 60; /* day++ */
	
	Amiga2Date(secs, &cd2);
	
	if (cd2.month != cd->month) break;
	
	monthday++;
    }
    
    return monthday;
}

/*********************************************************************************************/
