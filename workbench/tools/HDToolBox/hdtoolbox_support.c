/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>

#define DEBUG 1
#include <aros/debug.h>

#include "hdtoolbox_support.h"
#include "gadgets.h"

struct Node *getNumNode(struct List *list, int num) {
struct Node *node;

	node = list->lh_Head;
	while (num)
	{
		node = node->ln_Succ;
		num--;
	}
	return num ? 0 : node;
}

ULONG getNodeNum(struct Node *node) {
ULONG num = 0;

	for (;;)
	{
		node = node->ln_Pred;
		if (node->ln_Pred == 0)
			return num;
		num++;
	}
}

ULONG countNodes(struct List *list) {
ULONG count = 0;
struct Node *node;

	node = list->lh_Head;
	while (node->ln_Succ)
	{
		count++;
		node = node->ln_Succ;
	}
	return count;
}


extern APTR visual;
extern struct Screen *scr;

LONG RequestList(struct List *list, ULONG *result) {
struct Window *win;
struct Gadget *glist;
struct TagItem donetags[] = {{TAG_DONE, NULL}};
struct TagItem lvtags[] =
{
	{GTLV_Labels, (ULONG)list},
   {GTLV_MakeVisible, 0},
   {GTLV_Selected, ~0},
   {GTLV_ShowSelected, 0},
   {TAG_DONE,NULL}
};
struct creategadget rqlgadgets[] =
{
	{
		LISTVIEW_KIND,
		{
			5,20,200,100,
			NULL, NULL,
			0, NULL, NULL, NULL
		},
		lvtags
	},
	{
		BUTTON_KIND,
		{
			5,130,80,20,
			"Ok", NULL,
			1, PLACETEXT_IN, NULL, NULL
		},
		donetags
	},
	{
		BUTTON_KIND,
		{
			125,130,80,20,
			"Cancel", NULL,
			2, PLACETEXT_IN, NULL, NULL
		},
		donetags
	}
};
LONG retval=-1;
struct IntuiMessage *msg;

	glist = createGadgets(rqlgadgets, 0, 3, visual);
	if (glist)
	{
		win = OpenWindowTags
			(
				NULL,
				WA_PubScreen, scr,
				WA_Left, scr->Width/2-(220/2),
				WA_Top, scr->Height/2-(160/2),
				WA_Width, 220,
				WA_Height, 160,
				WA_Title, "Choose",
				WA_IDCMP,
					BUTTONIDCMP |
					SLIDERIDCMP |
					SCROLLERIDCMP |
					ARROWIDCMP|
					IDCMP_REFRESHWINDOW,
				WA_Gadgets, glist,
				WA_DragBar, TRUE,
				WA_DepthGadget, TRUE,
				WA_SizeGadget, TRUE,
				TAG_DONE
			);
		if (win)
		{
			retval = 0;
			*result = 0;
			while (retval == 0)
			{
				WaitPort(win->UserPort);
				while ((msg=GT_GetIMsg(win->UserPort)))
				{
					switch (msg->Class)
					{
					case IDCMP_REFRESHWINDOW:
						GT_BeginRefresh(win);
						GT_EndRefresh(win, TRUE);
						break;
					case IDCMP_GADGETUP:
						switch (((struct Gadget *) msg->IAddress)->GadgetID)
		            {
						case 0:
							*result = msg->Code;
							break;
						case 1:
						case 2:
							retval = ((struct Gadget *) msg->IAddress)->GadgetID;
							break;
						}
						break;
					}
					GT_ReplyIMsg(msg);
				}
			}
			if (retval == 2)
				retval = 0;
			CloseWindow(win);
		}
		freeGadgets(glist);
	}
	return retval;
}

void typestrncpy(STRPTR dst, STRPTR src, ULONG len) {

	while (len)
	{
		if (isprint(*src))
			*dst++ = *src++;
		else
		{
			*dst++ = '\\';
			sprintf(dst,"%o", *src++);
			while (*dst)
				dst++;
		}
		len--;
	}
}

UWORD ownsprintf(STRPTR dst, STRPTR fmt, ...) {
UWORD count=0;

	while (*fmt)
	{
		if (*fmt == '\\')
		{
			*fmt++;
			if (isdigit(*fmt))
			{
			ULONG val=0;

				for(;;)
				{
					val += (*fmt-'0');
					fmt++;
					if (!isdigit(*fmt))
						break;
					val *= 8;
				}
				*dst++ = (UBYTE)val;
				count++;
			}
			else
				kprintf("%s-%ld: unknown escape sequence\n", __FILE__, __LINE__);
		}
		else
		{
			*dst++ = *fmt++;
			count++;
		}
	}
	return count;
}

/* size in kB */
void getSizeStr(STRPTR str, ULONG size) {
UBYTE c='M';
ULONG r;

	r = size % 1024;
	size = size / 1024;
	if (size>512)
	{
		c='G';
		r = size % 1024;
		size = size / 1024;
	}
	r = r*10/1024;
	sprintf(str, "%ld,%ld%c",size,r,c);
}

LONG GetPartitionAttrsA(struct PartitionHandle *ph, LONG tag, ...) {

   return GetPartitionAttrs(ph, (struct TagItem *)&tag);
}

LONG SetPartitionAttrsA(struct PartitionHandle *ph, LONG tag, ...) {

	return SetPartitionAttrs(ph, (struct TagItem *)&tag);
}

LONG GetPartitionTableAttrsA(struct PartitionHandle *ph, LONG tag, ...) {

   return GetPartitionTableAttrs(ph, (struct TagItem *)&tag);
}

BOOL existsAttr(ULONG *attrlist, ULONG attr) {

	while (*attrlist)
	{
		if (*attrlist++ == attr)
			return TRUE;
	}
	return FALSE;
}

UBYTE getBitNum(ULONG val) {
UBYTE count=0;

	if (val==0)
		return 0xFF;
	for (;;)
	{
		val >>= 1;
		if (val==0)
			break;
		count++;
	}
	return count;
}

