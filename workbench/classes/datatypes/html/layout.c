/*
    Copyright © 2004, Martin Gierich. All rights reserved.
    Licensed under the terms of the AROS Public License (APL)
    $Id$

    Desc: Layout engine with wordwrapper
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "layout.h"

/*******************************************************************************************/
/* Prototypes */

#if SHOW_SEGLIST
static int		show_seglist( seg_struct *seglistpos );
#endif
static int		layout_seglist( layout_struct *ldata, seg_struct *seglistpos, int *maxx, int *maxy );
static void		new_line( layout_struct *ldata, int startx, int starty, int *width, int *height );
static int		layout_text( layout_struct *ldata, string str, int len, int startx, int starty, int *width, int *height );

/*******************************************************************************************/
/* Globals */

string List_of_SegCmds[SEG_CMD_MAX] =
{
	"next", "sublist", "blockstart", "blockend", "text", "linebreak", "image", "ruler", "parastyle", "softstyle"
};

/*******************************************************************************************/

int layout_init( page_struct *page )
{
	layout_struct	*ldata;

	ldata = MALLOC( page->mempool, sizeof( layout_struct ) );
	if( !ldata )
		return FALSE;
	page->ldata = ldata;
#if SHOW_SEGLIST
	D( printf("\n\n----------------------------------------------------\n\n"); )
	show_seglist( page->seglist );
#endif
	return TRUE;
}

int layout_do( page_struct *page, int winwidth, int *width, int *height )
{
	layout_struct	*ldata;

	ldata = page->ldata;
	D( printf("\n\n----------------------------------------------------\n\n"); )
	D( printf("layout - page: %p title %p seglist %p\n", page, page->title, page->seglist); )
	linelist_init( page->ldata );
	D( printf("\033[1mPage title: %s\033[0m\n", page->title); )
	ldata->nowordwrap = FALSE;
	ldata->xsize = winwidth;
	ldata->xpos = 0;
	layout_seglist( page->ldata, page->seglist, width, height );
	D( printf("width=%d height=%d\nLayout End.\n", *width, *height); )
	return TRUE;
}

void layout_free( page_struct *page )
{
	layout_struct	*ldata;

	ldata = page->ldata;
	linelist_free( ldata );
	MFREE( page->mempool, ldata );
}

/*******************************************************************************************/
/* Display for debug */

#if SHOW_SEGLIST
static int show_seglist( seg_struct *seglistpos )
{
	int	i;
	u_char	cmd, last;
	string	str;
	int	len;

	i = 1;
	do
	{
		cmd = seglistpos->cmd & SEG_CMD_MASK;
		last = seglistpos->cmd & SEG_CMD_LAST;
		if( cmd == SEG_CMD_Next )
		{
			printf("%2d cmd %2d: %-10s %p -> %p\n", i, cmd, List_of_SegCmds[cmd], seglistpos, seglistpos->next);
			seglistpos = seglistpos->next;
			continue;
		}
		else if( cmd == SEG_CMD_Sublist )
		{
			printf("%2d cmd %2d: %-10s %p -> %p <-\n", i, cmd, List_of_SegCmds[cmd], seglistpos, seglistpos->next);
			show_seglist( seglistpos->sublist );
			continue;
		}
		else
		{
			printf("%2d cmd %2d: %-10s %p", i, cmd, List_of_SegCmds[cmd], seglistpos);
			if( cmd == SEG_CMD_Text )
			{
				len = seglistpos->textlen;
				str = seglistpos->textseg;
				printf(" len=%2d: [%s]", len, str);
			}
			printf("\n");
		}
		seglistpos++;
		i++;
	}
	while( !last );
	return 0;
}
#endif

/*******************************************************************************************/
/* Layout Core */

