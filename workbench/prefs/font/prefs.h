#ifndef _PREFS_H
#define _PREFS_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Prototypes *************************************************************/
/* Setup ********************************************************************/
BOOL Prefs_Initialize(void);
void Prefs_Deinitialize(void);
void initDefaultPrefs(struct FontPrefs **fontPrefsPtr);

/* Main *********************************************************************/
BOOL WritePrefs(CONST_STRPTR filename, struct FontPrefs **fontPrefs);
BOOL ReadPrefs(CONST_STRPTR filename, struct FontPrefs **readFontPrefs);

#endif /* _PREFS_H */
