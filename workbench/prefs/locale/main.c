/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include "registertab.h"

#include <linklibs/coolimages.h>

#include <stdlib.h> /* for exit() */
#include <stdio.h>
#include <string.h>

/*********************************************************************************************/

#define ARG_TEMPLATE    "FROM,EDIT/S,USE/S,SAVE/S,MAP/K,PUBSCREEN/K"

#define ARG_FROM        0
#define ARG_EDIT    	1
#define ARG_USE     	2
#define ARG_SAVE      	3
#define ARG_MAP     	4
#define ARG_PUBSCREEN   5

#define NUM_ARGS        6

/*********************************************************************************************/

static struct libinfo
{
    APTR        var;
    STRPTR      name;
    WORD        version;
    BOOL    	required;
}
libtable[] =
{
    {&IntuitionBase     , "intuition.library"	 , 39, TRUE  },
    {&GfxBase           , "graphics.library" 	 , 39, TRUE  },
    {&GadToolsBase      , "gadtools.library" 	 , 39, TRUE  },
    {&UtilityBase       , "utility.library"  	 , 39, TRUE  },
    {&IFFParseBase      , "iffparse.library" 	 , 39, TRUE  },
    {&CyberGfxBase  	, "cybergraphics.library", 39, FALSE },
    {NULL                                            	     }
};

/*********************************************************************************************/

#define NUM_PAGES 3

static struct page
{
    LONG nameid;
    LONG (*handler)(LONG, IPTR);
    LONG minw;
    LONG minh;
}
pagetable[NUM_PAGES] =
{
    {MSG_GAD_TAB_LANGUAGE, page_language_handler},
    {MSG_GAD_TAB_COUNTRY , page_country_handler },
    {MSG_GAD_TAB_TIMEZONE, page_timezone_handler}
};

/*********************************************************************************************/

#define NUM_BUTTONS 3

static struct button
{
    LONG    	    	    nameid;
    const struct CoolImage  *image;
    struct Gadget   	    *gad;
}
buttontable[NUM_BUTTONS] =
{
    {MSG_GAD_SAVE  , &cool_saveimage  },
    {MSG_GAD_USE   , &cool_useimage   },
    {MSG_GAD_CANCEL, &cool_cancelimage}
};

/*********************************************************************************************/

static struct RegisterTabItem 	regitems[NUM_PAGES + 1];
static struct RegisterTab   	reg;
static struct Gadget	    	*okgad, *usegad, *cancelgad;
static struct RDArgs        	*myargs;
static WORD 	    	    	activetab;
static IPTR                 	args[NUM_ARGS];

/*********************************************************************************************/

static void CloseLibs(void);
static void FreeArguments(void);
static void FreeVisual(void);
static void KillPages(void);
static void KillWin(void);
static void KillGadgets(void);

/*********************************************************************************************/

WORD ShowMessage(STRPTR title, STRPTR text, STRPTR gadtext)
{
    struct EasyStruct es;
    
    es.es_StructSize   = sizeof(es);
    es.es_Flags        = 0;
    es.es_Title        = title;
    es.es_TextFormat   = text;
    es.es_GadgetFormat = gadtext;
   
    return EasyRequestArgs(win, &es, NULL, NULL);  
}

/*********************************************************************************************/

void Cleanup(STRPTR msg)
{
    if (msg)
    {
	if (IntuitionBase && !((struct Process *)FindTask(NULL))->pr_CLI)
	{
	    ShowMessage("Locale", msg, MSG(MSG_OK));     
	} else {
	    printf("Locale: %s\n", msg);
	}
    }
    
    KillGadgets();
    KillWin();
    KillPages();
    KillMenus();
    FreeVisual();
    CleanupPrefs();
    FreeArguments();
    CloseLibs();
    CleanupLocale();
    
    exit(prog_exitcode);
}


/*********************************************************************************************/

static void OpenLibs(void)
{
    struct libinfo *li;
    
    for(li = libtable; li->var; li++)
    {
	if (!((*(struct Library **)li->var) = OpenLibrary(li->name, li->version)))
	{
	    if (li->required)
	    {
	    	sprintf(s, MSG(MSG_CANT_OPEN_LIB), li->name, li->version);
	    	Cleanup(s);
	    }
	}       
    }
       
}

