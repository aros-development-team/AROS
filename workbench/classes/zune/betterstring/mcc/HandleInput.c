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

 $Id: HandleInput.c,v 1.1 2005/04/21 20:52:04 damato Exp $

***************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <clib/alib_protos.h>
#include <clib/macros.h>
#include <devices/clipboard.h>
#include <devices/inputevent.h>
#include <libraries/mui.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/keymap.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>
#include <proto/locale.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include "private.h"

#ifndef __AROS__
#include "SDI_stdarg.h"
#endif

static BOOL BlockEnabled(struct InstData *data)
{
	return((data->Flags & FLG_BlockEnabled) && data->BlockStart != data->BlockStop);
}

#if defined(__amigaos4__) || defined(__MORPHOS__)
static int VARARGS68K MySPrintf(char *buf, char *fmt, ...)
{
  VA_LIST args;

  VA_START(args, fmt);
  RawDoFmt(fmt, VA_ARG(args, void *), NULL, buf);
  VA_END(args);

  return(strlen(buf));
}
#elif defined(__AROS__)
#define MySPrintf __sprintf /* from amiga lib */
#else
static int STDARGS MySPrintf(char *buf, char *fmt,...)
{
	static const UWORD PutCharProc[2] = {0x16C0,0x4E75};
	/* dirty hack to avoid assembler part :-)
   	16C0: move.b d0,(a3)+
	   4E75: rts */
	RawDoFmt(fmt, (APTR)(((ULONG)&fmt)+4), (APTR)PutCharProc, buf);

	return(strlen(buf));
}
#endif

VOID AddToUndo (struct InstData *data)
{
	if(data->Undo)
		MyFreePooled(data->Pool, data->Undo);
	
  if((data->Undo = (STRPTR)MyAllocPooled(data->Pool, strlen(data->Contents)+1)))
	{
		strcpy(data->Undo, data->Contents);
		data->UndoPos = data->BufferPos;
		data->Flags &= ~FLG_RedoAvailable;
	}
}

