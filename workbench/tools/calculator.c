/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <libraries/locale.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/locale.h>
#include <proto/alib.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

#define ARG_TEMPLATE "PUBSCREEN,TAPE/K"

enum {ARG_PUBSCREEN,ARG_TAPE,NUM_ARGS};

#define MAX_VAL_LEN 13

#define INNER_SPACING_X 4
#define INNER_SPACING_Y 4

#define BUTTON_SPACING_X 4
#define BUTTON_SPACING_Y 4

#define BUTTON_LED_SPACING 4

#define NUM_BUTTONS 20
#define NUM_BUTTON_COLS 5
#define NUM_BUTTON_ROWS 4

#define BUTTON_EXTRA_WIDTH 8
#define BUTTON_EXTRA_HEIGHT 4

#define LED_EXTRA_HEIGHT 4

enum
{
    STATE_LEFTVAL, STATE_OP, STATE_RIGHTVAL, STATE_EQU
};

enum
{
    BTYPE_0,
    BTYPE_1,
    BTYPE_2,
    BTYPE_3,
    BTYPE_4,
    BTYPE_5,
    BTYPE_6,
    BTYPE_7,
    BTYPE_8,
    BTYPE_9,
    BTYPE_COMMA,
    BTYPE_BS,
    BTYPE_CA,
    BTYPE_CE,
    BTYPE_MUL,
    BTYPE_DIV,
    BTYPE_SUB,
    BTYPE_ADD,
    BTYPE_SIGN,
    BTYPE_EQU,
    
    BTYPE_LED
};

struct ButtonInfo
{
    char *text;
    WORD type;
    char key1;
    char key2;
};

static struct ButtonInfo bi[NUM_BUTTONS] =
{
    {"7"	,BTYPE_7	, '7'	, 0	},
    {"8"	,BTYPE_8	, '8'	, 0	},
    {"9"	,BTYPE_9	, '9'	, 0	},
    {"CA"	,BTYPE_CA	, 'A'	, 127	},
    {"CE"	,BTYPE_CE	, 'E'	, 0	},
    
    {"4"	,BTYPE_4	, '4'	, 0	},
    {"5"	,BTYPE_5	, '5'	, 0	},
    {"6"	,BTYPE_6	, '6'	, 0	},
    {"×"	,BTYPE_MUL	, '*'	, 'X'	},
    {":"	,BTYPE_DIV	, '/'	, ':'	},
    
    {"1"	,BTYPE_1	, '1'	, 0	},
    {"2"	,BTYPE_2	, '2'	, 0	},
    {"3"	,BTYPE_3	, '3'	, 0	},
    {"+"	,BTYPE_ADD	, '+'	, 0	},
    {"-"	,BTYPE_SUB	, '-'	, 0	},
    
    {"0"	,BTYPE_0	, '0'	, 0	},
    {"."	,BTYPE_COMMA	, '.'	, ','	},
    {"«"	,BTYPE_BS	, 8  	, 0	},
    {"±"	,BTYPE_SIGN	, 'S'	, 0	},
    {"="	,BTYPE_EQU	, '='	, 13	}
};


struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *GadToolsBase;
struct LocaleBase *LocaleBase;

static struct Screen *scr;
static struct DrawInfo *dri;
static struct Gadget *gadlist, *gad[NUM_BUTTONS + 2];
static struct Window *win;
static struct RDArgs *MyArgs;
static APTR vi;
static FILE *tapefh;

static WORD win_borderleft,win_bordertop;
static WORD buttonwidth,buttonheight,ledheight;
static WORD inner_winwidth,inner_winheight;
static WORD vallen,state,operation;

static BOOL dotape;

static double leftval,rightval;

static char comma,*pubscrname;
static char ledstring[256],visledstring[256],
				tempstring[256],tapename[256];

static char *deftapename = "RAW:%ld/%ld/%ld/%ld/Calculator Tape/INACTIVE/SCREEN%s";

UBYTE version[] = "$VER: Calculator 1.1 (1.5.2001)";

static LONG Args[NUM_ARGS];

