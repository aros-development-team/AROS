/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <libraries/commodities.h>
#include <libraries/locale.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/commodities.h>
#include <proto/alib.h>
#include <proto/locale.h>

#ifdef __AROS__
#    include <aros/debug.h>
#else
#    undef kprintf
#    define kprintf(...) (void)0
# ifndef __MORPHOS__
     typedef ULONG IPTR;
# endif
#endif
     

#include <stdio.h>
#include <stdlib.h>

/************************************************************************************/

UBYTE version[] = "$VER: Blanker 0.9 (02.10.2002)";

#define ARG_TEMPLATE "CX_PRIORITY=PRI/N/K,SECONDS=SEC/N/K,STARS=ST/N/K"

#define ARG_PRI     	0
#define ARG_SEC     	1
#define ARG_STARS   	2
#define NUM_ARGS    	3

#define MAX_STARS   	1000

#define CMD_STARTBLANK	1
#define CMD_STOPBLANK   2

#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#define CATCOMP_ARRAY

#include "strings.h"

#ifdef __MORPHOS__
static void BlankerAction(void);

static struct EmulLibEntry BlankerActionEntry =
{
   TRAP_LIB,
   0,
   (void (*)(void))BlankerAction
};
#endif

static struct NewBroker nb =
{
   NB_VERSION,
   NULL,
   NULL,
   NULL,
   NBU_NOTIFY | NBU_UNIQUE, 
   0,
   0,
   NULL,
   0 
};


/* Libraries to open */
struct LibTable
{
    APTR   lT_Library;
    STRPTR lT_Name;
    ULONG  lT_Version;
}
libTable[] =
{
    { &IntuitionBase, "intuition.library"   , 39L },
    { &GfxBase	    , "graphics.library"    , 39L },
    { &CxBase	    , "commodities.library" , 39L },
    { NULL  	    	    	    	    	  }
};

#ifdef __AROS__
struct LocaleBase   	*LocaleBase    = NULL;
#endif
struct Library      	*GadToolsBase  = NULL;
struct Library      	*CxBase        = NULL;
struct IntuitionBase 	*IntuitionBase = NULL;

static struct MsgPort 	*cxport;
static struct Window 	*win;
static struct RastPort  *rp;
static struct ColorMap  *cm;
static struct Task  	*maintask;

static struct Catalog 	*catalogPtr;
static struct RDArgs 	*myargs;
static CxObj 	    	*cxbroker, *cxcust;
static ULONG 	    	cxmask, actionmask;
static WORD 	    	scrwidth, scrheight, actioncmd, visible_sky;
static LONG 	    	blackpen, star1pen, star2pen, star3pen;
static WORD 	    	num_stars = 200, blankwait = 30;
static UBYTE 	    	actionsig;
static BOOL 	    	blanked, quitme, disabled, pens_allocated;

static LONG 	    	args[NUM_ARGS];
static char 	    	s[256];
static WORD 	    	star_x[MAX_STARS], star_y[MAX_STARS],
	    	    	star_speed[MAX_STARS], star_col[MAX_STARS];

/************************************************************************************/

static void FreePens(void)
{
    if (pens_allocated)
    {
    	ReleasePen(cm, blackpen);
    	ReleasePen(cm, star1pen);
    	ReleasePen(cm, star2pen);
    	ReleasePen(cm, star3pen);
	
	pens_allocated = FALSE;
    }
}

/************************************************************************************/

