/*
    Copyright © 2004, Martin Gierich. All rights reserved.
    Licensed under the terms of the AROS Public License (APL)
    $Id$

    Desc: Tables with HTML tags, attributes, character entities and other stuff
*/

#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "parse.h"
#include "tables.h"

/*******************************************************************************************/
/* Data Arrays */

/* HTML TAGs */
tag_struct List_of_Tags[MAX_Tags] =
{
/* XHTML Structure Module */	
	{"html",	&tag_html_open, NULL,	LEVEL_top, LEVEL_html, TSF_HASNOTEXT},
	{"head",	NULL, NULL,		LEVEL_html, LEVEL_head, TSF_HASNOTEXT},
	{"title",	NULL, &tag_title_close,	LEVEL_head, LEVEL_inline, TSF_PRELAYOUT},
	{"body",	NULL, NULL,		LEVEL_html, LEVEL_block, 0},
	{"!--",		&tag_comment, NULL,	LEVEL_none, LEVEL_none, TSF_NOCLOSETAG}, /* all levels */
/* XHTML Text Module */
	{"br",		&tag_br, NULL,		LEVEL_inline, LEVEL_inline, TSF_INLINE | TSF_NOCLOSETAG},
	{"div",		NULL, NULL,		LEVEL_block, LEVEL_block, TSF_NESTING},
	{"h1",		&tag_h1_open, NULL,	LEVEL_block, LEVEL_inline, TSF_PARAGRAPH},
	{"h2",		&tag_h2_open, NULL,	LEVEL_block, LEVEL_inline, TSF_PARAGRAPH},
	{"p",		&tag_p_open, NULL,	LEVEL_block, LEVEL_inline, TSF_PARAGRAPH},
	{"pre",		&tag_pre_open, NULL,	LEVEL_block, LEVEL_inline, TSF_PARAGRAPH | TSF_PRELAYOUT},
	{"span",	NULL, NULL,		LEVEL_inline, LEVEL_inline, TSF_INLINE | TSF_NESTING},
/* XHTML Presentation Module */
	{"b",		&tag_b_open, NULL,	LEVEL_inline, LEVEL_inline, TSF_INLINE},
	{"big",		&tag_big_open, NULL,	LEVEL_inline, LEVEL_inline, TSF_INLINE | TSF_NESTING},
	{"hr",		&tag_hr, NULL,		LEVEL_block, LEVEL_block, TSF_PARAGRAPH | TSF_NOCLOSETAG},
	{"i",		&tag_i_open, NULL,	LEVEL_inline, LEVEL_inline, TSF_INLINE},
	{"small",	&tag_small_open, NULL,	LEVEL_inline, LEVEL_inline, TSF_INLINE | TSF_NESTING},
	{"tt",		&tag_tt_open, NULL,	LEVEL_inline, LEVEL_inline, TSF_INLINE},
/* XHTML Hypertext Module */
	{"a",		&tag_a_open, NULL,	LEVEL_inline, LEVEL_inline, TSF_INLINE},
/* XHTML Image Module */
	{"img",		&tag_img, NULL,		LEVEL_inline, LEVEL_inline, TSF_INLINE | TSF_NOCLOSETAG},
};

/* Paragraphs */
para_struct List_of_Paras[MAX_Paras] =
{
/* PARA_h1 */	{paramask:	{fl:{ align: ALIGN_CENTER }},
		 paraflags:	{fl:{ align: ALIGN_CENTER }},
		 stylemask:	{fl:{ bold: 1, fontsize: 10 }},
		 styleflags:	{fl:{ bold: 1, fontsize: 10 }},
		 fontname:	"arial",
		 fontsize:	20,
		 indent:	0 },
/* PARA_h2 */	{paramask:	{fl:{ align: ALIGN_CENTER }},
		 paraflags:	{fl:{ align: ALIGN_CENTER }},
		 stylemask:	{fl:{ bold: 1, fontsize: 9 }},
		 styleflags:	{fl:{ bold: 1, fontsize: 9 }},
		 fontname:	"arial",
		 fontsize:	16,
		 indent:	0 },
/* PARA_p */	{paramask:	{fl:{ align: ALIGN_LEFT }},
		 paraflags:	{fl:{ align: ALIGN_LEFT }},
		 stylemask:	{fl:{ }},
		 styleflags:	{fl:{ }},
		 fontname:	"arial",
		 fontsize:	13,
		 indent:	0 },
/* PARA_pre */	{paramask:	{fl:{ align: ALIGN_LEFT, nowordwrap: 1 }},
		 paraflags:	{fl:{ align: ALIGN_LEFT, nowordwrap: 1 }},
		 stylemask:	{fl:{ fixedwidth: 1 }},
		 styleflags:	{fl:{ fixedwidth: 1 }},
		 fontname:	"ttcourier",
		 fontsize:	12,
		 indent:	20 },
};

