/*
*	A Simple program to display colorful lines on a custom screen.
*	Compiles with no errors under Lattice 3.03, However, it passes ints to
*	some subroutines and does structure assignments, so manx users will
*	probably have to do a little work.
*						Paul Jatkowski
*						{ihnp4!}cuuxb!pej
*/
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <exec/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <intuition/intuition.h>
#include <graphics/gfxmacros.h>
#include <graphics/view.h>
#include <intuition/iobsolete.h>
struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

struct Pt {
	int x;
	int y;
};
typedef struct Pt Point_New;

#define Q 5
#define N (Q*12)	/* N is the number of lines displayed on the screen */
			/* change the value of Q to change the number of    */
			/* lines so that it is evenly divisible by the      */
			/* number of color registers used		    */

void newdelta(Point_New *p,Point_New *dp);

int	curcolor;
Point_New	from[N], to[N];
Point_New	dfrom, dto;
long	i,j;
int	minx,maxx,miny,maxy;

extern int errno;
char errmsg[50];

#define TOPAZ_SIXTY 0

struct TextAttr MyFont = {
   "topaz.font",           /* Font Name */
   TOPAZ_SIXTY,            /* Font Height */
   FS_NORMAL,              /* Style */
   FPF_ROMFONT             /* Preferences */
};


struct NewScreen NewScreen = {
   0,             /* left edge */
   0,             /* top edge */
   640,           /* Width (high res) */
   400,           /* Height (interlace) */
   4,             /* Depth */
   0,1,           /* Detail and Block Pen Specification */
   HIRES|LACE,	  /* Hires Interlaced screen */
   CUSTOMSCREEN,  /* The Screen Type */
   &MyFont,       /* our font */
   "My Own Screen", /* title */
   NULL,          /* no special gadgets */
   NULL           /* no special custom BitMap */
};


struct   Window   *Window;
struct   Screen   *Screen;
struct   RastPort  *RP;

int getdelta()
{
	register int x;

	x = (8 - (rand() % 16));
  return x;
}

void die(char *s)
{
		
	SetWindowTitles(Window,"Fatal Error",s);
	Wait(1<< Window->UserPort->mp_SigBit);
	if (Window) CloseWindow(Window);
	if (Screen) CloseScreen(Screen);
	exit(0);
}

int range_rand(int minv, int maxv)
{
	register int i1;
	
	i1 = minv + (rand() % (maxv - minv));
  return i1;
}

void init()
{
	ULONG	seconds,micros;
	
	miny = Window->BorderTop;
	maxy = Window->Height - Window->BorderBottom;
	minx = Window->BorderLeft;
	maxx = Window->Width - Window->BorderRight;

	for (j=0 ; j <N ; j++)
	{
		from[j].x = 0; from[j].y = 0;
		to[j].x = 0; to[j].y = 0;
	}
	/* attempt to ramdomize the random number generator */
	/*CurrentTime(&seconds,&micros);*/
	srand(micros);
	
	from[0].x = range_rand(minx,maxx);
	from[0].y = range_rand(miny,maxy);
	from[1] = from[0];
	
	to[0].x = range_rand(minx,maxx);
	to[0].y = range_rand(miny,maxy);
	to[1] = to[0];

	newdelta(&from[0],&dfrom);
	newdelta(&to[0],&dto);	
	i = 0;
	curcolor = 4;
	
	/* clear the screen */
	SetAPen(RP,0);
	SetOutlinePen(RP,0);
	RectFill(RP,minx,miny,maxx,maxy);
}


void mv_point(Point_New *p,Point_New *dp)
{
	if ((p->x += dp->x) > maxx || p->x < minx)
	{
		dp->x = -dp->x;
		p->x += dp->x;
	}
	if ((p->y += dp->y) > maxy || p->y < miny)
	{
		dp->y = -dp->y;
		p->y += dp->y;
	}
}

void newdelta(Point_New *p,Point_New *dp)
{
	for (dp->x = getdelta() ;
			((p->x + dp->x) > maxx) || ((p->x + dp->x) < minx) ;
			dp->x = getdelta())
		;
	for (dp->y = getdelta() ;
			((p->y + dp->y) > maxy) || ((p->y + dp->y) < miny) ;
			dp->y = getdelta() )
		;
}



