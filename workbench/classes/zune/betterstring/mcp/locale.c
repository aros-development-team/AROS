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

 $Id$

***************************************************************************/

/****************************************************************
   This file was created automatically by `FlexCat V1.3'
****************************************************************/

#include <proto/locale.h>

struct FC_Type { long ID; char * Str; };

const struct FC_Type _MSG_Label_Active = { 0, "Active:" };
const struct FC_Type _MSG_Label_Background = { 1, "Background" };
const struct FC_Type _MSG_Label_Cursor = { 2, "Cursor:" };
const struct FC_Type _MSG_Label_Fonts = { 3, "Font:" };
const struct FC_Type _MSG_Label_Frame = { 4, "Frame:" };
const struct FC_Type _MSG_Label_Inactive = { 5, "Inactive:" };
const struct FC_Type _MSG_Label_Marked = { 6, "Marked:" };
const struct FC_Type _MSG_Label_Text = { 7, "Text" };
const struct FC_Type _MSG_String_TestString = { 8, "Test the power of BetterString here!" };

static struct Catalog *BS_Catalog = NULL;

/*** Catalog functions ***/
/// GetStr()
char *GetStr(APTR fcstr)
{
  char *defaultstr = ((struct FC_Type *)fcstr)->Str;

  if(LocaleBase)
  {
    return (char *)GetCatalogStr(BS_Catalog, ((struct FC_Type *)fcstr)->ID, (STRPTR)defaultstr);
  }

  return defaultstr;
}
///
/// GetStripStr()
// function that will strip out the special menusigns
char *GetStripStr(APTR fcstr)
{
   char *loc_str = GetStr(fcstr);

   return (0 == loc_str[1] ? &loc_str[2] : loc_str);
}
///
/// CloseCat()
void CloseCat(void)
{
  if (LocaleBase) CloseCatalog(BS_Catalog);
  BS_Catalog = NULL;
}
///
/// OpenCat(void)
void OpenCat(void)
{
  static const struct TagItem tags[] = {
                                         { OC_BuiltInLanguage, (ULONG)"english" },
                                         { OC_Version,         2 },
                                         { TAG_DONE,           0  }
                                       };

  if(LocaleBase && !BS_Catalog)
  {
    BS_Catalog = OpenCatalogA(NULL, "BetterString_mcp.catalog", (struct TagItem *)&tags[0]);
  }
}
///