static void Cleanup(char *msg)
{
    WORD rc;
    
    if (msg)
    {
	printf("Calculator: %s\n",msg);
	rc = RETURN_WARN;
    } else {
	rc = RETURN_OK;
    }
    
    if (tapefh) fclose(tapefh);
    
    if (win) CloseWindow(win);
    
    if (gadlist) FreeGadgets(gadlist);
    
    if (vi) FreeVisualInfo(vi);
    if (dri) FreeScreenDrawInfo(scr,dri);
    if (scr) UnlockPubScreen(0,scr);
    
    if (MyArgs) FreeArgs(MyArgs);
    
    if (LocaleBase) CloseLibrary((struct Library *)LocaleBase);
    if (GadToolsBase) CloseLibrary(GadToolsBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    
    exit (rc);
}

static void DosError(void)
{
    Fault(IoErr(),0,tempstring,255);
    Cleanup(tempstring);
}

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
    
    if (!(GadToolsBase = OpenLibrary("gadtools.library",39)))
    {
	Cleanup("Can't open gadtools.library V39!");
    }
    
    LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library",39);
}

static void GetArguments(void)
{
    if (!(MyArgs = ReadArgs(ARG_TEMPLATE,(LONG *)Args,0)))
    {
	DosError();
    }
    
    pubscrname = (char *)Args[ARG_PUBSCREEN];
    
    if (Args[ARG_TAPE])
    {
	strcpy(tapename,(char *)Args[ARG_TAPE]);
	dotape = TRUE;
    }
}

static void DoLocale(void)
{
    struct Locale *loc;
    
    comma = '.';
    
    if ((loc = OpenLocale(0)))
    {
    	comma = loc->loc_DecimalPoint[0];
    	CloseLocale(loc);
    }
    
    bi[16].text[0] = comma;
}

static void GetVisual(void)
{
    if (pubscrname) scr = LockPubScreen(pubscrname);
    
    if (!scr)
    {
	if (!(scr = LockPubScreen(0)))
	{
	    Cleanup("Can't lock screen!");
	}
    }
    
    if (!(dri = GetScreenDrawInfo(scr)))
    {
	Cleanup("Can't get drawinfo!");
    }
    
    if (!(vi = GetVisualInfo(scr,0)))
    {
	Cleanup("Can't get visual info!");
    }
    
    win_borderleft = scr->WBorLeft;

    /* SDuvan: was scr->WBorTop + scr->Font->ta_YSize + 1 */
    win_bordertop = scr->WBorTop + dri->dri_Font->tf_YSize + 1;

}

static void InitGUI(void)
{
    static struct RastPort temprp;
    
    WORD i,len;
    
    InitRastPort(&temprp);
    SetFont(&temprp,dri->dri_Font);
    
    buttonheight = dri->dri_Font->tf_YSize + BUTTON_EXTRA_HEIGHT;
    
    for(i = 0;i < NUM_BUTTONS;i++)
    {
	len = TextLength(&temprp,bi[i].text,strlen(bi[i].text));
	if (len > buttonwidth) buttonwidth = len;
    }
    
    buttonwidth += BUTTON_EXTRA_WIDTH;
    
    ledheight = dri->dri_Font->tf_YSize + LED_EXTRA_HEIGHT;
    
    inner_winwidth = buttonwidth * NUM_BUTTON_COLS +
	BUTTON_SPACING_X * (NUM_BUTTON_COLS - 1) + 
	INNER_SPACING_X * 2;
    
    inner_winheight = buttonheight * NUM_BUTTON_ROWS +
	BUTTON_SPACING_Y * (NUM_BUTTON_ROWS - 1) +
	BUTTON_LED_SPACING +
	ledheight +
	INNER_SPACING_Y * 2;
    
    DeinitRastPort(&temprp);
    strcpy(ledstring,"0");
}

