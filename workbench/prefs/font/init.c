/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <dos/dos.h> /* Return codes */
#include <prefs/font.h>
#include <exec/memory.h>
#include <stdio.h>

struct FontPrefs *fontPrefs[3];

void initDefaultPrefs(struct FontPrefs **fontPrefsPtr)
{
 UBYTE a;

 for(a = 0; a <= 2; a++)
 {
  fontPrefs[a]->fp_Type = a;		/* Is this 0, 1, 2 or 1, 2, 3? Look it up! */
  fontPrefs[a]->fp_FrontPen = 0;	/* Is this (really) default? Look it up! */
  fontPrefs[a]->fp_BackPen = 0;		/* Is this (really) default? Look it up! */
  fontPrefs[a]->fp_DrawMode = 0;	/* Is this (really) default? Look it up! */

  fontPrefs[a]->fp_TextAttr.ta_YSize = 8; /* Is this (really) default? Look it up! */
  fontPrefs[a]->fp_TextAttr.ta_Style = 0; /* Is this (really) default? Look it up! */
  fontPrefs[a]->fp_TextAttr.ta_Flags = 0; /* Is this (really) default? Look it up! */

  strcpy(fontPrefs[a]->fp_Name, "topaz.font"); /* Is this (really) default? Check it up! */
  fontPrefs[a]->fp_TextAttr.ta_Name = fontPrefs[a]->fp_Name;
 }
}

BOOL initPrefMem(void)
{
 UBYTE a;

 for(a = 0; a <= 2; a++)
  if(!(fontPrefs[a] = AllocMem(sizeof(struct FontPrefs), MEMF_ANY | MEMF_CLEAR)))
   return(FALSE); /* Some structures may have been allocated */

 initDefaultPrefs(fontPrefs);
 
 return(TRUE);
}
