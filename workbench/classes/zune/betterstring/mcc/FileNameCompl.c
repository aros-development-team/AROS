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

 $Id: FileNameCompl.c,v 1.1 2005/04/21 20:52:04 damato Exp $

***************************************************************************/

#include <stdio.h>
#include <string.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "BetterString_mcc.h"
#include "private.h"

BOOL OverwriteA (STRPTR text, UWORD x, UWORD length, UWORD ptrn_length, struct InstData *data)
{
		BOOL result = TRUE;

	if(length < ptrn_length)
	{
		UWORD expand = ptrn_length-length;
		
    if(data->MaxLength && strlen(data->Contents)+expand > data->MaxLength-1)
		{
			ptrn_length -= expand;
			ptrn_length += (data->MaxLength-1)-strlen(data->Contents);
			result = FALSE;
		}
		data->Contents = (STRPTR)ExpandPool(data->Pool, data->Contents, expand);
		strcpyback(data->Contents+x+ptrn_length, data->Contents+x+length);
	}
	else
	{
		strcpy(data->Contents+x+ptrn_length, data->Contents+x+length);
	}
	CopyMem(text, data->Contents+x, ptrn_length);

	if(data->BufferPos >= x)
	{
		data->BufferPos += ptrn_length-length;
		if(data->BufferPos < x)
			data->BufferPos = x;
	}

	if(!result)
		DisplayBeep(NULL);

	return(result);
}

BOOL Overwrite (STRPTR text, UWORD x, UWORD length, struct InstData *data)
{
	return(OverwriteA(text, x, length, strlen(text), data));
}

WORD VolumeStart (STRPTR text, WORD pos)
{
		BOOL searching = TRUE;

	while(pos > 0 && searching)
	{
		switch(*(text+pos-1))
		{
			case '"':
			case '>':
			case ' ':
			case '=':
				searching = FALSE;
				break;
			default:
				pos--;
		}
	}
	return(pos);
}

LONG FileNameStart (struct MUIP_BetterString_FileNameStart *msg)
{
	STRPTR buffer = msg->buffer;
	LONG pos = msg->pos;

	while(pos && buffer[pos] != ':')
		pos--;

	if(buffer[pos] == ':')
			pos = VolumeStart(buffer, pos);
	else	pos = MUIR_BetterString_FileNameStart_Volume;

	return(pos);
}

VOID InsertFileName (UWORD namestart, struct InstData *data)
{
		struct ExAllData	*ead1 = &data->FNCBuffer->buffer;
		struct ExAllData	*ead2;
		struct FNCData		*fncframe;
		struct FNCData		*fncframe1 = data->FNCBuffer;
		UWORD entrynum;
		UWORD findnum = data->FileNumber;
		UBYTE	tmpname[32];

	do	{

		entrynum = 0;
		fncframe = data->FNCBuffer;
		ead2 = &fncframe->buffer;
		while(ead2)
		{
			//if(CmpStrings(ead1->ed_Name, ead2->ed_Name) > 0)
      if(strcmp(ead1->ed_Name, ead2->ed_Name) > 0)
				entrynum++;

			ead2 = ead2->ed_Next;
			if(!ead2 && fncframe->next)
			{
				fncframe = fncframe->next;
				ead2 = &fncframe->buffer;
			}
		}

		if(entrynum != findnum)
			ead1 = ead1->ed_Next;
		if(!ead1 && fncframe1->next)
		{
			fncframe1 = fncframe1->next;
			ead1 = &fncframe1->buffer;
		}
	}	while(entrynum != findnum);

	strcpy(tmpname, ead1->ed_Name);
	strcat(tmpname, ead1->ed_Type == 2 ? "/" : " ");

	Overwrite(tmpname, namestart, data->BufferPos-namestart, data);
}

