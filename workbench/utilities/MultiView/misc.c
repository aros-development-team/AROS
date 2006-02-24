/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*********************************************************************************************/

#include "global.h"
#include "version.h"

#include <string.h>

#include "compilerspecific.h"
#include "debug.h"

/*********************************************************************************************/

static struct MenuItem * FindMenuItem( struct Menu *menu, ULONG msgid );
static void ChangeItemState( ULONG msgid, BOOL state );

/*********************************************************************************************/

struct NewMenu nm[] =
{
    {NM_TITLE, (STRPTR)MSG_MEN_PROJECT                                                  },	/* 0 */
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_OPEN                                             },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_SAVEAS                                           },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_SAVEAS_IFF                                       },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_PRINT                                            },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_ABOUT                                            },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_PROJECT_QUIT                                             },
    {NM_TITLE, (STRPTR)MSG_MEN_EDIT                                                     },	/* 1 */
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_MARK                                                },
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_COPY                                                },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_SELECTALL                                           },
     {NM_ITEM, (STRPTR)MSG_MEN_EDIT_CLEARSELECTED                                       },
    {NM_TITLE, (STRPTR)MSG_MEN_WINDOW                                                   },	/* 2 */
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_SEPSCREEN         , 0, CHECKIT | MENUTOGGLE       },
     {NM_ITEM, NM_BARLABEL                                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_MINIMIZE                                          },
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_NORMAL                                            },
     {NM_ITEM, (STRPTR)MSG_MEN_WINDOW_MAXIMIZE                                          },
    {NM_TITLE, (STRPTR)MSG_MEN_SETTINGS                                                 },	/* 3 */
     {NM_ITEM, (STRPTR)MSG_MEN_SETTINGS_SAVEDEF                                         },
    {NM_END}
};

struct NewMenu nmpict[] =
{
    {NM_TITLE, (STRPTR)MSG_MEN_PICT                                                      },
     {NM_ITEM, (STRPTR)MSG_MEN_PICT_ZOOM_IN                                              },
     {NM_ITEM, (STRPTR)MSG_MEN_PICT_ZOOM_OUT                                             },
     {NM_ITEM, (STRPTR)MSG_MEN_PICT_RESET                                                },
     {NM_ITEM, NM_BARLABEL                                                               },
     {NM_ITEM, (STRPTR)MSG_MEN_PICT_FIT_WIN              , 0, CHECKIT | MENUTOGGLE       },
     {NM_ITEM, (STRPTR)MSG_MEN_PICT_KEEP_ASPECT          , 0, CHECKIT | MENUTOGGLE       },
     {NM_ITEM, NM_BARLABEL                                                               },
     {NM_ITEM, (STRPTR)MSG_MEN_PICT_FORCE_MAP            , 0, CHECKIT | MENUTOGGLE       },
     {NM_ITEM, (STRPTR)MSG_MEN_PICT_DITHER               , 0, CHECKIT | MENUTOGGLE | CHECKED },
    {NM_END}
};

struct NewMenu nmtext[] =
{
    {NM_TITLE, (STRPTR)MSG_MEN_TEXT                                                      },
     {NM_ITEM, (STRPTR)MSG_MEN_TEXT_WORDWRAP             , 0, CHECKIT | MENUTOGGLE       },
     {NM_ITEM, (STRPTR)MSG_MEN_TEXT_SEARCH                                               },
     {NM_ITEM, (STRPTR)MSG_MEN_TEXT_SEARCH_PREV                                          },
     {NM_ITEM, (STRPTR)MSG_MEN_TEXT_SEARCH_NEXT                                          },
    {NM_END}
};

/*********************************************************************************************/

static struct MenuItem * FindMenuItem( struct Menu *menu, ULONG msgid )
{
    struct MenuItem *item;
    
    while( menu )
    {
	if( (ULONG)GTMENU_USERDATA(menu) == msgid )
	    return (struct MenuItem *)menu;
	item = menu->FirstItem;
	while( item )
	{
	    if( (ULONG)GTMENUITEM_USERDATA(item) == msgid )
		return item;
	    item = item->NextItem;
	}
	menu = menu->NextMenu;
    }
    return NULL;
}

/*********************************************************************************************/
			    
