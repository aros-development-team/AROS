/****************************************************************
   This file was created automatically by `FlexCat 2.4'
   from "preferenceswindow.cd".

   Do NOT edit by hand!
****************************************************************/

#ifndef preferenceswindow_H
#define preferenceswindow_H


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

#define MSG_OK 0
#define MSG_RESUME 1
#define MSG_WARNING_GUI 2
#define MSG_LOGINREQ_GUI 3
#define MSG_LOGINPROMPT_GUI 4

#endif /* CATCOMP_NUMBERS */


/***************************************************************/

#ifdef CATCOMP_STRINGS

#define MSG_OK_STR "_OK"
#define MSG_RESUME_STR "_Resume"
#define MSG_WARNING_GUI_STR "Security Warning"
#define MSG_LOGINREQ_GUI_STR "Security Login Request"
#define MSG_LOGINPROMPT_GUI_STR "\33cWelcome to the AROS Research OS\nAROS Version: %s\n\nPlease fill in your user details\nto log onto %s."

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
  {MSG_OK,(STRPTR)MSG_OK_STR},
  {MSG_RESUME,(STRPTR)MSG_RESUME_STR},
  {MSG_WARNING_GUI,(STRPTR)MSG_WARNING_GUI_STR},
  {MSG_LOGINREQ_GUI,(STRPTR)MSG_LOGINREQ_GUI_STR},
  {MSG_LOGINPROMPT_GUI,(STRPTR)MSG_LOGINPROMPT_GUI_STR},
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
