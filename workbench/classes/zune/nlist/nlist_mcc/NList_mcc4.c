/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2005 by NList Open Source Team

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

#include <stdlib.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

#include "private.h"

#include "NList_func.h"

/* DRI's draw pens */
/* #define DETAILPEN         (0x0000)    // compatible Intuition rendering pens    */
/* #define BLOCKPEN          (0x0001)    // compatible Intuition rendering pens    */
/* #define TEXTPEN           (0x0002)    // text on background            */
/* #define SHINEPEN          (0x0003)    // bright edge on 3D objects        */
/* #define SHADOWPEN         (0x0004)    // dark edge on 3D objects        */
/* #define FILLPEN           (0x0005)    // active-window/selected-gadget fill    */
/* #define FILLTEXTPEN       (0x0006)    // text over FILLPEN            */
/* #define BACKGROUNDPEN     (0x0007)    // always color 0            */
/* #define HIGHLIGHTTEXTPEN  (0x0008)    // special color text, on background    */
/* V39 only : */
/* #define BARDETAILPEN      (0x0009)    // text/detail in screen-bar/menus */
/* #define BARBLOCKPEN       (0x000A)    // screen-bar/menus fill */
/* #define BARTRIMPEN        (0x000B)    // trim under screen-bar */

/* MUI's draw pens */
/* #define MPEN_SHINE      0 */
/* #define MPEN_HALFSHINE  1 */
/* #define MPEN_BACKGROUND 2 */
/* #define MPEN_HALFSHADOW 3 */
/* #define MPEN_SHADOW     4 */
/* #define MPEN_TEXT       5 */
/* #define MPEN_FILL       6 */
/* #define MPEN_MARK       7 */
/* #define MPEN_COUNT      8 */


/*
 * SetABPenDrMd is V39+ !!!!
 *   SetABPenDrMd(data->rp,data->pens[MPEN_TEXT],data->pens[MPEN_BACKGROUND],JAM1);
 *   SetABPenDrMd(data->rp,_dri(obj)->dri_Pens[TEXTPEN],_dri(obj)->dri_Pens[BACKGROUNDPEN],JAM1);
 * replace with for < V39:
 *   SetAPen(data->rp,data->pens[MPEN_SHINE]);
 *   SetBPen(data->rp,data->pens[MPEN_SHADOW]);
 *   SetDrMd(data->rp,JAM2);
 */


/*
 *     \t          Tabulation. Go to the next tab boundary of the column.
 *                 tab positions are separated by 8 spaces by default.
 *     ESC -       Disable text engine, following chars will be printed
 *                 without further parsing.
 *     ESC u       Set the soft style to underline.
 *     ESC b       Set the soft style to bold.
 *     ESC i       Set the soft style to italic.
 *     ESC n       Set the soft style back to normal.
 *     ESC <n>     Use pen number n (2..9) as front pen. n must be a valid
 *                 DrawInfo pen as specified in "intuition/screens.h".
 *     ESC c       Center current line. only valid at the beginning.
 *     ESC r       Right justify current line. only valid at the beginning.
 *     ESC l       Left justify current line. only valid at the beginning.
 *
 *     These ones are new or modified :
 *
 *     ESC j       Justify left and right current line. only at beginning.
 *     ESC I[<s>]                             (ESC I[<s>|<width>|<height>])
 *                 Draw MUI image with specification <s>.
 *                 See Image.mui/MUIA_Image_Spec for image spec definition.
 *                 <width> and <height> should be omited because NList
 *                 draw the image to its standard size himself now.
 *     ESC O[<p>]  Draw the MUIM_NList_CreateImage at adress <p>.
 *                 (<p> should be an 8 hex digits number).
 *     ESC o[<n>]  Draw the MUIM_NList_UseImage number <n>. If the <n> UseImage
 *                 don't exist or has been set to NULL, no image is drawn.
 *     ESC P[]     Use default front pen.
 *     ESC P[<n>]  Use pen number <n>. (it's a direct pen number, so you must
 *                 make MUI_ObtainPen and MUI_ReleasePen for it yourself,
 *                 best to do it is in Setup() and Cleanup() of a subclass).
 *     ESC T       Draw horizontal line on top of the entry for the column.
 *     ESC C       Draw horizontal line centered in the entry for the column.
 *     ESC B       Draw horizontal line on bottom of the entry for the column
 *     ESC E       Draw horizontal line centered in the entry for the column,
 *                 but only on the left and right of the line contents.
 *
 *
 *
 *     "0:<x>" where <x> is between MUII_BACKGROUND and
 *             MUII_FILLBACK2 identifying a builtin pattern.
 *
 *     "1:<x>" where <x> identifies a builtin standard image.
 *             Don't use this, use "6:<x>" instead.
 *
 *     "2:<r>,<g>,<b>" where <r>, <g> and <b> are 32-bit RGB
 *                     color values specified as 8-digit hex
 *                     string (e.g. 00000000 or ffffffff).
 *                     Kick 2.x users will get an empty image.
 *
 *     "3:<n>" where <n> is the name of an external boopsi
 *             image class.
 *
 *     "4:<n>" where <n> is the name of an external MUI brush.
 *
 *     "5:<n>" where <n> is the name of an external picture
 *             file that should be loaded with datatypes.
 *             Kick 2.x users will get an empty image.
 *
 *     "6:<x>" where <x> is between MUII_WindowBack and
 *             MUII_Count-1 identifying a preconfigured
 *             image/background.
 */


BOOL DontDoColumn(struct NLData *data,LONG ent,WORD column)
{
  if ((ent >= 0) && (ent < data->NList_Entries) && (column >= 0) && (column < data->numcols) &&
      (data->cols[column].c->col < 8) &&
      (data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine) &&
      !(data->EntriesArray[ent]->Wrap & (1 << data->cols[column].c->col)))
    return (TRUE);
  else
    return (FALSE);
}


void FreeAffInfo(struct NLData *data)
{
  if(data->aff_infos)
  {
    FreeVecPooled(data->Pool, data->aff_infos);
    data->aff_infos = NULL;
  }
  data->numaff_infos = 0;
}


BOOL NeedAffInfo(struct NLData *data,WORD niask)
{
  struct affinfo *affinfotmp;
  WORD ni;

  if (data->numaff_infos-1 < niask)
  { LONG num = niask + niask/2;
    if (num < 12)
      num = 12;

    //D(bug( "Adding %ld aff infos.\n", num ));

    if((affinfotmp = (struct affinfo *)AllocVecPooled(data->Pool, sizeof(struct affinfo)*num)) != NULL)
    {
      ni = 0;
      while (ni < data->numaff_infos)
      { affinfotmp[ni].strptr = data->aff_infos[ni].strptr;
        affinfotmp[ni].pos = data->aff_infos->pos;
        affinfotmp[ni].addchar = data->aff_infos[ni].addchar;
        affinfotmp[ni].addinfo = data->aff_infos[ni].addinfo;
        affinfotmp[ni].pen = data->aff_infos[ni].pen;
        affinfotmp[ni].len = data->aff_infos[ni].len;
        affinfotmp[ni].style = data->aff_infos[ni].style;
        ni++;
      }
      while (ni < num)
      { affinfotmp[ni].strptr = NULL;
        affinfotmp[ni].pos = 0;
        affinfotmp[ni].addchar = 0;
        affinfotmp[ni].addinfo = 0;
        affinfotmp[ni].pen = 0;
        affinfotmp[ni].len = 0;
        affinfotmp[ni].style = 0;
        ni++;
      }
      FreeAffInfo(data);
      data->aff_infos = affinfotmp;
      data->numaff_infos = num;
      return (TRUE);
    }
    return (FALSE);
  }
  return (TRUE);
}


