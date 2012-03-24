/* $XConsortium: xev.c,v 1.15 94/04/17 20:45:20 keith Exp $ */
/*

Copyright (c) 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/

/*
 * Author:  Jim Fulton, MIT X Consortium
 */

/***************************************************************************/

#include <stdio.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define OUTER_WINDOW_MIN_WIDTH 100			
#define OUTER_WINDOW_MIN_HEIGHT 100
#define OUTER_WINDOW_DEF_WIDTH 800
#define OUTER_WINDOW_DEF_HEIGHT 100
#define OUTER_WINDOW_DEF_X 100
#define OUTER_WINDOW_DEF_Y 100
				
/***************************************************************************/

static void
set_sizehints (XSizeHints *hintp, int min_width, int min_height,
               int defwidth, int defheight, int defx, int defy, char *geom);

typedef unsigned long Pixel;

static char *ProgramName;
static Display *dpy;
static int screen;
static FILE *fh;
static Window w;
static GC gc;

static unsigned char table[256];

/***************************************************************************/

static struct _asktable
{
    char *title;
    int rawkeystart;
    int numkeys;
    KeySym nokey_ks;
}
asktable [] =
{
    {"1st row: Press ESCAPE Key", 0x45, 1, XK_VoidSymbol},
    {"1st row: Press F1 .. F10", 0x50, 10, XK_VoidSymbol},
    {"1st row: Press F11 (or ESCAPE if key doesn't exist)", 0x4B, 1, XK_Escape},
    {"1st row: Press F12 (or ESCAPE if key doesn't exist)", 0x6F, 1, XK_Escape},

    {"2nd row: Press the key left of 1, or ESCAPE if there's no key left of 1", 0x00, 1, XK_Escape},
    {"2nd row: Press 1 .. 0", 0x1, 10, XK_VoidSymbol},
    {"2nd row: Press the three normal (!!) keys right of 0. Press ESCAPE instead of third key if you only have two keys right of 0", 0X0B, 3, XK_Escape},
    {"2nd row: Press BACKSPACE", 0x41, 1, XK_VoidSymbol},

    {"3rd row: Press TAB", 0x42, 1, XK_VoidSymbol},
    {"3rd row: Press the twelve keys right of TAB (= upto and excl. RETURN)", 0x10, 12, XK_VoidSymbol},
    {"3rd row: Press RETURN", 0x44, 1, XK_VoidSymbol},

    {"4th row: Press the (left) CONTROL key which might also be in 6th row", 0x63, 1, XK_VoidSymbol},
    {"4th row: Press CAPS LOCK", 0x62, 1, XK_VoidSymbol},
    {"4th row: Press the twelve keys right of CAPS LOCK. Press ESCAPE for last if you only have eleven keys there", 0x20, 12, XK_Escape},

    {"5th row: Press LEFT SHIFT", 0x60, 1, XK_VoidSymbol},
    {"5th row: If in this row you have eleven normal keys, press the key right of LEFT SHIFT, otherwise ESCAPE", 0x30, 1, XK_Escape},
    {"5th row: Press the next ten keys in this row (= upto and excl. RIGHT SHIFT)", 0x31, 10, XK_VoidSymbol},
    {"5th row: Press RIGHT SHIFT", 0x61, 1, XK_VoidSymbol},

    {"6th row: Press left ALT", 0x64, 1, XK_VoidSymbol},
    {"6th row: Press left AMIGA", 0x66, 1, XK_VoidSymbol},
    {"6th row: Press SPACE", 0x40, 1, XK_VoidSymbol},
    {"6th row: Press right AMIGA", 0x67, 1, XK_VoidSymbol},
    {"6th row: Press right ALT", 0x65, 1, XK_VoidSymbol},
    {"6th row: Press right CONTROL (or ESCAPE if key doesn't exist)", 0x63, 1, XK_VoidSymbol},

    {"Above cursor keys: Press INSERT (or ESCAPE if key doesn't exist)", 0x47, 1, XK_Escape},
    {"Above cursor keys: Press HOME (or ESCAPE if key doesn't exist)", 0x70, 1, XK_Escape},
    {"Above cursor keys: Press PAGE UP (or ESCAPE if key doesn't exist)", 0x48, 1, XK_Escape},
    {"Above cursor keys: Press DELETE", 0x46, 1, XK_VoidSymbol},
    {"Above cursor keys: Press END (or ESCAPE if key doesn't exist)", 0x71, 1, XK_Escape},
    {"Above cursor keys: Press PAGE DOWN (or ESCAPE if key doesn't exist)", 0x49, 1, XK_Escape},
    {"Above cursor keys: Press HELP (or key you want to use for HELP if it doesn't exist)", 0x5F, 1, XK_VoidSymbol},
    
    {"Cursor keys: Press CURSOR UP", 0x4C, 1, XK_VoidSymbol},
    {"Cursor keys: Press CURSOR LEFT", 0x4F, 1, XK_VoidSymbol},
    {"Cursor keys: Press CURSOR DOWN", 0x4D, 1, XK_VoidSymbol},
    {"Cursor keys: Press CURSOR RIGHT", 0x4E, 1, XK_VoidSymbol},

    {"Numeric pad 1st row: Press the four keys above (!!!!) 7 8 9", 0x5A, 4, XK_VoidSymbol},
    {"Numeric pad 2nd row: Press 7 8 9", 0x3D, 3, XK_VoidSymbol},
    {"Numeric pad 2nd row: If key right of 9 is a normal size key press it, otherwise ESCAPE", 0x4A, 1, XK_Escape},
    {"Numeric pad 3rd row: Press 4 5 6", 0x2D, 3, XK_VoidSymbol},
    {"Numeric pad 3rd row: Press +", 0x5e, 1, XK_VoidSymbol},
    {"Numeric pad 4th row: Press 1 2 3", 0x1D, 3, XK_VoidSymbol},
    {"Numeric pad 5th row: Press 0", 0x0F, 1, XK_VoidSymbol},
    {"Numeric pad 5th row: Press COMMA", 0x3C, 1, XK_VoidSymbol},
    {"Numeric pad 5th row: Press ENTER", 0x43, 1, XK_VoidSymbol},

    {"Press PRINTSCREEN (or ESCAPE if key doesn't exist)", 0x6c, 1, XK_Escape},
    {"Press SCROLL LOCK (or ESCAPE if key doesn't exist)", 0x6b, 1, XK_Escape},
    {"Press PAUSE (or ESCAPE if key doesn't exist)", 0x6e, 1, XK_Escape},

    {0, 0, 0, XK_VoidSymbol}
};

