/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "global.h"

#include <proto/asl.h>
#include <libraries/asl.h>
#include <string.h>		// strcpy()

extern struct Library *AslBase;
extern struct Catalog *catalogPtr;

UBYTE fileName[128]; // Path and the actual file name. Should we increase the size even more?

// Return TRUE if user made a choice, otherwise false
BOOL getFont(struct FontPrefs *currentFont)
{
    BOOL returnCode = FALSE;
    struct FontRequester *fontReq;

    // Pass ASLXX_SleepWindow to this call
    if((fontReq = AllocAslRequestTags(ASL_FontRequest,
	ASL_FuncFlags,	FONF_FRONTCOLOR | FONF_BACKCOLOR,
	ASL_FontName, currentFont->fp_TextAttr.ta_Name,
	ASL_FontHeight, currentFont->fp_TextAttr.ta_YSize,
	TAG_DONE)))

    {
	kprintf("AllocAslRequest ok!\n");

	if(AslRequest(fontReq, NULL))
	{
	    strcpy(currentFont->fp_Name, fontReq->fo_TAttr.tta_Name);
	    kprintf("currentFont->fp_Name is >%s<\n", currentFont->fp_Name);

	    currentFont->fp_TextAttr.ta_Name = currentFont->fp_Name;

	    kprintf("currentFont->fp_TextAttr.ta_Name is >%s<\n", currentFont->fp_TextAttr.ta_Name);

	    currentFont->fp_TextAttr.ta_YSize = fontReq->fo_TAttr.tta_YSize;
	    currentFont->fp_TextAttr.ta_Style = fontReq->fo_TAttr.tta_Style; // What is tta_Style?
	    currentFont->fp_TextAttr.ta_Flags = fontReq->fo_TAttr.tta_Flags; // What is tta_Flags?

	    // These values can't be set right now as the ASL fontrequester is missing the necessary features
	    currentFont->fp_FrontPen = fontReq->fo_FrontPen;
	    currentFont->fp_BackPen = fontReq->fo_BackPen;
	    currentFont->fp_DrawMode = fontReq->fo_DrawMode;

	    returnCode = TRUE; // User made a choice!
	}
	else
	    kprintf("Unable to open font requester - likely cancelled\n");

	FreeAslRequest(fontReq);
    }
    else
	printf("%s\n", getCatalog(catalogPtr, MSG_CANT_CREATE_REQ));
}

STRPTR aslOpenPrefs()
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
	    kprintf("Requester cancelled?\n");

	FreeAslRequest(fileReq);
    }
    else
	printf("%s\n", getCatalog(catalogPtr, MSG_CANT_CREATE_REQ));

    if(fileName[0]) // Does the string contain anything to return?
	return(fileName);
    else
	return(FALSE);
}

STRPTR aslSavePrefs()
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
	    kprintf("Requester cancelled?\n");

	FreeAslRequest(fileReq);
    }
    else
	printf("%s\n", getCatalog(catalogPtr, MSG_CANT_CREATE_REQ));

    if(fileName[0]) // Does the string contain anything to return?
	return(fileName);
    else
	return(FALSE);
}