static void MakeGadgets(void)
{
    struct Gadget *mygad = 0;
    struct NewGadget ng = {0};
    WORD col,row,i;
    
    ng.ng_VisualInfo = vi;
    
    mygad = CreateContext(&gadlist);
    
    ng.ng_GadgetID = BTYPE_LED;
    
    ng.ng_LeftEdge = win_borderleft + INNER_SPACING_X;
    ng.ng_TopEdge  = win_bordertop + INNER_SPACING_Y;
    ng.ng_Width = inner_winwidth - INNER_SPACING_X * 2;
    ng.ng_Height = ledheight;
    
    mygad = gad[BTYPE_LED] = CreateGadget(TEXT_KIND,
					  mygad,
					  &ng,
					  GTTX_Text, (IPTR) ledstring,
					  GTTX_CopyText,TRUE,
					  GTTX_Border,TRUE,
					  GTTX_Justification,GTJ_RIGHT,
					  TAG_DONE);
    
    i = 0;
    
    ng.ng_TopEdge = win_bordertop +
	INNER_SPACING_Y + 
	ledheight +
	BUTTON_LED_SPACING;
    
    ng.ng_Width = buttonwidth;
    ng.ng_Height = buttonheight;
    
    for(row = 0; row < NUM_BUTTON_ROWS; row++)
    {
	for(col = 0; col < NUM_BUTTON_COLS; col++, i++)
	{
	    ng.ng_GadgetID = bi[i].type;
	    
	    ng.ng_LeftEdge = win_borderleft +
		INNER_SPACING_X +
		col * (buttonwidth + BUTTON_SPACING_X);
	    
	    ng.ng_GadgetText = bi[i].text;
	    
	    mygad = gad[bi[i].type] = CreateGadgetA(BUTTON_KIND,
						    mygad,
						    &ng,
						    0);
	    
	} /* for(col = 0;col < NUM_BUTTON_COLS; col++) */
	
	ng.ng_TopEdge += buttonheight + BUTTON_SPACING_Y;
	
    } /* for(row = 0; row < NUM_BUTTON_ROWS; row++) */
    
    if (!mygad)
    {
	Cleanup("Can't create gadgets!");
    }
    
}

static void MakeWin(void)
{
    win = OpenWindowTags(0,WA_PubScreen,(IPTR)scr,
			 WA_Left,scr->MouseX,
			 WA_Top,scr->MouseY,
			 WA_InnerWidth,inner_winwidth,
			 WA_InnerHeight,inner_winheight,
			 WA_AutoAdjust,TRUE,
			 WA_Title,(IPTR)"Calculator",
			 WA_CloseGadget,TRUE,
			 WA_DepthGadget,TRUE,
			 WA_DragBar,TRUE,
			 WA_Activate,TRUE,
			 WA_SimpleRefresh,TRUE,
			 WA_IDCMP,IDCMP_CLOSEWINDOW |
			 IDCMP_GADGETUP |
			 IDCMP_VANILLAKEY |
			 IDCMP_RAWKEY |
			 IDCMP_REFRESHWINDOW,
			 WA_Gadgets,(IPTR)gadlist,
			 TAG_DONE);

    if (!win) Cleanup("Can't open window!");
    
    GT_RefreshWindow(win,0);
    
    ScreenToFront(win->WScreen);
}

static void OpenTape(void)
{
    struct List *l;
    struct PubScreenNode *psn;
    char *scrname = "";
    WORD x,y,w,h;
    
    if (!(tapename[0]))
    {
	l = LockPubScreenList();
	
	psn = (struct PubScreenNode *)l->lh_Head;
	
	while (psn->psn_Node.ln_Succ)
	{
	    if (psn->psn_Screen == scr)
	    {
		if (psn->psn_Node.ln_Name)
		{
		    scrname = psn->psn_Node.ln_Name;
		}
		break;
	    }
	    psn = (struct PubScreenNode *)psn->psn_Node.ln_Succ;
	}
	
	UnlockPubScreenList();
	
	w = win->Width * 5 / 4;
	h = win->Height;
	
	x = win->LeftEdge;
	y = win->TopEdge;
	
	if (x > (scr->Width - (x + w)))
	{
	    x -= w;
	} else {
	    x += win->Width;
	}
	sprintf(tapename,deftapename,x,y,w,h,scrname);
    }
    
    if (!(tapefh = fopen(tapename,"w")))
    {
	DisplayBeep(scr);
    }
}

static double GetValue(void)
{
    double val;
    char c = 0,*sp;
    
    sp = strchr(ledstring,comma);
    if (sp)
    {
	c = *sp;
	*sp = '.';
    }
    
    val = strtod(ledstring,0);
    
    if (sp) *sp = c;
    
    return val;
}

static void GetLeftValue(void)
{	
    leftval = GetValue();
}

static void GetRightValue(void)
{
    rightval = GetValue();
}

static void LeftValToLED(void)
{
    char *sp;
    
    sprintf(ledstring,"%f",leftval);
    
    sp = strchr(ledstring,'.');
    if (!sp) sp = strchr(ledstring,',');
    if (sp) *sp = comma;	
}

