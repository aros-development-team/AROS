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
    {NM_TITLE, (STRPTR)MSG_MEN_PROJECT                                                  },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_OPEN                                             },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_SAVEAS                                           },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_QUIT                                             },
    {NM_TITLE, (STRPTR)MSG_MEN_EDIT                                                     },
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_DEFAULT                                           	},
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_LASTSAVED                                           },
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_RESTORE                                           	},
    {NM_TITLE, (STRPTR)MSG_MEN_SETTINGS                                                 },
     {NM_ITEM, (STRPTR)MSG_MEN_SETTINGS_CREATEICONS, NULL, CHECKIT | MENUTOGGLE         },
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

/*********************************************************************************************/

STRPTR GetFile(STRPTR title, STRPTR dir, BOOL savemode)
{
    static UBYTE         filebuffer[300];
    struct FileRequester *req;
    STRPTR               retval = NULL;
    
    AslBase = OpenLibrary("asl.library", 39);
    if (AslBase)
    {
	req = AllocAslRequestTags(ASL_FileRequest, ASLFR_TitleText    , (IPTR)title,
						   ASLFR_DoPatterns   , TRUE       ,
						   ASLFR_InitialDrawer, (IPTR)dir  ,
						   ASLFR_DoSaveMode   , savemode   ,
						   TAG_DONE);
	if (req)
	{
	    if (AslRequest(req, NULL))
	    {
		strncpy(filebuffer, req->fr_Drawer, 299);
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