void NL_GetDisplayArray(struct NLData *data, SIPTR ent)
{
  char *useptr;

  ENTER();

  if(ent >= 0 && ent < data->NList_Entries)
  {
    data->parse_ent = ent;

    if(data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine)
    {
      if(data->EntriesArray[ent]->len < -1)
      {
        useptr = NULL;
      }
      else
      {
        ent -= data->EntriesArray[data->parse_ent]->dnum;

        if(ent < 0 || ent >= data->NList_Entries)
          ent = 0;

        useptr = (char *)data->EntriesArray[ent]->Entry;
      }
    }
    else
    {
      useptr = (char *)data->EntriesArray[ent]->Entry;
    }
  }
  else
  {
    useptr = (char *)data->NList_Title;
    data->display_ptr = NULL;
    data->parse_ent = -1;
    ent = -1;
  }

  if(data->display_ptr == NULL || useptr != data->display_ptr || ent != (SIPTR)data->DisplayArray[1])
  {
    char **display_array = &data->DisplayArray[2];
    WORD column;

    data->display_ptr = useptr;
    data->DisplayArray[1] = (char *)ent;

    // set up the string and preparses pointers
    for(column = 0; column < data->numcols; column++)
    {
      display_array[data->cols[column].c->col] = NULL;
      display_array[data->cols[column].c->col + DISPLAY_ARRAY_MAX] = NULL;
    }
    if(useptr != NULL)
    {
      display_array[0] = useptr;

      if(ent < 0 || ent >= data->NList_Entries)
        useptr = NULL;

      // invoke the display method
      // if this method is not catched by a subclass our own implementation will
      // correctly choose one of the two possible hook functions as default
      data->DisplayArray[0] = (char *)useptr;
      DoMethod(data->this, MUIM_NList_Display, data->DisplayArray[0], data->DisplayArray[1], &data->DisplayArray[2], &data->DisplayArray[2+DISPLAY_ARRAY_MAX]);
    }
    data->parse_column = -1;
  }

  LEAVE();
}


