/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*********************************************************************************************/

#include <intuition/intuition.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/asl.h>

#define CATCOMP_NUMBERS
#include "strings.h"

#include "global.h"

#include <string.h>

/*********************************************************************************************/

struct Library *AslBase;

/*********************************************************************************************/

static struct NewMenu nm[] =
{
    {NM_TITLE, (STRPTR)MSG_MEN_PROJECT			},
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_OPEN		},
     {NM_ITEM, NM_BARLABEL				},
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_SAVEAS		},
     {NM_ITEM, NM_BARLABEL				},
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_PRINT		},
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_ABOUT		},
     {NM_ITEM, NM_BARLABEL				},
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_QUIT		},
    {NM_TITLE, (STRPTR)MSG_MEN_NAVIGATION		},
     {NM_ITEM, (STRPTR)MSG_MEN_NAVIGATION_FIND		},
     {NM_ITEM, (STRPTR)MSG_MEN_NAVIGATION_FIND_NEXT	},
     {NM_ITEM, (STRPTR)MSG_MEN_NAVIGATION_FIND_PREV	},
     {NM_ITEM, NM_BARLABEL				},
     {NM_ITEM, (STRPTR)MSG_MEN_NAVIGATION_JUMP		},
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
	    CONST_STRPTR str = MSG(id);
	    
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
    static UBYTE	 pathbuffer[300];
    static UBYTE	 filebuffer[300];
    struct FileRequester *req;
    STRPTR 		 retval = NULL;
    
    AslBase = OpenLibrary("asl.library", 39);
    if (AslBase)
    {
        filebuffer[299] = 0;
	pathbuffer[299] = 0;
	
        strncpy(filebuffer, FilePart(filenamebuffer), 299);
	strncpy(pathbuffer, filenamebuffer, 299);
	*(FilePart(pathbuffer)) = 0;
	
        req = AllocAslRequestTags(ASL_FileRequest, ASLFR_TitleText    , (IPTR)MSG(MSG_ASL_OPEN_TITLE),
						   ASLFR_DoPatterns   , TRUE			     ,
						   ASLFR_InitialFile  , (IPTR)filebuffer	     ,
						   ASLFR_InitialDrawer, (IPTR)pathbuffer	     ,
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
