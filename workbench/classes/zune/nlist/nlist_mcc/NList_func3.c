/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2014 NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

#include <string.h>
#include <ctype.h>

#include <dos/dos.h>
#include <exec/memory.h>
#include <libraries/iffparse.h>
#include <proto/console.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include "private.h"

#include "NList_func.h"

IPTR MyCallHookPkt(Object *obj,BOOL hdata,struct Hook *hook,APTR object,APTR message)
{
  if (hdata)
  { IPTR retval;
    APTR h_Data = hook->h_Data;
    if (!h_Data)
      hook->h_Data = obj;
    retval = CallHookPkt(hook,object,message);
    if (!h_Data)
      hook->h_Data = h_Data;
    return (retval);
  }
  else
    return (CallHookPkt(hook,object,message));
}

#ifdef __AROS__
/* AROS uses a macro to handle this */
#else
IPTR STDARGS VARARGS68K MyCallHookPktA(Object *obj, struct Hook *hook, ...)
{
  IPTR ret;
  VA_LIST va;

  VA_START(va, hook);
  ret = CallHookPkt(hook, obj, VA_ARG(va, APTR));
  VA_END(va);

  return ret;
}
#endif

LONG DeadKeyConvert(struct NLData *data,struct IntuiMessage *msg,STRPTR buf,LONG bufsize,struct KeyMap *kmap)
{
  int posraw,pos,postext = 0;
  STRPTR text = buf;

  if (msg->Class != IDCMP_RAWKEY)
    return (-2);

  text[0] = '\0';
/*
 *   if (msg->Qualifier)
 *   { D(bug("Qual:"));
 *     if (msg->Qualifier & IEQUALIFIER_LSHIFT)         D(bug("lshift "));
 *     if (msg->Qualifier & IEQUALIFIER_RSHIFT)         D(bug("rshift "));
 *     if (msg->Qualifier & IEQUALIFIER_CAPSLOCK)       D(bug("capslock "));
 *     if (msg->Qualifier & IEQUALIFIER_CONTROL)        D(bug("ctrl "));
 *     if (msg->Qualifier & IEQUALIFIER_LALT)           D(bug("lalt "));
 *     if (msg->Qualifier & IEQUALIFIER_RALT)           D(bug("ralt "));
 *     if (msg->Qualifier & IEQUALIFIER_LCOMMAND)       D(bug("lcommand "));
 *     if (msg->Qualifier & IEQUALIFIER_RCOMMAND)       D(bug("rcommand "));
 *     if (msg->Qualifier & IEQUALIFIER_NUMERICPAD)     D(bug("numpad "));
 *     if (msg->Qualifier & IEQUALIFIER_REPEAT)         D(bug("repeat "));
 *     if (msg->Qualifier & IEQUALIFIER_MIDBUTTON)      D(bug("mbutton "));
 *     if (msg->Qualifier & IEQUALIFIER_RBUTTON)        D(bug("rbutton "));
 *     if (msg->Qualifier & IEQUALIFIER_LEFTBUTTON)     D(bug("lbutton "));
 *     D(bug("\n"));
 *   }
 */
  switch (msg->Code & 0x7F)
  {
    case 0x41 : strlcpy(text,"bs", bufsize); break;
    case 0x42 : strlcpy(text,"tab", bufsize); break;
    case 0x43 : strlcpy(text,"enter", bufsize); break;
    case 0x44 : strlcpy(text,"return", bufsize); break;
    case 0x45 : strlcpy(text,"esc", bufsize); break;
    case 0x46 : strlcpy(text,"del", bufsize); break;
    case 0x4C : strlcpy(text,"up", bufsize); break;
    case 0x4D : strlcpy(text,"down", bufsize); break;
    case 0x4E : strlcpy(text,"right", bufsize); break;
    case 0x4F : strlcpy(text,"left", bufsize); break;
    case 0x50 : strlcpy(text,"f1", bufsize); break;
    case 0x51 : strlcpy(text,"f2", bufsize); break;
    case 0x52 : strlcpy(text,"f3", bufsize); break;
    case 0x53 : strlcpy(text,"f4", bufsize); break;
    case 0x54 : strlcpy(text,"f5", bufsize); break;
    case 0x55 : strlcpy(text,"f6", bufsize); break;
    case 0x56 : strlcpy(text,"f7", bufsize); break;
    case 0x57 : strlcpy(text,"f8", bufsize); break;
    case 0x58 : strlcpy(text,"f9", bufsize); break;
    case 0x59 : strlcpy(text,"f10", bufsize); break;
    case 0x5F : strlcpy(text,"help", bufsize); break;
    default:
      data->ievent.ie_NextEvent = NULL;
      data->ievent.ie_Class = IECLASS_RAWKEY;
      data->ievent.ie_SubClass = 0;
      data->ievent.ie_Code = msg->Code;
      data->ievent.ie_Qualifier = 0;
      data->ievent.ie_position.ie_addr = *((APTR *) msg->IAddress);
      posraw = RawKeyConvert(&data->ievent,text,bufsize-postext-1,kmap);
      if (posraw >= 0)
        text[posraw+postext] = '\0';
      if (posraw > 0)
      {
        ULONG  realc;
        data->rawtext[posraw] = '\0';
        for (pos = 0; pos < posraw; pos++)
        {
          realc = data->rawtext[pos];
/*D(bug("RAWKEY KEY=%ld \n",realc));*/
          if ((realc <= 0x1F)||((realc >= 0x80)&&(realc < 0xa0)))
            data->rawtext[pos] = 0x7f;
        }
      }
      break;
  }
  return ((LONG)strlen(buf));
}