static void ChangeItemState( ULONG msgid, BOOL state )
{
    struct MenuItem *item;

    item = FindMenuItem(menus, msgid);
    if (item)
    {
	if (state) item->Flags |= ITEMENABLED; else item->Flags &= ~ITEMENABLED;
    }
}

/*********************************************************************************************/

void InitMenus(struct NewMenu *newm)
{
    struct NewMenu *actnm;
    
    for(actnm = newm; actnm->nm_Type != NM_END; actnm++)
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

struct Menu * MakeMenus(struct NewMenu *newm)
{
    struct Menu *menu;
    struct TagItem menu_tags[] =
    {
	{GTMN_NewLookMenus, TRUE},
	{TAG_DONE               }
    };
    
    menu = CreateMenusA(newm, NULL);
    if (!menu) Cleanup(MSG(MSG_CANT_CREATE_MENUS));
    
    if (!LayoutMenusA(menu, vi, menu_tags))
    {
	FreeMenus(menu);
	Cleanup(MSG(MSG_CANT_CREATE_MENUS));
    }
    return menu;
}

/*********************************************************************************************/

void KillMenus(void)
{
    if (win) ClearMenuStrip(win);
    if (menus) FreeMenus(menus);
    if (pictmenus) FreeMenus(pictmenus);
    if (textmenus) FreeMenus(textmenus);
    
    menus = NULL;
    pictmenus = NULL;
    textmenus = NULL;
}

/*********************************************************************************************/

void SetMenuFlags(void)
{
    struct Menu *menu;
    struct MenuItem *item;
    IPTR val;
    BOOL ret;
    
    if (win) ClearMenuStrip(win);

    ChangeItemState( MSG_MEN_PROJECT_SAVEAS, dto_supports_write );
    ChangeItemState( MSG_MEN_PROJECT_SAVEAS_IFF, dto_supports_write_iff );
    ChangeItemState( MSG_MEN_PROJECT_PRINT, dto_supports_print );
    ChangeItemState( MSG_MEN_EDIT_COPY, dto_supports_copy );
    ChangeItemState( MSG_MEN_EDIT_SELECTALL, dto_supports_selectall );
    ChangeItemState( MSG_MEN_EDIT_CLEARSELECTED, dto_supports_clearselected );

    item = FindMenuItem(menus, MSG_MEN_SETTINGS);	/* Search last menu, then append dt group dependent menu */
    menu = (struct Menu *)item;
    if (menu)
    {
	if (dto_subclass_gid == GID_PICTURE)
	{
	    D(bug("Multiview: is picture.datatype\n"));
	    menu->NextMenu = pictmenus;
	}
	else if (dto_subclass_gid == GID_TEXT)
	{
	    D(bug("Multiview: is text.datatype\n"));
	    menu->NextMenu = textmenus;
	    ret = GetDTAttrs(dto, TDTA_WordWrap, (IPTR)&val, TAG_DONE);
	    item = FindMenuItem(menus, MSG_MEN_TEXT_WORDWRAP);
	    if (ret && item)
	    {
		if (val) item->Flags |= CHECKED; else item->Flags &= ~CHECKED;
	    }
	    ChangeItemState( MSG_MEN_TEXT_WORDWRAP, ret );
	    ChangeItemState( MSG_MEN_TEXT_SEARCH, dto_supports_search );
	    ChangeItemState( MSG_MEN_TEXT_SEARCH_PREV, dto_supports_search_prev );
	    ChangeItemState( MSG_MEN_TEXT_SEARCH_NEXT, dto_supports_search_next );
	}
	else
	{
	    D(bug("Multiview: is unknown datatype\n"));
	    menu->NextMenu = NULL;
	}
    }

    {
	struct TagItem menu_tags[] =
	{
	    {GTMN_NewLookMenus, TRUE},
	    {TAG_DONE               }
	};
    	
	LayoutMenusA(menus, vi, menu_tags);
    }
    
    if (win) SetMenuStrip(win, menus);
}

/*********************************************************************************************/

STRPTR GetFileName(ULONG msgtextid)
{
    static UBYTE         pathbuffer[300];
    static UBYTE         filebuffer[300];
    struct FileRequester *req;
    STRPTR               filepart, retval = NULL;
    
    AslBase = OpenLibrary("asl.library", 39);
    if (AslBase)
    {
	filebuffer[299] = 0;
	pathbuffer[299] = 0;
	
	strncpy(filebuffer, FilePart(filenamebuffer), 299);
	strncpy(pathbuffer, filenamebuffer, 299);
	filepart = FilePart(pathbuffer);
	*filepart = 0;
	
	req = AllocAslRequestTags(ASL_FileRequest, ASLFR_TitleText    , (IPTR)MSG(msgtextid),
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
 
    EasyRequest(win, &es, NULL, (IPTR)VERSION,
				(IPTR)REVISION,
				(IPTR)DATESTR, 
				(IPTR)dtver_string,
				(IPTR)name_string,
				(IPTR)gid_string);

}

/*********************************************************************************************/

ULONG DoTrigger(ULONG what)
{
    struct dtTrigger msg;

    msg.MethodID          = DTM_TRIGGER;
    msg.dtt_GInfo         = NULL;
    msg.dtt_Function      = what;
    msg.dtt_Data          = NULL;

    return DoDTMethodA(dto, win, NULL, (Msg)&msg);
}

/*********************************************************************************************/

ULONG DoWriteMethod(STRPTR name, ULONG mode)
{
    struct dtWrite msg;
    BPTR fh;
    ULONG retval;
    
    fh = NULL;
    if (name)
    {
	fh = Open( name, MODE_NEWFILE );
	if (!fh)
	{
	    D(bug("Multiview: Cannot open %s\n", name));
	    OutputMessage(MSG(MSG_SAVE_FAILED));
	    return FALSE;
	}
    }

    
    
    msg.MethodID          = DTM_WRITE;
    msg.dtw_GInfo         = NULL;
    msg.dtw_FileHandle    = fh;
    msg.dtw_Mode          = mode;
    msg.dtw_AttrList      = NULL;

    D(bug("Multiview: Saving %s mode %ld\n", name ? name : (STRPTR)"[nothing]", mode));
    retval = DoDTMethodA(dto, win, NULL, (Msg)&msg);
    if (fh)
    {
	Close( fh );
	if( !retval )
	{
	    D(bug("Multiview: Error during write !\n"));
	    OutputMessage(MSG(MSG_SAVE_FAILED));
	}
    }
    return retval;
}

/*********************************************************************************************/

ULONG DoLayout(ULONG initial)
{
    ULONG res;
    struct gpLayout msg;

    D(bug("=> erase\n"));
    EraseRect(win->RPort, win->BorderLeft,
			  win->BorderTop,
			  win->Width - 1 - win->BorderRight,
			  win->Height - 1 - win->BorderBottom);

#if 1
    msg.MethodID	= GM_LAYOUT;
    msg.gpl_GInfo	= NULL;
    msg.gpl_Initial	= initial;

#if 0
    D(bug("=> doasynclayout libcall\n"));
    res = DoAsyncLayout(dto, &msg);
#else
    D(bug("=> GM_Layout method\n"));
    res = DoDTMethodA(dto, win, 0, (Msg)&msg);
#endif
    D(bug("layout result %ld\n", res));
    return res;
#else
    RemoveDTObject(win, dto);
    AddDTObject(win, NULL, dto, -1);
#endif
}

/*********************************************************************************************/

ULONG DoScaleMethod(ULONG xsize, ULONG ysize, BOOL aspect)
{
    struct pdtScale msg;
    
    D(bug(" scale width %d height %d\n", xsize, ysize));
    msg.MethodID	= PDTM_SCALE;
    msg.ps_NewWidth	= xsize;
    msg.ps_NewHeight	= ysize;
    msg.ps_Flags	= aspect ? PScale_KeepAspect : 0;
    // D(bug("- method %08lx newwidth %ld newheight %ld flags %08lx\n", msg.MethodID, msg.ps_NewWidth, msg.ps_NewHeight, msg.ps_Flags));

    return DoMethodA(dto, (Msg)&msg);
}

/*********************************************************************************************/

void DoZoom(WORD zoomer)
{
    UWORD curwidth, curheight;
    
    if (zoomer > 0)
    {
	curwidth = pdt_origwidth * zoomer;
	curheight = pdt_origheight * zoomer;
    }
    else
    {
	curwidth = pdt_origwidth / -zoomer;
	curheight = pdt_origheight / -zoomer;
    }
    D(bug(" zoom %d width %d height %d\n", zoomer, curwidth, curheight));
    DoScaleMethod(curwidth, curheight, 0);
    DoLayout(TRUE);
}

/*********************************************************************************************/