void ParseColumn(struct NLData *data,WORD column,IPTR mypen)
{
  register struct colinfo *cinfo = data->cols[column].c;
  register struct affinfo *afinfo;
  char *ptr1,*ptr2,*ptro,*ptrs;
  char **display_array = &data->DisplayArray[2];
  IPTR pen;
  LONG col,ent = data->parse_ent,maxlen = 300;
  WORD ni,style,do_format,prep,dx,there_is_char;
  WORD tmppos = 0;
  LONG numimg = 0; // RHP: Added for Special ShortHelp
  int good = 1;

/*  if (data->parse_column == column)  return;*/

  data->parse_column = column;
  col = NL_ColumnToCol(data,column);

  there_is_char = 0;
  prep = 2;

  do_format = TRUE;
  cinfo->style = 0;

  ni = 0;
  afinfo = &data->aff_infos[ni];

  pen = mypen;
  style = 0;
  afinfo->len = 0;

  if ((ent >= 0) && data->EntriesArray[ent]->Wrap)
  { UBYTE colmask = (UBYTE) ((1 << col) & TE_Wrap_TmpMask);
    if (data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine)
    { prep = 0;
      if (data->EntriesArray[ent]->Wrap & colmask)
      { tmppos = data->EntriesArray[ent]->pos;
        cinfo->style = style = data->EntriesArray[ent]->style;
        if (IS_FIXPEN(style))
          pen = (IPTR)data->EntriesArray[ent]->Entry;
        if (data->EntriesArray[ent]->len >= 0)
          maxlen = data->EntriesArray[ent]->len;
      }
      else
        maxlen = 0;
    }
    else if (data->EntriesArray[ent]->Wrap & colmask)
    { cinfo->style = style = data->EntriesArray[ent]->style;
      if (data->EntriesArray[ent]->len >= 0)
        maxlen = data->EntriesArray[ent]->len;
    }
  }
  ptrs = ptr1 = display_array[cinfo->col];

  while ((prep>=0) && NeedAffInfo(data,ni+1))
  {
    if ((prep == 1) && !display_array[cinfo->col+DISPLAY_ARRAY_MAX])
      prep = 0;
    if (prep == 2)
    { ptr1 = cinfo->preparse;
      ptrs = ptr1 + PREPARSE_OFFSET_COL;

      //$$$WRAPCHANGE1
      ptr2 = ptr1 + 100;
      //ptr2 = ptr1 + 120;
    }
    else if (prep == 1)
    { ptr1 = display_array[cinfo->col+DISPLAY_ARRAY_MAX];
      ptrs = ptr1 + PREPARSE_OFFSET_ENTRY;

      //$$$WRAPCHANGE1
      ptr2 = ptr1 + 100;
      //ptr2 = ptr1 + 120;
    }
    else if ((ent >= 0) && (data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine))
    { ptrs = ptr1 = display_array[cinfo->col];
      ptr1 += tmppos;
      ptr2 = ptr1 + maxlen;
    }
    else
    { ptrs = ptr1 = display_array[cinfo->col];
      ptr2 = ptr1 + maxlen;
    }

    if (afinfo->len > 0)
    { ni++;
      afinfo = &data->aff_infos[ni];
    }
    afinfo->pen = pen;
    afinfo->len = 0;
    afinfo->addchar = 0;
    afinfo->addinfo = 0;
    afinfo->button = -1;
    afinfo->imgnum = -1; // RHP: Added for Special ShortHelp
    afinfo->style = style;
    afinfo->strptr = ptr1;
    afinfo->pos = (WORD) (ptr1 - ptrs);

    //D(bug( "%ld - Setting aff info %ld: %15.15s - pos: %ld, len: %ld, style: %ld.\n", __LINE__, ni, afinfo->strptr, afinfo->pos, afinfo->len, afinfo->style ));

    while (ptr1 && (ptr1<ptr2) && (ptr1[0] != '\0') && (ptr1[0] != '\n') && (ptr1[0] != '\r') && (good = NeedAffInfo(data,ni+2)))
    {
      if (data->NList_SkipChars)
      { char *sc = data->NList_SkipChars;
        while (sc[0] && (sc[0] != ptr1[0]))
          sc++;
        if (sc[0])
        {
          if (afinfo->len > 0)
          { ni++;
            afinfo = &data->aff_infos[ni];
          }
          ptr1++;
          afinfo->pen = pen;
          afinfo->len = 0;
          afinfo->addchar = 0;
          afinfo->addinfo = 0;
          afinfo->button = -1;
          afinfo->imgnum = -1; // RHP: Added for Special ShortHelp
          afinfo->style = style;
          afinfo->strptr = ptr1;
          afinfo->pos = (WORD) (ptr1 - ptrs);

          //D(bug( "%ld - Setting aff info %ld: %15.15s - pos: %ld, len: %ld, style: %ld.\n", __LINE__, ni, afinfo->strptr, afinfo->pos, afinfo->len, afinfo->style ));

          continue;
        }
      }
      if (do_format && (ptr1[0] == '\033'))
      {
        ptro = ptr1;
        ptr1++;

        //$$$WRAPADD1
        //ptr2 += 2;

        if (ptr1[0] == 'c')
        { SET_ALIGN_CENTER(cinfo->style); SET_ALIGN_CENTER(style); ptr1++; }
        else if (ptr1[0] == 'r')
        { SET_ALIGN_RIGHT(cinfo->style); SET_ALIGN_RIGHT(style); ptr1++; }
        else if (ptr1[0] == 'l')
        { SET_ALIGN_LEFT(cinfo->style); SET_ALIGN_LEFT(style); ptr1++; }
        else if (ptr1[0] == 'j')
        { SET_ALIGN_JUSTIFY(cinfo->style); SET_ALIGN_JUSTIFY(style); ptr1++; }
        else if (ptr1[0] == 'u')
        { SET_STYLE_UNDERLINE(style); ptr1++; }
        else if (ptr1[0] == 'b')
        { SET_STYLE_BOLD(style); ptr1++; }
        else if (ptr1[0] == 'i')
        { SET_STYLE_ITALIC(style); ptr1++; }
        else if (ptr1[0] == 'n')
        { SET_STYLE_NORMAL(style); ptr1++; }
        else if (ptr1[0] == '-')
        { do_format = FALSE; ptr1++; }
        else if (ptr1[0] == '2')
        { if (data->SHOW) { pen = _dri(data->this)->dri_Pens[2]; SET_FIXPEN(style); } ptr1++; }
        else if (ptr1[0] == '3')
        { if (data->SHOW) { pen = _dri(data->this)->dri_Pens[3]; SET_FIXPEN(style); } ptr1++; }
        else if (ptr1[0] == '4')
        { if (data->SHOW) { pen = _dri(data->this)->dri_Pens[4]; SET_FIXPEN(style); } ptr1++; }
        else if (ptr1[0] == '5')
        { if (data->SHOW) { pen = _dri(data->this)->dri_Pens[5]; SET_FIXPEN(style); } ptr1++; }
        else if (ptr1[0] == '6')
        { if (data->SHOW) { pen = _dri(data->this)->dri_Pens[6]; SET_FIXPEN(style); } ptr1++; }
        else if (ptr1[0] == '7')
        { if (data->SHOW) { pen = _dri(data->this)->dri_Pens[7]; SET_FIXPEN(style); } ptr1++; }
        else if (ptr1[0] == '8')
        { if (data->SHOW) { pen = _dri(data->this)->dri_Pens[8]; SET_FIXPEN(style); } ptr1++; }
        else if (ptr1[0] == '9')
        { if (data->SHOW) { pen = _dri(data->this)->dri_Pens[9]; SET_FIXPEN(style); } ptr1++; }
        else if (ptr1[0] == 'T')
        { SET_HLINE_T(cinfo->style); SET_HLINE_T(style); ptr1++; }
        else if (ptr1[0] == 'C')
        { SET_HLINE_C(cinfo->style); SET_HLINE_C(style); ptr1++; }
        else if (ptr1[0] == 'B')
        { SET_HLINE_B(cinfo->style); SET_HLINE_B(style); ptr1++; }
        else if (ptr1[0] == 'E')
        { SET_HLINE_E(cinfo->style); SET_HLINE_E(style); ptr1++; }
        else if (ptr1[0] == 't')
        { SET_HLINE_thick(cinfo->style);
          SET_HLINE_thick(style);
          if (muiRenderInfo(data->this) && _pens(data->this))
            data->HLINE_thick_pen = MUIPEN(_pens(data->this)[MPEN_FILL]);
          else
            data->HLINE_thick_pen = 3;
          ptr1++;
          if (ptr1[0] == '[')
          { long np = 1;
            long np2 = 1;
            long pnum;
            if (ptr1[np] == 'N')
            { SET_HLINE_nothick(cinfo->style);
              SET_HLINE_nothick(style);
              if (muiRenderInfo(data->this) && _pens(data->this))
                data->HLINE_thick_pen = MUIPEN(_pens(data->this)[MPEN_SHADOW]);
              else
                data->HLINE_thick_pen = 1;
              np = np2 = 2;
            }
            if ((ptr1[np2] == 'M') || (ptr1[np2] == 'I'))
              np++;

            while ((ptr1[np] != '\0') && (ptr1[np] != '\n') && (ptr1[np] != '\r') && (ptr1[np] != ']'))
              np++;
            if (ptr1[np] == ']')
            {
              if (np == np2)
              {
              }
              else if (ptr1[np2] == 'M')
              {
                pnum = atol(&ptr1[np2+1]);
                if ((pnum >= 0) && (pnum <= 8) && muiRenderInfo(data->this) && _pens(data->this))
                  data->HLINE_thick_pen = MUIPEN(_pens(data->this)[pnum]);
              }
              else if (ptr1[np2] == 'I')
              {
                pnum = atol(&ptr1[np2+1]);
                if ((pnum >= 0) && (pnum <= 11))
                {
                  if (pnum >= 9)
                    pnum = 2;
                  if ((pnum >= 0) && muiRenderInfo(data->this) && _dri(data->this) && _dri(data->this)->dri_Pens)
                    data->HLINE_thick_pen = _dri(data->this)->dri_Pens[pnum];
                }
              }
              else if (np < 8)
                data->HLINE_thick_pen = atol(&ptr1[np2]);
              ptr1 = &ptr1[np+1];

              //$$$WRAPADD1
              //ptr2 += np+1;
            }
          }
        }
        else if (ptr1[0] == 'P')
        {
          ptr1++;
          if (ptr1[0] == '[')
          {
            long np = 1;

            while ((ptr1[np] != '\0') && (ptr1[np] != '\n') && (ptr1[np] != '\r') && (ptr1[np] != ']'))
              np++;
            if (ptr1[np] == ']')
            {
              if (np == 1)
              {
                pen = mypen;
                SET_NOFIXPEN(style);
              }
              else if (np < 8)
              {
                pen = atol(&ptr1[1]);
                SET_FIXPEN(style);
              }
              ptr1 = &ptr1[np+1];

              //$$$WRAPADD1
              //ptr2 += np+1;
            }
          }
        }
        else if (ptr1[0] == 'I')
        {
          ptr1++;
          if (ptr1[0] == '[')
          {
            LONG dx = -1, dy = -1;
            LONG minx = -1, button = -1;
            ULONG tag=0L, tagval=0L;
            size_t np = 1;

            // skip everything until we find the final closing brace
            while(ptr1[np] != '\0' && ptr1[np] != ']')
              np++;

            if(ptr1[np] == ']')
            {
              if (afinfo->len > 0)
              { ni++;
                afinfo = &data->aff_infos[ni];
              }
              afinfo->tag = tag;
              afinfo->tagval = tagval;
              afinfo->button = button;
              afinfo->imgnum = -1; // RHP: Added for Special ShortHelp
              afinfo->addchar = 0;
              afinfo->addinfo = 0;
              afinfo->style = STYLE_IMAGE;
              strlcpy(data->imagebuf, &ptr1[1], MIN(np, sizeof(data->imagebuf)));
              afinfo->pos = (WORD) (ptro - ptrs);
              D(DBF_DRAW, "image spec '%s' tag %ld, tagval %ld, button %08lx, dx %ld, dy %ld, minx %ld", data->imagebuf, tag, tagval, button, dx, dy, minx);
              if (data->imagebuf[0] == '\0')
                afinfo->strptr = NULL;
              else
                afinfo->strptr = (APTR) GetNImage(data,data->imagebuf);
              if (afinfo->strptr)
              { if (dx <= 0)
                  dx = ((struct NImgList *) afinfo->strptr)->width;
                if (dy < 0)
                  dy = ((struct NImgList *) afinfo->strptr)->height;
              }
              if (dx < 0)
                dx = 0;
              if (dy <= 0)
                dy = data->vinc;
              if (minx < dx)
                minx = dx;
              afinfo->len = minx;
              afinfo->pen = ((dy & 0x0000FFFF) << 16) + (dx & 0x0000FFFF);

              //D(bug( "%ld - Setting aff info %ld: %15.15s - pos: %ld, len: %ld, style: %ld.\n", __LINE__, ni, afinfo->strptr, afinfo->pos, afinfo->len, afinfo->style ));

              if (afinfo->strptr || (data->imagebuf[0] == '\0')) {
                ptr1 = &ptr1[np+1];

                //$$$WRAPADD1
                //ptr2 += np+1;
              }
            }
          }
        }
        else if ((ptr1[0] == 'O') || (ptr1[0] == 'o'))
        {
          char imgtype = ptr1[0];
          ptr1++;
          if (ptr1[0] == '[')
          {
            LONG dy,dy2 = -2;
            LONG minx = -1,minx2 = -1,button = -1;
            ULONG tag=0L,tagval=0L;
            long np = 1;

            dx = -1;
            dy = -1;
            while ((ptr1[np] != '\0') && (ptr1[np] != '\n') && (ptr1[np] != '\r') && (ptr1[np] != ']') && (ptr1[np] != ';') && (ptr1[np] != '|') && (ptr1[np] != ',') && (ptr1[np] != '@'))
              np++;
            if (ptr1[np] == '@')
            { np++;
              button = atol(&ptr1[np]);
              while ((ptr1[np] != '\0') && (ptr1[np] != '\n') && (ptr1[np] != '\r') && (ptr1[np] != ']') && (ptr1[np] != ';') && (ptr1[np] != '|') && (ptr1[np] != ','))
                np++;
            }
            while (ptr1[np] == ';')
            { np++;
              if (!tag)
                tag = strtoul(&ptr1[np],NULL,16);
              else
                tagval = strtoul(&ptr1[np],NULL,16);
              while ((ptr1[np] != '\0') && (ptr1[np] != '\n') && (ptr1[np] != '\r') && (ptr1[np] != ']') && (ptr1[np] != ';') && (ptr1[np] != ','))
                np++;
            }
            if (ptr1[np] == ',')
            { np++;
              minx2 = minx = atol(&ptr1[np]);
              while ((ptr1[np] != '\0') && (ptr1[np] != '\n') && (ptr1[np] != '\r') && (ptr1[np] != ']') && (ptr1[np] != '|'))
                np++;
            }
            if (ptr1[np] == '|')
            { np++;
              if ((ptr1[np] != '\0') && (ptr1[np] != '\n') && (ptr1[np] != '\r') && (ptr1[np] != '|') && (ptr1[np] != ']') && (ptr1[np] != ','))
                dx = atol(&ptr1[np]);
              while ((ptr1[np] != '\0') && (ptr1[np] != '\n') && (ptr1[np] != '\r') && (ptr1[np] != '|') && (ptr1[np] != ']') && (ptr1[np] != ','))
                np++;
              if (ptr1[np] == '|')
              { np++;
                if ((ptr1[np] != '\0') && (ptr1[np] != '\n') && (ptr1[np] != '\r') && (ptr1[np] != ']') && (ptr1[np] != ','))
                  dy2 = dy = atol(&ptr1[np]);
                while ((ptr1[np] != '\0') && (ptr1[np] != '\n') && (ptr1[np] != '\r') && (ptr1[np] != ']') && (ptr1[np] != ','))
                  np++;
              }
            }
            if (ptr1[np] == ',')
            {
              np++;
              minx2 = minx = atol(&ptr1[np]);
              while ((ptr1[np] != '\0') && (ptr1[np] != '\n') && (ptr1[np] != '\r') && (ptr1[np] != ']'))
                np++;
            }
            if (ptr1[np] == ']')
            {
              struct BitMapImage *bitmapimage = NULL;
              LONG dx2;

              if (imgtype == 'O')
                bitmapimage = (struct BitMapImage *) strtoul(&ptr1[1],NULL,16);
              else
              {
                numimg = atol(&ptr1[1]); // RHP: Changed for Special ShortHelp
                if ((numimg >= 0) && (numimg < data->LastImage) && data->NList_UseImages)
                  bitmapimage = data->NList_UseImages[numimg].bmimg;
              }
              if (bitmapimage != NULL && (bitmapimage->control == MUIA_Image_Spec))
              {
                if (afinfo->len > 0)
                {
                  ni++;
                  afinfo = &data->aff_infos[ni];
                }
                afinfo->tag = tag;
                afinfo->tagval = tagval;
                afinfo->button = button;
                afinfo->imgnum = numimg; // RHP: Added for Special ShortHelp
                afinfo->addchar = 0;
                afinfo->addinfo = 0;
                afinfo->style = STYLE_IMAGE;
                afinfo->strptr = (APTR) GetNImage2(data,bitmapimage->obtainpens);
                afinfo->pos = (WORD) (ptro - ptrs);
                if ((afinfo->strptr) && !(((struct NImgList *) afinfo->strptr)->NImgObj))
                  afinfo->strptr = NULL;
                dx2 = dx;
                if (afinfo->strptr)
                {
                  if (dx <= 0)
                    dx = ((struct NImgList *) afinfo->strptr)->width;
                  if (dy < 0)
                    dy = ((struct NImgList *) afinfo->strptr)->height;
                }
                if (dx < 0)
                  dx = 0;
                if ((dy2 != -1) && ((dy <= 0) || ((minx2 >= 0) && (dx2 == -1))))
                  dy = data->vinc;
                if (minx < dx)
                  minx = dx;
                if ((dx < minx) && (minx2 >= 0) && (dx2 == -1))
                  dx = minx;
                afinfo->len = minx;
                afinfo->pen = ((dy & 0x0000FFFF) << 16) + (dx & 0x0000FFFF);

                D(DBF_DRAW, "Setting aff info %ld: %15.15s - pos: %ld, len: %ld, style: %ld", ni, afinfo->strptr, afinfo->pos, afinfo->len, afinfo->style);

                ptr1 = &ptr1[np+1];

                //$$$WRAPADD1
                //ptr2 += np+1;
              }
              else
              {
                if (bitmapimage == NULL || (bitmapimage->control != MUIM_NList_CreateImage))
                  bitmapimage = NULL;

                if (bitmapimage != NULL || (minx > 0))
                {
                  if (afinfo->len > 0)
                  {
                    ni++;
                    afinfo = &data->aff_infos[ni];
                  }
                  if (bitmapimage)
                  {
                    dx = bitmapimage->width;
                    if (dx <= 0)
                      dx = 1;
                    if ((dy > bitmapimage->height) || (dy < 0))
                      dy = bitmapimage->height;
                    else if (dy == 0)
                      dy = data->vinc;
                  }
                  if (minx < dx)
                    minx = dx;
                  afinfo->len = minx;
                  afinfo->addchar = 0;
                  afinfo->addinfo = 0;
                  afinfo->button = -1;
                  afinfo->imgnum = -1; // RHP: Added for Special ShortHelp
                  afinfo->pen = ((dy & 0x0000FFFF) << 16) + (dx & 0x0000FFFF);
                  afinfo->style = STYLE_IMAGE2;
                  afinfo->strptr = (APTR) bitmapimage;
                  afinfo->pos = (WORD) (ptro - ptrs);

                  //D(bug( "%ld - Setting aff info %ld: %15.15s - pos: %ld, len: %ld, style: %ld.\n", __LINE__, ni, afinfo->strptr, afinfo->pos, afinfo->len, afinfo->style ));
                }
                ptr1 = &ptr1[np+1];

                //$$$WRAPADD1
                //ptr2 += np+1;
              }
            }
          }
        }
        else
          ptr1++;

        if (afinfo->len > 0)
        {
          ni++;
          afinfo = &data->aff_infos[ni];
        }
        afinfo->pen = pen;
        afinfo->len = 0;
        afinfo->addchar = 0;
        afinfo->addinfo = 0;
        afinfo->button = -1;
        afinfo->imgnum = -1; // RHP: Added for Special ShortHelp
        afinfo->style = style;
        afinfo->strptr = ptr1;
        afinfo->pos = (WORD) (ptr1 - ptrs);

        //D(bug( "%ld - Setting aff info %ld: %15.15s - pos: %ld, len: %ld, style: %ld.\n", __LINE__, ni, afinfo->strptr, afinfo->pos, afinfo->len, afinfo->style ));
      }
      else if ((ptr1[0] == '\t' && (data->NList_IgnoreSpecialChars == NULL || strchr(data->NList_IgnoreSpecialChars, '\t') == 0)) ||
               (((unsigned char)ptr1[0]) == 0xA0 && (data->NList_IgnoreSpecialChars == NULL || strchr(data->NList_IgnoreSpecialChars, 0xa0) == 0)))
      {
        there_is_char = 1;
        if (afinfo->len > 0)
        { ni++;
          afinfo = &data->aff_infos[ni];
        }
        afinfo->pen = pen;
        afinfo->addchar = 0;
        afinfo->addinfo = 0;
        afinfo->button = -1;
        afinfo->imgnum = -1; // RHP: Added for Special ShortHelp
        afinfo->strptr = ptr1;
        afinfo->pos = (WORD) (ptr1 - ptrs);
        if (ptr1[0] == '\t')
        { afinfo->style = STYLE_TAB;
          afinfo->len = 1;
        }
        else if (((unsigned char)ptr1[0]) == 0xA0)
        { afinfo->style = STYLE_FIXSPACE;
          afinfo->len = data->spacesize;
        }
        else
        { afinfo->style = STYLE_SPACE;
          afinfo->len = data->spacesize;
        }

        //D(bug( "%ld - Setting aff info %ld: %15.15s - pos: %ld, len: %ld, style: %ld.\n", __LINE__, ni, afinfo->strptr, afinfo->pos, afinfo->len, afinfo->style ));

        ni++;
        afinfo = &data->aff_infos[ni];

        ptr1++;
        afinfo->pen = pen;
        afinfo->len = 0;
        afinfo->addchar = 0;
        afinfo->addinfo = 0;
        afinfo->button = -1;
        afinfo->imgnum = -1; // RHP: Added for Special ShortHelp
        afinfo->style = style;
        afinfo->strptr = ptr1;
        afinfo->pos = (WORD) (ptr1 - ptrs);

        //D(bug( "%ld - Setting aff info %ld: %15.15s - pos: %ld, len: %ld, style: %ld.\n", __LINE__, ni, afinfo->strptr, afinfo->pos, afinfo->len, afinfo->style ));
      }
      else
      { there_is_char = 1;
        afinfo->len += 1;
        ptr1++;
      }
    }
    prep--;
  }
  if (!there_is_char && IS_HLINE_E(style))
  { SET_HLINE_C(cinfo->style); SET_HLINE_C(style); }
  cinfo->ninfo = ni;
  if (NeedAffInfo(data,ni+1))
  { ni++;
    afinfo = &data->aff_infos[ni];

    afinfo->pen = pen;
    afinfo->style = style;
    afinfo->button = -1;
    afinfo->imgnum = -1; // RHP: Added for Special ShortHelp
    afinfo->len = 0;
    afinfo->pos = (WORD) (ptr1 - ptrs);

    //D(bug( "%ld - Setting aff info %ld: %15.15s - pos: %ld, len: %ld, style: %ld.\n", __LINE__, ni, afinfo->strptr, afinfo->pos, afinfo->len, afinfo->style ));
  }
}


