#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <libraries/commodities.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/commodities.h>
#include <proto/alib.h>

#include <aros/debug.h>

#include <stdio.h>


/************************************************************************************/

#define ARG_TEMPLATE "CX_PRIORITY=PRI/N/K,SECONDS=SEC/N/K,STARS=ST/N/K"

#define ARG_PRI   0
#define ARG_SEC   1
#define ARG_STARS 2
#define NUM_ARGS  3

#define MAX_STARS 1000

#define CMD_STARTBLANK 1
#define CMD_STOPBLANK  2

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *CxBase;

static struct NewBroker nb =
{
   NB_VERSION,
   "Blanker", 
   "System Screen Blanker", 
   "Blanks Screen When System Inactive", 
   NBU_NOTIFY | NBU_UNIQUE, 
   0,
   0,
   NULL,                             
   0 
};

static struct MsgPort *cxport;
static struct Window *win;
static struct RastPort *rp;
static struct Task *maintask;

static struct RDArgs *myargs;
static CxObj *cxbroker, *cxcust;
static ULONG cxmask, actionmask;
static WORD scrwidth, scrheight, actioncmd;
static WORD num_stars = 200, blankwait = 30;
static UBYTE actionsig;
static BOOL blanked, quitme, disabled;

static LONG args[NUM_ARGS];
static char s[256];
static WORD star_x[MAX_STARS], star_y[MAX_STARS],
	    star_speed[MAX_STARS], star_col[MAX_STARS];

/************************************************************************************/

static void Cleanup(char *msg)
{
    struct Message *cxmsg;
    
    if (msg)
    {
	printf("Blanker: %s\n",msg);
    }

    if (win) CloseWindow(win);

    if (cxbroker) DeleteCxObjAll(cxbroker);
    if (cxport)
    {
        while((cxmsg = GetMsg(cxport)))
	{
	    ReplyMsg(cxmsg);
	}
        DeleteMsgPort(cxport);
    }
    
    if (myargs) FreeArgs(myargs);

    if (CxBase) CloseLibrary(CxBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);

    if (actionsig) FreeSignal(actionsig);
    
    exit(0);
}

/************************************************************************************/

static void DosError(void)
{
    Fault(IoErr(),0,s,255);
    Cleanup(s);
}

/************************************************************************************/

static void Init(void)
{
    maintask = FindTask(0);
    actionsig = AllocSignal(-1);
    actionmask = 1L << actionsig;
}

/************************************************************************************/

static void OpenLibs(void)
{
    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",39)))
    {
	Cleanup("Can't open intuition.library V39!");
    }

    if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",39)))
    {
	Cleanup("Can't open graphics.library V39!");
    }

    if (!(CxBase = OpenLibrary("commodities.library",39)))
    {
	Cleanup("Can't open commodities.library V39!");
    }
}

/************************************************************************************/

static void GetArguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, 0)))
    {
	DosError();
    }
    
    if (args[ARG_PRI]) nb.nb_Pri = *(LONG *)args[ARG_PRI];
    if (args[ARG_SEC]) blankwait = *(LONG *)args[ARG_SEC];
    if (args[ARG_STARS])
    {
    	num_stars = *(LONG *)args[ARG_STARS];
	if (num_stars < 0)
	{
	    num_stars = 0;
	} else if (num_stars > MAX_STARS)
	{
	    num_stars = MAX_STARS;
	}
    }
}

/************************************************************************************/

static void BlankerAction(CxMsg *msg,CxObj *obj)
{
    struct InputEvent *ie = (struct InputEvent *)CxMsgData(msg);
    static ULONG timecounter = 0;

    if (ie->ie_Class == IECLASS_TIMER)
    {
	if (disabled)
	{
            timecounter = 0;
	}
	else if (!blanked)
	{
            timecounter++;
            if(timecounter >= blankwait * 10)
            {
        	actioncmd = CMD_STARTBLANK;
        	Signal(maintask, actionmask);

        	blanked = TRUE;
            }
	}
    } else {
	if (ie->ie_Class != IECLASS_TIMER)
	{
	    timecounter = 0;
	    
	    if (blanked)
	    {
        	actioncmd = CMD_STOPBLANK;
        	Signal(maintask, actionmask);

		blanked = FALSE;
	    }
	}
    }

}

/************************************************************************************/

static void InitCX(void)
{
    if (!(cxport = CreateMsgPort()))
    {
        Cleanup("Can't create MsgPort!\n");
    }
    
    nb.nb_Port = cxport;
    
    cxmask = 1L << cxport->mp_SigBit;
    
    if (!(cxbroker = CxBroker(&nb, 0)))
    {
        Cleanup("Can't create CxBroker object!\n");
    }
    
    if (!(cxcust = CxCustom(BlankerAction, 0)))
    {
        Cleanup("Can't create CxCustom object!\n");
    }
    
    AttachCxObj(cxbroker, cxcust);
    ActivateCxObj(cxbroker, 1);
    
}
/************************************************************************************/

#define MY_RAND_MAX 32767

