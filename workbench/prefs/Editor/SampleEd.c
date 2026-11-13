/***************************************************************
**** SampleEd.c: Show an sample editor in JanoPrefs.        ****
**** Free software under GNU license, started on 18/11/2000 ****
**** © T.Pierron, C.Guillaume.                              ****
***************************************************************/

/** Share the rendering routines of this module **/
#include <libraries/gadtools.h>
#include "Project.c"
#include <graphics/clip.h>
#include <graphics/layers.h>

#include <proto/layers.h>

#include "Sample.h"
#include "Jed.h"

struct gv   gui;
struct pens pen = {0,1,3,2,3,1,2,1,3,2,0,1};

struct TagItem BoxTags[] = {		/* Draws a recessed box */
	{GT_VisualInfo,0},
	{GTBB_Recessed,TRUE},
	{TAG_DONE}
};

UBYTE *SampleText[] = {LINE1, LINE2, LINE3, LINE4, LINE5};

extern struct RastPort RPT;

/*
** Used to remove a clipping region installed by clipWindow() or
** clipWindowToBorders(), disposing of the installed region and
** reinstalling the region removed. RKM source.
*/
void unclipWindow(struct Window *win)
{
	struct Region *old_region;

	/* Remove any old region by installing a NULL region,
	** then dispose of the old region if one was installed.
	*/
	if (NULL != (old_region = (void *) InstallClipRegion(win->WLayer, NULL)))
		DisposeRegion(old_region);
}

/*
** Clip a window to a specified rectangle (given by upper left and
** lower right corner.)  the removed region is returned so that it
** may be re-installed later. RKM source.
*/
struct Region *clipWindow(struct Window *win, LONG minX, LONG minY, LONG maxX, LONG maxY)
{
	struct Region    *new_region;
	struct Rectangle  my_rectangle;

	/* set up the limits for the clip */
	my_rectangle.MinX = minX;
	my_rectangle.MinY = minY;
	my_rectangle.MaxX = maxX;
	my_rectangle.MaxY = maxY;

	/* get a new region and OR in the limits. */
	if (NULL != (new_region = (void *) NewRegion()))
	{
		if (FALSE == OrRectRegion(new_region, &my_rectangle))
		{
			DisposeRegion(new_region);
			new_region = NULL;
		}
	}

	/* Install the new region, and return any existing region.
	** If the above allocation and region processing failed, then
	** new_region will be NULL and no clip region will be installed.
	*/
	return (struct Region *) InstallClipRegion(win->WLayer, new_region);
}

/*** Simple project creation ***/
PROJECT *new_project(PROJECT *ins, PREFS *prefs)
{
	PROJECT *new;
	if(( new = (void *) AllocVec(sizeof(*new), MEMF_PUBLIC | MEMF_CLEAR) ))
	{
		if(ins) ins->next = new, new->prev = ins;

		/* NbProject & first herited from project.c */
		NbProject++;
		if(NbProject == 1) first = new;
	}
	return new;
}

/*** Init size & pos of sample, depending fonts sizes ***/
void init_sample(struct Window *wnd, PREFS *p, WORD top)
{
	struct DrawInfo *di;
	PROJECT *new;

	gui.left   = 11;
	gui.oldtop = top+1;
	gui.right  = wnd->Width+(EXTEND_RIG-10);
	gui.bottom = top+SAMPLE_HEI;
	gui.xsize  = p->txtfont->tf_XSize;
	BoxTags[0].ti_Data = (IPTR) Vi;

	/* Find out drawing information */
	if(( di = (void *) GetScreenDrawInfo(Scr) ))
	{
		WORD *offset = &p->pen.bg, *dst;

		/* Get a copy of the correct pens for the screen */
		for(dst=(UWORD *)&pen; (char *)dst < (char *)&pen+sizeof(pen); offset++)
			*dst++ = (*offset <= 0 ? -(*offset)-1 : di->dri_Pens[ *offset ]);
/*			printf("pen %d = %d\n",dst-(WORD *)&pen, dst[-1]); */

		FreeScreenDrawInfo(Scr,di);
	}

	if(first == NULL)
	   if((new = new_project(NULL, &prefs))) {
	   	new->name = "sorrow";
	   	new->labsize = 6;
		   if((new = new_project(new,  &prefs))) {
		   	new->name = "JEd.c";
		   	new->labsize = 5;
		   	if((new = new_project(new,  &prefs)))
					new->name = "No title",
					new->labsize = 8;
			}
		}
}

/*** (Re)draw the sample editor ***/
void render_sample( struct Window *wnd, UBYTE what )
{
	struct RastPort *RP = wnd->RPort;

	/* Draw a recessed bevel box arround this sample */
	DrawBevelBoxA(RP,gui.left-1,gui.oldtop-1,gui.right-gui.left+(3-EXTEND_RIG),SAMPLE_HEI+2,BoxTags);
	clipWindow(wnd,gui.left,gui.oldtop,gui.right-EXTEND_RIG,gui.bottom);

	/* If font size changed */
	gui.ysize   = prefs.txtfont->tf_YSize;
	gui.topcurs = prefs.txtfont->tf_Baseline + (
	gui.top     = gui.oldtop+prefs.scrfont->tf_YSize+6);

	if( what & EDIT_GUI )
		/* Show factice project bar */
		SetFont(&RPT,prefs.scrfont), reshape_panel(first->next);

	if( what & EDIT_AREA )
	{
		/* Show a sample selected text in the fake editor */
		SetFont(RP,prefs.txtfont);
		SetAPen(RP,pen.bg); Move(RP,gui.left,gui.top-1); Draw(RP,gui.right,gui.top-1);
		SetABPenDrMd(RP,pen.fg,pen.bg,JAM2);
		Move(RP,gui.left,gui.topcurs);
		Text(RP,SampleText[0],15);
		SetABPenDrMd(RP,pen.fgfill,pen.bgfill,JAM2);
		Text(RP,SampleText[0]+15,44);
		Move(RP,gui.left,RP->cp_y+gui.ysize); Text(RP,SampleText[1],59);
		Move(RP,gui.left,RP->cp_y+gui.ysize); Text(RP,SampleText[2],20);

		SetABPenDrMd(RP,pen.fg,pen.bg,JAM2);
		Text(RP,SampleText[2]+20,39);
		Move(RP,gui.left,RP->cp_y+gui.ysize); Text(RP,SampleText[3],59);
		Move(RP,gui.left,RP->cp_y+gui.ysize); Text(RP,SampleText[4],59);
	}
	unclipWindow(wnd);
}

/*** Free allocated things here ***/
void free_sample()
{
	PROJECT *next;
	for(; first; next=first->next,FreeVec(first),first=next);
}
