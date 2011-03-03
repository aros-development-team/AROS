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
#include <stdlib.h>

const TEXT version[] = "$VER: SetKeyboard 41.3 (03.03.2011)\n";

#define ARG_TEMPLATE "KEYMAP/A"

enum
{
    ARG_NAME = 0,
    NOOFARGS
};

struct Library *KeymapBase = NULL;
struct KMSLibrary *KMSBase = NULL;

static struct RDArgs *myargs;
static IPTR args[NOOFARGS];
static char s[256];

static void Cleanup(char *msg, WORD rc)
{
    if (msg)
    {
    	Printf("SetKeyboard: %s\n",msg);
    }
    
    if (myargs)
    {
	FreeArgs(myargs);
    }

    if (KeymapBase)
    {
	CloseLibrary(KeymapBase);
    }

    if (KMSBase)
    	CloseLibrary(&KMSBase->kms_Lib);

    exit(rc);
}

static void OpenLibs(void)
{
    if (!(KeymapBase = OpenLibrary("keymap.library", 0)))
    {
    	Cleanup("Can't open keymap.library!", RETURN_FAIL);
    }

    KMSBase = (struct KMSLibrary *)OpenLibrary("kms.library", 0);
    if (!KMSBase)
    	Cleanup("Can't opem kms.library!", RETURN_FAIL);
}

static void GetArguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, 0)))
    {
    	Fault(IoErr(), 0, s, 255);
	Cleanup(s, RETURN_FAIL);
    }
}

static void Action(void)
{
    struct KeyMapNode *kmn = OpenKeymap((STRPTR)args[ARG_NAME]);

    if (kmn)    
	SetKeyMapDefault(&kmn->kn_KeyMap);
    /* TODO: add error processing */
}

int __nocommandline;

int main(void)
{
    OpenLibs();
    GetArguments();
    
    Action();
    Cleanup(0, RETURN_OK);
    
    return 0;
}
