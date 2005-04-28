/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id: PrintString.c,v 1.1 2005/04/21 20:52:04 damato Exp $

***************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <graphics/gfxmacros.h>
#include <libraries/mui.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "private.h"

VOID PrintString (struct IClass *cl, Object *obj)
{
		struct InstData			*data			= (struct InstData *)INST_DATA(cl, obj);
		struct RastPort		*oldrport	= muiRenderInfo(obj)->mri_RastPort;
		struct RastPort		*rport		= &data->rport;
		struct MUI_AreaData	*ad			= muiAreaData(obj);
		struct TextFont		*font			= data->Font ? data->Font : ad->mad_Font;
		STRPTR	contents = data->Contents;
		WORD 		x=10, y=0, width, height,
					crsr_x, crsr_width=0, crsr_color,
					dst_x, dst_y, length, offset = 0, StrLength;
		STRPTR	text;
		BOOL		BlockEnabled = (data->Flags & FLG_BlockEnabled && data->BlockStart != data->BlockStop);
		UWORD		Blk_Start, Blk_Width;

	dst_x = ad->mad_Box.Left + ad->mad_addleft;
	dst_y = ad->mad_Box.Top  + ad->mad_addtop;
	width = ad->mad_Box.Width - ad->mad_subwidth;
	height = font->tf_YSize;

	StrLength = strlen(contents);

	STRPTR fake_contents = NULL;
	if(data->Flags & FLG_Secret && (fake_contents = (STRPTR)MyAllocPooled(data->Pool, StrLength+1)))
	{
		contents = fake_contents;
		WORD strlength = StrLength;
		contents[strlength] = '\0';
		while(strlength--)
			contents[strlength] = '*';
	}

	crsr_width = (data->Flags & FLG_Active) && !BlockEnabled ? TextLength(rport, (*(contents+data->BufferPos) == '\0') ? "n" : contents+data->BufferPos, 1) : 0;

	if(data->DisplayPos > data->BufferPos)
		data->DisplayPos = data->BufferPos;

	if(StrLength)
	{
			UWORD backdistance = MyTextFit(font, contents+StrLength-1, StrLength, width/*-crsr_width*/, -1);

		if(backdistance > StrLength-data->DisplayPos)
			data->DisplayPos = StrLength-backdistance;
	}

	if(data->BufferPos)
	{
			UWORD distance = MyTextFit(font, contents+data->BufferPos-1, data->BufferPos, width-crsr_width, -1);

		if(distance < data->BufferPos-data->DisplayPos)
			data->DisplayPos = data->BufferPos - distance;
	}

	text = contents+data->DisplayPos;
	StrLength -= data->DisplayPos;

	if(BlockEnabled)
	{
		Blk_Start = (data->BlockStart < data->BlockStop) ? data->BlockStart : data->BlockStop;
		Blk_Width = abs(data->BlockStop-data->BlockStart);

		if(Blk_Start < data->DisplayPos)
		{
			Blk_Width -= data->DisplayPos-Blk_Start;
			Blk_Start  = data->DisplayPos;
		}
		crsr_x = TextLength(rport, text, Blk_Start-data->DisplayPos);
		crsr_width = TextLength(rport, contents+Blk_Start, Blk_Width);
		crsr_color = data->MarkedColor;
	}
	else
	{
		if(data->Flags & FLG_Active)
		{
			crsr_x = TextLength(rport, text, data->BufferPos-data->DisplayPos);
			crsr_color = data->CursorColor;
		}
	}

	muiRenderInfo(obj)->mri_RastPort = rport;
	DoMethod(obj, MUIM_DrawBackground, x, y, width, height, dst_x, dst_y, 0L);
	muiRenderInfo(obj)->mri_RastPort = oldrport;

	length = MyTextFit(font, text, StrLength, width, 1);
	if(data->Alignment != MUIV_String_Format_Left)
	{
			UWORD textlength = MyTextLength(font, text, length);

		if(crsr_width && !BlockEnabled && data->BufferPos == data->DisplayPos+StrLength)
		{
			length = MyTextFit(font, text, StrLength, width-crsr_width, 1);
			textlength += crsr_width;
		}

		switch(data->Alignment)
		{
			case MUIV_String_Format_Center:
				offset = (width - textlength)/2;
				break;
			case MUIV_String_Format_Right:
				offset = (width - textlength);
				break;
		}
	}

	if(crsr_width && crsr_x < width)
	{
		SetAPen(rport, crsr_color);
		if(crsr_x+crsr_width > width)
		{
			crsr_width = width-crsr_x;
		}
		RectFill(rport, x+offset+crsr_x, y, x+offset+crsr_x+crsr_width-1, y+font->tf_YSize-1);
	}

	if(length)
	{
			UWORD newlength;
			LONG textcolor = (data->Flags & FLG_Active) ? data->ActiveText : data->InactiveText;

		Move(rport, x+offset, y+font->tf_Baseline);

		if(BlockEnabled && textcolor != data->MarkedTextColor)
		{
			newlength = Blk_Start-data->DisplayPos;
			SetAPen(rport, textcolor);
			Text(rport, text, newlength);
			text += newlength;

			newlength = (((Blk_Start-data->DisplayPos) + Blk_Width) > length) ? length - (Blk_Start-data->DisplayPos) : Blk_Width;
			SetAPen(rport, data->MarkedTextColor);
			Text(rport, text, newlength);
			text += newlength;

			length -= newlength + (Blk_Start-data->DisplayPos);
		}

		SetAPen(rport, textcolor);
		Text(rport, text, length);
	}

	if(fake_contents)
		MyFreePooled(data->Pool, fake_contents);

	BltBitMapRastPort(data->rport.BitMap, x, y, muiRenderInfo(obj)->mri_RastPort, dst_x, dst_y, width, height, 0xc0);

	if(data->Flags & FLG_Ghosted)
	{
			UWORD GhostPattern[] = {0x4444, 0x1111};
			struct RastPort *rport = muiRenderInfo(obj)->mri_RastPort;

		SetAfPt(rport, GhostPattern, 1);
		SetAPen(rport, _pens(obj)[MPEN_SHADOW]);
		RectFill(rport, ad->mad_Box.Left, ad->mad_Box.Top, ad->mad_Box.Left+ad->mad_Box.Width-1, ad->mad_Box.Top+ad->mad_Box.Height-1);
		SetAfPt(rport, 0, 0);
	}
}
