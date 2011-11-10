/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include "version.h"

#include <exec/rawfmt.h>

/*********************************************************************************************/

struct NewMenu nm[] =
{
    {NM_TITLE, (STRPTR)MSG_MEN_PROJECT      },
    {NM_ITEM,  (STRPTR)MSG_MEN_PROJECT_QUIT },
    {NM_TITLE, (STRPTR)MSG_MEN_EDIT         },
    {NM_ITEM,  (STRPTR)MSG_MEN_EDIT_RESTORE },
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
            ULONG  id = (ULONG)actnm->nm_Label;
            STRPTR str = MSG(id);
            
            if (actnm->nm_Type == NM_TITLE)
            {
                actnm->nm_Label = str;
            } 
            else 
            {
                actnm->nm_Label = str + 2;
                if (str[0] != ' ') actnm->nm_CommKey = str;
            }
            actnm->nm_UserData = (APTR)id;
            
        } /* if (actnm->nm_Label != NM_BARLABEL) */
	
    } /* for(actnm = nm; nm->nm_Type != NM_END; nm++) */

}

/*********************************************************************************************/

void my_sprintf(UBYTE *buffer, UBYTE *format, ...)
{
    va_list args;

    va_start(args, format);
    VNewRawDoFmt(format, RAWFMTFUNC_STRING, buffer, args);
    va_end(args);
}

/*********************************************************************************************/
