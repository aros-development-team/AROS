/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    RequestFile CLI command.
*/

/*****************************************************************************

    NAME

        RequestFile

    SYNOPSIS

        DRAWER,FILE/K,PATTERN/K,TITLE/K,POSITIVE/K,NEGATIVE/K,
        ACCEPTPATTERN/K,REJECTPATTERN/K,SAVEMODE/S,MULTISELECT/S,
        DRAWERSONLY/S,NOICONS/S,PUBSCREEN/K,INITIALVOLUMES/S

    LOCATION

        C:

    FUNCTION
    
        Creates file requester. The selected files will be displayed separated
        by spaces. If no file is selected the return code is 5 (warn).
    
    INPUTS
        DRAWER          -- initial content of drawer field
        FILE            -- initial content of file field
        PATTERN         -- content of pattern field (e.g. #?.c)
        TITLE           -- title of the dialog box
        POSITIVE        -- string for the left button
        NEGATIVE        -- string for the right button
        ACCEPTPATTERN   -- only files which match the pattern are displayed
        REJECTPATTERN   -- files which match the pattern aren't displayed
        SAVEMODE        -- requester is displayed as save requester
        MULTISELECT     -- more than one file can be selected
        DRAWERSONLY     -- only drawers are displayed
        NOICONS         -- no icon files (#?.info) are displayed
        PUBSCREEN       -- requester is opened on the given public screen
        INITIALVOLUMES  -- shows the volumes
        
    RESULT

        Standard DOS error codes.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

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

#include <string.h>

#define ARG_TEMPLATE    "DRAWER,FILE/K,PATTERN/K,TITLE/K,POSITIVE/K," \
                        "NEGATIVE/K,ACCEPTPATTERN/K,REJECTPATTERN/K," \
                        "SAVEMODE/S,MULTISELECT/S,DRAWERSONLY/S," \
                        "NOICONS/S,PUBSCREEN/K,INITIALVOLUMES/S"
			
#define MAX_PATH_LEN    512
			

enum { ARG_DRAWER = 0, ARG_FILE, ARG_PATTERN, ARG_TITLE, ARG_POSITIVE,
       ARG_NEGATIVE, ARG_ACCEPTPAT, ARG_REJECTPAT, ARG_SAVEMODE,
       ARG_MULTISELECT, ARG_DRAWERSONLY, ARG_NOICONS, ARG_PUBSCREEN,
       ARG_INITIALVOLUMES, TOTAL_ARGS };

const TEXT version[] = "$VER: RequestFile 42.4 (3.4.2014)\n";

struct TagItem FileTags[] =
{
    /* Note: The ordering of these is _important_! */
    { ASLFR_InitialDrawer , (IPTR) NULL  },
    { ASLFR_InitialFile   , (IPTR) NULL  },
    { ASLFR_InitialPattern, (IPTR) NULL  },
    { ASLFR_TitleText     , (IPTR) NULL  },
    { ASLFR_PositiveText  , (IPTR) NULL  },
    { ASLFR_NegativeText  , (IPTR) NULL  },
    { ASLFR_AcceptPattern , (IPTR) NULL  },
    { ASLFR_RejectPattern , (IPTR) NULL  },
    { ASLFR_DoSaveMode    ,        FALSE },
    { ASLFR_DoMultiSelect ,        FALSE },
    { ASLFR_DrawersOnly   ,        FALSE },
    { ASLFR_RejectIcons   ,        FALSE },
    { ASLFR_PubScreenName , (IPTR) NULL  },
    { ASLFR_DoPatterns    ,        FALSE },
    { ASLFR_InitialShowVolumes,     TRUE },
    { TAG_DONE            , (IPTR) NULL  }
};

int __nocommandline;

static UBYTE *ParsePatternArg(IPTR **args, UWORD ArgNum);