/*********************************************************************************************/

static void CloseLibs(void)
{
    struct libinfo *li;
    
    for(li = libtable; li->var; li++)
    {
	if (*(struct Library **)li->var) CloseLibrary((*(struct Library **)li->var));
    }
}

/*********************************************************************************************/

static void GetArguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, NULL)))
    {
	Fault(IoErr(), 0, s, 256);
	Cleanup(s);
    }
}

/*********************************************************************************************/

static void FreeArguments(void)
{
    if (myargs) FreeArgs(myargs);
}

/*********************************************************************************************/

static void GetVisual(void)
{
    scr = LockPubScreen((CONST_STRPTR)args[ARG_PUBSCREEN]);
    if (!scr) Cleanup(MSG(MSG_CANT_LOCK_SCR));
    
    dri = GetScreenDrawInfo(scr);
    if (!dri) Cleanup(MSG(MSG_CANT_GET_DRI));
    
    vi = GetVisualInfoA(scr, NULL);
    if (!vi) Cleanup(MSG(MSG_CANT_GET_VI));
}

/*********************************************************************************************/

static void FreeVisual(void)
{
    if (dri) FreeScreenDrawInfo(scr, dri);
    if (scr) UnlockPubScreen(NULL, scr);
}

/*********************************************************************************************/

static void MakePages(void)
{
    WORD i;
    
    for(i = 0; i < NUM_PAGES; i++)
    {
    	regitems[i].text = MSG(pagetable[i].nameid);
	
	if (!(pagetable[i].handler(PAGECMD_INIT, 0)))
	{
	    Cleanup(MSG(MSG_CANT_CREATE_GADGET));
	}
    }
    
    InitRegisterTab(&reg, regitems);
    
}

/*********************************************************************************************/

static void KillPages(void)
{
    WORD i;
    
    for(i = 0; i < NUM_PAGES; i++)
    {
 	pagetable[i].handler(PAGECMD_CLEANUP, 0);
    }
}

/*********************************************************************************************/

static void LayoutButtons(void)
{
    struct RastPort temprp;
    WORD i, w, maxtextlen = 0, maximheight = 0;
    BOOL truecolor = GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH) >= 15;
    
    InitRastPort(&temprp);
    SetFont(&temprp, dri->dri_Font);
    
    for(i = 0; i < 3; i++)
    {
    	w = TextLength(&temprp, MSG(buttontable[i].nameid), strlen(MSG(buttontable[i].nameid)));
	if (truecolor)
	{
	    if (buttontable[i].image->height > maximheight)
	    	maximheight = buttontable[i].image->height;
		
	    w += IMBUTTON_EXTRAWIDTH + buttontable[i].image->width;
	}
	else
	{
	    buttontable[i].image = NULL;
	}
	if (w > maxtextlen) maxtextlen = w;
		
    }
    
    buttonwidth = w + BUTTON_EXTRAWIDTH;
    buttonheight = dri->dri_Font->tf_YSize + BUTTON_EXTRAHEIGHT;

    if (truecolor)
    {
    	maximheight += IMBUTTON_EXTRAHEIGHT;
	if (maximheight > buttonheight) buttonheight = maximheight;
    }
    
    DeinitRastPort(&temprp);
}

/*********************************************************************************************/

