/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RequestFile CLI command
    Lang: English
*/

/*****************************************************************************

    NAME

        RequestFile

    SYNOPSIS

        DRAWER,FILE/K,PATTERN/K,TITLE/K,POSITIVE/K,NEGATIVE/K,
        ACCEPTPATTERN/K,REJECTPATTERN/K,SAVEMODE/S,MULTISELECT/S,
        DRAWERSONLY/S,NOICONS/S,PUBSCREEN/K

    LOCATION

        Workbench:c

    FUNCTION

    INPUTS

    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
        ?         laguest  Initial version
	29.12.99  SDuvan   Fixes to make it compile; bugfixes; cleanup

******************************************************************************/

#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <dos/dos.h>
#include <dos/rdargs.h>
#include <dos/var.h>
#include <dos/dosasl.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/asl.h>
#include <utility/tagitem.h>
#include <workbench/startup.h>

#include <stdio.h>
#include <string.h>

#define ARG_TEMPLATE    "DRAWER,FILE/K,PATTERN/K,TITLE/K,POSITIVE/K," \
                        "NEGATIVE/K,ACCEPTPATTERN/K,REJECTPATTERN/K," \
                        "SAVEMODE/S,MULTISELECT/S,DRAWERSONLY/S," \
                        "NOICONS/S,PUBSCREEN/K"
			
#define MAX_PATH_LEN    512
			

enum { ARG_DRAWER = 0, ARG_FILE, ARG_PATTERN, ARG_TITLE, ARG_POSITIVE,
       ARG_NEGATIVE, ARG_ACCEPTPAT, ARG_REJECTPAT, ARG_SAVEMODE,
       ARG_MULTISELECT, ARG_DRAWERSONLY, ARG_NOICONS, ARG_PUBSCREEN,
       TOTAL_ARGS };

static const char version[] = "$VER: RequestFile 41.1 (29.12.1999)\n";

extern struct Library *AslBase;

struct TagItem FileTags[] =
{
    /* Note: The ordering of these is _important_! */
    { ASLFR_InitialDrawer , NULL  },
    { ASLFR_InitialFile   , NULL  },
    { ASLFR_InitialPattern, NULL  },
    { ASLFR_TitleText     , NULL  },
    { ASLFR_PositiveText  , NULL  },
    { ASLFR_NegativeText  , NULL  },
    { ASLFR_AcceptPattern , NULL  },
    { ASLFR_RejectPattern , NULL  },
    { ASLFR_DoSaveMode    , FALSE },
    { ASLFR_DoMultiSelect , FALSE },
    { ASLFR_DrawersOnly   , FALSE },
    { ASLFR_RejectIcons   , FALSE },
    { ASLFR_PubScreenName , NULL  },
    { ASLFR_DoPatterns    , FALSE },
    { TAG_DONE            , NULL  }
};

int __nocommandline;

