#include <proto/exec.h>
#include <proto/asl.h>
#include <proto/dos.h> /* AddPart() */
#include <libraries/asl.h>
#include <dos/dos.h> /* Return codes */
#include <prefs/font.h>
#include <stdio.h>

extern void quitApp(UBYTE *, UBYTE);  

struct Library *AslBase;

UBYTE fileName[128]; /* Path and the actual file name. Should we increase the size even more? */

/* Return TRUE if user made a choice, otherwise false */
BOOL getFont(struct FontPrefs *currentFont)
{
 BOOL returnCode = FALSE;
 struct FontRequester *fontReq;

 if(AslBase = OpenLibrary("asl.library", 0L))
 {
  /* Pass ASLXX_SleepWindow to this call*/
  if(fontReq = AllocAslRequestTags(ASL_FontRequest,
							ASL_FuncFlags,	FONF_FRONTCOLOR | FONF_BACKCOLOR,
							TAG_DONE))

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

  CloseLibrary(AslBase);
 }
 else
  quitApp("Can't open asl.library!", RETURN_FAIL);
}

STRPTR aslOpenPrefs(void)
{
 struct FileRequester *fileReq;

 if(AslBase = OpenLibrary("asl.library", 0))
 {
  if(fileReq = AllocAslRequestTags(ASL_FileRequest, TAG_END))
  {
   if(AslRequest(fileReq, NULL))
   {
    strcpy(fileName, fileReq->rf_Dir);
    AddPart(fileName, fileReq->rf_File, sizeof(fileName)); /* Check for success! */
   }

   FreeAslRequest(fileReq);
  }
  else
   printf("Unable to open ASL requester!\n");

  CloseLibrary(AslBase);
 }
 else
  /* We don't need to bail out in this case - do we? */
  printf("Can't open asl.library!");

 if(fileName) /* Is fileName guaranteed to be empty if user didn't choose anything? Look up! */
  return(fileName);
 else
  return(FALSE);
}

STRPTR aslSavePrefs(void)
{
 struct FileRequester *fileReq;

 if(AslBase = OpenLibrary("asl.library", 0))
 {
  if(fileReq = AllocAslRequestTags(ASL_FileRequest, ASL_FuncFlags, FILF_SAVE, TAG_END))
  {
   if(AslRequest(fileReq, NULL))
   {
    strcpy(fileName, fileReq->rf_Dir);
    AddPart(fileName, fileReq->rf_File, sizeof(fileName)); /* Check for success! */
   }

   FreeAslRequest(fileReq);
  }
  else
   printf("Unable to open ASL requester!\n");

  CloseLibrary(AslBase);
 }
 else
  /* We don't need to bail out altogether in this case - do we? */
  printf("Can't open asl.library!");

 if(fileName) /* Is fileName guaranteed to be empty if user didn't choose anything? Look up! */
  return(fileName);
 else
  return(FALSE);
}
