/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#include <linklibs/coolimages.h>

#include <stdlib.h> /* for exit() */
#include <stdio.h>
#include <string.h>

#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/iobsolete.h>
#include <libraries/gadtools.h>

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
    {&GfxBase           , "graphics.library" 	 , 40, TRUE  }, /* 40, because of WriteChunkyPixels */
    {&GadToolsBase      , "gadtools.library" 	 , 39, TRUE  },
    {&UtilityBase       , "utility.library"  	 , 39, TRUE  },
    {&IFFParseBase      , "iffparse.library" 	 , 39, TRUE  },
    {&CyberGfxBase  	, "cybergraphics.library", 39, FALSE },
    {NULL                                            	     }
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
    {MSG_GAD_USE   , &cool_dotimage   },
    {MSG_GAD_CANCEL, &cool_cancelimage}
};

/*********************************************************************************************/

static struct RDArgs        	*myargs;
static IPTR                 	args[NUM_ARGS];

/*********************************************************************************************/
static struct Gadget * gadlist, * gad;
struct Gadget * baudrate, *stopbits, *databits, *parity, *inputbuffersize, * outputbuffersize;

static UWORD maxlabelwidth, maxlabelwidth, maxcyclewidth, cycleheight;

/*********************************************************************************************/

static void CloseLibs(void);
static void FreeArguments(void);
static void FreeVisual(void);
static void KillWin(void);
static void KillGadgets(void);

/*********************************************************************************************/

CONST_STRPTR BaudrateLabels[] =
{
	"50",
	"75",
	"110",
	"134",
	"150",
	"200",
	"300",
	"600",
	"1200",
	"2400",
	"4800",
	"9600",
	"19200",
	"38400",
	"57600",
	"115200",
	NULL
};

CONST_STRPTR StopBitsLabels[] =
{
	"1",
	"1.5",
	"2",
	NULL
};

CONST_STRPTR DataBitsLabels[] =
{
	"8",
	"7",
	"6",
	"5",
	NULL
};

CONST_STRPTR ParityLabels[] =
{
	(CONST_STRPTR)MSG_PARITY_NONE,
	(CONST_STRPTR)MSG_PARITY_EVEN,
	(CONST_STRPTR)MSG_PARITY_ODD,
	(CONST_STRPTR)MSG_PARITY_MARK,
	(CONST_STRPTR)MSG_PARITY_SPACE,
	NULL
};

CONST_STRPTR BufferSizeLabels[] =
{
	"512",
	"1024",
	"2048",
	"4096",
	NULL
};

/*********************************************************************************************/

WORD ShowMessage(CONST_STRPTR title, CONST_STRPTR text, CONST_STRPTR gadtext)
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

