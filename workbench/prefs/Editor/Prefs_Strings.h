#ifndef PREFS_STRINGS_H
#define PREFS_STRINGS_H


/****************************************************************************/


/* This file was created automatically by CatComp.
 * Do NOT edit by hand!
 */


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


/****************************************************************************/


#ifdef CATCOMP_NUMBERS

#define MSG_RESETDEFAULT 0
#define MSG_LASTSAVED 1
#define MSG_TITLEWIN 2
#define MSG_TAB 3
#define MSG_SEPARATORS 4
#define MSG_TXTFONT 5
#define MSG_SCRFONT 6
#define MSG_PUBSCREEN 7
#define MSG_BACKDROP 8
#define MSG_LEFTMARGIN 9
#define MSG_AUTOINDENT 10
#define MSG_EXTEND 11
#define MSG_PREFSSAVE 12
#define MSG_USEPREFS 13
#define MSG_DISCARDPREFS 14
#define MSG_USEDEF 15
#define MSG_CHOOSEIT 16
#define MSG_CLONEPARENT 17
#define MSG_COLOR_BACK 18
#define MSG_COLOR_TEXT 19
#define MSG_COLOR_FILLTXT 20
#define MSG_COLOR_FILLSEL 21
#define MSG_COLOR_MARGINBACK 22
#define MSG_COLOR_MARGINTXT 23
#define MSG_COLOR_SHINE 24
#define MSG_COLOR_SHADE 25
#define MSG_COLOR_PANELBACK 26
#define MSG_COLOR_PANELTEXT 27
#define MSG_COLOR_GLYPH 28
#define MSG_COLOR_MARKEDLINES 29

#endif /* CATCOMP_NUMBERS */


/****************************************************************************/


#ifdef CATCOMP_STRINGS

#define MSG_RESETDEFAULT_STR "Default values"
#define MSG_LASTSAVED_STR "Last saved values"
#define MSG_TITLEWIN_STR "Main setup"
#define MSG_TAB_STR "Tabulation:"
#define MSG_SEPARATORS_STR "Separators:"
#define MSG_TXTFONT_STR "Text font:"
#define MSG_SCRFONT_STR "Screen font:"
#define MSG_PUBSCREEN_STR "Screen mode:"
#define MSG_BACKDROP_STR "Backdrop"
#define MSG_LEFTMARGIN_STR "Left margin"
#define MSG_AUTOINDENT_STR "Auto indent"
#define MSG_EXTEND_STR "Extended numeric pad"
#define MSG_PREFSSAVE_STR "Save"
#define MSG_USEPREFS_STR "Use"
#define MSG_DISCARDPREFS_STR "Cancel"
#define MSG_USEDEF_STR "Use Default"
#define MSG_CHOOSEIT_STR "Choose one..."
#define MSG_CLONEPARENT_STR "Clone parent"
#define MSG_COLOR_BACK_STR "Background"
#define MSG_COLOR_TEXT_STR "Standard text"
#define MSG_COLOR_FILLTXT_STR "Selected fill text"
#define MSG_COLOR_FILLSEL_STR "Selected text"
#define MSG_COLOR_MARGINBACK_STR "Margin background"
#define MSG_COLOR_MARGINTXT_STR "Margin text"
#define MSG_COLOR_SHINE_STR "Panel shine"
#define MSG_COLOR_SHADE_STR "Panel shade"
#define MSG_COLOR_PANELBACK_STR "Panel back"
#define MSG_COLOR_PANELTEXT_STR "Panel text"
#define MSG_COLOR_GLYPH_STR "Panel glyph"
#define MSG_COLOR_MARKEDLINES_STR "Active panel"

#endif /* CATCOMP_STRINGS */


/****************************************************************************/


struct LocaleInfo
{
    APTR li_LocaleBase;
    APTR li_Catalog;
};



#endif /* PREFS_STRINGS_H */
