/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <libraries/mui.h>
#include <libraries/asl.h>
#include <prefs/font.h>

#include <proto/dos.h>
#include <proto/muimaster.h>

#include <string.h>

#include "locale.h"
#include "asl.h"

#define DEBUG 1
#include <aros/debug.h>

/*** Functions **************************************************************/
STRPTR ASL_SelectFile(ULONG mode)
{
    struct FileRequester *request  = NULL;
    STRPTR                filename = NULL;
    
    request = MUI_AllocAslRequestTags
    (
        ASL_FileRequest,
        
        mode == ASL_MODE_EXPORT ?
                ASL_FuncFlags   :
                TAG_IGNORE,       FILF_SAVE,
        
        TAG_DONE
    );
        
    if (request == NULL)
    {
        /* Allocation failed */
        /* FIXME: error dialog? */
        return NULL;
    }
     
    if
    (
        MUI_AslRequestTags
        (
            request,
            
            ASL_Hail, mode == ASL_MODE_EXPORT ? 
                      (IPTR) MSG(MSG_ASL_SAVE_TITLE) : 
                      (IPTR) MSG(MSG_ASL_OPEN_TITLE),
            
            TAG_DONE
        )
    )
    {
        ULONG length = strlen(request->rf_Dir)  + 1  /* separating '/' */  
                     + strlen(request->rf_File) + 1; /* terminating '\0' */
        
        filename = AllocVec(length, MEMF_ANY);
        
        if (filename != NULL)
        {
            filename[0] = '\0';
            strlcat(filename, request->rf_Dir, length);
            if (!AddPart(filename, request->rf_File, length))
            {
                /* FIXME: Error dialog? */
            }
        }
        else
        {
            /* FIXME: Error dialog */
        }
    }
    
    MUI_FreeAslRequest(request);

    return filename;
}
