#include <proto/exec.h>
#include <proto/asl.h>
#include <libraries/asl.h>
#include <dos/dos.h> /* Return codes */
#include <prefs/font.h>
#include <stdio.h>

extern void quitApp(UBYTE *, UBYTE);  

struct Library *AslBase;

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
    printf("Opened font requester!\n");
    printf("Font: >%s< tta_YSize: %d tta_Style: %d Flags: %d\n", fontReq->fo_TAttr.tta_Name,
fontReq->fo_TAttr.tta_YSize, fontReq->fo_TAttr.tta_Style, fontReq->fo_TAttr.tta_Flags);
    printf("fo_FrontPen: %d fo_BackPen: %d fo_DrawMode: %d\n", fontReq->fo_FrontPen, fontReq->fo_BackPen,
 fontReq->fo_DrawMode);


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