static void Cleanup(char *msg)
{
    struct Message  *cxmsg;
    struct LibTable *tmpLibTable = libTable;
    
    if(msg)
    {
	puts(msg);
    }

    if(IntuitionBase)
    {
	if(win)
	{
	    FreePens();
	    CloseWindow(win);
	}
    }

    if(CxBase)
    {

	if(cxbroker)
	    DeleteCxObjAll(cxbroker);

	if(cxport)
	{
	    while((cxmsg = GetMsg(cxport)))
	    {
		ReplyMsg(cxmsg);
	    }

	    DeleteMsgPort(cxport);
	}
    }
    
    if(myargs)
	FreeArgs(myargs);

    if(LocaleBase)
    {
	CloseCatalog(catalogPtr);
	CloseLibrary((struct Library *)LocaleBase); /* Passing NULL is valid */
	kprintf("Closed locale.library!\n");
    }

    while(tmpLibTable->lT_Name) /* Check for name rather than pointer */
    {
	if((*(struct Library **)tmpLibTable->lT_Library))
	{
	    CloseLibrary((*(struct Library **)tmpLibTable->lT_Library));
	    kprintf("Closed %s!\n", tmpLibTable->lT_Name);
	}

	tmpLibTable++;
    }

    if(actionsig)
	FreeSignal(actionsig);
    
    exit(0);
}

/************************************************************************************/

