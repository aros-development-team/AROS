#ifndef GARSHNE_MUI_H
#define GARSHNE_MUI_H

#define ID_HIDE     2
#define ID_TOGGLE   3
#define ID_INFO     4
#define ID_PREFS    5
#define ID_BLANKERS 6
#define ID_SET      7

extern Object *BlankersLvw;
extern Object *PrefsBtn, *InfoBtn, *ToggleBtn;
extern Object *HideBtn, *SettingsBtn, *QuitBtn;
extern Object *BlankWnd, *BlankApp;
extern ULONG MUI_Sigs;

ULONG ISigs( VOID );
LONG OpenInterface( VOID );
VOID CloseInterface( VOID );
LONG HandleInterface( VOID );

#endif /* GARSHNE_MUI_H */