/*static char *stpncpy_noesc(char *to,const char *from,int len)*/
static char *stpncpy_noesc(char *to,char *from,int len)
{
  register char *to2 = to;

  while (*from && (len > 0))
  { if (*from == '\033')
    { from++;
      len--;
      if ((*from == 'P') || (*from == 'I') || (*from == 'O') || (*from == 'o'))
      { from++;
        len--;
        if (*from == '[')
        { while ((*from) && (*from != ']'))
          { from++;
            len--;
          }
          if (*from == ']')
          { from++;
            len--;
          }
        }
      }
      else if (*from)
      { from++;
        len--;
      }
    }
    else
    { *to2++ = *from++;
      len--;
    }
  }
  *to2 = '\0';
  return (to2);
}

#define FORMAT_TEMPLATE "DELTA=D/N,PREPARSE=P/K,COL=C/N,BAR/S,TBAR/S,NOBAR=NB/S,SIMPLEBAR=SBAR/S,"\
                        "NOTITLEBUTTON=NOTB/S,WEIGHT=W/N,MINWIDTH=MIW/N,MAXWIDTH=MAW/N,"\
                        "COLWIDTH=CW/N,MINCOLWIDTH=MICW/N,MAXCOLWIDTH=MACW/N,"\
                        "PIXWIDTH=PW/N,MINPIXWIDTH=MIPW/N,MAXPIXWIDTH=MAPW/N,"\
                        "PARTCOLSUBST=PCS/K\n"

struct parse_format
{
  LONG *delta;
  LONG *preparse;
  LONG *col;
  LONG bar;
  LONG tbar;
  LONG nobar;
  LONG sbar;
  LONG notb;
  LONG *weight;
  LONG *minwidth;
  LONG *maxwidth;
  LONG *colwidth;
  LONG *mincolwidth;
  LONG *maxcolwidth;
  LONG *pixwidth;
  LONG *minpixwidth;
  LONG *maxpixwidth;
  LONG *partcolsubst;
};


