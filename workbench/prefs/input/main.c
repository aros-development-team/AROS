/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include "registertab.h"

#if USE_SHARED_COOLIMAGES
#include <libraries/coolimages.h>
#include <proto/coolimages.h>
#else
#include <linklibs/coolimages.h>
#endif

#include <stdlib.h> /* for exit() */
#include <stdio.h>
#include <string.h>

/*********************************************************************************************/

#define ARG_TEMPLATE    "FROM,EDIT/S,USE/S,SAVE/S,PUBSCREEN/K"

#define ARG_FROM        0
#define ARG_EDIT    	1
#define ARG_USE     	2
#define ARG_SAVE      	3
#define ARG_PUBSCREEN   4

#define NUM_ARGS        5

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
    {&GfxBase           , "graphics.library" 	 , 40, TRUE  }, /* 40, because of WriteChunkyPixels */
    {&GadToolsBase      , "gadtools.library" 	 , 39, TRUE  },
    {&UtilityBase       , "utility.library"  	 , 39, TRUE  },
    {&IFFParseBase      , "iffparse.library" 	 , 39, TRUE  },
    {&CyberGfxBase  	, "cybergraphics.library", 39, FALSE },
#if USE_SHARED_COOLIMAGES
    {&CoolImagesBase  	, "coolimages.library"	 ,  1, TRUE  },
#endif
    {NULL                                            	     }
};

/*********************************************************************************************/

#define NUM_PAGES 2

static struct page
{
    LONG nameid;
    LONG (*handler)(LONG, IPTR);
    LONG minw;
    LONG minh;
}
pagetable[NUM_PAGES] =
{
    {MSG_GAD_TAB_KEYBOARD   , page_kbd_handler  },
    {MSG_GAD_TAB_MOUSE	    , page_mouse_handler}
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
#if USE_SHARED_COOLIMAGES
    {MSG_GAD_SAVE  , (const struct CoolImage *)COOL_SAVEIMAGE_ID  },
    {MSG_GAD_USE   , (const struct CoolImage *)COOL_DOTIMAGE_ID   },
    {MSG_GAD_CANCEL, (const struct CoolImage *)COOL_CANCELIMAGE_ID}
#else
    {MSG_GAD_SAVE  , &cool_saveimage  },
    {MSG_GAD_USE   , &cool_dotimage   },
    {MSG_GAD_CANCEL, &cool_cancelimage}
#endif
};

/*********************************************************************************************/

static struct RegisterTabItem 	regitems[NUM_PAGES + 1];
static struct RegisterTab   	reg;
static struct RDArgs        	*myargs;
static WORD 	    	    	activetab;
static IPTR                 	args[NUM_ARGS];

/*********************************************************************************************/

static void CloseLibs(void);
static void CloseInputDev(void);
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
	    ShowMessage("Input", msg, MSG(MSG_OK));     
	}
	else
	{
	    printf("Input: %s\n", msg);
	}
    }
    
    KillWin();
    KillGadgets();
    KillPages();
    KillMenus();
    FreeVisual();
    CleanupPrefs();
    FreeArguments();
    CloseInputDev();
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

static void OpenInputDev(void)
{
    if ((InputMP = CreateMsgPort()))
    {
    	if ((InputIO = (struct timerequest *) CreateIORequest(InputMP, sizeof(struct IOStdReq))))
	{
	    OpenDevice("input.device", 0, (struct IORequest *)InputIO, 0);
	}
    }
}

/*********************************************************************************************/

static void CloseInputDev(void)
{
    if (InputIO)
    {
    	CloseDevice((struct IORequest *)InputIO);
	DeleteIORequest((struct IORequest *)InputIO);
    }
    
    if (InputMP)
    {
    	DeleteMsgPort(InputMP);
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
    
    if (!args[ARG_FROM]) args[ARG_FROM] = (IPTR)CONFIGNAME_ENV;
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

    if (CyberGfxBase)
    {
    	truecolor = GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH) >= 15;
    }
}

/*********************************************************************************************/

static void FreeVisual(void)
{
    if (vi) FreeVisualInfo(vi);
    if (dri) FreeScreenDrawInfo(scr, dri);
    if (scr) UnlockPubScreen(NULL, scr);
}

/*********************************************************************************************/

