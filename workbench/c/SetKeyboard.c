/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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

#include <string.h>
#include <stdlib.h>

const TEXT version[] = "$VER: SetKeyboard 41.2 (16.1.2000)\n";

#define ARG_TEMPLATE "KEYMAP/A"

enum
{
    ARG_NAME = 0,
    NOOFARGS
};

struct Library *KeymapBase = NULL;
struct KeyMapResource *KeyMapResource;

static struct RDArgs *myargs;
static struct KeyMapNode *kmn;
static BPTR seg;
static IPTR args[NOOFARGS];
static char s[256];
static char *filename, *name;

static void Cleanup(char *msg, WORD rc)
{
    if (msg)
    {
    	Printf("SetKeyboard: %s\n",msg);
    }
    
    if (seg)
    {
	UnLoadSeg(seg);
    }

    if (myargs)
    {
	FreeArgs(myargs);
    }

    if (KeymapBase)
    {
	CloseLibrary(KeymapBase);
    }
    
    exit(rc);
}


static void OpenLibs(void)
{
    if (!(KeymapBase = OpenLibrary("keymap.library", 0)))
    {
    	Cleanup("Can´t open keymap.library!", RETURN_FAIL);
    }
}


static void OpenKeyMapResoure(void)
{
    if (!(KeyMapResource = OpenResource("keymap.resource")))
    {
    	Cleanup("Can´t open keymap.resoure!", RETURN_FAIL);
    }
}


static void GetArguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, 0)))
    {
    	Fault(IoErr(), 0, s, 255);
	Cleanup(s, RETURN_FAIL);
    }
    
    filename = (char *)args[ARG_NAME];
    name = FilePart(filename);
}


static struct KeyMapNode *KeymapAlreadyOpen(void)
{
    struct Node *node;
    struct KeyMapNode *kmn = NULL;
    
    Forbid();
    
    ForeachNode(&KeyMapResource->kr_List, node)
    {
	if (!stricmp(name, node->ln_Name))
	{
	    kmn = (struct KeyMapNode *)node;
	    break;
	}
    }
    
    Permit();
    
    return kmn;
}


static void Action(void)
{
    kmn = KeymapAlreadyOpen();
    
    if (!kmn)
    {
	struct KeyMapNode *kmn_check;
	
	if (name == filename)
	{
            strcpy(s, "DEVS:Keymaps");
	    AddPart(s, name, 255);
	}
	else
	{
            strcpy(s, filename);
	}
	
	if (!(seg = LoadSeg(s)))
	{
            Fault(IoErr(), 0, s, 255);
	    Cleanup(s, RETURN_FAIL);
	}
	
	kmn = (struct KeyMapNode *) (((UBYTE *)BADDR(seg)) + sizeof(APTR));
	
	Forbid();    
	
	if ((kmn_check = KeymapAlreadyOpen()))
	{
            kmn = kmn_check;
	}
	else
	{
	    AddHead(&KeyMapResource->kr_List, &kmn->kn_Node);
            seg = 0;
	}
	
	Permit();
	
    }  /* if (!kmn) */
    
    SetKeyMapDefault(&kmn->kn_KeyMap);
}


int __nocommandline;

int main(void)
{
    OpenLibs();
    OpenKeyMapResoure();
    GetArguments();
    
    Action();
    Cleanup(0, RETURN_OK);
    
    return 0;
}