void NL_Free_Format(struct NLData *data)
{
  if (data->cols)
  { WORD column = 0;
    while (column < data->numcols)
    {
      if(data->cols[column].preparse != NULL)
        FreeVecPooled(data->Pool, data->cols[column].preparse);

      column++;
    }
    FreeVecPooled(data->Pool, data->cols);
  }
  data->cols = NULL;
  data->numcols = data->numcols2 = 0;
  data->format_chge = 1;
}


BOOL NL_Read_Format(struct NLData *data,char *strformat,BOOL oldlist)
{
  LONG column,colmax,pos1,pos2,col = 0;
  char *sf = NULL;
  struct RDArgs *rdargs,*ptr;
  struct parse_format Line;
  struct colinfo *tmpcols;

  if (strformat && (sf = AllocVecPooled(data->Pool, strlen(strformat)+4)) != NULL)
  {
    if((ptr = AllocDosObject(DOS_RDARGS, NULL)))
    {
      colmax = 1;
      pos2 = 0;
      while (strformat[pos2] != '\0')
      { if (strformat[pos2] == ',')
          colmax++;
        pos2++;
      }
      if ((colmax > 0) && (colmax < DISPLAY_ARRAY_MAX) && (tmpcols = AllocVecPooled(data->Pool, (colmax+1)*sizeof(struct colinfo))) != NULL)
      {
        NL_Free_Format(data);
        data->cols = tmpcols;
        data->numcols = data->numcols2 = colmax;
        column = 0;
        while (column < colmax)
        {
          data->cols[column].c = &(data->cols[column]);
          data->cols[column].preparse = NULL;
          data->cols[column].colwidthmax = (WORD) -1;
          data->cols[column].colwidthbiggest = (WORD) -1;
          data->cols[column].colwidthbiggestptr = (LONG) -2;
          data->cols[column].delta = (WORD) 4;
          data->cols[column].col = (WORD) column;
          data->cols[column].userwidth = (WORD) -1;
          data->cols[column].titlebutton = (WORD) TRUE;
          if ((column == colmax-1) && !(oldlist && (column == 0)))
            data->cols[column].width = (WORD) 100;
          else
            data->cols[column].width = (WORD) -1;
          data->cols[column].minwidth = (WORD) 5;
          data->cols[column].maxwidth = (WORD) 0;
          data->cols[column].mincolwidth = (WORD) 0;
          data->cols[column].maxcolwidth = (WORD) 0;
          data->cols[column].minpixwidth = (WORD) 6;
          data->cols[column].maxpixwidth = (WORD) 0;
          data->cols[column].bar = (BYTE) 2;
          data->cols[column].width_type = (BYTE) CI_PERCENT;
          data->cols[column].partcolsubst = PCS_DISABLED;
          column++;
        }
        column = 0;
        pos2 = 0;
        while (strformat[pos2] && (column < colmax))
        { pos1 = 0;
          while ((strformat[pos2] != ',') && (strformat[pos2] != '\0'))
            sf[pos1++] = strformat[pos2++];
          sf[pos1++] = '\n';
          sf[pos1++] = '\0';
          if (strformat[pos2] != '\0')
            pos2++;
/*D(bug("col %ld ->%s",column,sf));*/

          ptr->RDA_Source.CS_Buffer = sf;
          ptr->RDA_Source.CS_Length = strlen(sf);
          ptr->RDA_Source.CS_CurChr = 0;
          ptr->RDA_DAList = 0;
          ptr->RDA_Buffer = NULL;
          ptr->RDA_BufSiz = 0L;
          ptr->RDA_ExtHelp = NULL;
          ptr->RDA_Flags = 0L;

          // clear the Line structure
          memset(&Line, 0, sizeof(Line));

          if((rdargs = ReadArgs(FORMAT_TEMPLATE, (APTR)&Line, ptr)))
          {
            if (Line.delta)    data->cols[column].delta = (WORD) *Line.delta;
            if (Line.preparse)
            {
              int len = strlen((char *)Line.preparse)+2;
              if((data->cols[column].preparse = AllocVecPooled(data->Pool,len)) != NULL)
                strlcpy(data->cols[column].preparse, (char *)Line.preparse, len);
            }
            if (Line.col)      data->cols[column].col = (WORD) *Line.col;
            if (Line.tbar)     data->cols[column].bar = (WORD) 2;
            if (Line.bar)      data->cols[column].bar = (WORD) 1;
            if (Line.nobar)    data->cols[column].bar = (WORD) 0;
            if (Line.sbar)     data->cols[column].bar |= (WORD) 4;
            if (Line.notb)     data->cols[column].titlebutton = (WORD) FALSE;

            if (Line.weight)
            {
              data->cols[column].width = (WORD) *Line.weight;
              data->cols[column].width_type = (BYTE) CI_PERCENT;
              if ((column == colmax-1) && !(oldlist && (column == 0)) && (data->cols[column].width == -1))
                data->cols[column].width = (WORD) 100;
              else if (data->cols[column].width < 0)
                data->cols[column].width = (WORD) -1;
            }
            if (Line.colwidth)
            { data->cols[column].width = (WORD) *Line.colwidth;
              data->cols[column].width_type = (BYTE) CI_COL;
              if (data->cols[column].width < 1)
                data->cols[column].width = 1;
            }
            if (Line.pixwidth)
            { data->cols[column].width = (WORD) *Line.pixwidth;
              data->cols[column].width_type = (BYTE) CI_PIX;
              if (data->cols[column].width < 4)
                data->cols[column].width = 4;
            }

            if (Line.minwidth)
              data->cols[column].minwidth = (WORD) *Line.minwidth;
            if (Line.maxwidth)
              data->cols[column].maxwidth = (WORD) *Line.maxwidth;

            if (Line.mincolwidth)
              data->cols[column].mincolwidth = (WORD) *Line.mincolwidth;
            if (Line.maxcolwidth)
              data->cols[column].maxcolwidth = (WORD) *Line.maxcolwidth;

            if (Line.minpixwidth)
              data->cols[column].minpixwidth = (WORD) *Line.minpixwidth;
            if (Line.maxpixwidth)
              data->cols[column].maxpixwidth = (WORD) *Line.maxpixwidth;

            if(Line.partcolsubst)
            {
              char c = toupper(((char *)Line.partcolsubst)[0]);

              switch(c)
              {
                case 'D':
                  data->cols[column].partcolsubst = PCS_DISABLED;
                break;

                case 'R':
                  data->cols[column].partcolsubst = PCS_RIGHT;
                break;

                case 'L':
                  data->cols[column].partcolsubst = PCS_LEFT;
                break;

                case 'C':
                  data->cols[column].partcolsubst = PCS_CENTER;
                break;
              }
            }

            data->cols[column].minx = (WORD) -1;
            data->cols[column].maxx = (WORD) -1;
            data->cols[column].dx = (WORD) 4;
            FreeArgs(rdargs);
          }

          if (data->cols[column].width < 1)
          {
/*
            if ((column == colmax-1) && (data->cols[column].width >= -1))
              data->cols[column].width = (WORD) 100;
            else
*/
              data->cols[column].width = (WORD) -1;
            data->cols[column].width_type = (BYTE) CI_PERCENT;
          }
          if (data->cols[column].delta < 0) data->cols[column].delta = 0;
          if ((data->cols[column].bar != 0) &&
              (data->cols[column].delta < 2)) data->cols[column].delta += 2;
          if (data->cols[column].minwidth < 1)
          {
/*
            if ((column == colmax-1) && (data->cols[column].minwidth >= -1))
              data->cols[column].minwidth = 5;
            else
*/
              data->cols[column].minwidth = -1;
          }
          if (data->cols[column].maxwidth <= 4) data->cols[column].maxwidth = 1000;
          if (data->cols[column].mincolwidth < 0) data->cols[column].mincolwidth = 0;
          if (data->cols[column].maxcolwidth <= 0) data->cols[column].maxcolwidth = 5000;
          if (data->cols[column].minpixwidth < 6) data->cols[column].minpixwidth = 6;
          if (data->cols[column].maxpixwidth <= 5) data->cols[column].maxpixwidth = 20000;
          if (data->cols[column].col > col) col = data->cols[column].col;
          column++;
        }
/*        data->cols[colmax-1].width = (WORD) 100;*/

        FreeDosObject(DOS_RDARGS, ptr);
        FreeVecPooled(data->Pool, sf);

        col = colmax;
        column = 1;
        while (column < data->numcols)
        { col = 0;
          colmax = 0;
          while (colmax < column)
          { if (data->cols[column].col == data->cols[colmax].col)
            { colmax = 0;
              while (colmax < data->numcols)
              { if (col == data->cols[colmax].col)
                { col++;
                  if (col >= DISPLAY_ARRAY_MAX)
                    col = 0;
                  colmax = 0;
                }
                else
                  colmax++;
              }
              data->cols[column].col = col++;
              if (col >= DISPLAY_ARRAY_MAX)
                col = 0;
              colmax = 0;
            }
            else
              colmax++;
          }
          column++;
        }

        column = 0;
        while (column < data->numcols)
        { data->column[column] = data->cols[column].col;
          data->cols[column].c = &(data->cols[column]);
          column++;
        }
        data->numcols2 = data->numcols;

        data->format_chge = 1;
        data->do_setcols = data->do_updatesb = data->do_wwrap = TRUE;
        data->display_ptr = NULL;
/*D(bug("%lx|NL_Read_Format >%s<\n",obj,strformat));*/
        if (data->SHOW)
        {
          if (!data->DRAW)
            NL_SetObjInfos(data,TRUE);
          NL_SetCols(data);
        }
        return (TRUE);
      }
      FreeDosObject(DOS_RDARGS, ptr);
    }
    FreeVecPooled(data->Pool ,sf);
  }
  return (FALSE);
}