static void ParseColumns(struct NLData *data,LONG ent)
{
  WORD column;

  NL_GetDisplayArray(data,ent);
  for (column = 0;column < data->numcols;column++)
  { if (DontDoColumn(data,ent,column))
      continue;
    ParseColumn(data,column,0);
  }
}

/*
 * void OneParseColumns(Object *obj,struct NLData *data,LONG ent)
 * {
 *   if ((ent >= -1) && (ent < data->NList_Entries))
 *   {
 *     if (!data->do_parse || data->NList_Quiet || data->NList_Disabled)
 *       return;
 *     if (data->SETUP)
 *     { if (data->adding_member != 2)
 *         data->adding_member = 1;
 *     }
 *     else
 *       data->adding_member = 3;
 *     data->parse_column = -1;
 *     ParseColumns(obj,data,ent);
 *     data->parse_column = -1;
 *     GetNImage_End(data);
 *     data->do_parse = FALSE;
 *   }
 * }
 */

void AllParseColumns(struct NLData *data)
{
  LONG ent;
  if (!data->do_parse || data->NList_Quiet || data->NList_Disabled)
    return;
  if (data->SETUP)
  { if (data->adding_member != 2)
      data->adding_member = 1;
  }
  else
    data->adding_member = 3;
  data->do_images = TRUE;
  data->display_ptr = NULL;
  data->parse_column = -1;
  if (data->NList_Title)
    ParseColumns(data,-1);
  for (ent = 0;ent < data->NList_Entries;ent++)
    ParseColumns(data,ent);
  data->display_ptr = NULL;
  data->parse_column = -1;
  GetImages(data);
  data->do_parse = FALSE;
}