/***************************************************************************/

void doaskuser(char *title,int rawkeystart, int numkeys, KeySym k);

/***************************************************************************/

void usage ()
{
    static char *msg[] = {
"    -display displayname                X server to contact",
"    -geometry geom                      size and location of window",
"     -o <filename>                      Table output file name",
"",
NULL};
    char **cpp;

    fprintf (stderr, "usage:  %s [-options ...]\n", ProgramName);
    fprintf (stderr, "where options include:\n");

    for (cpp = msg; *cpp; cpp++) {
	fprintf (stderr, "%s\n", *cpp);
    }

    exit (1);
}

/***************************************************************************/

static int MySysErrorHandler (Display * display)
{
    perror ("X11-Error");
    fflush (stderr);

    XAutoRepeatOn(display);
    XCloseDisplay(display);
    exit(0);
}

/***************************************************************************/

int main (int argc, char **argv)
{
    char *displayname = NULL;
    char *geom = NULL;
    char *tablefilename = NULL;
    int i;
    XSizeHints hints;
    int borderwidth = 2;
    XSetWindowAttributes attr;
    XWindowAttributes wattr;
    unsigned long mask = 0L;
    int done;
    char *name = "X11 Keycode to Rawkey table generation tool";
    Bool reverse = False;
    unsigned long back, fore;
    struct _asktable *ask;

    w = 0;
    ProgramName = argv[0];
    for (i = 1; i < argc; i++) {
	char *arg = argv[i];

	if (arg[0] == '-') {
	    switch (arg[1]) {
	      case 'd':			/* -display host:dpy */
		if (++i >= argc) usage ();
		displayname = argv[i];
		continue;
	      case 'g':			/* -geometry geom */
		if (++i >= argc) usage ();
		geom = argv[i];
		continue;
		
	      case 'o':			/* table file name */
	        if (++i >= argc) usage ();
		tablefilename = argv[i];
		continue;
		
	      default:
		usage ();
	    }				/* end switch on - */
	} else 
	  usage ();
    }					/* end for over argc */

    if (!tablefilename)
    {
        fprintf (stderr, "%s: output filename missing\n",
		 ProgramName);
	exit(1);
    }
    
    
    memset(table, 0xFF, 256);
    
    dpy = XOpenDisplay (displayname);
    if (!dpy) {
	fprintf (stderr, "%s:  unable to open display '%s'\n",
		 ProgramName, XDisplayName (displayname));
	exit (1);
    }

    XSetIOErrorHandler (MySysErrorHandler);

    XAutoRepeatOff(dpy);
    
    screen = DefaultScreen (dpy);

    /* select for all events */
    attr.event_mask = KeyPressMask; 

    set_sizehints (&hints, OUTER_WINDOW_MIN_WIDTH, OUTER_WINDOW_MIN_HEIGHT,
		   OUTER_WINDOW_DEF_WIDTH, OUTER_WINDOW_DEF_HEIGHT, 
		   OUTER_WINDOW_DEF_X, OUTER_WINDOW_DEF_Y, geom);

    if (reverse) {
	back = BlackPixel(dpy,screen);
	fore = WhitePixel(dpy,screen);
    } else {
	back = WhitePixel(dpy,screen);
	fore = BlackPixel(dpy,screen);
    }

    attr.background_pixel = back;
    attr.border_pixel = fore;
    mask |= (CWBackPixel | CWBorderPixel | CWEventMask);

    w = XCreateWindow (dpy, RootWindow (dpy, screen), hints.x, hints.y,
		       hints.width, hints.height, borderwidth, 0,
		       InputOutput, (Visual *)CopyFromParent,
		       mask, &attr);

    XSetStandardProperties (dpy, w, name, NULL, (Pixmap) 0,
			    argv, argc, &hints);

    gc = XCreateGC (dpy, w, 0, 0);

    XMapWindow (dpy, w);

    XStoreName(dpy, w, name);
    XFlush(dpy);
    
    sleep(3);
        
    for(ask = asktable; ask->title; ask++)
    {
        doaskuser(ask->title, ask->rawkeystart, ask->numkeys, ask->nokey_ks);
    }
      
    fh = fopen(tablefilename,"wb");
    if (!fh)
    {
	fprintf (stderr, "%s:  unable to open \"%s\" in write mode\n",
		 ProgramName, tablefilename);
	exit(1);
    }
    
    i = fwrite(table, 1, 256, fh);
    if (i != 256)
    {
	fprintf (stderr, "%s:  writing to \"%s\" failed\n",
		 ProgramName, tablefilename);
	exit(1);
    }
    fclose(fh);fh = 0;
    
    
    XAutoRepeatOn(dpy);
    
    XCloseDisplay (dpy);
    exit (0);
}

