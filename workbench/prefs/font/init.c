#include <proto/exec.h>
#include <dos/dos.h> /* Return codes */
#include <prefs/font.h>
#include <exec/memory.h>
#include <stdio.h>

struct FontPrefs *fontPrefs[3];

extern void quitApp(UBYTE *, UBYTE);

void initPrefMem(void)
{
 UBYTE a;

 for(a = 0; a <= 2; a++)
 {
  if(!(fontPrefs[a] = AllocMem(sizeof(struct FontPrefs), MEMF_ANY | MEMF_CLEAR)))
   quitApp("Unable to allocate FontPrefs memory!", RETURN_FAIL);

  fontPrefs[a]->fp_Type = a;
  fontPrefs[a]->fp_FrontPen = 0; /* Is this (really) default? Check it up! */
  fontPrefs[a]->fp_BackPen = 0; /* Is this (really) default? Check it up! */
  fontPrefs[a]->fp_DrawMode = 0; /* Is this (really) default? Check it up! */

  fontPrefs[a]->fp_TextAttr.ta_YSize = 8; /* Is this (really) default? Check it up! */
  fontPrefs[a]->fp_TextAttr.ta_Style = 0; /* Is this (really) default? Check it up! */
  fontPrefs[a]->fp_TextAttr.ta_Flags = 0; /* Is this (really) default? Check it up! */

  strcpy(fontPrefs[a]->fp_Name, "topaz.font"); /* Is this (really) default? Check it up! */
  fontPrefs[a]->fp_TextAttr.ta_Name = fontPrefs[a]->fp_Name;
 }
}