static WORD AddSpaceInfos(struct NLData *data,WORD column,WORD ni1)
{
  register struct colinfo *cinfo = data->cols[column].c;
  register struct affinfo *afinfo;
  WORD curlen,ni,ni2;
  WORD numinfo = 0;
  char *str;

  if (!data->DRAW || data->do_setcols)
    return (numinfo);

  ni2 = cinfo->ninfo;
  ni = ni1; afinfo = &data->aff_infos[ni];
  while ((ni <= cinfo->ninfo) && (afinfo->len > 0))
  { if (!(afinfo->style & STYLE_STRMASK))
    { str = (char *) afinfo->strptr;
      curlen = 0;
      while ((curlen < afinfo->len) && (str[curlen] != ' '))
        curlen++;
      curlen++;
      if ((curlen < afinfo->len) && NeedAffInfo(data,cinfo->ninfo+1))
      { ni2 = cinfo->ninfo++;
        while (ni2 >= ni)
        { data->aff_infos[ni2+1].strptr = data->aff_infos[ni2].strptr;
          data->aff_infos[ni2+1].pen = data->aff_infos[ni2].pen;
          data->aff_infos[ni2+1].len = data->aff_infos[ni2].len;
          data->aff_infos[ni2+1].style = data->aff_infos[ni2].style;
          data->aff_infos[ni2+1].pos = data->aff_infos[ni2].pos;
          data->aff_infos[ni2+1].addchar = data->aff_infos[ni2].addchar;
          data->aff_infos[ni2+1].addinfo = data->aff_infos[ni2].addinfo;
          ni2--;
        }
        afinfo->len = curlen;

        ni++; afinfo = &data->aff_infos[ni];

        afinfo->len -= curlen;
        afinfo->strptr = &str[curlen];
        afinfo->pos += curlen;
        numinfo++;
      }
      else
        ni++; afinfo = &data->aff_infos[ni];
    }
    else
      ni++; afinfo = &data->aff_infos[ni];
  }
  return (numinfo);
}


