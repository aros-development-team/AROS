#ifndef FONTLIST_CLASS_H
#define FONTLIST_CLASS_H

#include <exec/types.h>

#define FONTLIST_MBASE			TAG_USER
#define MUIM_FontList_AddDir		(FONTLIST_MBASE + 1)
#define MUIM_FontList_AddEntry		(FONTLIST_MBASE + 2)

struct MUI_CustomClass *FontListClass;

#define FontListObject	NewObject(FontListClass->mcc_Class, NULL //)

struct MUIS_FontList_Entry
{
    STRPTR FileName;
    STRPTR FamilyName;
    STRPTR StyleName;
};

void CleanupFontListClass(void);
int InitFontListClass(void);

#endif