static BOOL CCB_string(struct NLData *data, char **cbstr, char *str, LONG len, char lc, BOOL skipesc)
{
  char *tmpcb;
  char *tmp;
  LONG tmpcblen;

  if(str != NULL)
  {
    tmpcblen = len + 2;
    if(*cbstr != NULL)
      tmpcblen += strlen(*cbstr);

    if((tmpcb = AllocVecPooled(data->Pool, tmpcblen)) != NULL)
    {
      tmp = tmpcb;

      if(*cbstr != NULL)
      {
        tmp += strlcpy(tmp, *cbstr, tmpcblen);
        FreeVecPooled(data->Pool, *cbstr);
      }

      if(skipesc)
        tmp = stpncpy_noesc(tmp,str,len);
      else
        tmp += strlcpy(tmp, str, len);

      tmp[0] = lc;
      tmp[1] = '\0';

      *cbstr = tmpcb;

      return TRUE;
    }

    return FALSE;
  }

  return TRUE;
}


static BOOL CCB_entry(struct NLData *data,char **cbstr,APTR entptr,SIPTR ent,struct Hook *hook,SIPTR c1,SIPTR p1,SIPTR c2,SIPTR p2)
{
  char **display_array = &data->DisplayArray[2];
  char *str;
  SIPTR pos1,pos2,len,prep1,prep2;
  char lc;
  BOOL ln = FALSE;
  WORD colwrap,wrap = 0;

  if ((ent >= 0) && data->EntriesArray[ent]->Wrap)
  {
    SIPTR ent1 = ent;

    if (data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine)
      ent1 -= data->EntriesArray[ent]->dnum;
    entptr = data->EntriesArray[ent1]->Entry;
    wrap = data->EntriesArray[ent]->Wrap;
  }
  if (p1 < -PREPARSE_OFFSET_ENTRY)   /* begin in general column preparse string */
  {
    p1 += PREPARSE_OFFSET_COL;
    prep1 = 2;
  }
  else if (p1 < -2)
  {
    p1 += PREPARSE_OFFSET_ENTRY;     /* begin in display hook column preparse string */
    prep1 = 1;
  }
  else if (p1 == -1)                 /* begin at beginning of column */
    prep1 = 2;
  else
    prep1 = 0;

  if (p2 < -PREPARSE_OFFSET_ENTRY)   /* end in general column preparse string */
  { p2 += PREPARSE_OFFSET_COL;
    prep2 = 2;
  }
  else if (p2 < -2)
  {
    p2 += PREPARSE_OFFSET_ENTRY;     /* end in display hook column preparse string */
    prep2 = 1;
  }
  else if (p2 == -1)                 /* end at beginning of column */
    prep2 = 2;
  else
    prep2 = 0;

  if (entptr)
  {
    if (!hook)
      hook = data->NList_CopyEntryToClipHook;
    if (hook)
    { display_array[0] = (char *) entptr;
      data->DisplayArray[0] = (char *) data->NList_PrivateData;
      data->DisplayArray[1] = (char *) ent;
      display_array[1] = (char *) c1;
      display_array[2] = (char *) p1;
      display_array[3] = (char *) c2;
      display_array[4] = (char *) p2;
      display_array[5] = (char *) prep1;
      display_array[6] = (char *) prep2;
      if (c1 <= 0) display_array[1] = (char *) -1;
      if (c1 >= data->numcols-1) display_array[1] = (char *) -2;
      if (c2 <= 0) display_array[3] = (char *) -1;
      if (c2 >= data->numcols-1) display_array[3] = (char *) -2;
      if (data->NList_CopyEntryToClipHook2)
      { data->DisplayArray[0] = (char *) entptr;
        MyCallHookPkt(data->this,FALSE,hook,data->this,data->DisplayArray);
      }
      else
        MyCallHookPkt(data->this,TRUE,hook,display_array,entptr);
      data->display_ptr = NULL;
      return (CCB_string(data,cbstr,display_array[0],strlen(display_array[0]),'\0',FALSE));
    }
    else
    {
      WORD column,col1,col2;

      NL_GetDisplayArray(data,ent);

      if ((c1 == -2) || (c1 >= data->numcols-1))
        col1 = data->numcols-1;
      else if (c1 < 0)
        col1 = 0;
      else
        col1 = c1;

      if ((c2 == -2) || (c2 >= data->numcols-1))
      {
        col2 = data->numcols-1;
        if (p2==-2)
          ln = TRUE;
      }
      else if (c2 < 0)
        col2 = 0;
      else
        col2 = c2;
      for (column = col1;column <= col2;column++)
      {
        colwrap = (1 << data->cols[column].c->col) & TE_Wrap_TmpMask;
        str = display_array[data->cols[column].c->col];
        pos1 = -1;
        pos2 = -2;
        if (column == col1)
          pos1 = p1;
        if (column == col2)
          pos2 = p2;
        if (column == col2)
        { if (ln)
            lc = '\n';
          else
            lc = '\0';
        }
        else
          lc = '\t';
        if ((wrap & TE_Wrap_TmpLine) && !(wrap & colwrap))
        { if (!CCB_string(data,cbstr,str,0,lc,TRUE))
            return (FALSE);
          continue;
        }
        if (wrap && (wrap & colwrap))
        { if (pos1 < 0)
            pos1 = data->EntriesArray[ent]->pos;
          if (pos2 < 0)
            pos2 = data->EntriesArray[ent]->pos + data->EntriesArray[ent]->len;
        }
        if (data->NList_CopyColumnToClipHook)
        { display_array[0] = (char *) str;
          data->DisplayArray[0] = (char *) data->NList_PrivateData;
          data->DisplayArray[1] = (char *) ent;
          display_array[1] = (char *) pos1;
          display_array[2] = (char *) pos2;
          if (data->NList_CopyColumnToClipHook2)
          { data->DisplayArray[0] = (char *) str;
            MyCallHookPkt(data->this,FALSE,data->NList_CopyColumnToClipHook,data->this,data->DisplayArray);
          }
          else
            MyCallHookPkt(data->this,TRUE,data->NList_CopyColumnToClipHook,display_array,(APTR) str);
          data->display_ptr = NULL;
          len = (SIPTR) display_array[1];
          if (len < 0)
            len = 0;
          if (!CCB_string(data,cbstr,display_array[0],len,lc,FALSE))
            return (FALSE);
        }
        else
        { len = 0;
          while ((str[len] != '\0') && (str[len] != '\n'))
            len++;
          if (pos2 > len)
            pos2 = len;
          if (pos1 > len)
          { pos1 = len;
            len = 0;
          }
          if (pos1 <= -2)
            pos1 = len = 0;
          else if (pos1 == -1)
            pos1 = 0;
          if (pos2 >= pos1)
            len = pos2-pos1;
          else if (pos2 == -1)
            len = 0;
          if (!CCB_string(data,cbstr,&str[pos1],len,lc,TRUE /* FALSE */ ))
            return (FALSE);
        }
      }
      return (TRUE);
    }
    return (FALSE);
  }
  return (TRUE);
}


