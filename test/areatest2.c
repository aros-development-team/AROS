#include <intuition/intuition.h>
#include <graphics/gfxmacros.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <aros/debug.h>

#define MAX_POINTS 50

#define MODE_ADDPOINTS 1
#define MODE_MOVEPOINTS 2

#define MSGMOUSEX ( ((msg->MouseX - win->BorderLeft) < 0) ? 0 : \
    	    	    ((msg->MouseX - win->BorderLeft) >= win->GZZWidth) ? win->GZZWidth - 1 : \
		    (msg->MouseX - win->BorderLeft) )

#define MSGMOUSEY ( ((msg->MouseY - win->BorderTop) < 0) ? 0 : \
    	    	    ((msg->MouseY - win->BorderTop) >= win->GZZHeight) ? win->GZZHeight - 1 : \
		    (msg->MouseY - win->BorderTop) )

static struct Window *win;
static struct RastPort *winrp, *drawrp;
static struct BitMap *drawbm;
static struct AreaInfo ai;
static struct TmpRas tr;
static void *trbuf;
static UBYTE aibuf[(MAX_POINTS + 1) * 5];
static WORD mode, actpoint, hipoint, numpoints;
static WORD points[MAX_POINTS][2];
static char wintitle[256];

static void cleanup(char *msg)
{
    if (msg) printf("areatest2: %s\n", msg);
    
    if (drawbm) FreeBitMap(drawbm);
    if (drawrp) FreeRastPort(drawrp);
    
    if (trbuf)
    {
    	FreeRaster(trbuf, win->GZZWidth, win->GZZHeight);
    }
    
    if (win) CloseWindow(win);
    exit(0);    
}

static void updatetitle(void)
{
    char *title = "AreaTest2";
    
    switch(mode)
    {
    	case MODE_ADDPOINTS:
	    snprintf(wintitle,
	    	     sizeof(wintitle),
		     "Create points [%2d/%2d]. Press RETURN when done.",
		     actpoint + 1, MAX_POINTS);
	    title = wintitle;
	    break;

    	case MODE_MOVEPOINTS:
	    snprintf(wintitle,
	    	     sizeof(wintitle),
		     "Move points. Press RETURN to make new shape.");
	    title = wintitle;
	    break;
	    
    }
    
    SetWindowTitles(win, title, (char *)~0);
    
}

static void makewin(void)
{
    win = OpenWindowTags(0, WA_Title, (IPTR)"AreaTest2",
    	    	    	    WA_InnerWidth, 320,
			    WA_InnerHeight, 256,
			    WA_GimmeZeroZero, TRUE,
			    WA_CloseGadget, TRUE,
			    WA_DepthGadget, TRUE,
    	    	    	    WA_DragBar, TRUE,
			    WA_IDCMP, IDCMP_MOUSEBUTTONS |
			    	      IDCMP_MOUSEMOVE |
				      IDCMP_CLOSEWINDOW |
				      IDCMP_VANILLAKEY |
				      IDCMP_RAWKEY,
			    WA_Activate, TRUE,
			    WA_ReportMouse, TRUE,
			    TAG_DONE);
    if (!win) cleanup("Can't open window!");
    
    winrp = win->RPort;
     
    InitArea(&ai, aibuf, sizeof(aibuf) / 5);
    trbuf = AllocRaster(win->GZZWidth, win->GZZHeight);
    InitTmpRas(&tr, trbuf, RASSIZE(win->GZZWidth, win->GZZHeight));
    
    drawbm = AllocBitMap(win->GZZWidth, win->GZZHeight, 0, BMF_MINPLANES, win->RPort->BitMap);
    if (!drawbm) cleanup("Can't allocate draw bitmap!");
    
    drawrp = CreateRastPort();
    if (!drawrp) cleanup("Can't allocate draw rastport!");
    drawrp->BitMap = drawbm;
    
    drawrp->AreaInfo = &ai;
    drawrp->TmpRas = &tr;
    
    if (!trbuf) cleanup("TmpRas buffer allocation failed!");
}

static void hilightpoint(WORD point)
{
    WORD x = points[point][0];
    WORD y = points[point][1];
    
    //kprintf("hilightpoint %d,%d\n", x, y);
    
    SetABPenDrMd(winrp, 2, 0, COMPLEMENT);
    Move(winrp, x - 3, y - 3);
    Draw(winrp, x + 3, y - 3),
    Draw(winrp, x + 3, y + 3);
    Draw(winrp, x - 3, y + 3),
    Draw(winrp, x - 3, y - 3);
}

static void clear(struct RastPort *rp)
{
    SetABPenDrMd(rp, 2, 0, JAM1);
    RectFill(rp, 0, 0, win->GZZWidth - 1, win->GZZHeight - 1);
}

static void paint(void)
{
    int i;
    
    if (numpoints == 0) return;
    
    switch(mode)
    {
    	case MODE_ADDPOINTS:
	    SetABPenDrMd(winrp, 1, 0, JAM1);
	    Move(winrp, points[0][0], points[0][1]);
	    PolyDraw(winrp, numpoints, (WORD *)points);	    
	    break;
	    
	case MODE_MOVEPOINTS:
	    clear(drawrp);
	    SetABPenDrMd(drawrp, 1, 0, JAM1);
	    AreaMove(drawrp, points[0][0], points[0][1]);
	    for(i = 1; i < numpoints; i++)
	    {
	    	AreaDraw(drawrp, points[i][0], points[i][1]);
	    }
	    AreaEnd(drawrp);
	    BltBitMapRastPort(drawbm, 0, 0, winrp, 0, 0, win->GZZWidth, win->GZZHeight, 192);
	    break;
    }
}

