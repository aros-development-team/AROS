/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2013 by BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id$

***************************************************************************/

#include <string.h>
#include <stdio.h>

#include <dos/dosextens.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/dos.h>

#ifdef AROS_ABI_V1
#include <aros/config.h>
#endif

#include "private.h"

#include "Debug.h"

BOOL OverwriteA(STRPTR text, UWORD x, UWORD length, UWORD ptrn_length, struct InstData *data)
{
  BOOL result = TRUE;

  ENTER();

  if(length < ptrn_length)
  {
    ULONG expand = ptrn_length-length;

    if(data->MaxLength && strlen(data->Contents)+expand > (unsigned int)(data->MaxLength-1))
    {
      ptrn_length -= expand;
      ptrn_length += (data->MaxLength-1)-strlen(data->Contents);
      result = FALSE;
    }
    if(ExpandContentString(&data->Contents, expand) == TRUE)
      memmove(data->Contents+x+ptrn_length, data->Contents+x+length, strlen(data->Contents+x+length)+1);
    else
      E(DBF_ALWAYS, "content expansion by %ld bytes failed", expand);
  }
  else
  {
    memmove(data->Contents+x+ptrn_length, data->Contents+x+length, strlen(data->Contents+x+length)+1);
  }
  CopyMem(text, data->Contents+x, ptrn_length);

  if(data->BufferPos >= x)
  {
    data->BufferPos += ptrn_length-length;
    if(data->BufferPos < x)
      data->BufferPos = x;
  }

  if(result == FALSE)
    DisplayBeep(NULL);

  RETURN(result);
  return result;
}

BOOL Overwrite(STRPTR text, UWORD x, UWORD length, struct InstData *data)
{
  BOOL result;

  ENTER();

  result = OverwriteA(text, x, length, strlen(text), data);

  RETURN(result);
  return result;
}

WORD VolumeStart(STRPTR text, WORD pos)
{
  BOOL searching = TRUE;

  ENTER();

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
        break;
    }
  }

  RETURN(pos);
  return pos;
}

LONG mFileNameStart(struct MUIP_BetterString_FileNameStart *msg)
{
  STRPTR buffer = msg->buffer;
  LONG pos = msg->pos;

  ENTER();

  while(pos && buffer[pos] != ':')
    pos--;

  if(buffer[pos] == ':')
    pos = VolumeStart(buffer, pos);
  else
    pos = MUIR_BetterString_FileNameStart_Volume;

  RETURN(pos);
  return pos;
}

VOID InsertFileName(UWORD namestart, struct InstData *data)
{
  struct ExAllData *ead1 = &data->FNCBuffer->buffer;
  struct ExAllData *ead2;
  struct FNCData *fncframe;
  struct FNCData *fncframe1 = data->FNCBuffer;
  UWORD entrynum;
  UWORD findnum = data->FileNumber;
  char tmpname[32];

  ENTER();

  do
  {
    entrynum = 0;
    fncframe = data->FNCBuffer;
    ead2 = &fncframe->buffer;
    while(ead2 != NULL)
    {
      //if(CmpStrings(ead1->ed_Name, ead2->ed_Name) > 0)
      if(strcmp((const char *)ead1->ed_Name, (const char *)ead2->ed_Name) > 0)
        entrynum++;

      ead2 = ead2->ed_Next;
      if(ead2 == NULL && fncframe->next != NULL)
      {
        fncframe = fncframe->next;
        ead2 = &fncframe->buffer;
      }
    }

    if(entrynum != findnum)
      ead1 = ead1->ed_Next;
    if(ead1 == NULL && fncframe1->next != NULL)
    {
      fncframe1 = fncframe1->next;
      ead1 = &fncframe1->buffer;
    }
  }
  while(entrynum != findnum);

  snprintf(tmpname, sizeof(tmpname), "%s%s", ead1->ed_Name, ead1->ed_Type == 2 ? "/" : " ");

  Overwrite(tmpname, namestart, data->BufferPos-namestart, data);

  LEAVE();
}

BOOL FileNameComplete(Object *obj, BOOL backwards, struct InstData *data)
{
  BOOL edited = FALSE;

  ENTER();

  if(data->FNCBuffer != NULL)
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
    LONG pos = DoMethod(obj, MUIM_BetterString_FileNameStart, data->Contents, data->BufferPos);

    switch(pos)
    {
      case MUIR_BetterString_FileNameStart_Volume:
      {
        struct DosList *dl;
        STRPTR volumeName = NULL;
        char tmpBuffer[256];
        UWORD cut;

        pos = VolumeStart(data->Contents, data->BufferPos);
        if((cut = data->BufferPos-pos) != 0)
        {
          dl = LockDosList(LDF_READ|LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS);
          while((dl = NextDosEntry(dl, LDF_READ|LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS)) != NULL)
          {
            #ifdef __AROS__
              #ifdef AROS_ABI_V1
              strlcpy(tmpBuffer, AROS_BSTR_ADDR(dl->dol_Name), AROS_BSTR_strlen(dl->dol_Name));
              #else
              strlcpy(tmpBuffer, dl->dol_Ext.dol_AROS.dol_DevName, sizeof tmpBuffer);
              #endif
            #else
            // dol_Name is a BSTR, we have to convert it to a regular C string
            char *bstr = BADDR(dl->dol_Name);

            // a BSTR cannot exceed 255 characters, hence the buffer size of 256 is enough in any case
            strlcpy(tmpBuffer, &bstr[1], (unsigned char)bstr[0]);
            #endif

            if(Strnicmp(tmpBuffer, data->Contents+pos, cut) == 0)
            {
              volumeName = tmpBuffer;
              break;
            }
          }

          if(volumeName != NULL)
          {
            if(OverwriteA(volumeName, pos, cut, strlen(volumeName)+1, data))
              data->Contents[data->BufferPos-1] = ':';
            edited = TRUE;
          }
          UnLockDosList(LDF_READ|LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS);
        }
      }
      break;

      default:
      {
        struct FNCData *fncbuffer;
        struct FNCData *fncframe;
        struct ExAllControl *control;
        BPTR dirlock;
        char pattern[42];
        UWORD namestart = data->BufferPos;
        char oldletter = '\0';
        BOOL filename = TRUE;

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
          strlcpy(pattern, data->Contents+namestart, sizeof(pattern));
          strlcat(pattern, "~(#?.info)", sizeof(pattern));

          oldletter = data->Contents[namestart];
          data->Contents[namestart] = '\0';

          if((fncbuffer = (struct FNCData *)SharedPoolAlloc(4100)) != NULL)
          {
            fncbuffer->next = NULL;

            if((control = (struct ExAllControl *)AllocDosObject(DOS_EXALLCONTROL, NULL)))
            {
              char tokenized[sizeof(pattern) * 2 + 2];

              if(ParsePatternNoCase(pattern, tokenized, sizeof(tokenized)) != -1)
                control->eac_MatchString = tokenized;

              if((dirlock = Lock(data->Contents+pos, ACCESS_READ)))
              {
                UWORD entries = 0;

                fncframe = fncbuffer;
                while(fncframe && ExAll(dirlock, &fncframe->buffer, 4096, ED_TYPE, control))
                {
                  entries += control->eac_Entries;

                  if((fncframe->next = (struct FNCData *)SharedPoolAlloc(4100)))
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
              else
              {
                SharedPoolFree(fncbuffer);
              }

              FreeDosObject(DOS_EXALLCONTROL, (APTR)control);
            }
          }
        }

        if(edited == FALSE)
          data->Contents[namestart] = oldletter;
      }
      break;
    }
  }

  RETURN(edited);
  return edited;
}
