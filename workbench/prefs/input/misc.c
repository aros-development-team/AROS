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

static struct NewMenu nm[] =
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
    if (win) ClearMenuStrip(win);
    if (win) ResetMenuStrip(win, menus);
}

/*********************************************************************************************/

STRPTR GetFile(CONST_STRPTR title, CONST_STRPTR dir, BOOL savemode)
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

struct Node *FindListNode(struct List *list, WORD which)
{
    struct Node *node = NULL;
    
    if (which >= 0)
    {
	for(node = list->lh_Head; node->ln_Succ && which; node = node->ln_Succ, which--)
	{
	}
	if (!node->ln_Succ) node = NULL;
    }
    
    return node;
}

/*********************************************************************************************/

void SortInNode(struct List *list, struct Node *node)
{
    struct Node *sort, *prev = NULL;

    ForeachNode(list, sort)
    {
	if (Stricmp(node->ln_Name, sort->ln_Name) < 0) break;
	prev = sort;
    }

    Insert(list, node, prev);
}

/*********************************************************************************************/

void DrawFrameWithTitle(struct RastPort *rp, WORD x1, WORD y1, WORD x2, WORD y2, CONST_STRPTR title)
{
    UWORD pen1, pen2, slen, tlen, i;
   
    pen1 = dri->dri_Pens[SHADOWPEN];
    pen2 = dri->dri_Pens[SHINEPEN];
    slen = strlen(title);
    tlen = TextLength(rp, title, slen);
    
    SetDrMd(rp, JAM1);
    
    SetAPen(rp, dri->dri_Pens[TEXTPEN]);
    Move(rp, x1 + FRAMETITLE_OFFX + FRAMETITLE_TITLESPACE, y1 + rp->TxBaseline);
    Text(rp, title, slen);
    
    y1 += rp->TxBaseline / 2;
    
    for(i = 0; i < 2; i++)
    {
    	SetAPen(rp, pen1);
    	Move(rp, x1, y2);
	Draw(rp, x1, y1);
	Draw(rp, x1 + FRAMETITLE_OFFX - 1 - i, y1);
	Move(rp, x1 + FRAMETITLE_OFFX - 1 + FRAMETITLE_TITLESPACE + tlen + FRAMETITLE_TITLESPACE - i, y1);
	Draw(rp, x2, y1);
	
	SetAPen(rp, pen2);
	Draw(rp, x2, y2);
	Draw(rp, x1 + 1, y2);
	
	x1++; y1++; x2--; y2--;
	pen1 ^= pen2; pen2 ^= pen1; pen1 ^= pen2; /* swap pen1 and pen2 */
    }
    
}

/*********************************************************************************************/