static char *DoOperation(void)
{
    char *matherr = 0;
    
    switch (operation)
    {
    case BTYPE_ADD:
	leftval += rightval;
	break;
	
    case BTYPE_SUB:
	leftval -= rightval;
	break;
	
    case BTYPE_MUL:
	leftval *= rightval;
	break;
	
    case BTYPE_DIV:
	if (rightval == 0.0)
	{
	    matherr = "Division by zero!";
	} else {
	    leftval /= rightval;
	}
	break;
    }
    
    if (!matherr) LeftValToLED();
    
    return matherr;
}

static void RefreshLED(void)
{
    strcpy(visledstring,ledstring);
    
    if ((ledstring[0] == ',') ||
	(ledstring[0] == '\0') ||
	((ledstring[0] >= '0') && (ledstring[0] <= '9')))
    {
	visledstring[0] = '\0';
	
	if ((ledstring[0] == ',') ||
	    (ledstring[0] == '.') ||
	    (ledstring[0] == '\0'))
	{
	    strcpy(visledstring,"0");
	}
	strcat(visledstring,ledstring);
    }
    
    GT_SetGadgetAttrs(gad[BTYPE_LED],
		      win,
		      0,
		      GTTX_Text,(IPTR)visledstring,
		      TAG_DONE);
}

static void HandleButton(WORD type)
{
    char *matherr = 0;
    WORD checklen;
    BOOL refresh_led = FALSE;
    
    switch(type)
    {
    case BTYPE_0:
    case BTYPE_1:
    case BTYPE_2:
    case BTYPE_3:
    case BTYPE_4:
    case BTYPE_5:
    case BTYPE_6:
    case BTYPE_7:
    case BTYPE_8:
    case BTYPE_9:
	checklen = vallen;
	if ((strchr(ledstring,comma))) checklen--;
	if ((strchr(ledstring,'-'))) checklen--;
	
	if (checklen < MAX_VAL_LEN)
	{
	    if (state == STATE_OP)
	    {
		state = STATE_RIGHTVAL;
	    } else if (state == STATE_EQU)
	    {
		state = STATE_LEFTVAL;
	    }
	    
	    if ((vallen > 0) || (type != BTYPE_0))
	    {
		ledstring[vallen++] = type + '0';
	    }
	    ledstring[vallen] = '\0';
	    
	    refresh_led = TRUE;
	    
	} /* if (vallen < MAX_VAL_LEN) */
	break;
	
    case BTYPE_COMMA:
	if (!strchr(ledstring,comma))
	{
	    if (state == STATE_OP)
	    {
		state = STATE_RIGHTVAL;
	    } else if (state == STATE_EQU)
	    {
		state = STATE_LEFTVAL;
	    }
	    
	    ledstring[vallen++] = comma;
	    ledstring[vallen] = '\0';
	    
	    refresh_led = TRUE;
	    
	} /* if (!strchr(ledstring,comma)) */
	break;
	
    case BTYPE_CA:
	vallen = 0;
	leftval = 0.0;
	rightval = 0.0;
	operation = BTYPE_ADD;
	
	state = STATE_LEFTVAL;
	
	strcpy(ledstring,"0");
	refresh_led = TRUE;
	
	if (tapefh) fputs("\n",tapefh);
	break;
	
    case BTYPE_CE:
	vallen = 0;
	strcpy(ledstring,"0");
	refresh_led = TRUE;
	
	switch (state)
	{
	case STATE_LEFTVAL:
	    leftval = 0.0;
	    break;
	    
	case STATE_OP:
	case STATE_RIGHTVAL:
	    rightval = 0.0;
	    break;
	}
	break;
	
    case BTYPE_BS:
	if (vallen)
	{
	    ledstring[--vallen] = '\0';
	    if (vallen == 0) strcpy(ledstring,"0");				
	    refresh_led = TRUE;
	}
	break;
	
    case BTYPE_SIGN:
	switch(state)
	{
	case STATE_LEFTVAL:
	case STATE_RIGHTVAL:
	    if (ledstring[0] == '-')
	    {
		strcpy(ledstring,&ledstring[1]);
	    } else {
		strcpy(tempstring,ledstring);
		strcpy(ledstring,"-");
		strcat(ledstring,tempstring);
	    }
	    refresh_led = TRUE;
	    break;
	    
	case STATE_EQU:
	    leftval = -leftval;
	    LeftValToLED();
	    refresh_led = TRUE;
	    break;
	}
	break;
	
    case BTYPE_ADD:
    case BTYPE_SUB:
    case BTYPE_MUL:
    case BTYPE_DIV:
	switch(state)
	{
	case STATE_LEFTVAL:
	case STATE_EQU:
	    GetLeftValue();
	    rightval = leftval;
	    
	    state = STATE_OP;
	    vallen = 0;
	    strcpy(ledstring,"0");
	    
	    if (tapefh)
	    {
	        fprintf(tapefh,"\t%s\n",visledstring);
		fflush(tapefh);
	    }
	    break;

	case STATE_OP:
	    break;
	    
	case STATE_RIGHTVAL:
	    GetRightValue();
	    matherr = DoOperation();
	    state = STATE_OP;
	    vallen = 0;				
	    refresh_led = TRUE;
	    
	    if (tapefh)
	    {
	    	fprintf(tapefh,"%s\t%s\n",(operation == BTYPE_ADD) ? "+" :
				(operation == BTYPE_SUB) ? "-" :
				(operation == BTYPE_DIV) ? ":" :
				"×" ,visledstring);
		fflush(tapefh);
	    }
	    break;
	    
	} /* switch(state) */
	
	operation = type;
	break;
	
    case BTYPE_EQU:
	if (state == STATE_LEFTVAL)
	{
	    GetLeftValue();
	    if (tapefh)
	    {
	    	fprintf(tapefh,"\t%s\n",visledstring);
		fflush(tapefh);
	    }
	}	
	else if (state == STATE_RIGHTVAL)
	{
	    GetRightValue();
	    if (tapefh)
	    {
	        fprintf(tapefh,"%s\t%s\n",(operation == BTYPE_ADD) ? "+" :
				(operation == BTYPE_SUB) ? "-" :
				(operation == BTYPE_DIV) ? ":" :
				"×" ,visledstring);
		fflush(tapefh);
	    }
	}
	
	matherr = DoOperation();
	state = STATE_EQU;
	
	vallen = 0;
	
	if (!matherr)
	{
	    RefreshLED();
	    if (tapefh)
	    {
	        fprintf(tapefh,"=\t%s\n",visledstring);
		fflush(tapefh);
	    }
	} else {
	    refresh_led = TRUE;
	}
	break;
	
    } /* switch(type) */
    
    if (matherr)
    {
	leftval = rightval = 0.0;
	state = STATE_LEFTVAL;
	operation = BTYPE_ADD;
	vallen = 0;
	strcpy(ledstring,matherr);
	refresh_led = TRUE;
    }
    
    if (refresh_led) RefreshLED();
    
}