#define CCB_ENTRY_PTR_HOOK(ep,h) \
  ok = CCB_entry(data,&clipstr,ep,-1,h,-1,-1,-2,-2);

#define CCB_ENTRY_PTR(ep) \
  ok = CCB_entry(data,&clipstr,ep,-1,NULL,-1,-1,-2,-2);

#define CCB_ENTRY(e) \
  { if ((e >= 0) && (e < data->NList_Entries)) \
    ok = CCB_entry(data,&clipstr,data->EntriesArray[e]->Entry,e,NULL,-1,-1,-2,-2); \
  }

#define CCB_ENTRY_START_END(e,c1,p1,c2,p2) \
  { if ((e >= 0) && (e < data->NList_Entries)) \
    ok = CCB_entry(data,&clipstr,data->EntriesArray[e]->Entry,e,NULL,c1,p1,c2,p2); \
  }

static LONG CopyToFile(STRPTR filename, STRPTR buffer)
{
  LONG result = MUIV_NLCT_Failed;

  ENTER();

  if(buffer != NULL)
  {
    BPTR file = 0;

    if((file = Open(filename, MODE_NEWFILE)) != 0)
    {
      LONG pstr = 0;
      LONG lstr = strlen(buffer);
      LONG ret = 0;

      while(lstr > 0 && (ret = Write(file, &buffer[pstr], lstr)) >= 0)
      {
        lstr -= ret;
        pstr += ret;
      }

      if(ret < 0)
        result = MUIV_NLCT_WriteErr;
      else
        result = MUIV_NLCT_Success;

      Close(file);
    }
    else
      result = MUIV_NLCT_OpenErr;
  }

  RETURN(result);
  return result;
}