STRPTR getCatalog(struct Catalog *catalogPtr, ULONG id)
{
    STRPTR string;

    if(catalogPtr)
	string = GetCatalogStr(catalogPtr, id, CatCompArray[id].cca_Str);
    else
	string = CatCompArray[id].cca_Str;

    return(string);
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
    struct LibTable *tmpLibTable = libTable;
    UBYTE   	    tmpString[128]; /* petah: What if library name plus error message exceeds 128 bytes? */

    if((LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 40)))
    {
	catalogPtr = OpenCatalog
        (
            NULL, "System/Tools/Commodities.catalog", 
            OC_BuiltInLanguage, (IPTR) "english",
            TAG_DONE
        );
    }
    else
	kprintf("Warning: Can't open locale.library V40!\n");

    while(tmpLibTable->lT_Library)
    {
	if(!((*(struct Library **)tmpLibTable->lT_Library = OpenLibrary(tmpLibTable->lT_Name, tmpLibTable->lT_Version))))
	{
	    sprintf(tmpString, getCatalog(catalogPtr, MSG_CANT_OPEN_LIB), tmpLibTable->lT_Name, tmpLibTable->lT_Version);
            //showSimpleMessage(ec, tmpString);
            Cleanup(tmpString);
        }
        else
	    kprintf("Library %s opened!\n", tmpLibTable->lT_Name);

        tmpLibTable++;
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

#ifdef __MORPHOS__
static void BlankerAction(void)
#else
static void BlankerAction(CxMsg *msg,CxObj *obj)
#endif
{
#ifdef __MORPHOS__
    CxMsg *msg = (CxMsg *)REG_A0;
    CxObj *obj = (CxObj *)REG_A1;
#endif
    struct InputEvent *ie = (struct InputEvent *)CxMsgData(msg);
    static ULONG       timecounter = 0;

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
    }
    else if ((ie->ie_Class == IECLASS_RAWMOUSE) || (ie->ie_Class == IECLASS_RAWKEY))
    {
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
        Cleanup(getCatalog(catalogPtr, MSG_CANT_CREATE_MSGPORT));
    }
    
    nb.nb_Port = cxport;
    
    cxmask = 1L << cxport->mp_SigBit;
    
    if (!(cxbroker = CxBroker(&nb, 0)))
    {
        Cleanup(getCatalog(catalogPtr, MSG_CANT_CREATE_BROKER));
    }
    
#ifdef __MORPHOS__
    if (!(cxcust = CxCustom(&BlankerActionEntry, 0)))
#else
    if (!(cxcust = CxCustom(BlankerAction, 0)))
#endif
    {
        Cleanup(getCatalog(catalogPtr, MSG_CANT_CREATE_CUSTOM));
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

static void HandleWin(void);

/************************************************************************************/

static void MakeWin(void)
{
    struct Screen  *screenPtr;
    WORD    	    y, y2, stripheight = 20;
    LONG    	    i;

    if(!(screenPtr = LockPubScreen(NULL)))
     kprintf("Warning: LockPubScreen() failed!\n");

    win = OpenWindowTags(0, WA_Left, 0,
			    WA_Top, 0,
			    WA_Width, screenPtr->Width,
			    WA_Height, screenPtr->Height,
			    WA_AutoAdjust, TRUE,
			    WA_BackFill, (IPTR)LAYERS_NOBACKFILL,
			    WA_SimpleRefresh, TRUE,
			    WA_Borderless, TRUE,
			    TAG_DONE);

    if(screenPtr)
    	UnlockPubScreen(NULL, screenPtr);

    if(win)
    {
        rp = win->RPort;

        scrwidth  = win->Width;
	scrheight = win->Height;
	
	cm = win->WScreen->ViewPort.ColorMap;
	
	blackpen = ObtainBestPenA(cm, 0x00000000, 0x00000000, 0x00000000, NULL);
	star1pen = ObtainBestPenA(cm, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, NULL);
	star2pen = ObtainBestPenA(cm, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA, NULL);
	star3pen = ObtainBestPenA(cm, 0x88888888, 0x88888888, 0x88888888, NULL);
	
	pens_allocated = TRUE;
	
	for(i = 0;i < num_stars;i++)
	{
	    star_x[i] = myrand() * scrwidth / MY_RAND_MAX;
	    star_y[i] = 1 + (myrand() * (scrheight - 2)) / MY_RAND_MAX;
	    star_speed[i] = 1 + myrand() * 3 / MY_RAND_MAX;
	    if (star_speed[i] < 2)
	    {
		star_col[i] = star3pen;
	    }
	    else if (star_speed[i] < 3)
	    {
		star_col[i] = star2pen;
	    }
	    else
	    {
		star_col[i] = star1pen;
	    }
	}

	SetAPen(rp, blackpen);
	for(y = 0;y < scrheight - 1;y++, stripheight++)
	{
	    if (CheckSignal(actionmask))
	    {
	        if (actioncmd == CMD_STOPBLANK)
		{
		    FreePens();
		    CloseWindow(win);
		    win = 0;
		    break;
		}
	    }
	    
            for(y2 = y;y2 < scrheight - 1;y2 += stripheight)
	    {
        	ClipBlit(rp, 0, y2, rp, 0, y2 + 1, scrwidth, scrheight - y2 - 1, 192);
		SetAPen(rp, blackpen);
        	RectFill(rp, 0, y2, scrwidth - 1, y2);

    	    #if 0
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
	    #endif
	    
	    } /* for(y2 = y;y2 < scrheight - 1;y2 += stripheight) */
	    
	    visible_sky = y;
	    
	    HandleWin();
	    
	    WaitTOF();
	    
	} /* for(y = 0;y < scrheight - 1;y++, stripheight++) */
	
    } /* if (win) */
    else
    {
    	printf("%s", getCatalog(catalogPtr, MSG_CANT_CREATE_WIN));
    }
}

/************************************************************************************/

static void HandleWin(void)
{
    LONG i;
    
    for(i = 0; i < num_stars;i++)
    {
    	if (star_y[i] <= visible_sky)
	{
	    SetAPen(rp, blackpen);
	    WritePixel(rp, star_x[i], star_y[i]);

	    star_x[i] -= star_speed[i];
	    if (star_x[i] < 0) star_x[i] += scrwidth;

	    SetAPen(rp, star_col[i]);
	    WritePixel(rp, star_x[i], star_y[i]);
	}
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
	    	FreePens();
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
	    WaitTOF();
	    sigs = CheckSignal(cxmask | actionmask | SIGBREAKF_CTRL_C);
	}
	else
	{
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

    nb.nb_Name = getCatalog(catalogPtr, MSG_BLANKER_CXNAME);
    nb.nb_Title = getCatalog(catalogPtr, MSG_BLANKER_CXTITLE);
    nb.nb_Descr = getCatalog(catalogPtr, MSG_BLANKER_CXDESCR);

    GetArguments();
    InitCX();
    HandleAll();
    Cleanup(0);
    return 0;
}


/************************************************************************************/