static void LayoutGUI(void)
{
    WORD w, max_pagewidth = 0;
    WORD h, max_pageheight = 0;
    WORD i;
    
    LayoutButtons();
    
    for(i = 0; i < NUM_PAGES; i++)
    {
    	if (!(pagetable[i].handler(PAGECMD_LAYOUT, 0)))
	    Cleanup(MSG(MSG_CANT_CREATE_GADGET));
	    
	w = pagetable[i].handler(PAGECMD_GETMINWIDTH, 0);
	h = pagetable[i].handler(PAGECMD_GETMINHEIGHT, 0);
	
	if (w > max_pagewidth)  max_pagewidth  = w;
	if (h > max_pageheight) max_pageheight = h;
    }
    
    LayoutRegisterTab(&reg, scr, dri, TRUE);
    if (reg.width > max_pagewidth) max_pagewidth = reg.width;
    
    i = buttonwidth * NUM_BUTTONS + SPACE_X * (NUM_BUTTONS - 1) - TABBORDER_X * 2;
    if (i > max_pagewidth) max_pagewidth = i;
    
    SetRegisterTabPos(&reg, scr->WBorLeft + BORDER_X, scr->WBorTop + scr->Font->ta_YSize + 1 + BORDER_Y);
    
    pages_left   = scr->WBorLeft + BORDER_X + TABBORDER_X ;
    pages_top    = scr->WBorTop + scr->Font->ta_YSize + 1 + BORDER_Y + reg.height + TABBORDER_Y;
    pages_width  = max_pagewidth;
    pages_height = max_pageheight;
    
    SetRegisterTabFrameSize(&reg, pages_width  + TABBORDER_X * 2,
    	    	    	    	  pages_height + TABBORDER_Y * 2);
    
    for(i = 0; i < NUM_PAGES; i++)
    {
    	pagetable[i].handler(PAGECMD_SETDOMLEFT  , pages_left    );
	pagetable[i].handler(PAGECMD_SETDOMTOP 	 , pages_top     );
    	pagetable[i].handler(PAGECMD_SETDOMWIDTH , max_pagewidth );
	pagetable[i].handler(PAGECMD_SETDOMHEIGHT, max_pageheight);
    }
    
    winwidth  = pages_width + TABBORDER_X * 2 + BORDER_X * 2;
    winheight = pages_height + buttonheight + SPACE_Y + reg.height + TABBORDER_Y * 2 + BORDER_Y * 2;
}

/*********************************************************************************************/

static void MakeGadgets(void)
{
    WORD    	    x = scr->WBorLeft + BORDER_X;
    WORD    	    y = scr->WBorTop + scr->Font->ta_YSize + 1 + winheight - BORDER_Y - buttonheight;
    WORD    	    spacex;
    WORD    	    i;
    struct TagItem  tags[] =
    {
    	{GA_Left    	    , 0     	    },
	{GA_Top     	    , y     	    },
	{GA_Width   	    , buttonwidth   },
	{GA_Height  	    , buttonheight  },
	{GA_Text    	    , 0     	    },
	{GA_ID	    	    , 0     	    },
	{GA_Previous	    , 0     	    },
	{COOLBT_CoolImage   , 0     	    },
	{TAG_DONE   	    	    	    }
    };
    
    if (!InitCoolButtonClass(CyberGfxBase))
    	Cleanup(MSG(MSG_CANT_CREATE_GADGET));
	
    spacex = (pages_width + TABBORDER_X * 2 - buttonwidth * NUM_BUTTONS) * 16 / (NUM_BUTTONS - 1);
    
    for(i = 0; i < NUM_BUTTONS; i++)
    {
    	if (i == NUM_BUTTONS - 1)
	{
	    tags[0].ti_Data = x + pages_width + TABBORDER_X * 2 - buttonwidth;
	}
	else
	{
	    tags[0].ti_Data = x + (buttonwidth * 16 + spacex) * i / 16;
	}

	tags[4].ti_Data = (IPTR)MSG(buttontable[i].nameid);
	tags[5].ti_Data = buttontable[i].nameid;
	tags[7].ti_Data = (IPTR)buttontable[i].image;

	if (i > 0) tags[6].ti_Data = (IPTR)buttontable[i - 1].gad;
	
	buttontable[i].gad = NewObjectA(cool_buttonclass, NULL, tags);
	if (!buttontable[i].gad) Cleanup(MSG(MSG_CANT_CREATE_GADGET));
	
    }
    
    for(i = 0; i < NUM_PAGES; i++)
    {
    	if (!(pagetable[i].handler(PAGECMD_MAKEGADGETS, 0)))
	    Cleanup(MSG(MSG_CANT_CREATE_GADGET));
    }
}

/*********************************************************************************************/

static void KillGadgets(void)
{
    WORD i;
    
    if (win)
    {
    	RemoveGList(win, buttontable[0].gad, NUM_BUTTONS);
    }
    
    for(i = 0; i < 3; i++)
    {
    	if (buttontable[i].gad) DisposeObject((Object *)buttontable[i].gad);
    }
    
    CleanupCoolButtonClass();
}

/*********************************************************************************************/