static void HandleAll(void)
{
    struct IntuiMessage *msg;
    WORD icode,i;
    ULONG signals;
    
    BOOL quitme = FALSE;
    
    if (dotape) OpenTape();

    while(!quitme)
    {
	signals = Wait(1L << win->UserPort->mp_SigBit | SIGBREAKF_CTRL_C);

	if (signals & (1L << win->UserPort->mp_SigBit))
	{
	    while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	    {
	        switch(msg->Class)
	        {
	        case IDCMP_CLOSEWINDOW:
		    quitme = TRUE;
		    break;
		
	        case IDCMP_REFRESHWINDOW:
		    GT_BeginRefresh(win);
		    GT_EndRefresh(win,TRUE);
		    break;
		
	        case IDCMP_GADGETUP:
		    HandleButton(((struct Gadget *)msg->IAddress)->GadgetID);
		    break;
		
	        case IDCMP_VANILLAKEY:
		    icode = toupper(msg->Code);
		
		    for(i = 0;i < NUM_BUTTONS;i++)
		    {
		        if ((icode == bi[i].key1) ||
			    (icode == bi[i].key2))
		        {
			    icode = bi[i].type;
			    break;
		        }
		    }
		    if (i < NUM_BUTTONS)
		    {
		        HandleButton(icode);
		    } else if (icode == 27)
		    {
		        quitme = TRUE;
		    }
		    break;
		
	        } /* switch(msg->Class) */
	    
	        ReplyMsg((struct Message *)msg);
	    } /* while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) */
	} /* if(signals & (1L << win->UserPort->mp_SigBit)) */
	if (signals & SIGBREAKF_CTRL_C)
	    quitme = TRUE;

    } /* while(!quitme) */
}

int main(void)
{
    OpenLibs();
    GetArguments();
    DoLocale();
    GetVisual();
    InitGUI();
    MakeGadgets();
    MakeWin();
    HandleAll();
    Cleanup(0);
    return 0;
}
