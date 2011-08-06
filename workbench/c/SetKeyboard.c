/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/******************************************************************************

    NAME

        SetKeyboard

    SYNOPSIS

        KEYMAP/A

    LOCATION

        C:

    FUNCTION

        Set the keymap for the current shell.

    INPUTS

        KEYMAP  --  the keymap to use with the current shell

    RESULT

    NOTES

        To make a certain keymap be the default for all shells, use the
	preferences input program so specify your default choice.

    EXAMPLE

        SetKeyboard s

	Makes the current shell use the Swedish keymap.

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/


#include <exec/exec.h>
#include <dos/dos.h>
#include <devices/keymap.h>
#include <devices/console.h>
#include <devices/keymap.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/keymap.h>
#include <proto/kms.h>

#include <string.h>

const TEXT version[] = "$VER: SetKeyboard 41.3 (03.03.2011)\n";

#define ARG_TEMPLATE "KEYMAP/A"

enum
{
    ARG_NAME = 0,
    NOOFARGS
};

AROS_ENTRY(__startup static ULONG, Start,
	   AROS_UFHA(char *, argstr, A0),
	   AROS_UFHA(ULONG, argsize, D0),
	   struct ExecBase *, SysBase)
{
    AROS_USERFUNC_INIT

    struct DosLibrary *DOSBase;
    struct Library *KeymapBase;
    struct KMSLibrary *KMSBase;
    STRPTR err = NULL;
    ULONG rc = RETURN_FAIL;

    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36);
    if (!DOSBase)
    	return RETURN_FAIL;

    KeymapBase = OpenLibrary("keymap.library", 0);
    if (KeymapBase)
    {
    	KMSBase = (struct KMSLibrary *)OpenLibrary("kms.library", 0);
	if (KMSBase)
	{
	    IPTR args[NOOFARGS];
	    struct RDArgs *myargs = ReadArgs(ARG_TEMPLATE, args, 0);

	    if (myargs)
	    {
	        struct KeyMapNode *kmn = OpenKeymap((STRPTR)args[ARG_NAME]);

    		if (kmn)
    		{
		    SetKeyMapDefault(&kmn->kn_KeyMap);
		    rc = RETURN_OK;
		}
	    }
	    
	    if (rc != RETURN_OK)
	    	PrintFault(IoErr(), "SetKeyboard");

	    if (myargs)
	    	FreeArgs(myargs);

	    CloseLibrary(&KMSBase->kms_Lib);
	}
	else
	    err = "Can't opem kms.library!";

    	CloseLibrary(KeymapBase);
    }
    else
    	err = "Can't open keymap.library!";

    if (err)
    	Printf("SetKeyboard: %s\n", err);

    CloseLibrary(&DOSBase->dl_lib);

    return rc;

    AROS_USERFUNC_EXIT
}
