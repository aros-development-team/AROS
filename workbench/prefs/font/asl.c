/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/asl.h>
#include <prefs/font.h>

#include <proto/asl.h>
#include <proto/dos.h>

#include <string.h>
#include <stdio.h>

#include "locale.h"

#define DEBUG 1
#include <aros/debug.h>

TEXT fileName[128]; // Path and the actual file name. Should we increase the size even more?

STRPTR aslOpenPrefs()
{
    struct FileRequester *fileReq;

    if((fileReq = AllocAslRequestTags(ASL_FileRequest, TAG_END)))
    {
	if(AslRequestTags(fileReq, ASL_Hail, MSG(MSG_ASL_OPEN_TITLE), TAG_END))
	{
	    strcpy(fileName, fileReq->rf_Dir);
	    AddPart(fileName, fileReq->rf_File, sizeof(fileName)); /* Check for success! */
	}
	else
	    kprintf("Requester cancelled?\n");

	FreeAslRequest(fileReq);
    }
    else
	printf("%s\n", MSG(MSG_CANT_CREATE_REQ));

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
	if(AslRequestTags(fileReq, ASL_Hail, MSG(MSG_ASL_SAVE_TITLE), TAG_END))
	{
	    strcpy(fileName, fileReq->rf_Dir);
	    AddPart(fileName, fileReq->rf_File, sizeof(fileName)); /* Check for success! */
	}
	else
	    kprintf("Requester cancelled?\n");

	FreeAslRequest(fileReq);
    }
    else
	printf("%s\n", MSG(MSG_CANT_CREATE_REQ));

    if(fileName[0]) // Does the string contain anything to return?
	return(fileName);
    else
	return(FALSE);
}
