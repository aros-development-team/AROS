#ifndef FONTINFO_CLASS_H
#define FONTINFO_CLASS_H

#define FONTINFO_BASE			TAG_USER
#define MUIA_FontInfo_Filename		(FONTINFO_BASE + 1)
#define MUIA_FontInfo_Face		(FONTINFO_BASE + 2)

#define FONTINFO_MBASE			TAG_USER
#define MUIM_FontInfo_UpdatePreview	(FONTINFO_MBASE + 1)
#define MUIM_FontInfo_SetOTags		(FONTINFO_MBASE + 2)
#define MUIM_FontInfo_WriteFiles	(FONTINFO_MBASE + 3)

struct MUI_CustomClass *FontInfoClass;

#define FontInfoObject	NewObject(FontInfoClass->mcc_Class, NULL //)

void CleanupFontInfoClass(void);
int InitFontInfoClass(void);

#endif
