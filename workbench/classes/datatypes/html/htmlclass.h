/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

/*******************************************************************************************/
/* Internal Class Data */

struct HtmlData
{
	APTR		mempool;
	page_struct	*page;
	parse_struct	*pdata;
	layout_struct	*ldata;
	struct List	*linelist;
	struct RastPort	*rastport;
	struct TextFont	*font;
	ULONG		style;
	UBYTE		fgpen;
	UBYTE		bgpen;
	int		linenum;
};