int main(void)
{
    struct RDArgs        *rda;
    struct FileRequester *FileReq;
    struct WBArg         *WBFiles;
    IPTR                 *args[TOTAL_ARGS] = { NULL, NULL, NULL, NULL,
					       NULL, NULL, NULL, NULL,
					       NULL, NULL, NULL, NULL,
					       NULL, NULL };
    int                   Return_Value = RETURN_OK;
    IPTR                  DisplayArgs[1];
    char                 *Buffer;
    BOOL                  Success;
    int                   i;

    Buffer = (char *)AllocVec(MAX_PATH_LEN, MEMF_ANY | MEMF_CLEAR);
    if (Buffer != NULL)
    {
        rda = ReadArgs(ARG_TEMPLATE, (IPTR *)args, NULL);
        if(rda != NULL)
        {
            FileTags[ARG_DRAWER].ti_Data        = (IPTR)args[ARG_DRAWER];
            FileTags[ARG_FILE].ti_Data          = (IPTR)args[ARG_FILE];
            FileTags[ARG_PATTERN].ti_Data       = (IPTR)args[ARG_PATTERN];
            FileTags[ARG_TITLE].ti_Data         = (IPTR)args[ARG_TITLE];
            FileTags[ARG_POSITIVE].ti_Data      = (IPTR)args[ARG_POSITIVE];
            FileTags[ARG_NEGATIVE].ti_Data      = (IPTR)args[ARG_NEGATIVE];
            ParsePatternArg(args, ARG_ACCEPTPAT);
            ParsePatternArg(args, ARG_REJECTPAT);
            FileTags[ARG_SAVEMODE].ti_Data      = (IPTR)args[ARG_SAVEMODE];
            FileTags[ARG_MULTISELECT].ti_Data   = (IPTR)args[ARG_MULTISELECT];
            FileTags[ARG_DRAWERSONLY].ti_Data   = (IPTR)args[ARG_DRAWERSONLY];
            FileTags[ARG_NOICONS].ti_Data       = (IPTR)args[ARG_NOICONS];
            FileTags[ARG_PUBSCREEN].ti_Data     = (IPTR)args[ARG_PUBSCREEN];
            FileTags[ARG_PUBSCREEN + 1].ti_Data = args[ARG_PATTERN] != NULL;
	    if (!args[ARG_INITIALVOLUMES])
	    {
		FileTags[ARG_INITIALVOLUMES + 1].ti_Tag = TAG_IGNORE;
	    }

            FileReq = (struct FileRequester *)AllocAslRequest(ASL_FileRequest,
                FileTags);
            if(FileReq != NULL)
            {
                Success = AslRequest(FileReq, NULL); 

                if(Success != FALSE)
                {
                    if(!(IPTR)args[ARG_MULTISELECT])
                    {
			strncpy(Buffer, FileReq->fr_Drawer, MAX_PATH_LEN);
			
                        /* FileReq->fr_File is NULL when using DRAWERSONLY */
                        Success = AddPart(Buffer,
                                          FileReq->fr_File ? FileReq->fr_File : (STRPTR)"",
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
                PrintFault(IoErr(), "RequestFile");
		Return_Value = RETURN_ERROR;
            }
	    
            FreeArgs(rda);
        }
        else
        {
            PrintFault(IoErr(), "RequestFile");
	    Return_Value = RETURN_ERROR;
        }
	
        FreeVec((APTR)FileTags[ARG_ACCEPTPAT].ti_Data);
        FreeVec((APTR)FileTags[ARG_REJECTPAT].ti_Data);
        FreeVec(Buffer);
    }
    else
    {
	PrintFault(IoErr(), NULL);
	Return_Value = RETURN_FAIL;
    }
    
    return Return_Value;
    
} /* main */

static UBYTE *ParsePatternArg(IPTR **args, UWORD ArgNum)
{
    if (!args[ArgNum]) /* AROS crashes on strlen(NULL) */
    {
        FileTags[ArgNum].ti_Tag = TAG_IGNORE;
        return NULL;
    }

    UBYTE *PatternBuffer = NULL;
    LONG PatternBufferSize;
    STRPTR pattern = (STRPTR)args[ArgNum];

    PatternBufferSize = 2 * strlen((char *)pattern);
    PatternBuffer = AllocVec(PatternBufferSize, MEMF_PUBLIC);
    if (PatternBuffer != NULL)
    {
        if (ParsePatternNoCase((STRPTR)pattern,
            PatternBuffer, PatternBufferSize) >= 0)
        {
            FileTags[ArgNum].ti_Data = (IPTR)PatternBuffer;
        }
        else
        {
            FreeVec(PatternBuffer);
            PatternBuffer = NULL;
            FileTags[ArgNum].ti_Tag = TAG_IGNORE;
        }
    }

    return PatternBuffer;
}