int main(void)
{

	struct   NewWindow NewWindow;
	struct   ViewPort  *VP;
	int	notdone = 1;
	struct	IntuiMessage *msg;
	int	my_rgbi,my_rgb[3];
	int	inc,colorok;
	int	lc = -1;

	/* open intuition library */
	IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",0);
	if (IntuitionBase == NULL)
		exit(FALSE);
	GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",0);
	if (GfxBase == NULL)
		exit (FALSE);

	if ((Screen = (struct Screen *)OpenScreen(&NewScreen)) == NULL)
		exit(FALSE);
	
	/* set up new window structure */
	NewWindow.LeftEdge = 0;
	NewWindow.TopEdge = 0;
	NewWindow.Width = 640;
	NewWindow.Height = 400;
	NewWindow.DetailPen = 0;
	NewWindow.BlockPen = 1;
	NewWindow.Title = "A Simple Window";
	NewWindow.Flags = WINDOWCLOSE | SMART_REFRESH |
				WINDOWDRAG | WINDOWDEPTH | WINDOWSIZING;
	NewWindow.IDCMPFlags = CLOSEWINDOW | NEWSIZE ;
	NewWindow.Type = CUSTOMSCREEN;
	NewWindow.FirstGadget = NULL;
	NewWindow.CheckMark = NULL;
	NewWindow.Screen = Screen;
	NewWindow.BitMap = NULL;
	NewWindow.MinWidth = 100;
	NewWindow.MinHeight = 25;
	NewWindow.MaxWidth = 640;
	NewWindow.MaxHeight = 200;

	/* try to open the window */
	if ((Window = (struct Window *)OpenWindow(&NewWindow)) == NULL)
		exit(FALSE);
   
	RP = Window->RPort;
	VP = (struct ViewPort *) ViewPortAddress(Window);
   

	SetAPen(RP,14);
	SetBPen(RP,0);
	SetWindowTitles(Window,"Colorfull lines","");
	SetDrMd(RP,JAM1);
	SetAPen(RP,15);

	init();
	SetRGB4(VP,0,0,0,0);     
	SetRGB4(VP,1,1,3,4);     
	SetRGB4(VP,2,5,0,8);  
	SetRGB4(VP,3,0,8,5);     

	my_rgb[0] = rand() % 16;
	my_rgb[1] = rand() % 16;
	my_rgb[2] = rand() % 16;
	SetRGB4(VP,curcolor,my_rgb[0],my_rgb[1],my_rgb[2]);
	SetDrMd(RP,JAM1);			
	while(notdone)
	{
		j = i;
		if (++i >= N)
			i = 0;

		SetAPen(RP,0);			/* erase old line */
		Move(RP,from[i].x,from[i].y);
		Draw(RP,to[i].x,to[i].y);

		from[i] = from[j];		/* structure assignment */
		mv_point(&from[i], &dfrom);
		to[i] = to[j];			/* structure assignment */
		mv_point(&to[i], &dto);

		SetAPen(RP,curcolor);		/* draw a new line */
		Move(RP,from[i].x,from[i].y);
		Draw(RP,to[i].x,to[i].y);
		
		if ( (i % Q) == 0)
		{
			if (++curcolor > 15)
				curcolor = 4;
			colorok = 0;
			while (!colorok)
			{
				inc = 1;
				if (rand() & 8 )
					inc = -1;
				my_rgbi = rand() % 3;
				inc += my_rgb[my_rgbi];
				/* make sure that the color register
				   doesn't wrap and we don't change the same
				   color twice in a row
				*/
				if (inc <= 15 && inc >= 0 && lc != my_rgbi)
				{
					my_rgb[my_rgbi] = inc;
					colorok++;
					lc = my_rgbi;
				}
			}
			SetRGB4(VP,curcolor,my_rgb[0],my_rgb[1],my_rgb[2]);
		}
		if ( (rand() % 20) == 1)
		{
			if (rand() & 2)
			{
				newdelta(&to[i],&dto);
			}
			else
			{
				newdelta(&from[i],&dfrom);
			}
		}

		while ((msg = (struct IntuiMessage *)GetMsg(Window->UserPort)) != 0)
		{
			switch(msg->Class)
			{

			case CLOSEWINDOW:	/* that's all folks */
				notdone = 0;
				ReplyMsg((struct Message *)msg);
				continue;
			case NEWSIZE:
				ReplyMsg((struct Message *)msg);			
				init();
				break;			
			default:
				ReplyMsg((struct Message *)msg);
			}
		}
	}
	

	/* close the window and  exit */
	CloseWindow(Window);
	CloseScreen(Screen);
	CloseLibrary((struct Library *)IntuitionBase);
	CloseLibrary((struct Library *)GfxBase);
	return(TRUE);
}
