/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/asl.h>
#include <proto/dos.h> /* AddPart() */
#include <libraries/locale.h>   // Struct Catalog
#include <libraries/asl.h>
#include <dos/dos.h> /* Return codes */
#include <prefs/font.h>
#include <stdio.h>

#define CATCOMP_NUMBERS
#include "fontprefs_strings.h"

extern void quitApp(UBYTE *, UBYTE);  

extern struct Library *AslBase;
extern struct Catalog *catalogPtr;

UBYTE fileName[128]; /* Path and the actual file name. Should we increase the size even more? */

/* Return TRUE if user made a choice, otherwise false */
BOOL getFont(struct FontPrefs *currentFont)
{
 BOOL returnCode = FALSE;
 struct FontRequester *fontReq;

 /* Pass ASLXX_SleepWindow to this call*/
 if((fontReq = AllocAslRequestTags(ASL_FontRequest,
							ASL_FuncFlags,	FONF_FRONTCOLOR | FONF_BACKCOLOR,
							TAG_DONE)))

 {
  printf("AllocAslRequest ok!\n");

  if(AslRequest(fontReq, NULL))
  {
/*
    printf("Opened font requester!\n");
    printf("Font: >%s< tta_YSize: %d tta_Style: %d Flags: %d\n", fontReq->fo_TAttr.tta_Name,
fontReq->fo_TAttr.tta_YSize, fontReq->fo_TAttr.tta_Style, fontReq->fo_TAttr.tta_Flags);
    printf("fo_FrontPen: %d fo_BackPen: %d fo_DrawMode: %d\n", fontReq->fo_FrontPen, fontReq->fo_BackPen,
 fontReq->fo_DrawMode);
*/

   strcpy(currentFont->fp_Name, fontReq->fo_TAttr.tta_Name);
   printf("currentFont->fp_Name is >%s<\n", currentFont->fp_Name);

   currentFont->fp_TextAttr.ta_Name = currentFont->fp_Name;

   printf("currentFont->fp_TextAttr.ta_Name is >%s<\n", currentFont->fp_TextAttr.ta_Name);

   currentFont->fp_TextAttr.ta_YSize = fontReq->fo_TAttr.tta_YSize;
   currentFont->fp_TextAttr.ta_Style = fontReq->fo_TAttr.tta_Style; // What is tta_Style?
   currentFont->fp_TextAttr.ta_Flags = fontReq->fo_TAttr.tta_Flags; // What is tta_Flags?

   /* These values can't be set right now as the ASL fontrequester is missing the necessary features */
   currentFont->fp_FrontPen = fontReq->fo_FrontPen;
   currentFont->fp_BackPen = fontReq->fo_BackPen;
   currentFont->fp_DrawMode = fontReq->fo_DrawMode;

   returnCode = TRUE; /* User made a choice! */
  }
  else
   printf("Unable to open font requester - likely cancelled\n");

  FreeAslRequest(fontReq);
 }
 else
  printf("%s\n", getCatalog(catalogPtr, MSG_CANT_CREATE_REQ));
}

STRPTR aslOpenPrefs(void)
{
 struct FileRequester *fileReq;

 if((fileReq = AllocAslRequestTags(ASL_FileRequest, TAG_END)))
 {
  if(AslRequestTags(fileReq, ASL_Hail, getCatalog(catalogPtr, MSG_ASL_OPEN_TITLE), TAG_END))
  {
   strcpy(fileName, fileReq->rf_Dir);
   AddPart(fileName, fileReq->rf_File, sizeof(fileName)); /* Check for success! */
  }
  else
   printf("Requester cancelled?\n");

  FreeAslRequest(fileReq);
 }
 else
  printf("%s\n", getCatalog(catalogPtr, MSG_CANT_CREATE_REQ));

 if(fileName[0]) // Does the string contain anything to return?
  return(fileName);
 else
  return(FALSE);
}

STRPTR aslSavePrefs(void)
{
 struct FileRequester *fileReq;

 if((fileReq = AllocAslRequestTags(ASL_FileRequest, ASL_FuncFlags, FILF_SAVE, TAG_END)))
 {
  if(AslRequestTags(fileReq, ASL_Hail, getCatalog(catalogPtr, MSG_ASL_SAVE_TITLE), TAG_END))
  {
   strcpy(fileName, fileReq->rf_Dir);
   AddPart(fileName, fileReq->rf_File, sizeof(fileName)); /* Check for success! */
  }
  else
   printf("Requester cancelled?\n");

  FreeAslRequest(fileReq);
 }
 else
  printf("%s\n", getCatalog(catalogPtr, MSG_CANT_CREATE_REQ));

 if(fileName[0]) // Does the string contain anything to return?
  return(fileName);
 else
  return(FALSE);
}