BOOL FileNameComplete (Object *obj, BOOL backwards, struct InstData *data)
{
	BOOL edited = FALSE;
	if(data->FNCBuffer)
	{
		if(data->FileEntries == 1)
		{
			DisplayBeep(NULL);
		}
		else
		{
			if(backwards)
			{
				if(--data->FileNumber < 0)
					data->FileNumber = data->FileEntries-1;
			}
			else
			{
				if(++data->FileNumber >= data->FileEntries)
					data->FileNumber = 0;
			}
		}
		InsertFileName(data->FileNameStart, data);
		edited = TRUE;
	}
	else
	{
		LONG pos;
		switch(pos = DoMethod(obj, MUIM_BetterString_FileNameStart, data->Contents, data->BufferPos))
		{
			case MUIR_BetterString_FileNameStart_Volume:
			{
				struct DosList *dl;
				STRPTR VolumeName = NULL;
				UWORD cut;

				pos = VolumeStart(data->Contents, data->BufferPos);
				if((cut = data->BufferPos-pos))
				{
					dl = LockDosList(LDF_READ|LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS);
					while((dl = NextDosEntry(dl, LDF_READ|LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS)))
					{
					#ifdef __AROS__
						STRPTR NodeName = dl->dol_DevName;
					#else
						STRPTR NodeName = (STRPTR)((dl->dol_Name << 2)+1);
    	    	    	    	    	#endif
						if(!Strnicmp(NodeName, data->Contents+pos, cut))
						{
							VolumeName = NodeName;
							break;
						}
					}

					if(VolumeName)
					{
						if(OverwriteA(VolumeName, pos, cut, *(VolumeName-1)+1, data))
							data->Contents[data->BufferPos-1] = ':';
						edited = TRUE;
					}
					UnLockDosList(LDF_READ|LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS);
				}
			}
			break;

			default:
			{
					struct	FNCData			*fncbuffer;
					struct	FNCData			*fncframe;
					struct	ExAllControl	*control;
					BPTR		dirlock;
					UBYTE		pattern[42];
					UWORD		namestart = data->BufferPos;
					UBYTE		oldletter;
					BOOL		filename = TRUE;

				while(filename)
				{
					switch(*(data->Contents+namestart-1))
					{
						case '/':
						case ':':
							filename = FALSE;
							break;

						default:
							namestart--;
							break;
					}
				}
				if((data->BufferPos-namestart) < 32)
				{
					strncpy(pattern, data->Contents+namestart, data->BufferPos-namestart);
					oldletter = data->Contents[namestart];
					strcpy(pattern+(data->BufferPos-namestart), "~(#?.info)");
					data->Contents[namestart] = '\0';

					if((fncbuffer = (struct FNCData *)MyAllocPooled(data->Pool, 4100)))
					{
						fncbuffer->next = NULL;

						if((control = (struct ExAllControl *)AllocDosObject(DOS_EXALLCONTROL, NULL)))
						{
								UBYTE tokenized[80];

							if(ParsePatternNoCase(pattern, tokenized, 40) != -1)
								control->eac_MatchString = tokenized;

							if((dirlock = Lock(data->Contents+pos, ACCESS_READ)))
							{
  							UWORD entries = 0;

								fncframe = fncbuffer;
								while(fncframe && ExAll(dirlock, &fncframe->buffer, 4096, ED_TYPE, control))
								{
									entries += control->eac_Entries;
									
                  if((fncframe->next = (struct FNCData *)MyAllocPooled(data->Pool, 4100)))
										fncframe->next->next = NULL;
									
                  fncframe = fncframe->next;
								}
								control->eac_Entries += entries;

								data->FileNumber = backwards ? control->eac_Entries-1 : 0;
								data->FileEntries = control->eac_Entries;
								data->FileNameStart = namestart;
								data->FNCBuffer = fncbuffer;

								if(control->eac_Entries)
								{
									data->Contents[namestart] = oldletter;
									InsertFileName(namestart, data);
									edited = TRUE;
								}
								UnLock(dirlock);
							}
							FreeDosObject(DOS_EXALLCONTROL, (APTR)control);
						}
					}
				}
				if(!edited)
					data->Contents[namestart] = oldletter;
			}
			break;
		}
	}
	return(edited);
}
