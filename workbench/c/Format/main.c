#include <proto/dos.h>

#include <exec/types.h>
#include <dos/dos.h>

#define SH_GLOBAL_DOSBASE TRUE
#include <aros/shcommands.h>

AROS_SH2
(
    Format, 0.1,
    AROS_SHA(STRPTR, , DRIVE, /K/A, NULL),
    AROS_SHA(STRPTR, , NAME, /K/A, NULL)
)
{
    AROS_SHCOMMAND_INIT

    TEXT choice = 'N';

    Printf("About to format drive %s. ", SHArg(DRIVE));
    Printf("This will destroy all data on the drive!\n");
    Printf("Are you sure? (y/N)"); Flush(Output());
    
    Read(Input(), &choice, 1);
    if (choice == 'y' || choice == 'Y')
    {
        Printf("Formatting...");
        Flush(Output());
        
        if (Inhibit(SHArg(DRIVE), DOSTRUE))
        {
            if (Format(SHArg(DRIVE), SHArg(NAME), ID_FFS_DISK))
            {
                Printf("done\n");
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
    
    return 0;
    
error:
    Printf("ERROR!\n");
    return 20;
    
    AROS_SHCOMMAND_EXIT
}