SIPTR NL_CopyTo(struct NLData *data,LONG pos,char *filename,ULONG clipnum,APTR *entries,struct Hook *hook)
{
  char *retstr = NULL;
  char *clipstr = NULL;
  SIPTR ok = TRUE;
  LONG ent;

  ENTER();

  switch (pos)
  {
    case MUIV_NList_CopyToClip_Active :
      CCB_ENTRY(data->NList_Active);
      break;

    case MUIV_NList_CopyToClip_Selected:
    {
      if(!data->NList_TypeSelect)
      {
        ent = 0;
        while (ok && (ent < data->NList_Entries))
        {
          if (data->EntriesArray[ent]->Select != TE_Select_None)
          {
            CCB_ENTRY(ent);
          }
          ent++;
        }
      }
      else
      {
        LONG c1,p1,c2,p2;
        c1 = data->sel_pt[data->min_sel].column;
        p1 = data->sel_pt[data->min_sel].colpos;
        c2 = data->sel_pt[data->max_sel].column;
        p2 = data->sel_pt[data->max_sel].colpos;
        ent = data->sel_pt[data->min_sel].ent;
        D(DBF_STARTUP, "ok=%d ent=%d mincol=%d maxcol=%d minpos=%d maxpos=%d", ok, ent, c1, c2, p1, p2);
        if (ok && (ent >= 0) && (ent < data->NList_Entries))
        {
          if (ent == data->sel_pt[data->max_sel].ent)
          {
            CCB_ENTRY_START_END(ent,c1,p1,c2,p2);
            break;
          }
          else
          {
            CCB_ENTRY_START_END(ent,c1,p1,-2,-2);
            ent++;
          }
        }
        while (ok && (ent >= 0) && (ent < data->sel_pt[data->max_sel].ent) && (ent < data->NList_Entries))
        {
          CCB_ENTRY(ent);
          ent++;
        }
        if (ok && (ent >= 0) && (ent == data->sel_pt[data->max_sel].ent) && (ent < data->NList_Entries))
        {
          CCB_ENTRY_START_END(ent,-1,-1,c2,p2);
          ent++;
        }
      }
    }
    break;

    case MUIV_NList_CopyToClip_All :
      { ent = 0;
        while (ok && (ent < data->NList_Entries))
        {
          CCB_ENTRY(ent);
          ent++;
        }
      }
      break;
    case MUIV_NList_CopyToClip_Entries :
      {
        APTR *array = (APTR *) entries;
        ent = 0;
        while (ok && array[ent])
        {
          CCB_ENTRY_PTR(array[ent]);
          ent++;
        }
      }
      break;
    case MUIV_NList_CopyToClip_Entry :
      CCB_ENTRY_PTR(entries);
      break;

    case MUIV_NList_CopyToClip_Strings :
      {
        APTR *array = (APTR *) entries;
        ent = 0;
        while (ok && array[ent])
        {
          CCB_ENTRY_PTR_HOOK(array[ent],hook);
          ent++;
        }
      }
      break;

    case MUIV_NList_CopyToClip_String :
      CCB_ENTRY_PTR_HOOK(entries,hook);
      break;

    default :
      CCB_ENTRY(pos);
      break;
  }

  if(filename != NULL)
  {
    ok = CopyToFile(filename, clipstr);
  }
  else if((LONG)clipnum >= 0)
  {
    ok = StringToClipboard(clipnum, clipstr);
  }
  else
  {
    int len = strlen(clipstr) + 1;

    if((retstr = (char *)AllocVecShared(len, 0L)) != NULL)
      strlcpy(retstr, clipstr, len);
    ok = (SIPTR)retstr;
  }

  if(clipstr != NULL)
    FreeVecPooled(data->Pool, clipstr);

  RETURN(ok);
  return ok;
}


IPTR mNL_CopyToClip(struct IClass *cl,Object *obj,struct MUIP_NList_CopyToClip *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/

  if((LONG)msg->clipnum < 0)
    return (0);

  return ((IPTR) NL_CopyTo(data,msg->pos,NULL,msg->clipnum,msg->entries,msg->hook));
}


IPTR mNL_CopyTo(struct IClass *cl,Object *obj,struct MUIP_NList_CopyTo *msg)
{
  struct NLData *data = INST_DATA(cl,obj);
  SIPTR res;
  /*DoSuperMethodA(cl,obj,(Msg) msg);*/
  res = NL_CopyTo(data,msg->pos,msg->filename,-1,msg->entries,NULL);
  *msg->result = (APTR) res;
  return (IPTR)res;
}