void WidthColumn(struct NLData *data,WORD column,WORD updinfo)
{
  register struct colinfo *cinfo = data->cols[column].c;
  register struct affinfo *afinfo;
  WORD curlen,ni,ni1;
  WORD numchar = 0;
  WORD numinfo = 0;
  WORD  colwidth;
  struct TextExtent te;
  BOOL text_last = FALSE;
  BOOL is_text = FALSE;

  if (!data->SETUP)
  { if (updinfo)
    { cinfo->colwidthbiggest = -2;
      cinfo->colwidthbiggest2 = -2;
      cinfo->colwidthbiggestptr = -2;
      cinfo->colwidthbiggestptr2 = -2;
      if (cinfo->userwidth == -1)
        data->do_setcols = TRUE;
      data->do_images = TRUE;
    }
    return;
  }

  ReSetFont;

  curlen = 0;

  ni1 = 0;
  ni = 0;
  afinfo = &data->aff_infos[ni];

  while ((ni <= cinfo->ninfo) && (afinfo->len > 0))
  { numinfo++;
    text_last = FALSE;
    if (afinfo->style == STYLE_IMAGE)
      curlen += afinfo->len + 2;
    else if (afinfo->style == STYLE_IMAGE2)
      curlen += afinfo->len + 2;
    else if (afinfo->style == STYLE_TAB)
    { curlen = ((((curlen-1) / data->tabsize) + 1) * data->tabsize) + 1;
      numchar = 0;
      numinfo = 0;
      ni1 = ni;
    }
    else if ((afinfo->style == STYLE_FIXSPACE) || (afinfo->style == STYLE_SPACE))
    { curlen += afinfo->len;
      numchar++;
      is_text = TRUE;
    }
    else
    { SetSoftStyle(data->rp, GET_STYLE(afinfo->style), STYLE_MASK);
      TextExtent(data->rp, afinfo->strptr, afinfo->len, &te);
      curlen += te.te_Width;
      if ((ni == 0) && (te.te_Extent.MinX < 0))
        curlen -= te.te_Extent.MinX;
      numchar += afinfo->len;
      text_last = TRUE;
      is_text = TRUE;
    }
    ni++;
    afinfo = &data->aff_infos[ni];
  }
  if (is_text)
  { if (text_last && (te.te_Extent.MaxX > te.te_Width))
      curlen += (te.te_Extent.MaxX - te.te_Width);
    curlen += 2;
    cinfo->colwidth = curlen;
  }
  cinfo->colwidth = curlen;
  SetSoftStyle(data->rp, 0, STYLE_MASK);

  if (!data->DRAW || data->do_setcols)
    cinfo->xoffset = 0;
  else if (IS_ALIGN_CENTER(cinfo->style))
    cinfo->xoffset = (cinfo->dx - cinfo->colwidth)/2;
  else if (IS_ALIGN_RIGHT(cinfo->style))
    cinfo->xoffset = cinfo->dx - 1 - cinfo->colwidth;
  else if (IS_ALIGN_JUSTIFY(cinfo->style))
  { WORD diff = cinfo->dx - 1 - cinfo->colwidth;
    cinfo->xoffset = 0;
    if (diff > 0)
    { UBYTE addchar = 0;
      UBYTE addinfo = 0;
      BOOL cont = TRUE;
      if ((numchar > 0) && (numinfo > 0))
        numinfo += AddSpaceInfos(data,column,ni1);
      numinfo--;
      while (cont)
      { cont = FALSE;
        if ((diff > numinfo) && (numinfo > 0))
        { addinfo++;
          diff -= numinfo;
          cont = TRUE;
        }
        if ((diff > numinfo) && (numinfo > 0))
        { addinfo++;
          diff -= numinfo;
          cont = TRUE;
        }
        if ((diff > numchar) && (numchar > 0) && (addchar < data->hinc/3) && (addchar < data->spacesize))
        { addchar++;
          diff -= numchar;
          cont = TRUE;
        }
        if ((diff > numinfo) && (numinfo > 0))
        { addinfo++;
          diff -= numinfo;
          cont = TRUE;
        }
        if ((diff > numchar) && (numchar > 0) && (addchar < data->hinc/3) && (addchar < data->spacesize))
        { addchar++;
          diff -= numchar;
          cont = TRUE;
        }
      }
      if ((addchar > 0) || (addinfo > 0) || (diff > 0))
      { ni = ni1; afinfo = &data->aff_infos[ni];
        while ((ni <= cinfo->ninfo) && (afinfo->len > 0))
        { afinfo->addchar = addchar;
          if (ni < cinfo->ninfo)
          { afinfo->addinfo = addinfo;
            if (diff > 0)
            { diff--;
              afinfo->addinfo++;
            }
          }
          ni++; afinfo = &data->aff_infos[ni];
        }
      }
      cinfo->colwidth += (numchar * addchar) + (numinfo * addinfo);
    }
  }
  else
    cinfo->xoffset = 0;
  if (cinfo->xoffset < 0)
    cinfo->xoffset = 0;
  colwidth = cinfo->colwidth + 2;
  if ((data->parse_ent == -1) &&
      (((data->NList_TitleMark & MUIV_NList_TitleMark_TypeMask) != MUIV_NList_TitleMark_None) ||
       ((data->NList_TitleMark2 & MUIV_NList_TitleMark2_TypeMask) != MUIV_NList_TitleMark2_None)))
    colwidth += 3;

  if (colwidth > cinfo->colwidthmax)
    cinfo->colwidthmax = colwidth;

  if (updinfo)
  {
    if (updinfo == 1)
    {
      if ((colwidth > cinfo->colwidthbiggest) && (cinfo->colwidthbiggest >= -1))
      { cinfo->colwidthbiggest2 = cinfo->colwidthbiggest;
        cinfo->colwidthbiggestptr2 = cinfo->colwidthbiggestptr;
        cinfo->colwidthbiggest = colwidth;
        cinfo->colwidthbiggestptr = (IPTR) data->display_ptr;
        if (((cinfo->width == -1) || (cinfo->minwidth == -1)) && (cinfo->userwidth == -1))
          data->do_setcols = TRUE;
      }
      else if ((colwidth > cinfo->colwidthbiggest2) && (cinfo->colwidthbiggest2 >= -1))
      { cinfo->colwidthbiggest2 = colwidth;
        cinfo->colwidthbiggestptr2 = (SIPTR) data->display_ptr;
      }
    }
    else
    {
      if (((colwidth > cinfo->colwidthbiggest) && (cinfo->colwidthbiggest >= -1)) ||
          (cinfo->colwidthbiggestptr == (SIPTR) data->display_ptr))
      {
        cinfo->colwidthbiggestptr = cinfo->colwidthbiggestptr2;
        cinfo->colwidthbiggest = cinfo->colwidthbiggest2;
        cinfo->colwidthbiggestptr2 = -2;
        cinfo->colwidthbiggest2 = -2;
        if (((cinfo->width == -1) || (cinfo->minwidth == -1)) && (cinfo->userwidth == -1))
        { if (cinfo->colwidthbiggest < 0)
          { cinfo->colwidthbiggestptr = -2;
            cinfo->colwidthbiggest = -2;
          }
          data->do_setcols = TRUE;
        }
      }
      else if ((IPTR)cinfo->colwidthbiggestptr2 == (IPTR) data->display_ptr)
      {
        cinfo->colwidthbiggest2 = -2;
        cinfo->colwidthbiggestptr2 = -2;
      }
    }
  }
}


void FindCharInColumn(struct NLData *data,LONG ent,WORD column,WORD xoffset,WORD *charxoffset,WORD *charnum)
{
  register struct colinfo *cinfo = data->cols[column].c;
  register struct affinfo *afinfo;
  WORD curx,curx2,ni;
  struct TextExtent te;

  if (DontDoColumn(data,ent,column) || !data->DRAW || data->do_setcols)
  { *charnum = -1;
    *charxoffset = xoffset;
    return;
  }

  NL_GetDisplayArray(data,ent);
  ParseColumn(data,column,0);
  WidthColumn(data,column,0);

  curx = cinfo->xoffset;

  ni = 0;
  afinfo = &data->aff_infos[ni];

  if (xoffset < curx)
  { *charnum = -1;
    *charxoffset = xoffset - curx;
    return;
  }
  while ((ni <= cinfo->ninfo) && (afinfo->len > 0))
  { curx2 = curx;
    if (afinfo->style == STYLE_IMAGE)
    { curx += afinfo->len + afinfo->addinfo + 2;
      if (xoffset  < curx)
      { *charnum = afinfo->pos;
        *charxoffset = xoffset - curx2;
        if (*charxoffset > afinfo->len/2)
          *charxoffset = xoffset - curx;
        if (data->storebutton && ((LONG)afinfo->button >= 0))
        {
          data->affimage = afinfo->imgnum; // RHP: Added for Special Shorthelp
          data->affbutton = afinfo->button;
          data->affbuttonline = ent;
          data->affbuttoncol = column;
          data->affbuttonstate = 0;
          data->storebutton = FALSE;
        }
        return;
      }
    }
    else if (afinfo->style == STYLE_IMAGE2)
    { curx += afinfo->len + afinfo->addinfo + 2;
      if (xoffset  < curx)
      { *charnum = afinfo->pos;
        *charxoffset = xoffset - curx2;
        if (*charxoffset > afinfo->len/2)
          *charxoffset = xoffset - curx;
        if (data->storebutton && ((LONG)afinfo->button >= 0))
        {
          data->affimage = afinfo->imgnum; // RHP: Added for Special Shorthelp
          data->affbutton = afinfo->button;
          data->affbuttonline = ent;
          data->affbuttoncol = column;
          data->affbuttonstate = 0;
          data->storebutton = FALSE;
        }
        return;
      }
    }
    else if (afinfo->style == STYLE_TAB)
    { curx = ((((curx-cinfo->xoffset) / data->tabsize) + 1) * data->tabsize) + cinfo->xoffset;
      if (xoffset  < curx)
      { *charnum = afinfo->pos;
        *charxoffset = xoffset - curx2;
        if (*charxoffset > curx - xoffset)
          *charxoffset = xoffset - curx;
        return;
      }
    }
    else if ((afinfo->style == STYLE_FIXSPACE) || (afinfo->style == STYLE_SPACE))
    { curx += afinfo->len + afinfo->addinfo + afinfo->addchar;
      if (xoffset  < curx)
      { *charnum = afinfo->pos;
        *charxoffset = xoffset - curx2;
        if (*charxoffset > afinfo->len/2)
          *charxoffset = xoffset - curx;
        return;
      }
    }
    else
    { SetSoftStyle(data->rp, GET_STYLE(afinfo->style), STYLE_MASK);
      data->rp->TxSpacing = afinfo->addchar;
      TextExtent(data->rp, afinfo->strptr, afinfo->len, &te);
      curx += te.te_Width + afinfo->addinfo;
      if ((ni == 0) && (te.te_Extent.MinX < 0))
      { curx -= te.te_Extent.MinX;
        curx2 -= te.te_Extent.MinX;
      }
      if (xoffset  < curx)
      { WORD curx3 = curx2;
        char *strptr;
        WORD pos = 0;
        strptr = (char *) afinfo->strptr;
        while (pos < afinfo->len)
        { curx2 = curx3;
          TextExtent(data->rp, &strptr[pos], 1, &te);
          curx3 += te.te_Width;
          if (xoffset  < curx3)
          { *charnum = afinfo->pos + pos;
            *charxoffset = xoffset - curx2;
            if (*charxoffset > curx3 - xoffset)
              *charxoffset = xoffset - curx3;
            data->rp->TxSpacing = 0;
            return;
          }
          pos++;
        }
        *charnum = afinfo->pos + pos;
        *charxoffset = xoffset - curx;
        data->rp->TxSpacing = 0;
        return;
      }
      data->rp->TxSpacing = 0;
    }
    ni++;
    afinfo = &data->aff_infos[ni];
  }
  SetSoftStyle(data->rp, 0, STYLE_MASK);
  *charnum = -1;
  *charxoffset = xoffset - curx;
}