WORD AlignOffset (Object *obj, struct InstData *data)
{
		struct MUI_AreaData	*ad	= muiAreaData(obj);
		struct TextFont		*font	= data->Font ? data->Font : ad->mad_Font;
		WORD	 width = ad->mad_Box.Width - ad->mad_subwidth;
		WORD	 offset = 0;

	if(data->Alignment != MUIV_String_Format_Left)
	{
			STRPTR	text = data->Contents+data->DisplayPos;
			UWORD		StrLength = strlen(text);
			UWORD		length, textlength, crsr_width;

		length = MyTextFit(font, text, StrLength, width, 1);
		textlength = MyTextLength(font, text, length);

		crsr_width = (data->Flags & FLG_Active) ? MyTextLength(font, (*(data->Contents+data->BufferPos) == '\0') ? "n" : data->Contents+data->BufferPos, 1) : 0;
		if(crsr_width && !BlockEnabled(data) && data->BufferPos == data->DisplayPos+StrLength)
		{
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
	return(offset);
}

BOOL Reject (UBYTE code, STRPTR reject)
{
	if(reject)
	{
		while(*reject)
		{
			if(code == *reject++)
				return(FALSE);
		}
	}
	return(TRUE);
}

BOOL Accept (UBYTE code, STRPTR accept)
{
	return(accept ? !Reject(code, accept) : TRUE);
}

UWORD DecimalValue (UBYTE code)
{
	if(code >= '0' && code <= '9')
		return(code - '0');
	if(code >= 'a' && code <= 'f')
		return(code - 'a' + 10);
	if(code >= 'A' && code <= 'F')
		return(code - 'A' + 10);

	return(0);
}

BOOL IsHex (UBYTE code)
{
	return(
		(code >= '0' && code <= '9') ||
		(code >= 'a' && code <= 'f') ||
		(code >= 'A' && code <= 'F') ? TRUE : FALSE);
}

LONG FindDigit (struct InstData *data)
{
		WORD	pos = data->BufferPos;

	if(IsDigit(data->locale, *(data->Contents+pos)))
	{
		while(pos > 0 && IsDigit(data->locale, *(data->Contents+pos-1)))
			pos--;
	}
	else
	{
		while(*(data->Contents+pos) != '\0' && !IsDigit(data->locale, *(data->Contents+pos)))
			pos++;
	}

	if(*(data->Contents+pos) == '\0')
	{
		pos = data->BufferPos;
		while(pos && !IsDigit(data->locale, *(data->Contents+pos)))
			pos--;

		while(pos > 0 && IsDigit(data->locale, *(data->Contents+pos-1)))
			pos--;

		if(!pos && !IsDigit(data->locale, *data->Contents))
		{
			pos = -1;
		}
	}
	return(pos);
}

UWORD NextWord (STRPTR text, UWORD x, struct Locale *locale)
{
	while(IsAlNum(locale, (UBYTE)text[x]))
		x++;

	while(text[x] != '\0' && !IsAlNum(locale, (UBYTE)text[x]))
		x++;

	return(x);
}

UWORD PrevWord (STRPTR text, UWORD x, struct Locale *locale)
{
	if(x)
		x--;

	while(x && !IsAlNum(locale, (UBYTE)text[x]))
		x--;

	while(x > 0 && IsAlNum(locale, (UBYTE)text[x-1]))
		x--;

	return(x);
}

VOID strcpyback (STRPTR dest, STRPTR src)
{
		UWORD	length;

	length = strlen(src)+1;
	dest = dest + length;
	src = src + length;

	length++;
	while(--length)
	{
		*--dest = *--src;
	}
}

VOID DeleteBlock (struct InstData *data)
{
	AddToUndo(data);
	if(BlockEnabled(data))
	{
		UWORD Blk_Start, Blk_Width;
		Blk_Start = (data->BlockStart < data->BlockStop) ? data->BlockStart : data->BlockStop;
		Blk_Width = abs(data->BlockStop-data->BlockStart);
		strcpy(data->Contents+Blk_Start, data->Contents+Blk_Start+Blk_Width);
		data->BufferPos = Blk_Start;
	}
}

VOID CopyBlock (struct InstData *data)
{
		struct MsgPort		*port;
		struct IOClipReq	*iorequest;
		UWORD Blk_Start, Blk_Width;

	if(data->Flags & FLG_Secret)
		return;

	if(BlockEnabled(data))
	{
		Blk_Start = (data->BlockStart < data->BlockStop) ? data->BlockStart : data->BlockStop;
		Blk_Width = abs(data->BlockStop-data->BlockStart);
	}
	else
	{
		Blk_Start = 0;
		Blk_Width = strlen(data->Contents);
	}

	if((port = CreateMsgPort()))
	{
		if((iorequest = (struct IOClipReq *)CreateIORequest(port, sizeof(struct IOClipReq))))
		{
			if(!OpenDevice("clipboard.device", 0, (struct IORequest *)iorequest, 0))
			{
				ULONG IFF_Head[] = { MAKE_ID('F','O','R','M'),
                             12 + ((Blk_Width + 1) & ~1),
                             MAKE_ID('F','T','X','T'),
                             MAKE_ID('C','H','R','S'),
                             Blk_Width
                           };

				iorequest->io_ClipID		= 0;
				iorequest->io_Offset		= 0;
				iorequest->io_Data		= (STRPTR)IFF_Head;
				iorequest->io_Length		= sizeof(IFF_Head);
				iorequest->io_Command	= CMD_WRITE;
				DoIO((struct IORequest *)iorequest);

				iorequest->io_Data		= data->Contents+Blk_Start;
				iorequest->io_Length		= Blk_Width;
				DoIO((struct IORequest *)iorequest);

				iorequest->io_Command	= CMD_UPDATE;
				DoIO((struct IORequest *)iorequest);

				CloseDevice((struct IORequest *)iorequest);
			}
			DeleteIORequest((struct IORequest *)iorequest);
		}
		DeleteMsgPort(port);
	}
}

VOID Paste (struct InstData *data)
{
	struct MsgPort		*port;
	struct IOClipReq	*iorequest;

	if((port = CreateMsgPort()))
	{
		if((iorequest = (struct IOClipReq *)CreateIORequest(port, sizeof(struct IOClipReq))))
		{
			if(!OpenDevice("clipboard.device", 0, (struct IORequest *)iorequest, 0))
			{
				ULONG IFF_Head[3];
				LONG	length;

				iorequest->io_ClipID		= 0;
				iorequest->io_Offset		= 0;
				iorequest->io_Data		= (STRPTR)IFF_Head;
				iorequest->io_Length		= sizeof(IFF_Head);
				iorequest->io_Command	= CMD_READ;
				DoIO((struct IORequest *)iorequest);
				length = IFF_Head[1]-4;

				if(iorequest->io_Actual == sizeof(IFF_Head) && *IFF_Head == MAKE_ID('F','O','R','M') && IFF_Head[2] == MAKE_ID('F','T','X','T') && length > 8)
				{
					iorequest->io_Length = 8;
					DoIO((struct IORequest *)iorequest);
					length -= 8;

					while(length > 0 && *IFF_Head != MAKE_ID('C','H','R','S'))
					{
						iorequest->io_Offset += IFF_Head[1];
						length -= IFF_Head[1]+8;
						DoIO((struct IORequest *)iorequest);
					}

					if(*IFF_Head == MAKE_ID('C','H','R','S'))
					{
						ULONG pastelength = IFF_Head[1];

						if(data->MaxLength && strlen(data->Contents)+pastelength > data->MaxLength-1)
						{
							DisplayBeep(NULL);
							pastelength = (data->MaxLength-1)-strlen(data->Contents);
						}

						data->Contents = (STRPTR)ExpandPool(data->Pool, data->Contents, pastelength);
						strcpyback(data->Contents+data->BufferPos+pastelength, data->Contents+data->BufferPos);
						iorequest->io_Length	= pastelength;
						iorequest->io_Data	= data->Contents+data->BufferPos;
						DoIO((struct IORequest *)iorequest);

						while(pastelength--)
						{
							if(data->Contents[data->BufferPos] == '\0')
								data->Contents[data->BufferPos] = '?';
							data->BufferPos++;
						}
					}
					else
					{
						DisplayBeep(NULL);
					}
				}
				else
				{
					DisplayBeep(NULL);
				}

				iorequest->io_Offset		= 0xfffffff;
				iorequest->io_Data		= NULL;
				DoIO((struct IORequest *)iorequest);

				CloseDevice((struct IORequest *)iorequest);
			}
			DeleteIORequest((struct IORequest *)iorequest);
		}
		DeleteMsgPort(port);
	}
}

ULONG ConvertKey (struct IntuiMessage *imsg)
{
		struct	InputEvent  event;
		UBYTE		code = 0;

	event.ie_NextEvent      = NULL;
	event.ie_Class          = IECLASS_RAWKEY;
	event.ie_SubClass       = 0;
	event.ie_Code           = imsg->Code;
	event.ie_Qualifier      = imsg->Qualifier;
	event.ie_EventAddress   = (APTR *) *((ULONG *)imsg->IAddress);

	MapRawKey(&event, &code, 1, NULL);
	return(code);
}

ULONG HandleInput(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
		struct InstData *data = (struct InstData *)INST_DATA(cl, obj);
		struct MUI_AreaData *ad = muiAreaData(obj);
		struct TextFont *Font = data->Font ? data->Font : ad->mad_Font;
		ULONG	result = 0;
		BOOL	movement = FALSE;
		BOOL	deletion = FALSE;
		BOOL	edited = FALSE;
		BOOL	FNC = FALSE;

	Object *focus = NULL;
	if(msg->muikey == MUIKEY_UP)
		focus = data->KeyUpFocus;
	else if(msg->muikey == MUIKEY_DOWN)
		focus = data->KeyDownFocus;

	if(focus && _win(obj))
	{
		set(_win(obj), MUIA_Window_ActiveObject, focus);
		result = MUI_EventHandlerRC_Eat;
	}
	else if(msg->imsg)
	{
		WORD StringLength = strlen(data->Contents);

		if(msg->imsg->Class == IDCMP_RAWKEY && msg->imsg->Code >= IECODE_KEY_CODE_FIRST && msg->imsg->Code <= IECODE_KEY_CODE_LAST)
		{
			if(data->Flags & FLG_Active)
			{
				if(!(data->Flags & FLG_BlockEnabled))
					data->BlockStart = data->BufferPos;

				BOOL input = TRUE;
				if(data->Flags & FLG_NoInput)
				{
					switch(msg->imsg->Code)
					{
						case 66:		/* Tab */
						case 65:		/* Backspace */
						case 70:		/* Delete */
							input = FALSE;
						break;
					}
				}

				if(input) switch(msg->imsg->Code)
				{
					case 66:		/* Tab */
						if(!(msg->imsg->Qualifier & IEQUALIFIER_RCOMMAND))
							return(0);

						if(!(edited = FileNameComplete(obj, (msg->imsg->Qualifier & (IEQUALIFIER_RSHIFT | IEQUALIFIER_LSHIFT)) ? TRUE : FALSE, data)))
							DisplayBeep(NULL);
						FNC = TRUE;
						break;

					case 78:		/* Right */
						if(data->BufferPos < StringLength)
						{
							if(msg->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
							{
								data->BufferPos = StringLength;
							}
							else
							{
								if(msg->imsg->Qualifier & (IEQUALIFIER_RALT | IEQUALIFIER_LALT))
								{
									data->BufferPos = NextWord(data->Contents, data->BufferPos, data->locale);
								}
								else
								{
									if(BlockEnabled(data) && !(msg->imsg->Qualifier & IEQUALIFIER_CONTROL))
									  data->BufferPos = MAX(data->BlockStart, data->BlockStop);
									else
                    data->BufferPos++;
								}
							}
						}
						movement = TRUE;
						break;

					case 79:		/* Left */
						if(data->BufferPos)
						{
							if(msg->imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
							{
								data->BufferPos = 0;
							}
							else
							{
								if(msg->imsg->Qualifier & (IEQUALIFIER_RALT | IEQUALIFIER_LALT))
								{
									data->BufferPos = PrevWord(data->Contents, data->BufferPos, data->locale);
								}
								else
								{
									if(BlockEnabled(data) && !(msg->imsg->Qualifier & IEQUALIFIER_CONTROL))
										data->BufferPos = MIN(data->BlockStart, data->BlockStop);

									if(data->BufferPos)
										data->BufferPos--;
								}
							}
						}
						movement = TRUE;
						break;

					case 65:		/* Backspace */
						if(BlockEnabled(data))
						{
							DeleteBlock(data);
						}
						else
						{
							if(data->BufferPos)
							{
								if(msg->imsg->Qualifier & (IEQUALIFIER_CONTROL | IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
								{
									AddToUndo(data);
									strcpy(data->Contents, data->Contents+data->BufferPos);
									data->BufferPos = 0;
								}
								else
								{
									if(msg->imsg->Qualifier & (IEQUALIFIER_RALT | IEQUALIFIER_LALT))
									{
											UWORD NewPos = PrevWord(data->Contents, data->BufferPos, data->locale);

										AddToUndo(data);
										strcpy(data->Contents+NewPos, data->Contents+data->BufferPos);
										data->BufferPos = NewPos;
									}
									else
									{
										strcpy(data->Contents+data->BufferPos-1, data->Contents+data->BufferPos);
										data->BufferPos--;
									}
								}
							}
						}
						deletion = TRUE;
						break;

					case 70:		/* Delete */
						if(BlockEnabled(data))
						{
							DeleteBlock(data);
						}
						else
						{
							if(data->BufferPos < StringLength)
							{
								if(msg->imsg->Qualifier & (IEQUALIFIER_CONTROL | IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
								{
									AddToUndo(data);
									*(data->Contents+data->BufferPos) = '\0';
								}
								else
								{
									if(msg->imsg->Qualifier & (IEQUALIFIER_RALT | IEQUALIFIER_LALT))
									{
										AddToUndo(data);
										strcpy(data->Contents+data->BufferPos, data->Contents+NextWord(data->Contents, data->BufferPos, data->locale));
									}
									else
									{
										strcpy(data->Contents+data->BufferPos, data->Contents+data->BufferPos+1);
									}
								}
							}
						}
						deletion = TRUE;
						break;

					default:
					{
						if(data->Popup && msg->muikey == MUIKEY_POPUP)
						{
							DoMethod(data->Popup, MUIM_Popstring_Open);
						}
						else
						{
							UBYTE	code = ConvertKey(msg->imsg);
							if((((code >= 32 && code <= 126) || code >= 160) && !(msg->imsg->Qualifier & IEQUALIFIER_RCOMMAND)) || (code && msg->imsg->Qualifier & IEQUALIFIER_CONTROL))
							{
								if(!(data->Flags & FLG_NoInput))
								{
									DeleteBlock(data);
									if((data->MaxLength == 0 || data->MaxLength-1 > strlen(data->Contents)) && Accept(code, data->Accept) && Reject(code, data->Reject))
									{
										data->Contents = (STRPTR)ExpandPool(data->Pool, data->Contents, 1);
										strcpyback(data->Contents+data->BufferPos+1, data->Contents+data->BufferPos);
										*(data->Contents+data->BufferPos) = code;
										data->BufferPos++;
										edited = TRUE;
									}
									else
									{
										DisplayBeep(NULL);
									}
								}
							}
							else
							{
								if(data->Flags & FLG_NoInput)
								{
									switch(code)
									{
										case '\r':
										case 'c':
										break;

										default:
											return(MUI_EventHandlerRC_Eat);
										break;
									}
								}

								switch(code)
								{
									case '\r':
										if(!(data->Flags & FLG_StayActive))
											set(_win(obj), MUIA_Window_ActiveObject, (data->Flags & FLG_AdvanceOnCr) ? (msg->imsg->Qualifier & (IEQUALIFIER_RSHIFT | IEQUALIFIER_LSHIFT) ?  MUIV_Window_ActiveObject_Prev : MUIV_Window_ActiveObject_Next) : MUIV_Window_ActiveObject_None);
										set(obj, MUIA_String_Acknowledge, data->Contents);
										return(MUI_EventHandlerRC_Eat);
										/* Skip the "un-block" code */

									case 'g':
									{
											UBYTE key = *(data->Contents+data->BufferPos);

										if(data->BufferPos < StringLength)
										{
											*(data->Contents+data->BufferPos) = IsLower(data->locale, key) ? ConvToUpper(data->locale, key) : ConvToLower(data->locale, key);
											data->BufferPos++;
											edited = TRUE;
										}
									}
									break;

									case 'G':
									{
											UWORD Stop = NextWord(data->Contents, data->BufferPos, data->locale);

										while(data->BufferPos < Stop)
										{
												UBYTE key = *(data->Contents+data->BufferPos);

											*(data->Contents+data->BufferPos) = IsLower(data->locale, key) ? ConvToUpper(data->locale, key) : ConvToLower(data->locale, key);
											data->BufferPos++;
											edited = TRUE;
										}
									}
									break;

									case 'c':
										CopyBlock(data);
										break;

									case 'x':
										AddToUndo(data);
										if(BlockEnabled(data))
										{
											CopyBlock(data);
											DeleteBlock(data);
										}
										else
										{
											*data->Contents = '\0';
											data->BufferPos = 0;
										}
										deletion = TRUE;
										break;

									case 'v':
										DeleteBlock(data);
										Paste(data);
										edited = TRUE;
										break;

									case 'i':
									{
											LONG	pos, cut;
											ULONG	result;

										if((pos = FindDigit(data)) >= 0)
										{
											if((cut = StrToLong(data->Contents+pos, (LONG *)&result)))
											{
													UBYTE string[12], format[12];

												result++;
                        MySPrintf(format, "%%0%ldlu", cut);
												MySPrintf(string, format, result);
												Overwrite(string, pos, cut, data);
												edited = TRUE;
											}
										}
										if(!edited)
											DisplayBeep(NULL);
										break;
									}

									case 'd':
									case '$':
									{
											LONG	pos, cut;
											ULONG	result;

										if((pos = FindDigit(data)) >= 0)
										{
											if((cut = StrToLong(data->Contents+pos, (LONG *)&result)))
											{
												if(result || code == '$')
												{
													UBYTE string[12], format2[12];
													STRPTR format = "%lx";
													if(code == 'd')
													{
														result--;
														format = format2;
														MySPrintf(format, "%%0%ldlu", cut);
													}
													MySPrintf(string, format, result);
													Overwrite(string, pos, cut, data);
													edited = TRUE;
												}
											}
										}
										if(!edited)
											DisplayBeep(NULL);
										break;
									}

									case '#':
									{
											LONG	cut = 0;
											UWORD pos = data->BufferPos;
											ULONG	result = 0;

										while(pos && IsHex(*(data->Contents+pos-1)))
											pos--;

										while(IsHex(*(data->Contents+pos+cut)))
										{
											result = (result << 4) + DecimalValue(*(data->Contents+pos+cut));
											cut++;
										}

										if(cut)
										{
												UBYTE string[12];

											MySPrintf(string, "%lu", result);
											Overwrite(string, pos, cut, data);
											edited = TRUE;
										}
										if(!edited)
											DisplayBeep(NULL);
										break;
									}

									case 'q':
									{
										STRPTR oldcontents = data->Contents;
										data->Contents = data->Original;
										data->Original = oldcontents;
										data->Flags |= FLG_Original;
										data->Flags &= ~FLG_BlockEnabled;
										data->BufferPos = strlen(data->Contents);
										edited = TRUE;
										break;
									}

									case 'z':
									case 'Z':
									{
										if(data->Undo && (((code == 'Z') && (data->Flags & FLG_RedoAvailable)) || ((code == 'z') && !(data->Flags & FLG_RedoAvailable))))
										{
												STRPTR	oldcontents = data->Contents;
												UWORD		oldpos = data->BufferPos;

											data->Contents = data->Undo;
											data->Undo = oldcontents;
											data->Flags ^= FLG_RedoAvailable;
											data->Flags &= ~FLG_BlockEnabled;
											data->BufferPos = data->UndoPos;
											data->UndoPos = oldpos;
											edited = TRUE;
										}
										else
										{
											DisplayBeep(NULL);
										}
										break;
									}

									default:
										msg->imsg->Qualifier &= ~IEQUALIFIER_RSHIFT;
										return(0);
								}
							}
						}
					}
					break;
				}

				if(data->FNCBuffer && !FNC)
				{
						struct FNCData *fncbuffer = data->FNCBuffer, *fncframe;

					while(fncbuffer)
					{
						fncframe = fncbuffer;
						fncbuffer = fncbuffer->next;
						MyFreePooled(data->Pool, fncframe);
					}
					data->FNCBuffer = NULL;
				}

				if(movement && msg->imsg->Qualifier & IEQUALIFIER_CONTROL)
						data->Flags |=  FLG_BlockEnabled;
				else	data->Flags &= ~FLG_BlockEnabled;

				if(data->Flags & FLG_BlockEnabled)
				{
					data->BlockStop = data->BufferPos;
				}

				if(deletion || edited)
				{
					struct TagItem tags[] =
					{
						{ MUIA_String_Contents, (ULONG)data->Contents },
						{ TAG_DONE, NULL }
					};
					DoSuperMethod(cl, obj, OM_SET, tags, NULL);
//					set(obj, MUIA_String_Contents, data->Contents);
				}

				MUI_Redraw(obj, MADF_DRAWUPDATE);
				result = MUI_EventHandlerRC_Eat;
			}
			else
			{
				if(data->CtrlChar && ConvertKey(msg->imsg) == data->CtrlChar)
				{
					set(_win(obj), MUIA_Window_ActiveObject, obj);
					result = MUI_EventHandlerRC_Eat;
				}
			}
		}
		else
		{
			if(msg->imsg->Class == IDCMP_MOUSEBUTTONS)
			{
				if(msg->imsg->Code == (IECODE_LBUTTON | IECODE_UP_PREFIX))
				{
					if(data->ehnode.ehn_Events & (IDCMP_MOUSEMOVE | IDCMP_INTUITICKS))
					{
						DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);
						data->ehnode.ehn_Events &= ~(IDCMP_MOUSEMOVE | IDCMP_INTUITICKS);
						DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehnode);
					}

#ifdef ALLOW_OUTSIDE_MARKING
					if((data->Flags & (FLG_Active|FLG_DragOutside)) == (FLG_Active|FLG_DragOutside))
					{
						Object *active;
						get(_win(obj), MUIA_Window_ActiveObject, &active);
						if(obj != active)
							kprintf("MUI error: %lx, %lx\n", active, obj);

						WORD x = ad->mad_Box.Left + ad->mad_addleft;
						WORD y = ad->mad_Box.Top  + ad->mad_addtop;
						WORD width = ad->mad_Box.Width - ad->mad_subwidth;
						WORD height = Font->tf_YSize;

						if(!(msg->imsg->MouseX >= x && msg->imsg->MouseX < x+width && msg->imsg->MouseY >= y && msg->imsg->MouseY < y+height))
						{
							kprintf("Detected LMB+up outside (drag: %ld)\n", data->Flags & FLG_DragOutside ? 1:0);
							set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
						}
					}
#endif
				}
				else if(msg->imsg->Code == IECODE_LBUTTON)
				{
					WORD x = ad->mad_Box.Left + ad->mad_addleft;
					WORD y = ad->mad_Box.Top  + ad->mad_addtop;
					WORD width = ad->mad_Box.Width - ad->mad_subwidth;
					WORD height = Font->tf_YSize;

					if(msg->imsg->MouseX >= x && msg->imsg->MouseX < x+width && msg->imsg->MouseY >= y && msg->imsg->MouseY < y+height)
					{
							WORD offset = msg->imsg->MouseX - x;

						offset -= AlignOffset(obj, data);
						data->BufferPos = data->DisplayPos + MyTextFit(Font, data->Contents+data->DisplayPos, StringLength-data->DisplayPos, offset+1, 1);

						if(DoubleClick(data->StartSecs, data->StartMicros, msg->imsg->Seconds, msg->imsg->Micros))
								data->ClickCount++;
						else	data->ClickCount = 0;
						data->StartSecs	= msg->imsg->Seconds;
						data->StartMicros	= msg->imsg->Micros;

						switch(data->ClickCount)
						{
							case 0:
								if(!(data->Flags & FLG_BlockEnabled && msg->imsg->Qualifier & IEQUALIFIER_CONTROL))
									data->BlockStart = data->BufferPos;
								break;

							case 1:
								if(*(data->Contents+data->BufferPos) != '\0')
								{
										UWORD start = data->BufferPos,
												stop  = data->BufferPos;
										BOOL	alpha	= IsAlNum(data->locale, (UBYTE)*(data->Contents+data->BufferPos));

									while(start > 0 && alpha == IsAlNum(data->locale, (UBYTE)*(data->Contents+start-1)))
										start--;

									while(alpha == IsAlNum(data->locale, (UBYTE)*(data->Contents+stop)) && *(data->Contents+stop) != '\0')
										stop++;

									data->BlockStart = start;
									data->BufferPos = stop;
								}
								break;

							case 2:
								data->BlockStart = 0;
								data->BufferPos = strlen(data->Contents);
								break;

							case 3:
								data->BlockStart = data->BufferPos;
								data->ClickCount = 0;
								break;
						}
						data->BlockStop = data->BufferPos;
						data->Flags |= FLG_BlockEnabled;

						DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);
						data->ehnode.ehn_Events |= (IDCMP_MOUSEMOVE | IDCMP_INTUITICKS);
						DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehnode);

						if(data->Flags & FLG_Active)
								MUI_Redraw(obj, MADF_DRAWUPDATE);
						else	set(_win(obj), MUIA_Window_ActiveObject, obj);
						result = MUI_EventHandlerRC_Eat;
					}
					else
					{
						data->ClickCount = 0;
						if(data->Flags & FLG_Active && !(data->Flags & FLG_StayActive))
						{
#ifdef ALLOW_OUTSIDE_MARKING
							kprintf("Clicked outside gadget\n");
							data->Flags |= FLG_DragOutside;

							DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);
							data->ehnode.ehn_Events |= IDCMP_MOUSEMOVE;
							DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehnode);
#else
							set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
#endif
						}
					}
				}
			}
			else
			{
#ifdef ALLOW_OUTSIDE_MARKING
				if(msg->imsg->Class == IDCMP_MOUSEMOVE)
				{
					data->Flags &= ~FLG_DragOutside;
					kprintf("Detected drag\n");
				}
#endif
				if((msg->imsg->Class == IDCMP_MOUSEMOVE || msg->imsg->Class == IDCMP_INTUITICKS) && data->Flags & FLG_Active)
				{
						WORD x, width, mousex;

					x = ad->mad_Box.Left + ad->mad_addleft;
					mousex = msg->imsg->MouseX - AlignOffset(obj, data);
					width = ad->mad_Box.Width - ad->mad_subwidth;

					switch(data->ClickCount)
					{
						case 0:
						{
							if(mousex < x)
							{
								if(data->DisplayPos)
									data->DisplayPos--;
								data->BufferPos = data->DisplayPos;
							}
							else
							{
								if(mousex >= x+width)
								{
									if(data->DisplayPos < StringLength)
									{
										data->DisplayPos++;
										data->BufferPos = data->DisplayPos + MyTextFit(Font, data->Contents+data->DisplayPos, StringLength-data->DisplayPos, ad->mad_Box.Width - ad->mad_subwidth, 1);
									}
									else
									{
										data->BufferPos = StringLength;
									}
								}
								else
								{
										WORD offset = mousex - x;

/*									if(offset < 0)
										data->BufferPos = 0;
									else
*/									data->BufferPos = data->DisplayPos + MyTextFit(Font, data->Contents+data->DisplayPos, StringLength-data->DisplayPos, offset+1, 1);
								}
							}
							data->BlockStop = data->BufferPos;
							MUI_Redraw(obj, MADF_DRAWUPDATE);
						}
						break;

						case 1:
						{
								WORD	offset = mousex - x,
										newpos;

							if(mousex < x && data->DisplayPos)
							{
								data->DisplayPos--;
								newpos = data->DisplayPos;
							}
							else
							{
//								offset -= AlignOffset(obj, data);
								if(offset > 0)
									newpos = data->DisplayPos + MyTextFit(Font, data->Contents+data->DisplayPos, StringLength-data->DisplayPos, offset+1, 1);
							}

							if(newpos >= data->BlockStart)
							{
								while(IsAlNum(data->locale, (UBYTE)*(data->Contents+newpos)))
									newpos++;
							}
							else
							{
								while(newpos > 0 && IsAlNum(data->locale, (UBYTE)*(data->Contents+newpos-1)))
									newpos--;
							}

							if(data->BufferPos != newpos)
							{
								data->BlockStop = data->BufferPos = newpos;
								MUI_Redraw(obj, MADF_DRAWUPDATE);
							}
						}
						break;
					}
				}
			}
		}
	}
	return(result);
}