static int layout_seglist( layout_struct *ldata, seg_struct *seglistpos, int *maxx, int *maxy )
{
	int		i;
	int		width, height, startx;
	u_char		cmd, last;
	string		str;
	int		len;

	*maxx = 0;
	*maxy = 0;
	startx = 0;
	ldata->fontheight = text_height( ldata );
	ldata->styleflags.value = 0;
	i = 1;
	do
	{
		cmd = seglistpos->cmd & SEG_CMD_MASK;
		last = seglistpos->cmd & SEG_CMD_LAST;
//		D( printf("%2d: cmd %2d: %-10s seglistpos %p\n", i, cmd, List_of_SegCmds[cmd], seglistpos); )
		if( cmd == SEG_CMD_Next )
		{
			seglistpos = seglistpos->next;
		}
		else if( cmd == SEG_CMD_Sublist )
		{
			layout_seglist( ldata, seglistpos->sublist, &width, &height );
			if( width > *maxx )
				*maxx = width;
			*maxy += height;
		}
		else if( cmd == SEG_CMD_Blockstart )
		{
			D( printf("%2d: ........................ maxx=%2d maxy=%2d\n", i, *maxx, *maxy); )
		}
		else if( cmd == SEG_CMD_Blockend )
		{
			if( ldata->xpos > 0 )
			{
				new_line( ldata, startx, *maxy, &width, &height );
				if( width > *maxx )
					*maxx = width;
				*maxy += height;
			}
			D( printf("%2d: ^^^^^^^^^^^^^^^^^^^^^^^^ maxx=%2d maxy=%2d\n", i, *maxx, *maxy); )
			new_line( ldata, startx, *maxy, &width, &height );
			if( width > *maxx )
				*maxx = width;
			*maxy += height;
		}
		else if( cmd == SEG_CMD_Text )
		{
			len = seglistpos->textlen;
			str = seglistpos->textseg;
			layout_text( ldata, str, len, startx, *maxy, &width, &height );
			if( width > *maxx )
				*maxx = width;
			*maxy += height;
		}
		else if( cmd == SEG_CMD_Linebreak )
		{
			new_line( ldata, startx, *maxy, &width, &height );
			if( width > *maxx )
				*maxx = width;
			*maxy += height;
		}
		else if( cmd == SEG_CMD_Image )
		{
			str = "[Image]";
			layout_text( ldata, str, strlen(str), startx, *maxy, &width, &height );
			if( width > *maxx )
				*maxx = width;
			*maxy += height;
		}
		else if( cmd == SEG_CMD_Ruler )
		{
			str = "========================";
			layout_text( ldata, str, strlen(str), startx, *maxy, &width, &height );
			if( width > *maxx )
				*maxx = width;
			*maxy += height;
		}
		else if( cmd == SEG_CMD_Parastyle )
		{
			para_flags	pflags;
			
			pflags = seglistpos->paraflags;
			D( printf("%2d: Paragraph flags %02x\n", i, pflags.value); )
			if( pflags.fl.nowordwrap )
				ldata->nowordwrap = TRUE;
			else
				ldata->nowordwrap = FALSE;
		}
		else if( cmd == SEG_CMD_Softstyle )
		{
			ldata->styleflags = seglistpos->styleflags;
			//ldata->fontheight = seglistpos->styleflags.fl.fontsize - 8;
//			D( printf("%2d: Style %s%s%s%s Size %d\n", i, ts.bold?"bold ":"", ts.italics?"italics ":"",
//				ts.underlined?"underlined ":"", ts.fixedwidth?"fixedwidth ":"", fontsize); )
#if 0
			{
				int		modded;
				style_flags	ts;
			
				ts = seglistpos->styleflags;
				printf("\033[0m");
				if( ts.fl.bold )
				{
					printf("\033[1m");
					modded = TRUE;
				}
				if( ts.fl.underlined )
				{
					printf("\033[4m");
					modded = TRUE;
				}
				if( ts.fl.italics )
				{
					printf("\033[7m");
					modded = TRUE;
				}
				if( ts.fl.fixedwidth )
				{
					printf("\033[34m");
					modded = TRUE;
				}
				if( fontsize != oldfontsize )
				{
					oldfontsize = fontsize;
					modded = FALSE;
					if( fontsize == 1 )
					{
						printf("\033[31m");
						modded = TRUE;
					}
					if( fontsize == 2 )
					{
						printf("\033[32m");
						modded = TRUE;
					}
					if( !modded )
						printf("\033[30m");
				}
			}
#endif
		}
		else
		{
			D( printf("%2d: cmd %d\n", i, cmd); )
		}
		seglistpos++;
		i++;
	}
	while( !last );
	return 0;
}

/*******************************************************************************************/
/* Text */

static void new_line( layout_struct *ldata, int startx, int starty, int *width, int *height )
{
	if( ldata->xpos == 0 )
	{
		/* create new empty line */
		ldata->oldline = linelist_store( ldata, "", 0, startx, starty, 0, ldata->fontheight, ldata->styleflags, TRUE /*linebreak*/ );
	}
	else
	{
		/* end current line */
		linelist_addlf( ldata, ldata->oldline );
	}
	*width = ldata->xpos;
	ldata->xpos = 0;
	ldata->oldline = NULL;
	*height = ldata->fontheight;
}