/***************************************************************************/

void doaskuser(char *title,int rawkeystart, int numkeys, KeySym k)
{
    int i;
    
    XStoreName(dpy, w, title);

    for(i = 0;i < numkeys;)
    {
	XEvent event;
    	char buffer[10];
    	KeySym ks;
	
	XNextEvent (dpy, &event);

	switch(((XKeyEvent *)&event)->type)
	{
	    case KeyPress:
	        ks = XK_VoidSymbol;
    		XLookupString ((XKeyEvent *)&event, buffer, 10, &ks, NULL);

	        i++;
		XSetForeground(dpy, gc, WhitePixel(dpy, screen));
		XFillRectangle(dpy, w, gc, 0, 0, 1000, 1000);
		XSetForeground(dpy, gc, BlackPixel(dpy,screen));
		
		if ((k == XK_VoidSymbol) || (ks != k))
		{
		    char s[6];
		    
		    unsigned char kc = (unsigned char)((XKeyEvent *)&event)->keycode;
		    table[kc] = rawkeystart++;
		    
		    sprintf(s, "%03ld",(long)((XKeyEvent *)&event)->keycode);
		    XDrawString(dpy, w, gc, 50, 20, s, strlen(s));

		} else {
		    XDrawString(dpy, w, gc, 50, 20, "NOP", 3);
		}
		XFlush(dpy);
		break;
		
	} /* switch(event.type) */
	
    } /* for(i = 0;i < numkeys;) */
}

/***************************************************************************/

static void
set_sizehints (XSizeHints *hintp, int min_width, int min_height,
               int defwidth, int defheight, int defx, int defy, char *geom)
{
    int geom_result;

    /* set the size hints, algorithm from xlib xbiff */

    hintp->width = hintp->min_width = min_width;
    hintp->height = hintp->min_height = min_height;
    hintp->flags = PMinSize;
    hintp->x = hintp->y = 0;
    geom_result = NoValue;
    if (geom != NULL) {
        geom_result = XParseGeometry (geom, &hintp->x, &hintp->y,
				      (unsigned int *)&hintp->width,
				      (unsigned int *)&hintp->height);
	if ((geom_result & WidthValue) && (geom_result & HeightValue)) {
#define max(a,b) ((a) > (b) ? (a) : (b))
	    hintp->width = max (hintp->width, hintp->min_width);
	    hintp->height = max (hintp->height, hintp->min_height);
	    hintp->flags |= USSize;
	}
	if ((geom_result & XValue) && (geom_result & YValue)) {
	    hintp->flags += USPosition;
	}
    }
    if (!(hintp->flags & USSize)) {
	hintp->width = defwidth;
	hintp->height = defheight;
	hintp->flags |= PSize;
    }
/*
    if (!(hintp->flags & USPosition)) {
	hintp->x = defx;
	hintp->y = defy;
	hintp->flags |= PPosition;
    }
 */
    if (geom_result & XNegative) {
	hintp->x = DisplayWidth (dpy, DefaultScreen (dpy)) + hintp->x -
		    hintp->width;
    }
    if (geom_result & YNegative) {
	hintp->y = DisplayHeight (dpy, DefaultScreen (dpy)) + hintp->y -
		    hintp->height;
    }
    return;
}

