#include <exec/types.h>
#include <dos/dos.h>

#include <proto/dos.h>
#include <proto/workbench.h>

#define SH_GLOBAL_DOSBASE TRUE
#include <aros/shcommands.h>

AROS_SH1
(
    Open, 0.1,
    AROS_SHA(STRPTR, , NAME, /A, NULL)
)
{
    AROS_SHCOMMAND_INIT

    struct Library *WorkbenchBase = OpenLibrary("workbench.library", 44L);
    BOOL            success       = FALSE;
    
    if (WorkbenchBase != NULL)
    {
        success = OpenWorkbenchObject(SHArg(OBJECT), TAG_DONE);
    }
    else
    {
        PutStr("ERROR: Could not open workbench.library v44+.\n");
    }
    
    return success ? RETURN_OK : RETURN_FAIL;
    
    AROS_SHCOMMAND_EXIT
}
