/**************************************************************
**** gui.h: datatypes of main interface.                   ****
**** Free software under GNU license, written in 3/1/2000  ****
**** © T.Pierron, C.Guillaume.                             ****
**************************************************************/

#ifndef	GUI_H
#define	GUI_H

#ifdef	INTUITION_INTUITION_H      /* Reduce includes required */
struct Scroll
{
	struct Gadget   scroller;        /* proportionnal gadget */
	struct Gadget   down;            /* down gadget */
	struct Gadget   up;              /* up gadget */
	struct PropInfo pinfo;           /* PropInfo for scroller */
	struct Image    simage;          /* image for scroller */
	struct Image *  upimage;         /* Boopsi image for up arrow */
	struct Image *  downimage;       /* ditto for down arrow */
};

/** This very looooong names are quite boring to write... **/
#define	GetSI( gad )         ((struct StringInfo *)gad->SpecialInfo)

#endif

/*** Global variables common to all projects ***/
struct gv
{
	UWORD left,    top;              /* Upper left corner of edit area */
	UWORD right,   bottom;           /* Bottom right corner */
	UWORD nbline,  nbcol;            /* Nb. of visible lines/columns */
	UWORD topcurs, botcurs;          /* Max positions before to scroll */
	UBYTE xsize,   ysize, basel;     /* Font size */
	UWORD rcurs,   xstep;            /* Right-most window pos of cursor */
	UBYTE txtmask, selmask;          /* Mask to optimize scrolling */
	UBYTE depthwid;                  /* Depth-arrange image width */
	UWORD oldtop;                    /* Top change if project bar is added */
	UWORD xinfo,   yinfo;            /* Position of line/col [INS|OVR] marker */
};

/*** Pens number to render editor ***/
struct pens
{
	WORD bg,    fg;                  /* Foreground & background normal text pens */
	WORD bgfill,fgfill;              /* Dito with selected text pens */
	WORD bgbar, fgbar;               /* Left margin background & foreground */
	WORD shine, shade;               /* Shine & shadow pens for project bar */
	WORD bgpan, fgpan;               /* Foreground & background panel pens */
	WORD panel, abpan;               /* Project bar glyph & activ bg panel */
};

extern struct Screen *  Scr;
extern APTR             Vi;
extern struct Window *  Wnd;
extern struct Menu *    Menu;
extern struct Scroll *  Prop;
extern struct RastPort *RP,RPT;
extern struct gv        gui;
extern struct pens      pen;

long            setup       ( void );
void            load_pens   ( void );
void            free_prop   ( struct Scroll * );
void            adjust_win  ( struct Window *, BYTE PrjBar );
struct Scroll * add_prop    ( struct Window * );
CONST_STRPTR    GetMenuText ( ULONG );
void            prop_adj    ( Project );
void            clear_brcorner( void );
void            CloseMainWnd( BOOL );
void            recalc_sigbits(void);

/*** Easy access to commonly used variables: ***/
#define	YSIZE					(gui.ysize)
#define	XSIZE					(gui.xsize)
#define	BASEL					(gui.basel)

#endif
