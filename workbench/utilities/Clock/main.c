/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

#include <stdlib.h> /* for exit() */
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "compilerspecific.h"
#include "debug.h"

/*********************************************************************************************/

#define ARG_TEMPLATE    "DIGITAL/S,LEFT/N,TOP/N,WIDTH/N,HEIGHT/N," \
    	    	    	"24HOUR/S,SECONDS/S,DATE/S,FORMAT/N,PUBSCREEN/K"

#define ARG_DIGITAL      0
#define ARG_LEFT    	 1
#define ARG_TOP     	 2
#define ARG_WIDTH     	 3
#define ARG_HEIGHT  	 4
#define ARG_24HOUR  	 5
#define ARG_SECONDS   	 6
#define ARG_DATE    	 7
#define ARG_FORMAT    	 8
#define ARG_PUBSCREEN    9

#define NUM_ARGS        10

/*********************************************************************************************/

#define BORDER_CLOCK_SPACING_X 4
#define BORDER_CLOCK_SPACING_Y 4

#define MY_PI 3.14159265358979

/*********************************************************************************************/

static struct libinfo
{
    APTR        var;
    STRPTR      name;
    WORD        version;
} libtable[] =
{
    {&IntuitionBase     , "intuition.library"           , 39    },
    {&GfxBase           , "graphics.library"            , 39    },
    {&GadToolsBase      , "gadtools.library"            , 39    },
    {&LayersBase        , "layers.library"              , 39    },
    {&UtilityBase       , "utility.library"             , 39    },
    {&KeymapBase        , "keymap.library"              , 39    },
    {&DiskfontBase      , "diskfont.library"            , 39    },
    {NULL                                                       }
};

static struct TextAttr  textattr;
static struct TextFont  *font;
static struct DateStamp currdate;
static struct RDArgs    *myargs;
static struct AreaInfo	areainfo;
static struct TmpRas	tmpras;
static struct BitMap	*clockbm1;
static struct BitMap	*clockbm2;
static struct RastPort	clockrp1;
static struct RastPort	clockrp2;
static PLANEPTR     	clockraster1;
static PLANEPTR     	clockraster2;
static ULONG	    	winmask, timermask;
static WORD 	    	clockcx, clockcy, clockrx, clockry;
static WORD 	    	clockraster1_w, clockraster1_h;
static WORD 	    	clockraster2_w, clockraster2_h;
static WORD 	    	areabuffer[30];
static WORD             opt_winleft = 0,
    	    	    	opt_wintop = -1,
			opt_winwidth = 150,
			opt_winheight = 150;
static BOOL 	    	quitme, timer_running;

static IPTR             args[NUM_ARGS];

/*********************************************************************************************/

static void CloseLibs(void);
static void CloseTimer(void);
static void FreeArguments(void);
static void FreeVisual(void);
static void KillWindow(void);

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
	    ShowMessage("Clock", msg, MSG(MSG_OK));     
	}
	else
	{
	    printf("Clock: %s\n", msg);
	}
    }
    
    if (clockraster1) FreeRaster(clockraster1, clockraster1_w, clockraster1_h);
    if (clockraster2) FreeRaster(clockraster2, clockraster2_w, clockraster2_h);
    if (clockbm1) FreeBitMap(clockbm1);
    if (clockbm2) FreeBitMap(clockbm2);
    
    KillWindow();
    KillMenus();
    FreeVisual();
    FreeArguments();
    CloseTimer();
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
	    sprintf(s, MSG(MSG_CANT_OPEN_LIB), li->name, li->version);
	    Cleanup(s);
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

static void OpenTimer(void)
{
    BOOL ok = FALSE;
    
    if ((TimerMP = CreateMsgPort()))
    {
    	if ((TimerIO = (struct timerequest *)CreateIORequest(TimerMP, sizeof(*TimerIO))))
	{
	    if (!OpenDevice(TIMERNAME, UNIT_VBLANK, &TimerIO->tr_node, 0))
	    {
	    	timermask = 1L << TimerMP->mp_SigBit;
	    	ok = TRUE;
	    }
	}
    }
    
    if (!ok)
    {
    	sprintf(s, MSG(MSG_CANT_OPEN_LIB), TIMERNAME, 0);
    	Cleanup(s);
    }
}