void Cleanup(CONST_STRPTR msg)
{
    if (msg)
    {
	if (IntuitionBase && !((struct Process *)FindTask(NULL))->pr_CLI)
	{
	    ShowMessage("Serial", msg, MSG(MSG_OK));     
	} else {
	    printf("Serial: %s\n", msg);
	}
    }
    
    KillWin();
    KillGadgets();
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

static void LayoutButtons(void)
{
    struct RastPort temprp;
    WORD i, w, maxtextlen = 0, maximheight = 0;
    
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

WORD MaxStringsWidth(struct RastPort * rp, CONST_STRPTR * strings)
{
	WORD w, maxw = 0;
	ULONG i = 0;
	while (NULL != strings[i]) {
		w = TextLength(rp, strings[i], strlen(strings[i]));
		if (w > maxw) maxw = w;
		i++;
	} 
	return maxw;
}

/*********************************************************************************************/

static void LayoutGUI(void)
{
    WORD w;
    WORD maxw;
    struct RastPort temprp;
    
    LocalizeLabels(ParityLabels);
    
    LayoutButtons();
    

    InitRastPort(&temprp);
    SetFont(&temprp, dri->dri_Font);
    
    maxw = TextLength(&temprp, MSG(MSG_GAD_BAUDRATE)  , strlen(MSG(MSG_GAD_BAUDRATE)));
    w    = TextLength(&temprp, MSG(MSG_GAD_PARITY)    , strlen(MSG(MSG_GAD_PARITY)));
    if (w > maxw) maxw = w;
    w    = TextLength(&temprp, MSG(MSG_GAD_STOPBITS)  , strlen(MSG(MSG_GAD_STOPBITS)));
    if (w > maxw) maxw = w;
    w    = TextLength(&temprp, MSG(MSG_GAD_DATABITS)  , strlen(MSG(MSG_GAD_DATABITS)));
    if (w > maxw) maxw = w;
    w    = TextLength(&temprp, MSG(MSG_GAD_INPUTBUFFERSIZE) , strlen(MSG(MSG_GAD_INPUTBUFFERSIZE)));
    if (w > maxw) maxw = w;
    w    = TextLength(&temprp, MSG(MSG_GAD_OUTPUTBUFFERSIZE), strlen(MSG(MSG_GAD_OUTPUTBUFFERSIZE)));
    if (w > maxw) maxw = w;
    
    maxlabelwidth = maxw;

    maxw = MaxStringsWidth(&temprp, BaudrateLabels);
    w    = MaxStringsWidth(&temprp, StopBitsLabels);
    if (w > maxw) maxw = w;
    w    = MaxStringsWidth(&temprp, DataBitsLabels);
    if (w > maxw) maxw = w;
    w    = MaxStringsWidth(&temprp, ParityLabels);
    if (w > maxw) maxw = w;
    w    = MaxStringsWidth(&temprp, BufferSizeLabels);
    if (w > maxw) maxw = w;

    maxcyclewidth = maxw + CYCLE_EXTRAWIDTH;
    
    cycleheight = dri->dri_Font->tf_YSize + BUTTON_EXTRAHEIGHT;
    
    DeinitRastPort(&temprp);

    winwidth  = scr->WBorLeft + BORDER_X + TABBORDER_X + maxlabelwidth + maxcyclewidth + 
                scr->WBorRight+ BORDER_X + TABBORDER_X;
                
    w = buttonwidth * 3 + scr->WBorLeft + BORDER_X * 2 + SPACE_X * 2 + scr->WBorRight;
    
    if (w > winwidth) winwidth = w;
    
    winheight = scr->WBorTop  + scr->Font->ta_YSize + BORDER_Y + TABBORDER_Y + 6 * cycleheight + 5 * SPACE_Y + 2 * SPACE_Y + 
                buttonheight  + BORDER_Y + TABBORDER_Y + scr->WBorBottom;
}

/*********************************************************************************************/
VOID RefreshBaudrateGadget(VOID)
{
	ULONG i = 0;

	while (NULL != BaudrateLabels[i]) {
		ULONG baud = atol(BaudrateLabels[i]);
		
		if (baud == serialprefs.sp_BaudRate) {
			GT_SetGadgetAttrs(baudrate, win, NULL, GTCY_Active , i,
			                                       TAG_DONE);
			return;
		}
		i++;
	}

	GT_SetGadgetAttrs(baudrate, win, NULL, GTCY_Active , 0,
	                                       TAG_DONE);
}

/*********************************************************************************************/
VOID RefreshDataBitsGadget(VOID)
{
	ULONG i = 0;

	while (NULL != DataBitsLabels[i]) {
		ULONG bits = atol(DataBitsLabels[i]);
		
		if (bits == serialprefs.sp_BitsPerChar) {
			GT_SetGadgetAttrs(databits, win, NULL, GTCY_Active , i,
			                                       TAG_DONE);
			return;
		}
		i++;
	}

	GT_SetGadgetAttrs(databits, win, NULL, GTCY_Active , 0,
	                                       TAG_DONE);
}

/*********************************************************************************************/
VOID RefreshParityGadget(VOID)
{
	GT_SetGadgetAttrs(parity, win, NULL, GTCY_Active , serialprefs.sp_Parity,
	                                     TAG_DONE);
}

/*********************************************************************************************/
VOID RefreshStopBitsGadget(VOID)
{
	GT_SetGadgetAttrs(parity, win, NULL, GTCY_Active , serialprefs.sp_StopBits,
	                                     TAG_DONE);
}

/*********************************************************************************************/
VOID RefreshInputBufferSizeGadget(VOID)
{
	ULONG i = 0;

	while (NULL != BufferSizeLabels[i]) {
		ULONG size = atol(BufferSizeLabels[i]);
		
		if (size == serialprefs.sp_InputBuffer) {
			GT_SetGadgetAttrs(inputbuffersize, win, NULL, GTCY_Active , i,
			                                              TAG_DONE);
			return;
		}
		i++;
	}

	GT_SetGadgetAttrs(inputbuffersize, win, NULL, GTCY_Active , 3,
	                                              TAG_DONE);
}

/*********************************************************************************************/
VOID RefreshOutputBufferSizeGadget(VOID)
{
	ULONG i = 0;

	while (NULL != BufferSizeLabels[i]) {
		ULONG size = atol(BufferSizeLabels[i]);
		
		if (size == serialprefs.sp_OutputBuffer) {
			GT_SetGadgetAttrs(outputbuffersize, win, NULL, GTCY_Active , i,
			                                               TAG_DONE);
			return;
		}
		i++;
	}

	GT_SetGadgetAttrs(outputbuffersize, win, NULL, GTCY_Active , 3,
	                                               TAG_DONE);
}

/*********************************************************************************************/
VOID ReadGadgets(VOID)
{
	ULONG index;
	GT_GetGadgetAttrs(baudrate, win, NULL, GTCY_Active, (IPTR) &index, TAG_DONE);
	serialprefs.sp_BaudRate = atol(BaudrateLabels[index]);

	GT_GetGadgetAttrs(databits, win, NULL, GTCY_Active, (IPTR) &index, TAG_DONE);
	serialprefs.sp_BitsPerChar = index;

	GT_GetGadgetAttrs(parity  , win, NULL, GTCY_Active, (IPTR) &index, TAG_DONE);
	serialprefs.sp_Parity = index;

	GT_GetGadgetAttrs(stopbits, win, NULL, GTCY_Active, (IPTR) &index, TAG_DONE);
	serialprefs.sp_StopBits = index;

	GT_GetGadgetAttrs(inputbuffersize , win, NULL, GTCY_Active, (IPTR) &index, TAG_DONE);
	serialprefs.sp_InputBuffer  = atol(BufferSizeLabels[index]);

	GT_GetGadgetAttrs(outputbuffersize, win, NULL, GTCY_Active, (IPTR) &index, TAG_DONE);
	serialprefs.sp_OutputBuffer = atol(BufferSizeLabels[index]);
	
}

/*********************************************************************************************/


static void MakeGadgets(void)
{
    WORD    	    x = scr->WBorLeft + BORDER_X;
    WORD    	    y = scr->WBorTop + scr->Font->ta_YSize + 1 + winheight - BORDER_Y - buttonheight;
    WORD    	    spacex;
    WORD    	    i;
    struct NewGadget ng;

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
    
    if (!InitCoolButtonClass(CyberGfxBase))
    	Cleanup(MSG(MSG_CANT_CREATE_GADGET));


    spacex = (winwidth - buttonwidth * NUM_BUTTONS - x - scr->WBorRight - BORDER_X) * 16 / (NUM_BUTTONS - 1);

    for(i = 0; i < NUM_BUTTONS; i++)
    {
#if 0
    	if (i == NUM_BUTTONS - 1)
	{
	    tags[0].ti_Data = x + winwidth - buttonwidth - scr->WBorRight - BORDER_X;
	}
	else
#endif
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
    
    gad = CreateContext(&gadlist);
    
    ng.ng_VisualInfo = vi;
    ng.ng_TextAttr   = 0;
    ng.ng_LeftEdge   = scr->WBorLeft + BORDER_X + TABBORDER_X + maxlabelwidth;
    ng.ng_TopEdge    = scr->WBorTop  + scr->Font->ta_YSize + BORDER_Y + TABBORDER_Y;
    ng.ng_Width      = maxcyclewidth;
    ng.ng_Height     = cycleheight;
    ng.ng_GadgetID   = MSG_GAD_BAUDRATE;
    ng.ng_GadgetText = MSG(MSG_GAD_BAUDRATE);
    ng.ng_Flags      = PLACETEXT_LEFT;

    gad = CreateGadget(CYCLE_KIND, gad, &ng, GTCY_Labels, (IPTR) BaudrateLabels,
                                             GTCY_Active, 0,
                                             TAG_END);

    baudrate = gad;

    ng.ng_TopEdge   += SPACE_Y + cycleheight;
    ng.ng_GadgetID   = MSG_GAD_STOPBITS;
    ng.ng_GadgetText = MSG(MSG_GAD_STOPBITS);

    gad = CreateGadget(CYCLE_KIND, gad, &ng, GTCY_Labels, (IPTR) StopBitsLabels,
                                             GTCY_Active, 0,
                                             TAG_END);

    stopbits = gad;

    ng.ng_TopEdge   += SPACE_Y + cycleheight;
    ng.ng_GadgetID   = MSG_GAD_DATABITS;
    ng.ng_GadgetText = MSG(MSG_GAD_DATABITS);

    gad = CreateGadget(CYCLE_KIND, gad, &ng, GTCY_Labels, (IPTR) DataBitsLabels,
                                             GTCY_Active, 0,
                                             TAG_END);

    databits = gad;

    ng.ng_TopEdge   += SPACE_Y + cycleheight;
    ng.ng_GadgetID   = MSG_GAD_PARITY;
    ng.ng_GadgetText = MSG(MSG_GAD_PARITY);

    gad = CreateGadget(CYCLE_KIND, gad, &ng, GTCY_Labels, (IPTR) ParityLabels,
                                             GTCY_Active, 0,
                                             TAG_END);

    parity = gad;

    ng.ng_TopEdge   += SPACE_Y + cycleheight;
    ng.ng_GadgetID   = MSG_GAD_INPUTBUFFERSIZE;
    ng.ng_GadgetText = MSG(MSG_GAD_INPUTBUFFERSIZE);

    gad = CreateGadget(CYCLE_KIND, gad, &ng, GTCY_Labels, (IPTR) BufferSizeLabels,
                                             GTCY_Active, 0,
                                             TAG_END);

    inputbuffersize = gad;

    ng.ng_TopEdge   += SPACE_Y + cycleheight;
    ng.ng_GadgetID   = MSG_GAD_OUTPUTBUFFERSIZE;
    ng.ng_GadgetText = MSG(MSG_GAD_OUTPUTBUFFERSIZE);

    gad = CreateGadget(CYCLE_KIND, gad, &ng, GTCY_Labels, (IPTR) BufferSizeLabels,
                                             GTCY_Active, 0,
                                             TAG_END);

    outputbuffersize = gad;

#if 0
    gad = CreateGadget(SLIDER_KIND, gad, &ng, GTSL_Min , 110,
                                              GTSL_Max , 57600,
                                              GTSL_MaxLevelLen, 6,
                                              GTSL_LevelFormat, "%ld",
                                              PGA_FREEDOM, LORIENT_HORIZ,
                                              GA_RelVerify, TRUE,
                                              TAG_DONE);
#endif
    
}

/*********************************************************************************************/

static void KillGadgets(void)
{
    WORD i;
    
    for(i = 0; i < 3; i++)
    {
    	if (buttontable[i].gad) DisposeObject((Object *)buttontable[i].gad);
    }
    
    CleanupCoolButtonClass();
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
			    WA_IDCMP	    , BUTTONIDCMP   	  |
				      	      LISTVIEWIDCMP 	  |
				      	      IDCMP_CLOSEWINDOW   |
					      IDCMP_VANILLAKEY    |
					      IDCMP_RAWKEY        |
					      IDCMP_MENUPICK	  |
					      IDCMP_REFRESHWINDOW   	,
			    TAG_DONE);

    SetMenuStrip(win, menus);
    
    AddGList(win, gadlist, -1, -1, NULL);
    RefreshGList(gadlist, win, NULL, -1);
    GT_RefreshWindow(win, NULL);
    
    RefreshBaudrateGadget();
    RefreshDataBitsGadget();
    RefreshParityGadget();
    RefreshOutputBufferSizeGadget();
    RefreshInputBufferSizeGadget();
}

/*********************************************************************************************/

static void KillWin(void)
{
    if (win)
    {
    	RemoveGList(win, buttontable[0].gad, NUM_BUTTONS);
	ClearMenuStrip(win);
    	CloseWindow(win);
    	FreeGadgets(gadlist);
	win = NULL;
    }
}

/*********************************************************************************************/

void TellGUI(LONG cmd)
{
#if 0
    WORD i;
    for(i = 0; i < NUM_PAGES; i++)
    {
    	pagetable[i].handler(cmd, 0);
    }
#endif
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
#if 0
	    if (HandleRegisterTabInput(&reg, msg))
	    {
	    	ActivatePage(reg.active);
	    }
	    else if (pagetable[activetab].handler(PAGECMD_HANDLEINPUT, (IPTR)msg))
	    {
	    }
	    else
#endif
	    switch (msg->Class)
	    {
		case IDCMP_CLOSEWINDOW:
		    quitme = TRUE;
		    break;
		
		case IDCMP_REFRESHWINDOW:
		    GT_BeginRefresh(win);
		    
		    GT_RefreshWindow(win, NULL);
#if 0 
    	    	    RenderRegisterTab(win->RPort, &reg, TRUE);	
		    pagetable[activetab].handler(PAGECMD_REFRESH, 0);
#endif		    	    
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
		    	    ReadGadgets();
			    if (!SavePrefs(CONFIGNAME_ENVARC)) break;
			    /* fall through */
			    
			case MSG_GAD_USE:
			    ReadGadgets();
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
    InitLocale("System/Prefs/Serial.catalog", 1);
    InitMenus();
    OpenLibs();
    GetArguments();
    InitPrefs((STRPTR)args[ARG_FROM], (args[ARG_USE] ? TRUE : FALSE), (args[ARG_SAVE] ? TRUE : FALSE));
    GetVisual();
    MakeMenus();
    LayoutGUI();
    MakeGadgets();
    MakeWin();
    HandleAll();
    Cleanup(NULL);
    
    return 0;
}

/*********************************************************************************************/