/* Escape sequences */
esc_struct List_of_Escs[MAX_Escs] =
{
	{"quot",	34},
	{"amp",		38},
	{"lt",		60},
	{"gt",		62},
	{"euro",	128},
	{"nbsp",	160},
	{"iexcl",	161},
	{"cent",	162},
	{"pound",	163},
	{"curren",	164},
	{"yen",		165},
	{"brvbar",	166},
	{"sect",	167},
	{"uml",		168},
	{"copy",	169},
	{"ordf",	170},
	{"laquo",	171},
	{"not",		172},
	{"shy",		173},
	{"reg",		174},
	{"macr",	175},
	{"deg",		176},
	{"plusmn",	177},
	{"sup2",	178},
	{"sup3",	179},
	{"acute",	180},
	{"micro",	181},
	{"para",	182},
	{"middot",	183},
	{"cedil",	184},
	{"sup1",	185},
	{"ordm",	186},
	{"raquo",	187},
	{"frac14",	188},
	{"frac12",	189},
	{"frac34",	190},
	{"iquest",	191},
	{"Agrave",	192},
	{"Aacute",	193},
	{"Acirc",	194},
	{"Atilde",	195},
	{"Auml",	196},
	{"Aring",	197},
	{"AElig",	198},
	{"Ccedil",	199},
	{"Egrave",	200},
	{"Eacute",	201},
	{"Ecirc",	202},
	{"Euml",	203},
	{"Igrave",	204},
	{"Iacute",	205},
	{"Icirc",	206},
	{"Iuml",	207},
	{"ETH",		208},
	{"Ntilde",	209},
	{"Ograve",	210},
	{"Oacute",	211},
	{"Ocirc",	212},
	{"Otilde",	213},
	{"Ouml",	214},
	{"times",	215},
	{"Oslash",	216},
	{"Ugrave",	217},
	{"Uacute",	218},
	{"Ucirc",	219},
	{"Uuml",	220},
	{"Yacute",	221},
	{"THORN",	222},
	{"szlig",	223},
	{"agrave",	224},
	{"aacute",	225},
	{"acirc",	226},
	{"atilde",	227},
	{"auml",	228},
	{"aring",	229},
	{"aelig",	230},
	{"ccedil",	231},
	{"egrave",	232},
	{"eacute",	233},
	{"ecirc",	234},
	{"euml",	235},
	{"igrave",	236},
	{"iacute",	237},
	{"icirc",	238},
	{"iuml",	239},
	{"eth",		240},
	{"ntilde",	241},
	{"ograve",	242},
	{"oacute",	243},
	{"ocirc",	244},
	{"otilde",	245},
	{"ouml",	246},
	{"divide",	247},
	{"oslash",	248},
	{"ugrave",	249},
	{"uacute",	250},
	{"ucirc",	251},
	{"uuml",	252},
	{"yacute",	253},
	{"thorn",	254},
	{"yuml",	255},
};

/* Colors */
color_struct List_of_Colors[MAX_Colors] =
{
	{"black",	0x000000},
	{"green",	0x008000},
	{"silver",	0xc0c0c0},
	{"lime",	0x00ff00},
	{"gray",	0x808080},
	{"olive",	0x808000},
	{"white",	0xffffff},
	{"yellow",	0xffff00},
	{"maroon",	0x800000},
	{"navy",	0x000080},
	{"red",		0xff0000},
	{"blue",	0x0000ff},
	{"purple",	0x800080},
	{"teal",	0x008080},
	{"fuchsia",	0xff00ff},
	{"aqua",	0x00ffff},
};

/* Levels */
string List_of_Levels[MAX_Levels] =
{
	"none", "top", "html", "head", "block", "inline", "list", "table",
};

char charlist[256] =
{
/* 00-0f */	CHRGROUP_CTRL, CHRGROUP_CTRL, CHRGROUP_CTRL, CHRGROUP_CTRL,
		CHRGROUP_CTRL, CHRGROUP_CTRL, CHRGROUP_CTRL, CHRGROUP_CTRL,
		CHRGROUP_CTRL, CHRGROUP_SPACE /*TAB*/, CHRGROUP_SPACE /*LF*/, CHRGROUP_CTRL,
		CHRGROUP_CTRL, CHRGROUP_SPACE /*CR*/, CHRGROUP_CTRL, CHRGROUP_CTRL,
/* 10-1f */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 20-2f */	CHRGROUP_SPACE /*SPACE*/, 0, CHR_QUOTE /*"*/, 0, 0, 0, CHR_ESCAPE /*&*/, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
/* 30-3f */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CHR_TAGSTART /*<*/, 0, CHR_TAGEND /*>*/, 0,
/* 40-4f */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 50-5f */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 60-6f */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 70-7f */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, CHRGROUP_CTRL,
/* 80-8f */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 90-9f */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* a0-af */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* b0-bf */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* c0-cf */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* d0-df */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* e0-ef */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* f0-ff */	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