/*********************************************************************************************/

static void CloseTimer(void)
{
    if (timer_running)
    {
    	if (!CheckIO(&TimerIO->tr_node)) AbortIO(&TimerIO->tr_node);
	WaitIO(&TimerIO->tr_node);
    }
    
    if (TimerIO)
    {
    	CloseDevice(&TimerIO->tr_node);
	DeleteIORequest(&TimerIO->tr_node);
    }
    
    if (TimerMP)
    {
    	DeleteMsgPort(TimerMP);
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

    if (args[ARG_DIGITAL] == 0) opt_analogmode = 1;
    if (args[ARG_24HOUR]) opt_24hour = 1;
    if (args[ARG_SECONDS]) opt_showsecs = 1;
    if (args[ARG_DATE]) opt_showdate = 1;
    
    if (args[ARG_LEFT]) opt_winleft = *(IPTR *)args[ARG_LEFT];
    if (args[ARG_TOP]) opt_wintop = *(IPTR *)args[ARG_TOP];
    if (args[ARG_WIDTH]) opt_winwidth = *(IPTR *)args[ARG_WIDTH];
    if (args[ARG_HEIGHT]) opt_winheight = *(IPTR *)args[ARG_HEIGHT];
    
}

/*********************************************************************************************/

static void FreeArguments(void)
{
    if (myargs) FreeArgs(myargs);
}

/*********************************************************************************************/

static void GetVisual(void)
{
    scr = LockPubScreen((STRPTR)args[ARG_PUBSCREEN]);
    if (!scr) Cleanup(MSG(MSG_CANT_LOCK_SCR));

    dri = GetScreenDrawInfo(scr);
    if (!dri) Cleanup(MSG(MSG_CANT_GET_DRI));

    vi = GetVisualInfoA(scr, NULL);
    if (!vi) Cleanup(MSG(MSG_CANT_GET_VI));
}

/*********************************************************************************************/

static void FreeVisual(void)
{
    if (vi) FreeVisualInfo(vi);
    if (dri) FreeScreenDrawInfo(scr, dri);
    if (scr) UnlockPubScreen(NULL, scr);
}


/*********************************************************************************************/

static void CalcClockMetrics(void)
{
    clockrx = (win->GZZWidth - BORDER_CLOCK_SPACING_X * 2) / 2;
    clockry = (win->GZZHeight - BORDER_CLOCK_SPACING_Y * 2) / 2;
    
    clockcx = win->BorderLeft + BORDER_CLOCK_SPACING_X + clockrx;
    clockcy = win->BorderTop  + BORDER_CLOCK_SPACING_Y + clockry;    
}

/*********************************************************************************************/

#define HOURHANDSIZE_PERCENT 	    60.0
#define HOURHANDSIZE_PERCENT2 	    80.0
#define HOURHANDSIZE_ANGLEOFF 	    0.1

#define MINUTEHANDSIZE_PERCENT      82.0
#define MINUTEHANDSIZE_PERCENT2     77.0
#define MINUTEHANDSIZE_ANGLEOFF     0.06

#define SECONDHANDSIZE_PERCENT	    82

#define TICK_PERCENT 	    	    95
#define TICK_PERCENT2 	    	    90
#define TICK_PERCENT3 	    	    85

#define TICK_ANGLEOFF 	    	    (M_PI * 2.0 / 60.0 / 2.0)

/*********************************************************************************************/

static void RenderTicks(void)
{
    LONG i, x, y, x2, y2;
    double angle;
    
    if ((clockrx < 10) || (clockry < 10)) return;
    
    SetAPen(&clockrp1, dri->dri_Pens[SHADOWPEN]);
    
    for(angle = 0.0, i = 0; angle < 2.0 * MY_PI; angle += 2 * MY_PI / 60.0, i++)
    {
    	x = clockrx + cos(angle) * clockrx * TICK_PERCENT / 100.0;
	y = clockry - sin(angle) * clockry * TICK_PERCENT / 100.0;

	
	if ((i % 5) == 0)
	{
	    AreaMove(&clockrp1, x, y);
	    
    	    x = clockrx + cos(angle + TICK_ANGLEOFF) * clockrx * TICK_PERCENT2 / 100.0;
	    y = clockry - sin(angle + TICK_ANGLEOFF) * clockry * TICK_PERCENT2 / 100.0;
	    
	    AreaDraw(&clockrp1, x, y);

    	    x = clockrx + cos(angle) * clockrx * TICK_PERCENT3 / 100.0;
	    y = clockry - sin(angle) * clockry * TICK_PERCENT3 / 100.0;
	    
	    AreaDraw(&clockrp1, x, y);

    	    x = clockrx + cos(angle - TICK_ANGLEOFF) * clockrx * TICK_PERCENT2 / 100.0;
	    y = clockry - sin(angle - TICK_ANGLEOFF) * clockry * TICK_PERCENT2 / 100.0;
	    
	    AreaDraw(&clockrp1, x, y);
	    
	    AreaEnd(&clockrp1);
	    
	}
	else if ((clockrx > 30) && (clockry > 30))
	{
    	    x2 = clockrx + cos(angle) * clockrx * TICK_PERCENT2 / 100.0;
	    y2 = clockry - sin(angle) * clockry * TICK_PERCENT2 / 100.0;
	    
	    Move(&clockrp1, x, y);
	    Draw(&clockrp1, x2, y2);
	}
		
    }
    
}

/*********************************************************************************************/

static void RenderHand(double angle, double percent, double percent2, double angleoff,
    	    	       UBYTE color)
{
    LONG x, y;

    SetAPen(&clockrp1, color);
    AreaMove(&clockrp1, clockrx, clockry);
    
    x = clockrx + cos(angle + angleoff) * clockrx * percent * percent2 / 10000.0;
    y = clockry - sin(angle + angleoff) * clockry * percent * percent2 / 10000.0;

    AreaDraw(&clockrp1, x, y);

    x = clockrx + cos(angle) * clockrx * percent / 100.0;
    y = clockry - sin(angle) * clockry * percent / 100.0;

    AreaDraw(&clockrp1, x, y);

    x = clockrx + cos(angle - angleoff) * clockrx * percent * percent2 / 10000.0;
    y = clockry - sin(angle - angleoff) * clockry * percent * percent2 / 10000.0;

    AreaDraw(&clockrp1, x, y);

    AreaEnd(&clockrp1);
}

/*********************************************************************************************/

static void RenderHourHand(void)
{
    double hour = (currdate.ds_Minute / 60.0);
    double angle;

    if (hour >= 12.0) hour -= 12.0;
    
    angle = MY_PI / 2.0 + MY_PI * 2.0 * (12.0 - hour) / 12.0;
   
    RenderHand(angle,
    	       HOURHANDSIZE_PERCENT,
	       HOURHANDSIZE_PERCENT2,
	       HOURHANDSIZE_ANGLEOFF,
	       dri->dri_Pens[SHADOWPEN]);
}

/*********************************************************************************************/

static void RenderMinuteHand(void)
{
    LONG minute = currdate.ds_Minute;
    double angle;

    angle = MY_PI / 2.0 + MY_PI * 2.0 * ((double)60 - minute) / 60.0;
   
    RenderHand(angle,
    	       MINUTEHANDSIZE_PERCENT,
	       MINUTEHANDSIZE_PERCENT2,
	       MINUTEHANDSIZE_ANGLEOFF,
	       dri->dri_Pens[SHADOWPEN]);
}

/*********************************************************************************************/

static void RenderSecondHand(void)
{
    LONG second = currdate.ds_Tick / TICKS_PER_SECOND;
    LONG x, y;
    double angle;
    
    angle = MY_PI / 2.0 + MY_PI * 2.0 * ((double)60 - second) / 60.0;
    
    SetAPen(&clockrp2, dri->dri_Pens[FILLPEN]);
    Move(&clockrp2, clockrx, clockry);
    
    x = clockrx + cos(angle) * clockrx * SECONDHANDSIZE_PERCENT / 100.0;
    y = clockry - sin(angle) * clockry * SECONDHANDSIZE_PERCENT / 100.0;
    
    Draw(&clockrp2, x, y);
}

/*********************************************************************************************/

static void RenderClock1(void)
{
    WORD rasterw, rasterh;
    
    rasterw = clockrx * 2 + 1;
    rasterh = clockry * 2 + 1;
    
    if (!clockbm1 || (rasterw != clockraster1_w) || (rasterh != clockraster1_h))
    {
    	if (clockbm1) FreeBitMap(clockbm1);
    	if (clockraster1) FreeRaster(clockraster1, clockraster1_w, clockraster1_h);
	clockraster1 = NULL;

    	clockraster1_w = rasterw;
	clockraster1_h = rasterh;
	
	    
    	clockbm1 = AllocBitMap(rasterw, rasterh, 0, BMF_MINPLANES, scr->RastPort.BitMap);
    	if (!clockbm1) return;
 
    	clockraster1 = AllocRaster(rasterw, rasterh);
    	if (!clockraster1)
    	{
    	    FreeBitMap(clockbm1);
	    clockbm1 = NULL;
	    return;
    	}
    
    }
 
    InitRastPort(&clockrp1);
    clockrp1.BitMap = clockbm1;
       
    InitArea(&areainfo, areabuffer, sizeof(areabuffer) / 5);
    clockrp1.AreaInfo = &areainfo;
    
    InitTmpRas(&tmpras, clockraster1, RASSIZE(rasterw, rasterh));
    clockrp1.TmpRas = &tmpras;
    
    SetRast(&clockrp1, dri->dri_Pens[BACKGROUNDPEN]);
    SetAPen(&clockrp1, dri->dri_Pens[SHINEPEN]);
    AreaEllipse(&clockrp1, clockrx, clockry, clockrx - 2, clockry - 2);
    AreaEnd(&clockrp1);
    SetAPen(&clockrp1, dri->dri_Pens[SHADOWPEN]);
    DrawEllipse(&clockrp1, clockrx, clockry, clockrx, clockry);
    DrawEllipse(&clockrp1, clockrx, clockry, clockrx - 1, clockry);
    DrawEllipse(&clockrp1, clockrx, clockry, clockrx, clockry - 1);
    DrawEllipse(&clockrp1, clockrx, clockry, clockrx - 1, clockry - 1);

    RenderTicks();
    RenderHourHand();
    RenderMinuteHand();

    DeinitRastPort(&clockrp1);
}

/*********************************************************************************************/

static BOOL RenderClock2(void)
{
    static struct DateStamp olddate;
    static BOOL firsttime = TRUE;
    static BOOL old_opt_showsecs;
    BOOL newdate, newsec, blit1to2 = FALSE;
    BOOL retval = FALSE;
    
    WORD rasterw, rasterh;
    
    rasterw = clockrx * 2 + 1;
    rasterh = clockry * 2 + 1;
   
    newdate = (olddate.ds_Days != currdate.ds_Days) ||
    	      (olddate.ds_Minute != currdate.ds_Minute);
    newsec = (olddate.ds_Tick / TICKS_PER_SECOND) != (currdate.ds_Tick / TICKS_PER_SECOND);
    
    if (!clockbm1 || firsttime || newdate ||
    	(rasterw != clockraster2_w) ||
	(rasterh != clockraster2_h))
    {
	RenderClock1();
	blit1to2 = TRUE;
    }
    
    if (!clockbm1) return;
   
    if (!clockbm2 || (rasterw != clockraster2_w) || (rasterh != clockraster2_h))
    {
    	if (clockbm2) FreeBitMap(clockbm2);
    	if (clockraster2) FreeRaster(clockraster2, clockraster2_w, clockraster2_h);
	clockraster2 = NULL;

    	clockraster2_w = rasterw;
	clockraster2_h = rasterh;
		
    	clockbm2 = AllocBitMap(rasterw, rasterh, 0, BMF_MINPLANES, scr->RastPort.BitMap);
    	if (!clockbm2) return;
    
    	clockraster2 = AllocRaster(rasterw, rasterh);
    	if (!clockraster2)
    	{
    	    FreeBitMap(clockbm2);
	    clockbm2 = NULL;
	    return;
    	}
    
    	blit1to2 = TRUE;
    }
    
    if (blit1to2) newsec = TRUE;
    if (newsec && opt_showsecs) blit1to2 = TRUE;
    if (opt_showsecs != old_opt_showsecs) blit1to2 = TRUE;
    old_opt_showsecs = opt_showsecs;
    
    if (blit1to2)
    {
    	BltBitMap(clockbm1, 0, 0, clockbm2, 0, 0, rasterw, rasterh, 192, 255, 0);
	retval = TRUE;
    }

    if (newsec && opt_showsecs)
    {
	InitRastPort(&clockrp2);
	clockrp2.BitMap = clockbm2;

	InitArea(&areainfo, areabuffer, sizeof(areabuffer) / 5);
	clockrp2.AreaInfo = &areainfo;

	InitTmpRas(&tmpras, clockraster2, RASSIZE(rasterw, rasterh));
	clockrp2.TmpRas = &tmpras;

	RenderSecondHand();
	
    	DeinitRastPort(&clockrp2);
    }
        
    olddate = currdate;
    firsttime = FALSE;

}

/*********************************************************************************************/

static void RenderClock(void)
{
    LockLayerInfo(&scr->LayerInfo);

    CalcClockMetrics();
      
    if (RenderClock2())
    {

	SetAPen(win->RPort, dri->dri_Pens[BACKGROUNDPEN]);

	if (clockbm2)
	{
	    RectFill(win->RPort,
		     win->BorderLeft,
		     win->BorderTop,
		     win->Width - win->BorderRight - 1,
		     win->BorderTop + BORDER_CLOCK_SPACING_Y - 1);
	    RectFill(win->RPort,
		     win->Width - win->BorderRight- BORDER_CLOCK_SPACING_X,
		     win->BorderTop + BORDER_CLOCK_SPACING_Y,
		     win->Width - win->BorderRight - 1,
		     win->Height - win->BorderBottom - 1);

	    RectFill(win->RPort,
		     win->BorderLeft,
	    	     win->Height - win->BorderBottom - BORDER_CLOCK_SPACING_Y,
		     win->Width - win->BorderRight - BORDER_CLOCK_SPACING_X,
		     win->Height - win->BorderBottom - 1);

	    RectFill(win->RPort,
		     win->BorderLeft,
		     win->BorderTop + BORDER_CLOCK_SPACING_Y,
		     win->BorderLeft + BORDER_CLOCK_SPACING_X - 1,
		     win->Height - win->BorderBottom - BORDER_CLOCK_SPACING_Y);

    	    BltBitMapRastPort(clockbm2,
	    	    	      0,
			      0,
			      win->RPort,
			      clockcx - clockrx,
			      clockcy - clockry,
			      clockraster2_w,
			      clockraster2_h,
			      192);
	}
	else
	{
	    RectFill(win->RPort,
		     win->BorderLeft,
		     win->BorderTop,
		     win->Width - win->BorderRight - 1,
		     win->Height - win->BorderBottom - 1);
	}
	
    } /* if (RenderClock2()) */
    
    UnlockLayerInfo(&scr->LayerInfo);
	
}

/*********************************************************************************************/

static void MakeWindow(void)
{
    WORD minwidth, minheight;
    
    if (opt_wintop == -1) opt_wintop = scr->BarHeight + 1;
    
    minwidth  = scr->WBorLeft + scr->WBorRight + 50;
    minheight = scr->WBorTop + scr->Font->ta_YSize + 1 + scr->WBorBottom + 50;

    SetMenuFlags();
    
    win = OpenWindowTags(0, WA_PubScreen        , (IPTR)scr                 ,
			    WA_Title            , (IPTR)MSG(MSG_WINTITLE)   ,
			    WA_CloseGadget      , TRUE                      ,
			    WA_DepthGadget      , TRUE                      ,
			    WA_DragBar          , TRUE                      ,
			    WA_SizeGadget       , TRUE                      ,
			    WA_Activate         , TRUE                      ,
			    WA_SizeBBottom  	, TRUE	    	    	    ,
			    WA_SimpleRefresh    , TRUE                      ,
			    WA_NewLookMenus     , TRUE                      ,
			    WA_Left             , opt_winleft               ,
			    WA_Top              , opt_wintop	            ,
			    WA_Width        	, opt_winwidth              ,
			    WA_Height       	, opt_winheight             ,
			    WA_AutoAdjust       , TRUE                      ,
			    WA_MinWidth         , minwidth                  ,
			    WA_MinHeight        , minheight                 ,
			    WA_MaxWidth         , 16383                     ,
			    WA_MaxHeight        , 16383                     ,
			    WA_BackFill     	, (IPTR)LAYERS_NOBACKFILL   ,
			    WA_IDCMP            , IDCMP_CLOSEWINDOW |
						  IDCMP_MOUSEMOVE   |
						  IDCMP_VANILLAKEY  |                                             
						  IDCMP_RAWKEY      |
						  IDCMP_MENUPICK    |
						  IDCMP_INTUITICKS  |
						  IDCMP_NEWSIZE     |
						  IDCMP_REFRESHWINDOW	    ,
			    TAG_DONE);

    if (!win) Cleanup(MSG(MSG_CANT_CREATE_WIN));                            

    winmask = 1L << win->UserPort->mp_SigBit;
    
    SetMenuStrip(win, menus);
}

/*********************************************************************************************/

static void KillWindow(void)
{
    if (win)
    {
	if (menus) ClearMenuStrip(win);
	CloseWindow(win);
	win = NULL;	
    }
}

/*********************************************************************************************/

static void HandleWin(void)
{
    struct IntuiMessage *msg;
    struct MenuItem     *item;
    UWORD               men;
    
    while((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
    {
	switch (msg->Class)
	{
	    case IDCMP_CLOSEWINDOW:
		quitme = TRUE;
		break;

	    case IDCMP_REFRESHWINDOW:
		BeginRefresh(win);
		RenderClock();
		EndRefresh(win, TRUE);
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
			    case MSG_MEN_PROJECT_QUIT:
				quitme = TRUE;
				break;

    	    	    	    case MSG_MEN_SETTINGS_SECONDS:
			    	opt_showsecs = MenuChecked(FULLMENUNUM(1, 1, NOSUB));
			    	break;
				
			} /* switch(GTMENUITEM_USERDATA(item)) */

			men = item->NextSelect;
		    }
		    else
		    {
			men = MENUNULL;
		    }

		} /* while(men != MENUNULL) */
		break;

	    case IDCMP_NEWSIZE:
		RenderClock();
		break;

	} /* switch (msg->Class) */

	ReplyMsg((struct Message *)msg);

    } /* while((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) */

}

