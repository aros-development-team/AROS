#ifndef FONTBITMAP_CLASS_H
#define FONTBITMAP_CLASS_H

#define FONTBITMAP_BASE			TAG_USER
#define MUIA_FontBitmap_Filename	(FONTBITMAP_BASE + 1)
#define MUIA_FontBitmap_OTags		(FONTBITMAP_BASE + 2)
#define MUIA_FontBitmap_Size		(FONTBITMAP_BASE + 3)
#define MUIA_FontBitmap_String		(FONTBITMAP_BASE + 4)
#define MUIA_FontBitmap_Gray		(FONTBITMAP_BASE + 5)

struct MUI_CustomClass *FontBitmapClass;

#define FontBitmapObject	NewObject(FontBitmapClass->mcc_Class, NULL //)

void CleanupFontBitmapClass(void);
int InitFontBitmapClass(void);

#endif
