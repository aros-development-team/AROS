#include <proto/dos.h>

#include <exec/types.h>
#include <dos/dos.h>

#define SH_GLOBAL_DOSBASE TRUE
#include <aros/shcommands.h>

AROS_SH4
(
    Format, 0.1,
    AROS_SHA(STRPTR, , DRIVE, /K/A, NULL),
    AROS_SHA(STRPTR, , NAME,  /K/A, NULL),
    AROS_SHA(BOOL, ,   FORCE, /S,   FALSE),
    AROS_SHA(BOOL, ,   QUIET, /S,   FALSE)
)
{
    AROS_SHCOMMAND_INIT

    TEXT choice = 'N';

    if (SHArg(QUIET) && !SHArg(FORCE))
    {
        PutStr("ERROR: Cannot specify QUIET without FORCE.\n");
        return RETURN_FAIL;
    }

    if (!SHArg(FORCE))
    {
        Printf("About to format drive %s. ", SHArg(DRIVE));
        Printf("This will destroy all data on the drive!\n");
        Printf("Are you sure? (y/N)"); Flush(Output());
    
        Read(Input(), &choice, 1);
    }
    else
    {
        choice = 'y';
    }
    
    if (choice == 'y' || choice == 'Y')
    {
        if (!SHArg(QUIET))
        {
            Printf("Formatting...");
            Flush(Output());
        }
        
        if (Inhibit(SHArg(DRIVE), DOSTRUE))
        {
            if (Format(SHArg(DRIVE), SHArg(NAME), ID_FFS_DISK))
            {
                if (!SHArg(QUIET)) Printf("done\n");
            }
            else
            {
                goto error;
            }
            Inhibit(SHArg(DRIVE), DOSFALSE);
        }
        else
        {
            goto error;
        }
    }
    
    return RETURN_OK;
    
error:
    if (!SHArg(QUIET)) Printf("ERROR!\n");
    
    return RETURN_FAIL;
    
    AROS_SHCOMMAND_EXIT
}
