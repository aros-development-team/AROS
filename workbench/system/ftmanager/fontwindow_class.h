#ifndef FONTWINDOW_CLASS_H
#define FONTWINDOW_CLASS_H

#define FONTWINDOW_BASE			TAG_USER
#define MUIA_FontWindow_Filename	(FONTWINDOW_BASE + 1)

struct MUI_CustomClass *FontWindowClass;

#define FontWindowObject	NewObject(FontWindowClass->mcc_Class, NULL //)

struct Hook CloseWinHook;

void CleanupFontWindowClass(void);
int InitFontWindowClass(void);

#endif
