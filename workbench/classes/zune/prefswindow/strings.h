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

#define MSG_TEST 0
#define MSG_REVERT 1
#define MSG_SAVE 2
#define MSG_USE 3
#define MSG_CANCEL 4

#endif /* CATCOMP_NUMBERS */


/***************************************************************/

#ifdef CATCOMP_STRINGS

#define MSG_TEST_STR "_Test"
#define MSG_REVERT_STR "_Revert"
#define MSG_SAVE_STR "_Save"
#define MSG_USE_STR "_Use"
#define MSG_CANCEL_STR "_Cancel"

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
  {MSG_TEST,(STRPTR)MSG_TEST_STR},
  {MSG_REVERT,(STRPTR)MSG_REVERT_STR},
  {MSG_SAVE,(STRPTR)MSG_SAVE_STR},
  {MSG_USE,(STRPTR)MSG_USE_STR},
  {MSG_CANCEL,(STRPTR)MSG_CANCEL_STR},
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