static void ActivatePage(WORD which)
{
    if (which == activetab) return;
    
    if (activetab >= 0)
    {
    	pagetable[activetab].handler(PAGECMD_REMGADGETS, 0);
    }
    
    SetDrMd(win->RPort, JAM1),
    SetAPen(win->RPort, dri->dri_Pens[BACKGROUNDPEN]);
    RectFill(win->RPort, pages_left, pages_top, pages_left + pages_width - 1, pages_top + pages_height - 1);
    
    activetab = which;

    pagetable[activetab].handler(PAGECMD_ADDGADGETS, 0);
    
}


/*********************************************************************************************/

static void MakeWin(void)
{
    WORD wx, wy;
    
    wx = (scr->Width - (winwidth + scr->WBorLeft + scr->WBorTop)) / 2;
    wy = (scr->Height - (winheight + scr->WBorTop + scr->Font->ta_YSize + 1 + scr->WBorBottom)) / 2;
    
    win = OpenWindowTags(0, WA_PubScreen    , (IPTR)scr,
    	    	    	    WA_Left 	    , wx,
			    WA_Top  	    , wy,
			    WA_InnerWidth   , winwidth,
			    WA_InnerHeight  , winheight,
			    WA_Title	    , (IPTR)MSG(MSG_WINTITLE),
			    WA_CloseGadget  , TRUE,
			    WA_DragBar	    , TRUE,
			    WA_DepthGadget  , TRUE,
			    WA_Activate     , TRUE,
			    WA_Gadgets	    , (IPTR)buttontable[0].gad,
			    WA_IDCMP	    , REGISTERTAB_IDCMP |
			    	      	      BUTTONIDCMP   	|
				      	      LISTVIEWIDCMP 	|
				      	      IDCMP_CLOSEWINDOW,
			    TAG_DONE);

    SetMenuStrip(win, menus);
    
    RenderRegisterTab(win->RPort, &reg, TRUE);
    
    activetab = -1;
    ActivatePage(0);
}

/*********************************************************************************************/

static void KillWin(void)
{
    pagetable[reg.active].handler(PAGECMD_REMGADGETS, 0);
    
    if (win) CloseWindow(win);
}

/*********************************************************************************************/

static void HandleAll(void)
{
    struct IntuiMessage *msg;
    struct MenuItem     *item;
    UWORD               men;
    BOOL                quitme = FALSE;
    
    while (!quitme)
    {
	WaitPort(win->UserPort);
	
	while((msg = GT_GetIMsg(win->UserPort)))
	{
	    if (HandleRegisterTabInput(&reg, msg))
	    {
	    	ActivatePage(reg.active);
	    }
	    else if (pagetable[activetab].handler(PAGECMD_HANDLEINPUT, (IPTR)msg))
	    {
	    }
	    else switch (msg->Class)
	    {
		case IDCMP_CLOSEWINDOW:
		    quitme = TRUE;
		    break;
		
		case IDCMP_VANILLAKEY:
		    switch(msg->Code)
		    {
			case 27: /* ESC */
			    quitme = TRUE;
			    break;
			    
		    } /* switch(msg->Code) */
		    break;

		case IDCMP_MENUPICK:
		    men = msg->Code;            
		    while(men != MENUNULL)
		    {
			if ((item = ItemAddress(menus, men)))
			{
			    switch((ULONG)GTMENUITEM_USERDATA(item))
			    {
    	    	    	    	default:
				    break;
				
			    } /* switch(GTMENUITEM_USERDATA(item)) */
			    
			    men = item->NextSelect;
			} else {
			    men = MENUNULL;
			}
			
		    } /* while(men != MENUNULL) */
		    break;
		    		
	    } /* else switch (msg->Class) */
	    
	    GT_ReplyIMsg(msg);
	    
	} /* while((msg = GT_GetIMsg(win->UserPort))) */
	
    } /* while (!quitme) */
}

/*********************************************************************************************/

int main(void)
{
    InitLocale("Sys/localeprefs.catalog", 1);
    InitMenus();
    OpenLibs();
    GetArguments();
    InitPrefs();
    GetVisual();
    MakeMenus();
    MakePages();
    LayoutGUI();
    MakeGadgets();
    MakeWin();
    HandleAll();
    Cleanup(NULL);
    
    return 0;
}

/*********************************************************************************************/