static void MakePages(void)
{
    static const struct CoolImage *tabimages[] =
    {
#if USE_SHARED_COOLIMAGES
    	(const struct CoolImage *)COOL_KBDIMAGE_ID,
    	(const struct CoolImage *)COOL_MOUSEIMAGE_ID
#else
	&cool_kbdimage,
    	&cool_mouseimage
#endif
    };
    ULONG bgcol = 0;
    WORD i;
    BOOL cool_imageclass_ok = FALSE;
    
    if (truecolor)
    {
    #if USE_SHARED_COOLIMAGES
    	cool_imageclass_ok = (CoolImagesBase != NULL);
    #else
    	cool_imageclass_ok = InitCoolImageClass(CyberGfxBase);
    #endif
    	if (cool_imageclass_ok)
	{
	    ULONG col[3];
	    
	    GetRGB32(scr->ViewPort.ColorMap,
	    	     dri->dri_Pens[BACKGROUNDPEN],
		     1,
		     col);
		     
	    bgcol = ((col[0] & 0xFF000000) >> 8) +
	    	    ((col[1] & 0xFF000000) >> 16) +
		    ((col[2] & 0xFF000000) >> 24);
	}
	
    }
    
    for(i = 0; i < NUM_PAGES; i++)
    {
    	regitems[i].text = MSG(pagetable[i].nameid);
	if (cool_imageclass_ok)
	{
	#if USE_SHARED_COOLIMAGES
	    tabimages[i] = (const struct CoolImage *)COOL_ObtainImageA((ULONG)tabimages[i], NULL);
	    
	    regitems[i].image = NewObject(NULL, COOLIMAGECLASS, IA_Width   	 , tabimages[i]->width ,
	    	    	    	    	    	    		IA_Height  	 , tabimages[i]->height,
								COOLIM_CoolImage , (IPTR)tabimages[i]  ,
								COOLIM_BgColor   , bgcol    	       ,
								TAG_DONE);
	#else
	    regitems[i].image = NewObject(cool_imageclass, NULL, IA_Width   	 , tabimages[i]->width ,
	    	    	    	    	    	    	         IA_Height  	 , tabimages[i]->height,
								 COOLIM_CoolImage, (IPTR)tabimages[i]  ,
								 COOLIM_BgColor  , bgcol    	       ,
								 TAG_DONE);
    	#endif								 
	}
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
	if (regitems[i].image) DisposeObject(regitems[i].image);
    }
    
#if !USE_SHARED_COOLIMAGES    
    CleanupCoolImageClass();
#endif
}

/*********************************************************************************************/