static LONG myrand(void)
{
    static LONG a = 1;
    
    return (a = a * 1103515245 + 12345) & MY_RAND_MAX;
}

/************************************************************************************/

static void MakeWin(void)
{
    WORD y, y2, stripheight = 20;
    LONG i;
    
    win = OpenWindowTags(0,WA_Left,0,
			   WA_Top,0,
			   WA_Width,10000,
			   WA_Height,10000,
			   WA_AutoAdjust,TRUE,
			   WA_BackFill,(IPTR)LAYERS_NOBACKFILL,
			   WA_SimpleRefresh,TRUE,
			   WA_Borderless,TRUE,
			   TAG_DONE);

    if (win)
    {
        rp = win->RPort;

        scrwidth  = win->WScreen->Width;
	scrheight = win->WScreen->Height;
	
	for(i = 0;i < num_stars;i++)
	{
	    star_x[i] = myrand() * scrwidth / MY_RAND_MAX;
	    star_y[i] = myrand() * scrheight / MY_RAND_MAX;
	    star_speed[i] = 1 + myrand() * 3 / MY_RAND_MAX;
	    if (star_speed[i] < 2)
	    {
		star_col[i] = 12;
	    } else if (star_speed[i] < 3)
	    {
		star_col[i] = 0;
	    } else {
		star_col[i] = 2;
	    }
	}

	SetAPen(rp, 1);
	for(y = 0;y < scrheight - 1;y++, stripheight++)
	{
	    if (CheckSignal(actionmask))
	    {
	        if (actioncmd == CMD_STOPBLANK)
		{
		    CloseWindow(win);
		    win = 0;
		    break;
		}
	    }
	    
            for(y2 = y;y2 < scrheight - 1;y2 += stripheight)
	    {
        	ClipBlit(rp, 0, y2, rp, 0, y2 + 1, scrwidth, scrheight - y2 - 1, 192);
		SetAPen(rp, 1);
        	RectFill(rp, 0, y2, scrwidth - 1, y2);

        	if (y2 == y)
		{
		    for(i = 0; i < num_stars; i++)
		    {
			if (star_y[i] == y2)
			{
			    SetAPen(rp, star_col[i]);
			    WritePixel(rp, star_x[i], y2);
			}
		    }
		} /* if (y2 == y) */
		
	    } /* for(y2 = y;y2 < scrheight - 1;y2 += stripheight) */
	    
	} /* for(y = 0;y < scrheight - 1;y++, stripheight++) */
	
    } /* if (win) */
}

/************************************************************************************/

static void HandleWin(void)
{
    LONG i;
    
    for(i = 0; i < num_stars;i++)
    {
	SetAPen(rp, 1);
	WritePixel(rp, star_x[i], star_y[i]);

	star_x[i] += star_speed[i];
	if (star_x[i] >= scrwidth) star_x[i] -= scrwidth;

	SetAPen(rp, star_col[i]);
	WritePixel(rp, star_x[i], star_y[i]);
    }
	
}

/************************************************************************************/

static void HandleAction(void)
{
    switch(actioncmd)
    {
        case CMD_STARTBLANK:
	    if (!win) MakeWin();
	    break;
	    
	case CMD_STOPBLANK:
	    if (win)
	    {
	        CloseWindow(win);
		win = 0;
	    }
	    break;		
    }
}

/************************************************************************************/

static void HandleCx(void)
{
    CxMsg *msg;
    while((msg = (CxMsg *)GetMsg(cxport)))
    {
       switch(CxMsgType(msg))
       {
	    case CXM_COMMAND:
               switch(CxMsgID(msg))
               {
        	  case CXCMD_DISABLE:
        	     ActivateCxObj(cxbroker,0L);
		     disabled = TRUE;
        	     break;

        	  case CXCMD_ENABLE:
        	     ActivateCxObj(cxbroker,1L);
		     disabled = FALSE;
        	     break;

        	  case CXCMD_KILL:
        	     quitme = TRUE;
        	     break;

               } /* switch(CxMsgID(msg)) */
               break;

       } /* switch (CxMsgType(msg))*/
       
       ReplyMsg((struct Message *)msg);
       
   } /* while((msg = (CxMsg *)GetMsg(cxport))) */
}

/************************************************************************************/

static void HandleAll(void)
{
    ULONG sigs;
    
    while(!quitme)
    {
        if (win)
	{
	    HandleWin();
	    sigs = CheckSignal(cxmask | actionmask | SIGBREAKF_CTRL_C);
	} else {
            sigs = Wait(cxmask | actionmask | SIGBREAKF_CTRL_C);
	}
	
	if (sigs & cxmask) HandleCx();
	if (sigs & actionmask) HandleAction();
	if (sigs & SIGBREAKF_CTRL_C) quitme = TRUE;
	
    } /* while(!quitme) */
    
}

/************************************************************************************/

int main(void)
{
    Init();
    OpenLibs();
    GetArguments();
    InitCX();
    HandleAll();
    Cleanup(0);
    return 0;
}


/************************************************************************************/
