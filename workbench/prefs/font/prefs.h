#ifndef _PREFS_H
#define _PREFS_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Prototypes *************************************************************/
BOOL SavePrefs(CONST_STRPTR filename, struct FontPrefs **fontPrefs);
BOOL LoadPrefs(CONST_STRPTR filename, struct FontPrefs **readFontPrefs);

BOOL Prefs_Initialize(void);
void Prefs_Deinitialize(void);
void initDefaultPrefs(struct FontPrefs **fontPrefsPtr);


#endif /* _PREFS_H */
