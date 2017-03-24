/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <utility/hooks.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include <clib/alib_protos.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/****************************************************************************************/

#define SIGF_READY   SIGBREAKF_CTRL_C

#define SIGF_INIT    SIGBREAKF_CTRL_C
#define SIGF_STOP    SIGBREAKF_CTRL_D
#define SIGF_HELLO   SIGBREAKF_CTRL_E

/****************************************************************************************/

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

static struct Screen *scr;
static struct Window *win;
static struct Task   *task1, *task2, *maintask;

static LONG counter;

static char s[255];

/****************************************************************************************/

static void Cleanup(char *msg)
{
    WORD rc;

    if (msg)
    {
	printf("taskswitchbench: %s\n",msg);
	rc = RETURN_WARN;
    } else {
	rc = RETURN_OK;
    }

    Forbid();
    if (task1) DeleteTask(task1);
    if (task2) DeleteTask(task2);
    Permit();
    
    if (win) CloseWindow(win);
    if (scr) UnlockPubScreen(0,scr);

    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);

    exit(rc);
}

/****************************************************************************************/

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

}

/****************************************************************************************/

static void GetVisual(void)
{
    if (!(scr = LockPubScreen(0)))
    {
	Cleanup("Can't lock pub screen!");
    }
}

/****************************************************************************************/

static void MakeWin(void)
{	
    if (!(win = OpenWindowTags(0,WA_PubScreen,(IPTR)scr,
				 WA_Left,10,
				 WA_Top,10,
				 WA_Width,600,
				 WA_Height,scr->WBorTop + scr->Font->ta_YSize + 1 +
				 	   scr->WBorBottom +
					   scr->Font->ta_YSize * 2,
				 WA_Title,(IPTR)"Press RETURN key to start and any key to stop!",
				 WA_SimpleRefresh,TRUE,
				 WA_CloseGadget,TRUE,
				 WA_DepthGadget,TRUE,
				 WA_DragBar,TRUE,
				 WA_Activate,TRUE,
				 WA_IDCMP,IDCMP_CLOSEWINDOW |
				 	  IDCMP_VANILLAKEY |
					  IDCMP_REFRESHWINDOW,
				 TAG_DONE)))
    {
	Cleanup("Can't open window!");
    }

    ScreenToFront(win->WScreen);

}

/****************************************************************************************/

#define OTHERTASK task2

static void Task1(void)
{
    for(;;)
    {
        ULONG sigs;
	BOOL stop = FALSE;
	
	SetSignal(0, SIGF_HELLO);
        Wait(SIGF_INIT);
	Signal(maintask, SIGF_READY);
	
	while(!stop)
	{
	    sigs = Wait(SIGF_STOP | SIGF_HELLO);
	    if (sigs & SIGF_STOP) stop = TRUE;
	    if (sigs & SIGF_HELLO) Signal(OTHERTASK, SIGF_HELLO);
	}
    }
}

/****************************************************************************************/

#undef OTHERTASK
#define OTHERTASK task1

static void Task2(void)
{
    for(;;)
    {
        ULONG sigs;
	BOOL stop = FALSE;

	SetSignal(0, SIGF_HELLO);	
        Wait(SIGF_INIT);
	Signal(maintask, SIGF_READY);
	
	while(!stop)
	{
	    sigs = Wait(SIGF_STOP | SIGF_HELLO);
	    if (sigs & SIGF_STOP) stop = TRUE;
	    if (sigs & SIGF_HELLO)
	    {
	        Signal(OTHERTASK, SIGF_HELLO);
		counter++;
	    }
	}
    }
}

/****************************************************************************************/

static void MakeTasks(void)
{
    maintask = FindTask(NULL);
    
    task1 = CreateTask("Task 1", 0, Task1, AROS_STACKSIZE);
    task2 = CreateTask("Task 2", 0, Task2, AROS_STACKSIZE);
    
    if (!task1 || !task2) Cleanup("Can't create tasks!");
   
}

/****************************************************************************************/

static void Action(void)
{
    struct RastPort *rp = win->RPort;
    char *msg = "Starting to benchmark when countdown reaches 0. Countdown: 5";
    WORD i;
    
    WORD x = win->BorderLeft + 8;
    WORD y = win->BorderTop + rp->TxHeight / 2;
    WORD len = strlen(msg);
    WORD pixellen = TextLength(rp, msg, len);

    Signal(task1, SIGF_INIT);
    Wait(SIGF_READY);
    
    Signal(task2, SIGF_INIT);
    Wait(SIGF_READY);
    
    ModifyIDCMP(win, IDCMP_VANILLAKEY);
    
    SetAPen(rp, 0);
    RectFill(rp, x, y, win->Width - win->BorderRight - 1, y + rp->TxHeight - 1);
    
    SetAPen(rp, 1);
    Move(rp, x, y + rp->TxBaseline);
    Text(rp, msg, len);
    
    x += pixellen - rp->TxWidth;
    
    for(i = 5; i>= 0; i--)
    {
        char c = '0' + i;
 
        Delay(25); /* should be 50 (1 sec), but AROS clock is somewhat slow */
	
	Move(rp, x, y + rp->TxBaseline);
	Text(rp, &c, 1);
    }
    
    /* start everything by sending hello to first task */
    
    Signal(task1, SIGF_HELLO);

    WaitPort(win->UserPort);
    ReplyMsg(GetMsg(win->UserPort));
    
    /* stop */
    Signal(task1, SIGF_STOP);
    Signal(task2, SIGF_STOP);
    
    ModifyIDCMP(win, IDCMP_VANILLAKEY | IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW);
    
    x -= (pixellen - rp->TxWidth);
    
    SetAPen(rp, 0);
    RectFill(rp, x, y, win->Width - win->BorderRight - 1, y + rp->TxHeight - 1);

    SetAPen(rp, 1);
    
    sprintf(s, "Benchmark result: %ld",counter);

    Move(rp, x, y + rp->TxBaseline);
    Text(rp, s, strlen(s));
    
    counter = 0;
}

/****************************************************************************************/

static void HandleAll(void)
{
    struct IntuiMessage *msg;

    BOOL quitme = FALSE;

    while(!quitme)
    {
	WaitPort(win->UserPort);
	while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
	    switch(msg->Class)
	    {
		case IDCMP_CLOSEWINDOW:
		    quitme = TRUE;
		    break;

		case IDCMP_REFRESHWINDOW:
		    BeginRefresh(win);
		    EndRefresh(win,TRUE);
		    break;
			
		case IDCMP_VANILLAKEY:
		    if (msg->Code == 13) Action();
		    break;
		
	    }
	    ReplyMsg((struct Message *)msg);
	}
    }
}

/****************************************************************************************/

int main(void)
{
    OpenLibs();
    GetVisual();
    MakeWin();
    MakeTasks();
    HandleAll();
    Cleanup(0);
    return 0;
}

/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