static void LayoutButtons(void)
{
    struct RastPort temprp;
    WORD i, w, maxtextlen = 0, maximheight = 0;
    
    InitRastPort(&temprp);
    SetFont(&temprp, dri->dri_Font);
    
    for(i = 0; i < 3; i++)
    {
    #if USE_SHARED_COOLIMAGES
    	buttontable[i].image = (const struct CoolImage *)COOL_ObtainImageA((ULONG)buttontable[i].image, NULL);
	
    #endif
    
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
	{GA_RelVerify	    , TRUE  	    },
	{TAG_DONE   	    	    	    }
    };
    
#if USE_SHARED_COOLIMAGES
    if (!CoolImagesBase)
    	Cleanup(MSG(MSG_CANT_CREATE_GADGET));  
#else
    if (!InitCoolButtonClass(CyberGfxBase))
    	Cleanup(MSG(MSG_CANT_CREATE_GADGET));
#endif
	
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
	
    #if USE_SHARED_COOLIMAGES
	buttontable[i].gad = NewObjectA(NULL, COOLBUTTONGCLASS, tags);
    #else
	buttontable[i].gad = NewObjectA(cool_buttonclass, NULL, tags);
    #endif
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
    
    for(i = 0; i < 3; i++)
    {
    	if (buttontable[i].gad) DisposeObject((Object *)buttontable[i].gad);
    }
    
#if !USE_SHARED_COOLIMAGES
    CleanupCoolButtonClass();
#endif
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
    static struct IBox zoom;
    WORD    	       w, t, h, wx, wy;
    
    w = winwidth + scr->WBorLeft + scr->WBorRight;
    t = scr->WBorTop + scr->Font->ta_YSize + 1;
    h = winheight + t + scr->WBorBottom;
    
    wx = (scr->Width - w) / 2;
    wy = (scr->Height - h) / 2;
    
    zoom.Left   = -1;
    zoom.Top    = -1;
    zoom.Width  = w;
    zoom.Height = t;
    
    win = OpenWindowTags(0, WA_PubScreen    , (IPTR)scr     	    	,
    	    	    	    WA_Left 	    , wx    	    	    	,
			    WA_Top  	    , wy    	    	    	,
			    WA_InnerWidth   , winwidth	    	    	,
			    WA_InnerHeight  , winheight     	    	,
			    WA_Title	    , (IPTR)MSG(MSG_WINTITLE)	,
			    WA_CloseGadget  , TRUE  	    	    	,
			    WA_DragBar	    , TRUE  	    	    	,
			    WA_DepthGadget  , TRUE  	    	    	,
			    WA_Activate     , TRUE  	    	    	,
			    WA_Gadgets	    , (IPTR)buttontable[0].gad	,
			    WA_NewLookMenus , TRUE  	    	    	,
			    WA_Zoom 	    , (IPTR)&zoom   	        ,
			    WA_IDCMP	    , REGISTERTAB_IDCMP   |
			    	      	      BUTTONIDCMP   	  |
				      	      LISTVIEWIDCMP 	  |
				      	      IDCMP_CLOSEWINDOW   |
					      IDCMP_VANILLAKEY    |
					      IDCMP_RAWKEY        |
					      IDCMP_MENUPICK	  |
					      IDCMP_REFRESHWINDOW   	,
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
    
    if (win)
    {
    	RemoveGList(win, buttontable[0].gad, NUM_BUTTONS);
	ClearMenuStrip(win);
    	CloseWindow(win);
	win = NULL;
    }
}

/*********************************************************************************************/

void TellGUI(LONG cmd)
{
    WORD i;
    
    for(i = 0; i < NUM_PAGES; i++)
    {
    	pagetable[i].handler(cmd, 0);
    }
}

/*********************************************************************************************/

static void HandleAll(void)
{
    struct IntuiMessage *msg;
    struct MenuItem     *item;
    struct Gadget   	*gad;
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
		
		case IDCMP_REFRESHWINDOW:
		    GT_BeginRefresh(win);
		    
		    GT_RefreshWindow(win, NULL);
    	    	    RenderRegisterTab(win->RPort, &reg, TRUE);	
		    pagetable[activetab].handler(PAGECMD_REFRESH, 0);
		    	    
		    GT_EndRefresh(win, TRUE);
		    break;
		    
		case IDCMP_VANILLAKEY:
		    switch(msg->Code)
		    {
			case 27: /* ESC */
			    quitme = TRUE;
			    break;
			    
		    } /* switch(msg->Code) */
		    break;

    	    	case IDCMP_GADGETUP:
    	    	    gad = (struct Gadget *)msg->IAddress;
		    switch(gad->GadgetID)
		    {
		    	case MSG_GAD_SAVE:
			    if (!SavePrefs(CONFIGNAME_ENVARC)) break;
			    /* fall through */
			    
			case MSG_GAD_USE:
			    if (!SavePrefs(CONFIGNAME_ENV)) break;
			    /* fall through */
			    
			case MSG_GAD_CANCEL:
			    quitme = TRUE;
			    break;
			    
		    }		    
		    break;
		    
		case IDCMP_MENUPICK:
		    men = msg->Code;            
		    while(men != MENUNULL)
		    {
			if ((item = ItemAddress(menus, men)))
			{
			    STRPTR filename;
			    
			    switch((ULONG)GTMENUITEM_USERDATA(item))
			    {
			    	case MSG_MEN_PROJECT_OPEN:
				    if ((filename = GetFile(MSG(MSG_ASL_OPEN_TITLE), "SYS:Prefs/Presets", FALSE)))
				    {
				    	LoadPrefs(filename);
				    }
				    break;
				
				case MSG_MEN_PROJECT_SAVEAS:
				    if ((filename = GetFile(MSG(MSG_ASL_SAVE_TITLE), "SYS:Prefs/Presets", TRUE)))
				    {
				    	SavePrefs(filename);
				    }
				    break;
				    
			    	case MSG_MEN_PROJECT_QUIT:
				    quitme = TRUE;
				    break;
				
				case MSG_MEN_EDIT_DEFAULT:
				    DefaultPrefs();
				    break;
				
				case MSG_MEN_EDIT_LASTSAVED:
				    LoadPrefs(CONFIGNAME_ENVARC);
				    break;
				    
				case MSG_MEN_EDIT_RESTORE:
				    RestorePrefs();
				    break;
				    
				case MSG_MEN_SETTINGS_CREATEICONS:
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
    InitLocale("System/Prefs/Input.catalog", 1);
    InitMenus();
    OpenLibs();
    OpenInputDev();
    GetArguments();
    InitPrefs((STRPTR)args[ARG_FROM], (args[ARG_USE] ? TRUE : FALSE), (args[ARG_SAVE] ? TRUE : FALSE));
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


