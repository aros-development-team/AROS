/*
    Copyright © 2004, Martin Gierich. All rights reserved.
    Licensed under the terms of the AROS Public License (APL)
    $Id$

    Desc: Structs for tables with HTML tags, attributes, character entities and other stuff
*/

/*******************************************************************************************/
/* HTML Tags */
enum Num_of_Tags
{
/* Structure */		TAG_html, TAG_head, TAG_title, TAG_body, TAG_comment,
/* Text */		TAG_br, TAG_div, TAG_h1, TAG_h2, TAG_p, TAG_pre, TAG_span,
/* Presentation */	TAG_b, TAG_big, TAG_hr, TAG_i, TAG_small, TAG_tt,
/* Hypertext */		TAG_a,
/* Image */		TAG_img,
			MAX_Tags
};

typedef struct
{
	string	name;
	tagfunc	openfunc;
	tagfunc	closefunc;
	char	prevlevel;
	char	nextlevel;
	short	flags;
#define TSF_NOCLOSETAG	0x0001	/* implies HASNOTEXT */
#define TSF_PARAGRAPH	0x0002
#define TSF_INLINE	0x0004
#define TSF_NESTING	0x0008
#define TSF_HASNOTEXT	0x0010
#define TSF_PRELAYOUT	0x0020
} __attribute__((packed)) tag_struct;

extern tag_struct	List_of_Tags[MAX_Tags];

/*******************************************************************************************/
/* HTML Tag Attributes */
enum Num_of_Attrs
{
	ATTR_href, ATTR_src, ATTR_alt,
	MAX_Attrs
};

typedef struct
{
	string	name;
	int	pad;
} __attribute__((packed)) attr_struct;

extern attr_struct	List_of_Attrs[MAX_Attrs];

/*******************************************************************************************/
/* Paragraphs */
enum Num_of_Paras
{
	PARA_h1, PARA_h2, PARA_p, PARA_pre,
	MAX_Paras
};

typedef struct
{
	para_flags	paramask;
	para_flags	paraflags;
	style_flags	stylemask;
	style_flags	styleflags;
	string		fontname;
	u_short		fontsize;
	int		indent;
} __attribute__((packed)) para_struct;

extern para_struct	List_of_Paras[MAX_Paras];

/*******************************************************************************************/
/* Escape sequences */
#define MAX_Escs (101)

typedef struct
{
	string	name;
	char	value;
} __attribute__((packed)) esc_struct;

extern esc_struct	List_of_Escs[MAX_Escs];

/*******************************************************************************************/
/* Colors */
enum Colors
{
	COLOR_black, COLOR_green, COLOR_silver, COLOR_lime, COLOR_gray, COLOR_olive, COLOR_white, COLOR_yellow,
	COLOR_maroon, COLOR_navy, COLOR_red, COLOR_blue, COLOR_purple, COLOR_teal, COLOR_fuchsia, COLOR_aqua,
	MAX_Colors
};

typedef struct
{
	string	name;
	long	value;
} __attribute__((packed)) color_struct;

/*******************************************************************************************/
/* Levels */
enum Levels
{
	LEVEL_none = 0, LEVEL_top, LEVEL_html, LEVEL_head, LEVEL_block, LEVEL_inline, LEVEL_list, LEVEL_table,
	MAX_Levels
};

extern string		List_of_Levels[MAX_Levels];
extern char		charlist[256];

