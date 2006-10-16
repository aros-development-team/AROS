/****************************************************************
   This file was created automatically by `FlexCat 2.4'
   from "wanderer.cd".

   Do NOT edit by hand!
****************************************************************/

#ifndef wanderer_H
#define wanderer_H


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif


#ifdef CATCOMP_ARRAY
#undef CATCOMP_NUMBERS
#undef CATCOMP_STRINGS
#define CATCOMP_NUMBERS
#define CATCOMP_STRINGS
#endif

#ifdef CATCOMP_BLOCK
#undef CATCOMP_STRINGS
#define CATCOMP_STRINGS
#endif

/***************************************************************/


#ifdef CATCOMP_NUMBERS

#define MSG_TITLE 0
#define MSG_DESCRIPTION 1
#define MSG_SCREENTITLE 2
#define MSG_MEM_B 3
#define MSG_MEM_K 4
#define MSG_MEM_M 5
#define MSG_MEM_G 6
#define MSG_MEN_WANDERER 7
#define MSG_MEN_BACKDROP 8
#define MSG_MEN_SC_BACKDROP 9
#define MSG_MEN_EXECUTE 10
#define MSG_MEN_SC_EXECUTE 11
#define MSG_MEN_SHELL 12
#define MSG_MEN_SC_SHELL 13
#define MSG_MEN_GUISET 14
#define MSG_MEN_ABOUT 15
#define MSG_MEN_SC_ABOUT 16
#define MSG_MEN_QUIT 17
#define MSG_MEN_SC_QUIT 18
#define MSG_MEN_WINDOW 19
#define MSG_MEN_NEWDRAW 20
#define MSG_MEN_SC_NEWDRAW 21
#define MSG_MEN_OPENPAR 22
#define MSG_MEN_CLOSE 23
#define MSG_MEN_SC_CLOSE 24
#define MSG_MEN_UPDATE 25
#define MSG_MEN_CONTENTS 26
#define MSG_MEN_SC_CONTENTS 27
#define MSG_MEN_CLRSEL 28
#define MSG_MEN_SC_CLRSEL 29
#define MSG_MEN_SNAPSHT 30
#define MSG_MEN_ALL 31
#define MSG_MEN_VIEW 32
#define MSG_MEN_ICVIEW 33
#define MSG_MEN_DCVIEW 34
#define MSG_MEN_ALLFIL 35
#define MSG_MEN_SORTIC 36
#define MSG_MEN_CLNUP 37
#define MSG_MEN_SC_CLNUP 38
#define MSG_MEN_BYNAME 39
#define MSG_MEN_BYDATE 40
#define MSG_MEN_BYSIZE 41
#define MSG_MEN_REVERSE 42
#define MSG_MEN_DRWFRST 43
#define MSG_MEN_ICON 44
#define MSG_MEN_OPEN 45
#define MSG_MEN_SC_OPEN 46
#define MSG_MEN_RENAME 47
#define MSG_MEN_SC_RENAME 48
#define MSG_MEN_INFO 49
#define MSG_MEN_SC_INFO 50
#define MSG_MEN_DELETE 51
#define MSG_MEN_TOOLS 52
#define MSG_REALLYQUIT 53
#define MSG_YESNO 54
#define MSG_DUMMY 55

#endif /* CATCOMP_NUMBERS */


/***************************************************************/

#ifdef CATCOMP_STRINGS

