#ifndef _PREFS_H
#define _PREFS_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

/*** Constants **************************************************************/
#define FP_PATH_ENV      "ENV:sys/font.prefs"
#define FP_PATH_ENVARC   "ENVARC:sys/font.prefs"
#define FP_COUNT         (3)     /* FP_WBFONT, FP_SYSFONT and FP_SCREENFONT */

/*** Variables **************************************************************/
extern struct FontPrefs *fp_Current[FP_COUNT];

/*** Prototypes *************************************************************/
/* Setup ********************************************************************/
BOOL FP_Initialize(void);
void FP_Deinitialize(void);

/* File IO (high-level) *****************************************************/
BOOL FP_LoadFrom(CONST_STRPTR filename);
BOOL FP_Load(void);
BOOL FP_Test(void);
BOOL FP_Revert(void);
BOOL FP_Save(void);
BOOL FP_Use(void);
BOOL FP_Cancel(void);

#endif /* _PREFS_H */