static WORD pointundermouse(LONG x, LONG y)
{
    LONG i, dist;
    LONG best_i = 0, best_dist = 0x7fffffff;

    for(i = 0; i < numpoints; i++)
    {
	dist = (points[i][0] - x) * (points[i][0] - x) +
	       (points[i][1] - y) * (points[i][1] - y);

	if (dist < best_dist)
	{
	    best_dist = dist;
	    best_i = i;
	}
    }

    return (best_dist < 200) ? best_i : -1;
}

static void handleall(void)
{
    struct IntuiMessage *msg;
    BOOL    	    	 quitme = FALSE, lmbdown = FALSE;
    
    mode = MODE_ADDPOINTS;
    updatetitle();
    clear(winrp);
    
    while(!quitme)
    {
    	WaitPort(win->UserPort);
    	while((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
	    switch(msg->Class)
	    {
	    	case IDCMP_CLOSEWINDOW:
		    quitme = TRUE;
		    break;
		    
		case IDCMP_MOUSEBUTTONS:
		    if (msg->Code == SELECTDOWN)
		    {
		    	lmbdown = TRUE;
			
			switch(mode)
			{
		    	    case MODE_ADDPOINTS:
				points[actpoint][0] = MSGMOUSEX;
				points[actpoint][1] = MSGMOUSEY;
				actpoint++;
				numpoints++;
				if (numpoints == MAX_POINTS)
				{
			    	    mode = MODE_MOVEPOINTS;
				    actpoint = -1;
				    hipoint = -1;
				}
				paint();
				updatetitle();
				break;
				
			    case MODE_MOVEPOINTS:
			    	actpoint = pointundermouse(MSGMOUSEX, MSGMOUSEY);
			    	break;
				
			}
		    }
		    else if (msg->Code == SELECTUP)
		    {
		    	lmbdown = FALSE;
			
			switch(mode)
			{
			    case MODE_MOVEPOINTS:
			    	actpoint = -1;
				hipoint = -1;
				break;
			}
		    }
		    
		    break;
		    
		case IDCMP_MOUSEMOVE:
		    switch(mode)
		    {
		    	case MODE_MOVEPOINTS:
			    if ((actpoint >= 0) && lmbdown)
			    {
			    	points[actpoint][0] = MSGMOUSEX;
				points[actpoint][1] = MSGMOUSEY;
				paint();
			    }
			    else if (!lmbdown)
			    {
			    	WORD new_hipoint = pointundermouse(MSGMOUSEX, MSGMOUSEY);
				
				if (new_hipoint != hipoint)
				{
				    if (hipoint >= 0) hilightpoint(hipoint);
				    hipoint = new_hipoint;
				    if (hipoint >= 0) hilightpoint(hipoint);
				}
			    }
			    
			    break;
		    }
		    break;
		    
		case IDCMP_VANILLAKEY:
		    switch(msg->Code)
		    {
		    	case 27:
			    quitme = TRUE;
			    break;
			    
		    	case 13:
			    switch(mode)
			    {
			    	case MODE_ADDPOINTS:
				    if (numpoints >= 2)
				    {
			    	    	mode = MODE_MOVEPOINTS;
				    	actpoint = -1;
					hipoint = -1;
					paint();
					updatetitle();
				    }
				    break;
				    
				case MODE_MOVEPOINTS:
				    actpoint = numpoints = 0;
				    mode = MODE_ADDPOINTS;
				    clear(winrp);
				    updatetitle();
				    break;
			    }
			    break;
		    }
		    break;
		    
		case IDCMP_RAWKEY:
		    switch(mode)
		    {
		    	case MODE_MOVEPOINTS:
			    if (lmbdown && actpoint >= 0)
			    {
			    	BOOL changed = FALSE;
				
			    	switch(msg->Code)
				{
				    case CURSORLEFT:
				    	if (points[actpoint][0] > 0)
					{
					    points[actpoint][0]--;
					    changed = TRUE;
					}
					break;
					
				    case CURSORRIGHT:
				    	if (points[actpoint][0] < win->GZZWidth - 1)
					{
					    points[actpoint][0]++;
					    changed = TRUE;
					}
				    	break;
					
				    case CURSORUP:
				    	if (points[actpoint][1] > 0)
					{
					    points[actpoint][1]--;
					    changed = TRUE;
					}
					break;
					
				    case CURSORDOWN:
				    	if (points[actpoint][1] < win->GZZHeight - 1)
					{
					    points[actpoint][1]++;
					    changed = TRUE;
					}
				    	break;
				    	
				} /* switch(msg->Code) */
				
				if (changed) paint();
				
			    } /* if (!lmbdown && hipoint >= 0) */
			    
		    } /* switch(mode) */
		    break;
		    
	    } /* switch(msg->Class) */
	    ReplyMsg((struct Message *)msg);
	    
	} /* while((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) */
	
    } /*while(!quitme) */
    
}

int main(void)
{
    makewin();
    handleall();
    cleanup(0);    
    return 0;
}