#define MSG_TITLE_STR "Wanderer"
#define MSG_DESCRIPTION_STR "File Manager"
#define MSG_SCREENTITLE_STR "Wanderer %s graphics mem %s other mem"
#define MSG_MEM_B_STR "B"
#define MSG_MEM_K_STR "K"
#define MSG_MEM_M_STR "M"
#define MSG_MEM_G_STR "G"
#define MSG_MEN_WANDERER_STR "Wanderer"
#define MSG_MEN_BACKDROP_STR "Backdrop"
#define MSG_MEN_SC_BACKDROP_STR "B"
#define MSG_MEN_EXECUTE_STR "Execute ..."
#define MSG_MEN_SC_EXECUTE_STR "E"
#define MSG_MEN_SHELL_STR "Shell ..."
#define MSG_MEN_SC_SHELL_STR "W"
#define MSG_MEN_GUISET_STR "GUI Settings ..."
#define MSG_MEN_ABOUT_STR "About ..."
#define MSG_MEN_SC_ABOUT_STR "?"
#define MSG_MEN_QUIT_STR "Quit ..."
#define MSG_MEN_SC_QUIT_STR "Q"
#define MSG_MEN_WINDOW_STR "Window"
#define MSG_MEN_NEWDRAW_STR "New Drawer ..."
#define MSG_MEN_SC_NEWDRAW_STR "N"
#define MSG_MEN_OPENPAR_STR "Open Parent"
#define MSG_MEN_CLOSE_STR "Close"
#define MSG_MEN_SC_CLOSE_STR "K"
#define MSG_MEN_UPDATE_STR "Update"
#define MSG_MEN_CONTENTS_STR "Select Contents"
#define MSG_MEN_SC_CONTENTS_STR "A"
#define MSG_MEN_CLRSEL_STR "Clear selection"
#define MSG_MEN_SC_CLRSEL_STR "Z"
#define MSG_MEN_SNAPSHT_STR "Snapshot"
#define MSG_MEN_ALL_STR "All"
#define MSG_MEN_VIEW_STR "View"
#define MSG_MEN_ICVIEW_STR "Icon view"
#define MSG_MEN_DCVIEW_STR "Detail view"
#define MSG_MEN_ALLFIL_STR "All files"
#define MSG_MEN_SORTIC_STR "Sorted"
#define MSG_MEN_CLNUP_STR "Clean Up"
#define MSG_MEN_SC_CLNUP_STR "U"
#define MSG_MEN_BYNAME_STR "By name"
#define MSG_MEN_BYDATE_STR "By Date"
#define MSG_MEN_BYSIZE_STR "By size"
#define MSG_MEN_REVERSE_STR "Reverse"
#define MSG_MEN_DRWFRST_STR "Drawers first"
#define MSG_MEN_ICON_STR "Icon"
#define MSG_MEN_OPEN_STR "Open"
#define MSG_MEN_SC_OPEN_STR "O"
#define MSG_MEN_RENAME_STR "Rename ..."
#define MSG_MEN_SC_RENAME_STR "R"
#define MSG_MEN_INFO_STR "Information ..."
#define MSG_MEN_SC_INFO_STR "I"
#define MSG_MEN_DELETE_STR "Delete ..."
#define MSG_MEN_TOOLS_STR "Tools"
#define MSG_REALLYQUIT_STR "Do you really want to quit Wanderer?"
#define MSG_YESNO_STR "*Yes|No"
#define MSG_DUMMY_STR "DUMMY"

#endif /* CATCOMP_STRINGS */


/***************************************************************/


#ifdef CATCOMP_ARRAY

struct CatCompArrayType
{
  LONG   cca_ID;
  STRPTR cca_Str;
};

