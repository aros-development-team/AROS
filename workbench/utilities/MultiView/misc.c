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
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_OPEN                                             },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_SAVEAS                                           },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_PRINT                                            },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_ABOUT                                            },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_QUIT                                             },
    {NM_TITLE, (STRPTR)MSG_MEN_EDIT                                                     },
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_MARK                                                },
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_COPY                                                },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_SELECTALL                                           },
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_CLEARSELECTED                                       },
    {NM_TITLE, (STRPTR)MSG_MEN_WINDOW                                                   },
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_SEPSCREEN         , 0, CHECKIT | MENUTOGGLE       },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_MINIMIZE                                          },
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_NORMAL                                            },
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_MAXIMIZE                                          },
    {NM_TITLE, (STRPTR)MSG_MEN_SETTINGS                                                 },
     {NM_ITEM, (STRPTR)MSG_MEN_SETTINGS_SAVEDEF                                         },
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

    item = ItemAddress(menus, FULLMENUNUM(1, 1, NOSUB));
    if (item)
    {
	if (dto_supports_copy) item->Flags |= ITEMENABLED; else item->Flags &= ~ITEMENABLED;
    }
    
    item = ItemAddress(menus, FULLMENUNUM(1, 4, NOSUB));
    if (item)
    {
	if (dto_supports_clearselected) item->Flags |= ITEMENABLED; else item->Flags &= ~ITEMENABLED;
    }
    
    if (win) ResetMenuStrip(win, menus);
}

/*********************************************************************************************/

STRPTR GetFile(void)
{
    static UBYTE         pathbuffer[300];
    static UBYTE         filebuffer[300];
    struct FileRequester *req;
    STRPTR               retval = NULL;
    
    AslBase = OpenLibrary("asl.library", 39);
    if (AslBase)
    {
	filebuffer[299] = 0;
	pathbuffer[299] = 0;
	
	strncpy(filebuffer, FilePart(filenamebuffer), 299);
	strncpy(pathbuffer, filenamebuffer, 299);
	*(FilePart(pathbuffer)) = 0;
	
	req = AllocAslRequestTags(ASL_FileRequest, ASLFR_TitleText    , (IPTR)MSG(MSG_ASL_OPEN_TITLE),
						   ASLFR_DoPatterns   , TRUE                         ,
						   ASLFR_InitialFile  , (IPTR)filebuffer             ,
						   ASLFR_InitialDrawer, (IPTR)pathbuffer             ,
						   ASLFR_Window       , (IPTR)win   	    	     ,
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

void About(void)
{
    struct DataType     *dt = NULL;
    struct EasyStruct   es;
    STRPTR              gid_string = NULL;
    STRPTR              name_string = NULL;
    STRPTR              sp;
    WORD                i;
    UBYTE               dtver_string[100];
    
    if (GetDTAttrs(dto, DTA_DataType, (IPTR)&dt, TAG_DONE))
    {
	if (dt)
	{
	    gid_string = GetDTString(dt->dtn_Header->dth_GroupID);
	    name_string = dt->dtn_Header->dth_Name;
	}
    }
    
    if (!gid_string) gid_string = "";
    if (!name_string) name_string = "";
    
    for(sp = DataTypesBase->lib_IdString;
	(*sp != 0) && ((*sp < '0') || (*sp > '9'));
	sp++)
    {
    }
      
    i = 0;
    while ((*sp != 0) && (*sp != '\r') && (*sp != '\n') && (i < 99))
    {
	dtver_string[i++] = *sp++;
    }
    dtver_string[i++] = '\0';
    
    es.es_StructSize   = sizeof(es);
    es.es_Flags        = 0;
    es.es_Title        = MSG(MSG_ABOUT_TITLE);
    es.es_TextFormat   = MSG(MSG_ABOUT);
    es.es_GadgetFormat = MSG(MSG_CONTINUE);
 
    EasyRequest(win, &es, NULL, VERSION,
				REVISION,
				DATESTR, 
				dtver_string,
				name_string,
				gid_string);

}

/*********************************************************************************************/

void DoTrigger(ULONG what)
{
    struct dtTrigger m;

    m.MethodID          = DTM_TRIGGER;
    m.dtt_GInfo         = NULL;
    m.dtt_Function      = what;
    m.dtt_Data          = NULL;

    DoDTMethodA(dto, win, NULL, (Msg)&m);                               
}

/*********************************************************************************************/
