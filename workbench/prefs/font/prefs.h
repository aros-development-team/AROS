#ifndef _PREFS_H
#define _PREFS_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Constants **************************************************************/
#define FP_PATH_ENV      "ENV:sys/font.prefs"
#define FP_PATH_ENVARC   "ENVARC:sys/font.prefs"
#define FP_COUNT         (3)     /* FP_WBFONT, FP_SYSFONT and FP_SCREENFONT */

/*** Prototypes *************************************************************/
/* Setup ********************************************************************/
BOOL FP_Initialize(void);
void FP_Deinitialize(void);
void initDefaultPrefs(struct FontPrefs **fontPrefsPtr);

/* File IO (high-level) *****************************************************/
BOOL FP_Test(void);
BOOL FP_Revert(void);
BOOL FP_Save(void);
BOOL FP_Use(void);
BOOL FP_Cancel(void);

/* File IO (low-level) ******************************************************/
BOOL FP_Write(CONST_STRPTR filename, struct FontPrefs **fontPrefs);
BOOL FP_Read(CONST_STRPTR filename, struct FontPrefs **readFontPrefs);

#endif /* _PREFS_H */
