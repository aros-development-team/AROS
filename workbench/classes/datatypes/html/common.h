/*
    Copyright © 2004, Martin Gierich. All rights reserved.
    Licensed under the terms of the AROS Public License (APL)
    $Id$

    Desc: Structs for both HTML parser and layout engine
*/

#ifndef TRUE
#define TRUE -1
#define FALSE 0
#endif

typedef char* string;
#ifdef __AROS__
#include <clib/alib_protos.h>
#include <proto/exec.h>
#define MALLOC(pool,size) AllocPooled((pool),(size))
#define MFREE(pool,ptr)
typedef unsigned char u_char;
typedef unsigned short u_short;
#define G(o) ((struct Gadget *)(o))
#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>
#define printf bug

#else

#define MALLOC(pool,size) malloc(size)
#define MFREE(pool,ptr) free(ptr)

#endif

/*******************************************************************************************/
/* Prototypes */

/* Types */
typedef union _para_flags para_flags;
typedef union _style_flags style_flags;
typedef struct _image_struct image_struct;
typedef struct _seg_struct seg_struct;
typedef struct _page_struct page_struct;
typedef struct _parse_struct parse_struct;
typedef struct _layout_struct layout_struct;

/* General */
page_struct *	parse_init( void *mempool );
int		parse_do( page_struct *page, string inbuf, int inbufbytes );
int		parse_end( page_struct *page );
void		parse_free( page_struct *page );
int		layout_init( page_struct *page );
int		layout_do( page_struct *page, int winwidth, int *width, int *height );
void		layout_free( page_struct *page );

/*******************************************************************************************/
/* Structures common to parse and layout */

struct _page_struct
{
	string		title;
	void		*mempool;
	seg_struct	*seglist;
	parse_struct	*pdata;
	layout_struct	*ldata;
};

struct p_flags
{
	u_short align:2;
#define ALIGN_LEFT	0
#define ALIGN_RIGHT	1
#define ALIGN_CENTER	2
#define ALIGN_JUSTIFY	3
	u_short nowordwrap:1;
} __attribute__((packed));

union _para_flags
{
	u_short		value;
	struct p_flags	fl;
} __attribute__((packed));

struct s_flags
{
	u_short fontsize:4;
	u_short bold:1;
	u_short italics:1;
	u_short underlined:1;
	u_short fixedwidth:1;
} __attribute__((packed));

union _style_flags
{
	u_short		value;
	struct s_flags	fl;
} __attribute__((packed));

struct _image_struct
{
	string	src;
	string	alt;
	int	width;
	int	height;
};

/*******************************************************************************************/
/* Text Segments */

enum SegCmd
{
	SEG_CMD_Next,
	SEG_CMD_Sublist,
	SEG_CMD_Blockstart,
	SEG_CMD_Blockend,
	SEG_CMD_Text,
	SEG_CMD_Linebreak,
	SEG_CMD_Image,
	SEG_CMD_Ruler,
	SEG_CMD_Parastyle,
	SEG_CMD_Softstyle,
	SEG_CMD_MAX
};

#define SEG_CMD_MASK	0x1f
#define SEG_CMD_LAST	0x80

struct _seg_struct
{
	u_char		cmd;
	u_char		pad;
	union
	{
		u_short		textlen;
		u_short		flags;
		style_flags	stylemask;
		para_flags	paramask;
	};
	union
	{
		string		textseg;
		seg_struct	*next;
		seg_struct	*sublist;
		image_struct	*image;
		style_flags	styleflags;
		para_flags	paraflags;
	};
} __attribute__((packed));

/*******************************************************************************************/
/* Layout Prototypes */
int		text_len( layout_struct *ldata, string str, int strlen );
int		text_height( layout_struct *ldata );
int		text_fit( layout_struct *ldata, string str, int strlen, int *strsize, int maxwidth );

int		linelist_init( layout_struct *ldata );
void *		linelist_store( layout_struct *ldata, string textseg, u_short textlen,
			u_short xpos, u_short ypos, u_short width, u_short height, style_flags style, int linebreak );
void *		linelist_addlf( layout_struct *ldata, void * line );
void		linelist_free( layout_struct *ldata );

/*******************************************************************************************/
/* Private Layout Data */

struct _layout_struct
{
	void		*userdata;
	void		*oldline;
	int		nowordwrap;
	int		xsize;
	int		xpos;
	int		fontheight;
	int		indent;
	para_flags	paraflags;
	style_flags	styleflags;
};