static LONG NL_DoWrapLine(struct NLData *data,LONG ent,BOOL force)
{
  register struct colinfo *cinfo;
  register struct affinfo *afinfo;
  struct TextExtent te;
  LONG ent1 = ent,selects,column,col = 0;
  WORD curlen,endpos,ni,colwidth,style;
  UWORD dnum = 0;
  IPTR  pen;
  UBYTE colmask;

  if (!data->SHOW)
    return (ent+1);

  if ((ent < 0) || !data->EntriesArray[ent]->Wrap ||
      (data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine))
    return (ent+1);

  if ((data->EntriesArray[ent]->len >= 0) && !force && (data->EntriesArray[ent1]->dnum > 0))
  {
    ent += data->EntriesArray[ent1]->dnum;
    return (ent);
  }

  colmask = data->EntriesArray[ent]->Wrap & TE_Wrap_TmpMask;
  while((colmask = colmask >> 1))
    col++;
  column = NL_ColToColumn(data,col);

  if (data->EntriesArray[ent]->len < 0)
    selects = data->EntriesArray[ent]->pos;
  else
    selects = NL_GetSelects(data,ent);

  data->EntriesArray[ent]->PixLen = -1;
  data->EntriesArray[ent]->Wrap &= TE_Wrap_TmpMask;
  data->EntriesArray[ent]->pos = (WORD) selects;
  data->EntriesArray[ent]->len = -1;
  data->EntriesArray[ent]->dnum = 1;
  if (((selects & 0x0001) && data->multiselect) || ((ent == data->NList_Active) && (data->multiselect == MUIV_NList_MultiSelect_None)))
    data->EntriesArray[ent]->Select = TE_Select_Line;
  else
    data->EntriesArray[ent]->Select = TE_Select_None;

  if ((column < 0) || (column >= data->numcols))
    return (ent+1);

  data->EntriesArray[ent]->pos = 0;

  cinfo = data->cols[column].c;
  /*colwidth = cinfo->colwidth;*/
  colwidth = cinfo->dx - 4;
  if (colwidth < 3*data->hinc)
    colwidth = 3*data->hinc;

  colmask = data->EntriesArray[ent]->Wrap | TE_Wrap_TmpLine;

  ReSetFont;

  pen = 1;
  style = data->EntriesArray[ent]->style;

  while (TRUE)
  {
    data->EntriesArray[ent]->len = -1;
    NL_GetDisplayArray(data,ent);
    ParseColumn(data,column,pen);
    NL_Changed(data,ent);
    ni = 0;
    afinfo = &data->aff_infos[ni];

    curlen = 0;
    endpos = -1;

    while ((ni <= cinfo->ninfo) && (afinfo->len > 0))
    {
      if (curlen > colwidth)
        break;
      if (afinfo->style == STYLE_IMAGE)
        curlen += afinfo->len + 2;
      else if (afinfo->style == STYLE_IMAGE2)
        curlen += afinfo->len + 2;
      else if (afinfo->style == STYLE_TAB)
      {
        pen = afinfo->pen;
        curlen = ((((curlen-1) / data->tabsize) + 1) * data->tabsize) + 1;
        if ((curlen > colwidth) && (ni > 0) && (ni < cinfo->ninfo))
        {
          ni++;
          afinfo = &data->aff_infos[ni];
          break;
        }
      }
      else if ((afinfo->style == STYLE_FIXSPACE) || (afinfo->style == STYLE_SPACE))
      {
        pen = afinfo->pen;
        curlen += afinfo->len;
        if ((curlen > colwidth) && (ni > 0) && (ni < cinfo->ninfo))
        {
          ni++;
          afinfo = &data->aff_infos[ni];
          break;
        }
      }
      else
      {
        WORD clen = afinfo->len;
        WORD blanklen = clen;
        WORD oldcurlen,difflen;
        char *strptr;

        strptr = (char *) afinfo->strptr;
        pen = afinfo->pen;
        style = afinfo->style;
        SetSoftStyle(data->rp, GET_STYLE(style), STYLE_MASK);
        TextExtent(data->rp, strptr, clen, &te);
        if ((ni == 0) && (te.te_Extent.MinX < 0))
          curlen -= te.te_Extent.MinX;
        oldcurlen = curlen;
        curlen += te.te_Width;
        if (te.te_Extent.MaxX > te.te_Width)
          difflen = te.te_Extent.MaxX - te.te_Width;
        else
          difflen = 0;
        if ((clen > 0) && ((curlen + difflen) >= colwidth))
        {
          if (afinfo->pos < 0)
          {
            afinfo->pos = 0;
            afinfo->len = 0;
            break;
          }
          curlen += difflen;
          while ((clen > 0) && (curlen >= colwidth))
          {
            clen--;
            while ((clen > 0) && (strptr[clen] != ' '))
              clen--;
            if (clen > 0)
              blanklen = clen;
            else if (clen <= 0)
              break;
            TextExtent(data->rp, strptr, clen, &te);
            if (te.te_Extent.MaxX > te.te_Width)
              curlen = oldcurlen + te.te_Extent.MaxX;
            else
              curlen = oldcurlen + te.te_Width;
          }
          if ((clen <= 0) && (ni == 0) && (blanklen > 0))
          {
            clen = blanklen;
            TextExtent(data->rp, strptr, clen, &te);
            if (te.te_Extent.MaxX > te.te_Width)
              curlen = oldcurlen + te.te_Extent.MaxX;
            else
              curlen = oldcurlen + te.te_Width;
            while ((clen > 0) && (curlen >= colwidth))
            {
              clen--;
              if (clen <= 0)
                break;
              TextExtent(data->rp, strptr, clen, &te);
              if (te.te_Extent.MaxX > te.te_Width)
                curlen = oldcurlen + te.te_Extent.MaxX;
              else
                curlen = oldcurlen + te.te_Width;
            }
            afinfo->pos += clen;
          }
          else
          {
            endpos = afinfo->pos + clen;
            if (clen > 0)
              afinfo->pos += (clen + 1);
          }
          break;
        }
      }

      if ((curlen > colwidth) && (ni > 0))
        break;
      ni++;
      afinfo = &data->aff_infos[ni];
    }

    if (afinfo->pos <= 0)
      data->EntriesArray[ent]->len = 0;
    else if (endpos >= 0)
      data->EntriesArray[ent]->len = endpos - data->EntriesArray[ent]->pos;
    else
      data->EntriesArray[ent]->len = afinfo->pos - data->EntriesArray[ent]->pos;

    dnum++;

    if ((ni > cinfo->ninfo) || (afinfo->len <= 0) || (afinfo->pos < 0))
    {
      if (IS_ALIGN_JUSTIFY(data->EntriesArray[ent]->style))
      {
        SET_ALIGN_LEFT(data->EntriesArray[ent]->style);
      }
      ent++;
      break;
    }
    else
      ent++;

    if ((ent >= data->NList_Entries) || !(data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine))
    {
      if (!NL_InsertTmpLine(data,ent))
        break;
    }

    if (dnum <= 15)
      selects = selects >> 1;
    if (((selects & 0x0001) && data->multiselect) || ((ent == data->NList_Active) && (data->multiselect == MUIV_NList_MultiSelect_None)))
      data->EntriesArray[ent]->Select = TE_Select_Line;
    else
      data->EntriesArray[ent]->Select = TE_Select_None;
    data->EntriesArray[ent]->PixLen = -1;
    data->EntriesArray[ent]->Wrap = colmask;
    data->EntriesArray[ent]->pos = afinfo->pos;
    data->EntriesArray[ent]->len = -2;
    data->EntriesArray[ent]->style = style;
    data->EntriesArray[ent]->Entry = (APTR)pen;
    data->EntriesArray[ent]->dnum = dnum;
  }
  data->EntriesArray[ent1]->dnum = dnum;
  SetSoftStyle(data->rp, 0, STYLE_MASK);
  while ((ent < data->NList_Entries) && (data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine))
    NL_DeleteTmpLine(data,ent);

  return (ent);
}