static int layout_text( layout_struct *ldata, string str, int len, int startx, int starty, int *width, int *height )
{
	int		i, idx, linebreak, xpos, xsize, xmax, ypos;
	int		strlen, xstrsize, xskip, fontheight;
	int		charlen, charskip, nospace;

	xpos = ldata->xpos;
	xsize = ldata->xsize;
	xmax = 0;
	ypos = 0;
	fontheight = ldata->fontheight;
//	D( printf("[x=%d y=%d f=%d len=%d %s]", startx, starty, fontheight, len, str); )
	while( len > 0 )
	{
		if( ldata->nowordwrap )
		{
			/* simple: word wrap is disabled */
			xstrsize = text_len( ldata, str, len );
			charlen = len;
			charskip = len;
			xskip = xstrsize;
			linebreak = FALSE;
			idx = 1;
		}
		else
		{
			if( xpos == 0 && str[0] == ' ' )
			{
				/* skip leading space when at start of line */
				str++;
				len--;
				D( printf("(s)"); )
			}
			/* determine how many characters will fit in remaining line */
			strlen = text_fit( ldata, str, len, &xstrsize, xsize - xpos );
			if( strlen == len )
			{
				/* jep, it fits in current line */
				charlen = strlen;
				charskip = strlen;
				xskip = xstrsize;
				linebreak = FALSE;
				idx = 2;
			}
			else
			{
				/* nope, we have to wrap words */
				linebreak = TRUE;
				nospace = TRUE;
				for( i=strlen-1; i>0; i-- )
				{
					/* search backwards for last space to detect end of last word */
					if( str[i] == ' ' )
					{
						charlen = i;
						charskip = i+1;
						xstrsize = text_len( ldata, str, charlen ); //debug only
						xskip = xstrsize;	/* this is bigger than neccessary,
									   but small enough to fit xsize */
						nospace = FALSE;
						idx = 3;
						break;
					}
				}
				if( nospace )
				{
					/* this word actually is bigger than strlen */
					if( xpos > 0 )
					{
						/* this word doesnt't fit in already begun line, try a new line */
						charlen = 0;
						charskip = 0;
						xskip = 0;
						idx = 4;
					}
					else
					{
						/* it is a monster word being longer than the line, search end of word */
						for( i=strlen; i<len; i++ )
						{
							if( str[i] == ' ' )
							{
								charlen = i;
								charskip = i+1;
								nospace = FALSE;
								idx = 5;
								break;
							}
						}
						if( nospace )
						{
							/* no space at end of monster word */
							charlen = len;
							charskip = len;
							idx = 6;
						}
						xskip = xstrsize;
						if( charlen > strlen )
							xskip += text_len( ldata, str+strlen, charlen-strlen );
					}
				}
			}
		}

		if( charlen )
		{
			ldata->oldline = linelist_store( ldata, str, charlen, startx+xpos, starty+ypos, xskip, fontheight, ldata->styleflags, linebreak );
			//D( for(i=0; i<charlen; i++) printf("%c", str[i]); )
		}
		else if( linebreak )
		{
			linelist_addlf( ldata, ldata->oldline );
		}
//		D( printf("(%d %d %d i=%d x=%d y=%d %d %d %s)", len, charlen, charskip, idx, startx+xpos, starty+ypos, xstrsize, xskip, linebreak ? "br":""); )
		xpos += xskip;
		if( xpos > xmax )
			xmax = xpos;
		if( linebreak )
		{
			xpos = 0;
			ldata->oldline = NULL;
			ypos += fontheight;
		}
		str += charskip;
		len -= charskip;
	}
	ldata->xpos = xpos;
	*width = xmax;
	*height = ypos;
	return 0;
}

/*******************************************************************************************/
/* Text Size */

#ifndef __AROS__
int text_len( layout_struct *ldata, string str, int strlen )
{
	return strlen;
}

int text_height( layout_struct *ldata )
{
	return 1;
}

int text_fit( layout_struct *ldata, string str, int strlen, int *strsize, int maxwidth )
{
	int t;
	
	if( strlen > maxwidth )
		t = maxwidth;
	else
		t = strlen;
	*strsize = t;
	return t;
}
#endif

/*******************************************************************************************/
/* Line List */

#ifndef __AROS__
int linelist_init( layout_struct *ldata )
{
	return 0;
}

void * linelist_store( layout_struct *ldata, string textseg, u_short textlen,
		u_short xpos, u_short ypos, u_short width, u_short height, style_flags styleflags, int linebreak )
{
#if 1
	D( {
		int	i;
		printf("\nLINE len=%2d: x=%2d y=%2d w=%2d h=%2d %s [", textlen, xpos, ypos, width, height, linebreak?"lf":"no");
		for(i=0; i<textlen; i++)
			printf("%c", textseg[i]);
		printf("]");
		if( linebreak )
			printf("<BR>\n");
	} )
#endif
	return "";
}

void * linelist_addlf( layout_struct *ldata, void * line )
{
	D( printf("<BR>\n"); )
	return "";
}

void linelist_free( layout_struct *ldata )
{
}
#endif

