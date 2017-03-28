/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/semaphores.h>
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

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

static struct Screen *scr;
static struct Window *win;
static struct Task   *task1, *task2, *maintask;

static struct SignalSemaphore sem;

/****************************************************************************************/

static void Cleanup(char *msg)
{
    WORD rc;

    if (msg)
    {
	printf("semtorture: %s\n",msg);
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

static void Init(void)
{
    InitSemaphore(&sem);
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
				 WA_Width,300,
				 WA_Height,scr->WBorTop + scr->Font->ta_YSize + 1,
				 WA_Title,(IPTR)"Press RETURN key to start!",
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

static void Task1(void)
{
    int a = 1, b = 2, c = 3;
    
    for(;;)
    {
	ObtainSemaphore(&sem);
	a += b + c;
	b += a + c;
	c += a + b;
	ReleaseSemaphore(&sem);
    }
}

/****************************************************************************************/

static void Task2(void)
{
    int a = 1, b = 2;
    
    for(;;)
    {
    	ObtainSemaphore(&sem);
	a -= b;
	b += a;
	ReleaseSemaphore(&sem);
    }
}

/****************************************************************************************/

static void MakeTasks(void)
{
    maintask = FindTask(NULL);
    
    task1 = CreateTask("Task 1", 0, Task1, 40000);
    task2 = CreateTask("Task 2", 0, Task2, 40000);
    
    if (!task1 || !task2) Cleanup("Can't create tasks!");
   
}

/****************************************************************************************/

static void Action(void)
{
    SetWindowTitles(win, "Test started! Cannot be aborted!", (STRPTR)~0);
    Delay(50);
    MakeTasks();
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
    Init();
    OpenLibs();
    GetVisual();
    MakeWin();
    HandleAll();
    Cleanup(0);
    return 0;
}

/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
/****************************************************************************************/