/*********************************************************************************************/

static void HandleTimer(void)
{
    struct DateStamp ds;
    
    if (timer_running) GetMsg(TimerMP);

    DateStamp(&currdate);
    
    RenderClock();
    
    DateStamp(&ds);
    
    TimerIO->tr_node.io_Command = TR_ADDREQUEST;
    
    if (ds.ds_Tick == 0)
    {
    	TimerIO->tr_time.tv_secs = 1;
    	TimerIO->tr_time.tv_micro = 0;
    }
    else
    {
     	TimerIO->tr_time.tv_secs = 0;
    	TimerIO->tr_time.tv_micro = 1000000 - (1000000 / TICKS_PER_SECOND) * (ds.ds_Tick % TICKS_PER_SECOND);
   }
    
    SendIO(&TimerIO->tr_node);
    timer_running = TRUE;
}

/*********************************************************************************************/

static void HandleAll(void)
{
    ULONG sigs;
    
    HandleTimer();
    
    while(!quitme)
    {
    	sigs = Wait(winmask | timermask | SIGBREAKF_CTRL_C);
	
	if (sigs & SIGBREAKF_CTRL_C) quitme = TRUE;
	if (sigs & winmask) HandleWin();
	if (sigs & timermask) HandleTimer();
    }
}

/*********************************************************************************************/

int main(void)
{
    InitLocale("Sys/clock.catalog", 1);
    InitMenus();
    OpenLibs();
    OpenTimer();
    GetArguments();
    GetVisual();
    MakeMenus();
    MakeWindow();
    HandleAll();
    Cleanup(NULL);
    
    return 0;
}

/*********************************************************************************************/


