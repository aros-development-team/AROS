/*
    (C) 1997-2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#include <string.h>

#define DEBUG 0
#include <aros/debug.h>

/*********************************************************************************************/

static struct NewMenu nm[] =
{
    {NM_TITLE, (STRPTR)MSG_MEN_PROJECT							},
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_OPEN						},
     {NM_ITEM, NM_BARLABEL								},
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_SAVEAS						},
     {NM_ITEM, NM_BARLABEL								},
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_PRINT						},
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_ABOUT						},
     {NM_ITEM, NM_BARLABEL								},
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_QUIT						},
    {NM_TITLE, (STRPTR)MSG_MEN_EDIT							},
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_MARK						},
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_COPY						},
     {NM_ITEM, NM_BARLABEL								},
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_SELECTALL						},
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_CLEARSELECTED					},
    {NM_TITLE, (STRPTR)MSG_MEN_WINDOW							},
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_SEPSCREEN		, 0, CHECKIT | MENUTOGGLE	},
     {NM_ITEM, NM_BARLABEL								},
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_MINIMIZE						},
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_NORMAL						},
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_MAXIMIZE						},
    {NM_TITLE, (STRPTR)MSG_MEN_SETTINGS							},
     {NM_ITEM, (STRPTR)MSG_MEN_SETTINGS_SAVEDEF						},
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
    menus = CreateMenusA(nm, NULL);
    if (!menus) Cleanup(MSG(MSG_CANT_CREATE_MENUS));
    
    if (!LayoutMenusA(menus, vi, NULL)) Cleanup(MSG(MSG_CANT_CREATE_MENUS));
    
}

/*********************************************************************************************/

void KillMenus(void)
{
    if (win) ClearMenuStrip(win);
    if (menus) FreeMenus(menus);
    
    menus = NULL;
}

/*********************************************************************************************/

STRPTR GetFile(void)
{
    static UBYTE	 filebuffer[300];
    struct FileRequester *req;
    STRPTR 		 retval = NULL;
    
    AslBase = OpenLibrary("asl.library", 39);
    if (AslBase)
    {
        req = AllocAslRequestTags(ASL_FileRequest, ASLFR_TitleText , (IPTR)MSG(MSG_ASL_OPEN_TITLE),
						   ASLFR_DoPatterns, TRUE			  ,
						   TAG_DONE);
	if (req)
	{
	    if (AslRequest(req, NULL))
	    {
	        strcpy(filebuffer, req->fr_Drawer);
		AddPart(filebuffer, req->fr_File, 299);
		
		retval = filebuffer;
		
	    } /* if (AslRequest(req, NULL) */
	    
	    FreeAslRequest(req);
	    
	} /* if (req) */
	
	CloseLibrary(AslBase);
	
    } /* if (AslBase) */
    
    return retval;
}

/*********************************************************************************************/
