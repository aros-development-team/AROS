/*
    Copyright © 2003-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Format CLI command
    Lang: English
*/
/******************************************************************************


    NAME

        Format

USAGE

        Format DRIVE=<Drive> [NAME=<name>] [OFS | FFS] [INTL | NOINTL] [FORCE] [QUIET]
		
    SYNOPSIS

        DRIVE/K/A, NAME/K/A, OFS/S, FFS/S, INTL=INTERNTIONAL/S, NOINTL=NOINTERNATIONAL/S, FORCE/S, QUIET/S

    LOCATION

        Sys:C

    FUNCTION

        Format will initialise a disk to be useable by the AROS.

    INPUTS

        DRIVE    --  The device disk unit designation
        NAME   --  Sets the volume name
        OFS  --  Formats the drive withe the Old File system (not recommended for HD’s)
        FFS  --  Formats the drive withe the Fast File system.
        INTL  --  Enables the FFS support of international characters.
        FORCE –QUIET – It will not display any output, ordinary or error messages.

    RESULT

    NOTES

	This command only supports OFS and FFS filesystem.
	
    EXAMPLE

        Format DRIVE=DF0: NAME="MyDisk" FFS
        (This will format a disk present in the internal floppy drive with the name 
        “Mydisk” and with Fast File System.)
	
    BUGS

    SEE ALSO

        SFSformat, Info
	
    INTERNALS

    HISTORY

******************************************************************************/
#define DEBUG 0

#include <proto/dos.h>

#include <aros/debug.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/filehandler.h>

#define MASK_FFS  AROS_MAKE_ID(0x00, 0x00, 0x00, 1)
#define MASK_INTL AROS_MAKE_ID(0x00, 0x00, 0x00, 2)
#define MASK_DOS  AROS_MAKE_ID(0xFF, 0xFF, 0xFF, 0)

#define SH_GLOBAL_DOSBASE TRUE
#include <aros/shcommands.h>

AROS_SH8
(
    Format, 41.1,
    AROS_SHA(STRPTR, , DRIVE, /K/A, NULL),
    AROS_SHA(STRPTR, , NAME,  /K/A, NULL),
    AROS_SHA(BOOL, ,   OFS,   /S,   FALSE),
    AROS_SHA(BOOL, ,   FFS,   /S,   FALSE),
    AROS_SHA(BOOL, INTL=, INTERNATIONAL, /S, FALSE),
    AROS_SHA(BOOL, NOINTL=, NOINTERNATIONAL, /S, FALSE),
    AROS_SHA(BOOL, ,   FORCE, /S,   FALSE),
    AROS_SHA(BOOL, ,   QUIET, /S,   FALSE)
)
{
    AROS_SHCOMMAND_INIT

    TEXT choice = 'N';
    ULONG dostype;
    struct DevProc *dp;
    struct DosList *dl;
    struct FileSysStartupMsg *fssm;
    struct DosEnvec *env;

    if (SHArg(QUIET) && !SHArg(FORCE))
    {
        PutStr("ERROR: Cannot specify QUIET without FORCE.\n");
        return RETURN_FAIL;
    }
    if (SHArg(OFS) && SHArg(FFS))
    {
        PutStr("ERROR: Cannot specify OFS with FFS.\n");
        return RETURN_FAIL;
    }
    if (SHArg(INTERNATIONAL) && SHArg(NOINTERNATIONAL))
    {
        PutStr("ERROR: Cannot specify INTL with NOINTL.\n");
        return RETURN_FAIL;
    }
    dp = GetDeviceProc(SHArg(DRIVE), NULL);
    if (!dp)
    {
	Printf("ERROR: Device %s not found.\n", SHArg(DRIVE));
	return RETURN_FAIL;
    }
    dl = dp->dvp_DevNode;
    FreeDeviceProc(dp);
    if (dl->dol_Type != DLT_DEVICE)
    {
	Printf("ERROR: %s is not a physical device.\n", SHArg(DRIVE));
	return RETURN_FAIL;
    }

    fssm = (struct FileSysStartupMsg *)BADDR(dp->dvp_DevNode->dol_misc.dol_handler.dol_Startup);
    env = (struct DosEnvec *)BADDR(fssm->fssm_Environ);
    dostype = env->de_DosType;
    D(Printf("DOS type queried: 0x%08lX\n", dostype));
    if (((dostype & MASK_DOS) != ID_DOS_DISK) &&
        (SHArg(OFS) || SHArg(FFS) || SHArg(INTERNATIONAL) || SHArg(NOINTERNATIONAL)))
    {
	Printf("ERROR: OFS, FFS, INTL and NOINTL can't be used on device %s./n", SHArg(DRIVE));
	return RETURN_FAIL;
    }
    
    if (SHArg(OFS))
        dostype &= ~MASK_FFS;
    if (SHArg(FFS))
        dostype |= MASK_FFS;
    if (SHArg(INTERNATIONAL))
        dostype |= MASK_INTL;
    if (SHArg(NOINTERNATIONAL))
        dostype &= ~MASK_INTL;
    D(Printf("Resulting DOS type: 0x%08lX\n", dostype));

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
            if (Format(SHArg(DRIVE), SHArg(NAME), dostype))
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