void NL_DoWrapAll(struct NLData *data,BOOL force,BOOL update)
{
  LONG entorig = data->NList_Entries;
  LONG ent = 0;

  if (!data->do_wwrap || !data->EntriesArray || data->do_setcols)
    return;
  else if (!data->SHOW || !data->DRAW || data->NList_Quiet || data->NList_Disabled)
  {
    LONG len = -1;

    while (ent < data->NList_Entries)
    {
      if (data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine)
      {
        if (len < 0)
          data->EntriesArray[ent]->len = -2;
      }
      else if (data->EntriesArray[ent]->Wrap)
        len = data->EntriesArray[ent]->len;
      else
        len = 0;
      ent++;
    }
    return;
  }
  else if (!data->DRAW)
    return;

  if (data->force_wwrap)
    force = TRUE;

  while (ent < data->NList_Entries)
  {
    if (data->EntriesArray[ent]->Wrap & TE_Wrap_TmpLine)
    {
      NL_DeleteTmpLine(data,ent);
      NL_SegChanged(data,ent,data->NList_Entries);
      data->do_draw = TRUE;
    }
    else if (data->EntriesArray[ent]->Wrap)
    {
      if ((data->EntriesArray[ent]->len < 0) || force || (data->EntriesArray[ent]->dnum < 1))
      {
        ent = NL_DoWrapLine(data,ent,force);
        if (data->NList_Entries != entorig)
          NL_SegChanged(data,ent,data->NList_Entries);
        data->do_draw = TRUE;
      }
      else
        ent += data->EntriesArray[ent]->dnum;
    }
    else
      ent++;
  }
  if (data->NList_Entries != entorig)
  {
    data->do_updatesb = TRUE;
    DO_NOTIFY(NTF_Entries|NTF_MinMax);
  }
  if ((data->NList_First > 0) && (data->NList_First + data->NList_Visible >= data->NList_Entries))
  {
    data->NList_First = data->NList_Entries - data->NList_Visible;
    if (data->NList_First < 0)
      data->NList_First = 0;
    DO_NOTIFY(NTF_First);
  }
  data->do_wwrap = data->force_wwrap = FALSE;
  if (update)
  {
    data->do_updatesb = TRUE;
    REDRAW;
/*    do_notifies(NTF_AllChanges|NTF_MinMax);*/
  }
}


static void WidthColumns(struct NLData *data,LONG ent,WORD updinfo)
{
  WORD column;

//D(bug( "%ld - Calling NL_GDA() width %ld entries!\n", __LINE__, ent ));

  /* sba: was > 1 but in this case the images weren't parsed */
  if ( data->numcols > 0 )
  {
    NL_GetDisplayArray(data,ent);

    for (column = 0;column < data->numcols;column++) /* sba: was (data->numcols-1) */
    { if (DontDoColumn(data,ent,column))
        continue;
      if (updinfo != 2)
        ParseColumn(data,column,0);
      if (data->SHOW)
        WidthColumn(data,column,updinfo);
    }
  }
}


void AllWidthColumns(struct NLData *data)
{
  LONG ent;
  WORD column;

  /*
  ** We must omit this, because floattext will be broken if not.
  */
  //if ( data->numcols > 1 )
  {
    data->display_ptr = NULL;
    data->parse_column = -1;
    for (column = 0;column < data->numcols;column++)
    //for (column = 0;column < ( data->numcols - 1 );column++)
    { data->cols[column].c->colwidthmax = 4;
      data->cols[column].c->dx = 4;
    }
    if (data->NList_Title)
      WidthColumns(data,-1,0);
    if (data->EntriesArray)
    { for (ent = 0;ent < data->NList_Entries;ent++)
      WidthColumns(data,ent,0);
    }
    data->display_ptr = NULL;
    data->parse_column = -1;
  }
}


void NL_SetColsAdd(struct NLData *data,LONG ent,WORD addimages)
{
  WORD column;

  //if ( !data->NList_Pause )
  {
    if (data->SETUP)
    { if (data->adding_member != 2)
        data->adding_member = 1;
    }
    else
      data->adding_member = 3;

    data->display_ptr = NULL;
    data->parse_column = -1;
    if ((ent == -2) || (ent == -3))
    { for (column = 0;column < data->numcols;column++)
      { data->cols[column].c->colwidthbiggest = -1;
        data->cols[column].c->colwidthbiggest2 = -1;
        data->cols[column].c->colwidthbiggestptr = -2;
        data->cols[column].c->colwidthbiggestptr2 = -2;
        data->cols[column].c->colwidthmax = 4;
        data->cols[column].c->dx = 4;
        data->Title_PixLen = -1;
        if (((data->cols[column].c->width == -1) || (data->cols[column].c->minwidth == -1)) && (data->cols[column].c->userwidth == -1))
          data->do_setcols = TRUE;
      }

      if (data->NList_Title)
        WidthColumns(data,-1,1);

      if (data->EntriesArray)
      {
        if (ent == -2 )
        {
          for (ent = 0;ent < data->NList_Entries;ent++)
            WidthColumns(data,ent,1);
        }
        else /* -3 == Recalculate only visible (cs@aphaso.de) */
        {
          for (ent = data->NList_First; ent < ( data->NList_First + data->NList_Visible ); ent++ )
            WidthColumns(data,ent,1);
        }
      }
      data->display_ptr = NULL;
      data->parse_column = -1;
    }
    else
    {
      //D(bug( "%ld - Calling WidthColumns() width entry %ld!\n", __LINE__, ent ));
      if (data->SETUP != 3)
        WidthColumns(data,ent,1);
    }

    if (addimages)
      GetNImage_End(data);
    else
      data->do_images = TRUE;
    if (!data->SHOW)
      data->do_setcols = TRUE;
  }
}


void NL_SetColsRem(struct NLData *data,LONG ent)
{
  WORD column;

  //if ( !data->NList_Pause )
  {
    if (!data->SHOW || (ent == -2))
    { data->parse_column = -1;
      data->display_ptr = NULL;
      for (column = 0;column < data->numcols;column++)
      { data->cols[column].c->colwidthbiggest = -1;
        data->cols[column].c->colwidthbiggest2 = -1;
        data->cols[column].c->colwidthbiggestptr = -2;
        data->cols[column].c->colwidthbiggestptr2 = -2;
        data->cols[column].c->colwidthmax = 4;
        data->cols[column].c->dx = 4;
        data->Title_PixLen = -1;
        if (((data->cols[column].c->width == -1) || (data->cols[column].c->minwidth == -1)) && (data->cols[column].c->userwidth == -1))
          data->do_setcols = TRUE;
      }
      if (!data->SHOW)
        data->do_setcols = TRUE;
    }
    else
    {
      //D(bug( "%ld - Calling WidthColumns() width entry %ld!\n", __LINE__, ent ));

      if (data->SETUP != 3)
        WidthColumns(data,ent,2);

      if (ent == -1)
        data->Title_PixLen = -1;
    }
  }
}


