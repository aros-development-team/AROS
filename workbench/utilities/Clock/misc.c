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

#include "compilerspecific.h"
#include "debug.h"

/*********************************************************************************************/

static struct NewMenu nm[] =
{
    {NM_TITLE, (STRPTR)MSG_MEN_PROJECT                                                  },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_ANALOG 	, 0, CHECKIT, 2               	    	},
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_DIGITAL  , 0, CHECKIT, 1,                        },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_QUIT                                             },
    {NM_TITLE, (STRPTR)MSG_MEN_SETTINGS                                                 },
     {NM_ITEM, (STRPTR)MSG_MEN_SETTINGS_DATE, 0, CHECKIT | MENUTOGGLE                   },
     {NM_ITEM, (STRPTR)MSG_MEN_SETTINGS_SECONDS, 0, CHECKIT | MENUTOGGLE                },
     {NM_ITEM, (STRPTR)MSG_MEN_SETTINGS_DIGITALFORMAT                	    	    	},
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_SETTINGS_ALARM, 0, CHECKIT | MENUTOGGLE	    	    	},
     {NM_ITEM, (STRPTR)MSG_MEN_SETTINGS_SETALARM	    	    	    	    	},
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_SETTINGS_SAVE                                        	},
    {NM_END}
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
	    } else {
		actnm->nm_Label = str + 2;
		if (str[0] != ' ') actnm->nm_CommKey = str;
	    }
	    actnm->nm_UserData = (APTR)id;
	    
	} /* if (actnm->nm_Label != NM_BARLABEL) */
	
    } /* for(actnm = nm; nm->nm_Type != NM_END; nm++) */

}

/*********************************************************************************************/

void MakeMenus(void)
{
    struct TagItem menu_tags[] =
    {
	{GTMN_NewLookMenus, TRUE},
	{TAG_DONE               }
    };
    
    menus = CreateMenusA(nm, NULL);
    if (!menus) Cleanup(MSG(MSG_CANT_CREATE_MENUS));
    
    if (!LayoutMenusA(menus, vi, menu_tags)) Cleanup(MSG(MSG_CANT_CREATE_MENUS));
    
}

/*********************************************************************************************/

void KillMenus(void)
{
    if (win) ClearMenuStrip(win);
    if (menus) FreeMenus(menus);
    
    menus = NULL;
}

/*********************************************************************************************/

void SetMenuFlags(void)
{
    struct MenuItem *item;
    
    if (win) ClearMenuStrip(win);

    item = ItemAddress(menus, FULLMENUNUM(0, 0, NOSUB));
    if (item)
    {
	if (opt_analogmode) item->Flags |= CHECKED; else item->Flags &= ~CHECKED;
    }
    
    item = ItemAddress(menus, FULLMENUNUM(0, 1, NOSUB));
    if (item)
    {
	if (!opt_analogmode) item->Flags |= CHECKED; else item->Flags &= ~CHECKED;
    }

    item = ItemAddress(menus, FULLMENUNUM(1, 0, NOSUB));
    if (item)
    {
	if (opt_showdate) item->Flags |= CHECKED; else item->Flags &= ~CHECKED;
    }

    item = ItemAddress(menus, FULLMENUNUM(1, 1, NOSUB));
    if (item)
    {
	if (opt_showsecs) item->Flags |= CHECKED; else item->Flags &= ~CHECKED;
    }

    item = ItemAddress(menus, FULLMENUNUM(0, 4, NOSUB));
    if (item)
    {
	if (opt_alarm) item->Flags |= CHECKED; else item->Flags &= ~CHECKED;
    }
        
    if (win) ResetMenuStrip(win, menus);
}

/*********************************************************************************************/