static const struct CatCompArrayType CatCompArray[] =
{
  {MSG_TITLE,(STRPTR)MSG_TITLE_STR},
  {MSG_DESCRIPTION,(STRPTR)MSG_DESCRIPTION_STR},
  {MSG_SCREENTITLE,(STRPTR)MSG_SCREENTITLE_STR},
  {MSG_MEM_B,(STRPTR)MSG_MEM_B_STR},
  {MSG_MEM_K,(STRPTR)MSG_MEM_K_STR},
  {MSG_MEM_M,(STRPTR)MSG_MEM_M_STR},
  {MSG_MEM_G,(STRPTR)MSG_MEM_G_STR},
  {MSG_MEN_WANDERER,(STRPTR)MSG_MEN_WANDERER_STR},
  {MSG_MEN_BACKDROP,(STRPTR)MSG_MEN_BACKDROP_STR},
  {MSG_MEN_SC_BACKDROP,(STRPTR)MSG_MEN_SC_BACKDROP_STR},
  {MSG_MEN_EXECUTE,(STRPTR)MSG_MEN_EXECUTE_STR},
  {MSG_MEN_SC_EXECUTE,(STRPTR)MSG_MEN_SC_EXECUTE_STR},
  {MSG_MEN_SHELL,(STRPTR)MSG_MEN_SHELL_STR},
  {MSG_MEN_SC_SHELL,(STRPTR)MSG_MEN_SC_SHELL_STR},
  {MSG_MEN_GUISET,(STRPTR)MSG_MEN_GUISET_STR},
  {MSG_MEN_ABOUT,(STRPTR)MSG_MEN_ABOUT_STR},
  {MSG_MEN_SC_ABOUT,(STRPTR)MSG_MEN_SC_ABOUT_STR},
  {MSG_MEN_QUIT,(STRPTR)MSG_MEN_QUIT_STR},
  {MSG_MEN_SC_QUIT,(STRPTR)MSG_MEN_SC_QUIT_STR},
  {MSG_MEN_WINDOW,(STRPTR)MSG_MEN_WINDOW_STR},
  {MSG_MEN_NEWDRAW,(STRPTR)MSG_MEN_NEWDRAW_STR},
  {MSG_MEN_SC_NEWDRAW,(STRPTR)MSG_MEN_SC_NEWDRAW_STR},
  {MSG_MEN_OPENPAR,(STRPTR)MSG_MEN_OPENPAR_STR},
  {MSG_MEN_CLOSE,(STRPTR)MSG_MEN_CLOSE_STR},
  {MSG_MEN_SC_CLOSE,(STRPTR)MSG_MEN_SC_CLOSE_STR},
  {MSG_MEN_UPDATE,(STRPTR)MSG_MEN_UPDATE_STR},
  {MSG_MEN_CONTENTS,(STRPTR)MSG_MEN_CONTENTS_STR},
  {MSG_MEN_SC_CONTENTS,(STRPTR)MSG_MEN_SC_CONTENTS_STR},
  {MSG_MEN_CLRSEL,(STRPTR)MSG_MEN_CLRSEL_STR},
  {MSG_MEN_SC_CLRSEL,(STRPTR)MSG_MEN_SC_CLRSEL_STR},
  {MSG_MEN_SNAPSHT,(STRPTR)MSG_MEN_SNAPSHT_STR},
  {MSG_MEN_ALL,(STRPTR)MSG_MEN_ALL_STR},
  {MSG_MEN_VIEW,(STRPTR)MSG_MEN_VIEW_STR},
  {MSG_MEN_ICVIEW,(STRPTR)MSG_MEN_ICVIEW_STR},
  {MSG_MEN_DCVIEW,(STRPTR)MSG_MEN_DCVIEW_STR},
  {MSG_MEN_ALLFIL,(STRPTR)MSG_MEN_ALLFIL_STR},
  {MSG_MEN_SORTIC,(STRPTR)MSG_MEN_SORTIC_STR},
  {MSG_MEN_CLNUP,(STRPTR)MSG_MEN_CLNUP_STR},
  {MSG_MEN_SC_CLNUP,(STRPTR)MSG_MEN_SC_CLNUP_STR},
  {MSG_MEN_BYNAME,(STRPTR)MSG_MEN_BYNAME_STR},
  {MSG_MEN_BYDATE,(STRPTR)MSG_MEN_BYDATE_STR},
  {MSG_MEN_BYSIZE,(STRPTR)MSG_MEN_BYSIZE_STR},
  {MSG_MEN_REVERSE,(STRPTR)MSG_MEN_REVERSE_STR},
  {MSG_MEN_DRWFRST,(STRPTR)MSG_MEN_DRWFRST_STR},
  {MSG_MEN_ICON,(STRPTR)MSG_MEN_ICON_STR},
  {MSG_MEN_OPEN,(STRPTR)MSG_MEN_OPEN_STR},
  {MSG_MEN_SC_OPEN,(STRPTR)MSG_MEN_SC_OPEN_STR},
  {MSG_MEN_RENAME,(STRPTR)MSG_MEN_RENAME_STR},
  {MSG_MEN_SC_RENAME,(STRPTR)MSG_MEN_SC_RENAME_STR},
  {MSG_MEN_INFO,(STRPTR)MSG_MEN_INFO_STR},
  {MSG_MEN_SC_INFO,(STRPTR)MSG_MEN_SC_INFO_STR},
  {MSG_MEN_DELETE,(STRPTR)MSG_MEN_DELETE_STR},
  {MSG_MEN_TOOLS,(STRPTR)MSG_MEN_TOOLS_STR},
  {MSG_REALLYQUIT,(STRPTR)MSG_REALLYQUIT_STR},
  {MSG_YESNO,(STRPTR)MSG_YESNO_STR},
  {MSG_DUMMY,(STRPTR)MSG_DUMMY_STR},
  {0,NULL}
};

#endif /* CATCOMP_ARRAY */

/***************************************************************/


#ifdef CATCOMP_BLOCK

//static const chat CatCompBlock[] =
//{
//     
//};

#endif /* CATCOMP_BLOCK */

/***************************************************************/

struct LocaleInfo
{
  APTR li_LocaleBase;
  APTR li_Catalog;
};


#ifdef CATCOMP_CODE

STRPTR GetString(struct LocaleInfo *li, LONG stringNum)
{
  LONG *l;
  UWORD *w;
  STRPTR  builtIn;

  l = (LONG *)CatCompBlock;

  while (*l != stringNum)
  {
    w = (UWORD *)((ULONG)l + 4);
    l = (LONG *)((ULONG)l + (LONG)*w + 6);
  }
  builtIn = (STRPTR)((ULONG)l + 6);

#define XLocaleBase LocaleBase
#define LocaleBase li->li_LocaleBase

  if (LocaleBase)
    return(GetCatalogStr(li->li_Catalog,stringNum,builtIn));
#define LocaleBase XLocaleBase
#undef XLocaleBase

  return (builtIn);
}  

#endif /* CATCOMP_CODE */

/***************************************************************/


#endif