int main(void)
{
    struct RDArgs        *rda;
    struct FileRequester *FileReq;
    struct WBArg         *WBFiles;
    IPTR                 *args[TOTAL_ARGS] = { NULL, NULL, NULL, NULL,
					       NULL, NULL, NULL, NULL,
					       NULL, NULL, NULL, NULL,
					       NULL };
    int                   Return_Value = RETURN_OK;
    IPTR                  DisplayArgs[1];
    char                 *Buffer;
    BOOL                  Success;
    int                   i;

#define  DoSave    (args[ARG_SAVEMODE] != NULL)
#define  DoMulti   (args[ARG_MULTISELECT] != NULL)
#define  DoDrawers (args[ARG_DRAWERSONLY] != NULL)
#define  DoIcons   (args[ARG_NOICONS] != NULL)
#define  DoPattern (args[ARG_PATTERN] != NULL)

    Buffer = (char *)AllocVec(MAX_PATH_LEN, MEMF_ANY | MEMF_CLEAR);
    if (Buffer != NULL)
    {
        rda = ReadArgs(ARG_TEMPLATE, (IPTR *)args, NULL);
        if(rda != NULL)
        {
            FileTags[ARG_DRAWER].ti_Data        = (ULONG)args[ARG_DRAWER];
            FileTags[ARG_FILE].ti_Data          = (ULONG)args[ARG_FILE];
            FileTags[ARG_PATTERN].ti_Data       = (ULONG)args[ARG_PATTERN];
            FileTags[ARG_TITLE].ti_Data         = (ULONG)args[ARG_TITLE];
            FileTags[ARG_POSITIVE].ti_Data      = (ULONG)args[ARG_POSITIVE];
            FileTags[ARG_NEGATIVE].ti_Data      = (ULONG)args[ARG_NEGATIVE];
            FileTags[ARG_ACCEPTPAT].ti_Data     = (ULONG)args[ARG_ACCEPTPAT];
            FileTags[ARG_REJECTPAT].ti_Data     = (ULONG)args[ARG_REJECTPAT];
            FileTags[ARG_SAVEMODE].ti_Data      = DoSave;
            FileTags[ARG_MULTISELECT].ti_Data   = DoMulti;
            FileTags[ARG_DRAWERSONLY].ti_Data   = DoDrawers;
            FileTags[ARG_NOICONS].ti_Data       = DoIcons;
            FileTags[ARG_PUBSCREEN].ti_Data     = (ULONG)args[ARG_PUBSCREEN];
            FileTags[ARG_PUBSCREEN + 1].ti_Data = DoPattern;

            FileReq = (struct FileRequester *)AllocAslRequest(ASL_FileRequest,
                                                              FileTags);
            if(FileReq != NULL)
            {
                Success = AslRequest(FileReq, NULL); 

                if(Success != FALSE)
                {
                    if(DoMulti == FALSE)
                    {
			strncpy(Buffer, FileReq->fr_Drawer, MAX_PATH_LEN);
			
                        Success = AddPart(Buffer,
                                          FileReq->fr_File,
                                          MAX_PATH_LEN);

                        if(Success != FALSE)
                        {
                            DisplayArgs[0] = (IPTR)Buffer;
                            VPrintf("\"%s\"\n", DisplayArgs);
                        }
                    }
                    else
                    {
                        WBFiles = FileReq->fr_ArgList;
			
                	for (i = 0; i != FileReq->fr_NumArgs; i++)
                	{
			    strncpy(Buffer, FileReq->fr_Drawer, MAX_PATH_LEN);

                            Success = AddPart(Buffer,
                                              WBFiles[i].wa_Name,
                                              MAX_PATH_LEN);

                            if(Success != FALSE)
                            {
                                DisplayArgs[0] = (IPTR)Buffer;
                                VPrintf("\"%s\" ", DisplayArgs);
                            }
                        }    
			
                        PutStr("\n");
                    }
                }
                else
                {
                    if (IoErr() == 0)
                    {
                        /* We cancelled it!!
                         */
                        Return_Value = RETURN_WARN;
			
                        SetIoErr(ERROR_BREAK);
                    }
                    else
                    {
			PrintFault(IoErr(), NULL);
                        Return_Value = RETURN_FAIL;
                    }
                }
		
                FreeAslRequest((struct FileRequester *)FileReq);
            }
            else
            {
                PrintFault(IoErr(), "RequestFile #2");
		Return_Value = RETURN_ERROR;
            }
	    
            FreeArgs(rda);
        }
        else
        {
            PrintFault(IoErr(), "RequestFile #1");
	    Return_Value = RETURN_ERROR;
        }
	
        FreeVec(Buffer);
    }
    else
    {
	SetIoErr(ERROR_NO_FREE_STORE);
	PrintFault(IoErr(), NULL);
	Return_Value = RETURN_FAIL;
    }
    
    return Return_Value;
    
} /* main */
