/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/exall.h>
#include <utility/tagitem.h>
#include <proto/utility.h>
#include <dos/rdargs.h>
#include <dos/dostags.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(APTR, AllocDosObject,

/*  SYNOPSIS */
	AROS_LHA(ULONG           , type, D1),
	AROS_LHA(struct TagItem *, tags, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 38, Dos)

/*  FUNCTION
	Creates a new dos object of a given type. This memory has to be
	freed with FreeDosObject().

    INPUTS
	type - Object type.
	tags - Pointer to taglist array with additional information. See
	       <dos/dostags.h> for a list of all supported tags.

    RESULT
	Pointer to new object or NULL, to indicate an error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    APTR mem;

    switch(type)
    {
    case DOS_FILEHANDLE:
	mem = AllocMem(sizeof(struct FileHandle), MEMF_CLEAR);

	if (mem != NULL)
	{
	    struct FileHandle *fh = (struct FileHandle *)mem;

	    /* We set fh->fh_Arg1 to point back to 'fh' to make packet
	       emulation possible */
	    fh->fh_CompatibilityHack = fh;
	}
	else
	{
	    SetIoErr(ERROR_NO_FREE_STORE);
	}

	return mem;

    case DOS_FIB:
	mem = AllocMem(sizeof(struct FileInfoBlock), MEMF_CLEAR);

	if(mem == NULL)
	    SetIoErr(ERROR_NO_FREE_STORE);

	return mem;

    case DOS_STDPKT:
        mem = AllocMem(sizeof(struct StandardPacket), MEMF_CLEAR);
        
        if (mem == NULL)
            SetIoErr(ERROR_NO_FREE_STORE);
        
        return &((struct StandardPacket *)mem)->sp_Pkt;

    case DOS_EXALLCONTROL:
	mem = AllocMem(sizeof(struct InternalExAllControl), MEMF_CLEAR);

	if(mem == NULL)
	    SetIoErr(ERROR_NO_FREE_STORE);

	return mem;

    case DOS_CLI:
	{
	    struct CommandLineInterface *cli = NULL;
	    struct TagItem defaults[] =
	    {
		/* 0 */ { ADO_DirLen,       255 },
		/* 1 */ { ADO_CommNameLen,  255 },
		/* 2 */ { ADO_CommFileLen,  255 },
		/* 3 */ { ADO_PromptLen,    255 },
		        { TAG_END, 0 }
	    };
	    
	    STRPTR  dir     = NULL;
	    STRPTR  command = NULL;
	    STRPTR  file    = NULL;
	    STRPTR  prompt  = NULL;

	    /* C has no exceptions. This is a simple replacement. */
#define ENOMEM_IF(a)  if(a) goto enomem      /* Throw out of memory. */

	    cli = AllocMem(sizeof(struct CommandLineInterface), MEMF_CLEAR);
	    ENOMEM_IF(cli == NULL);
	    
	    cli->cli_FailLevel  = RETURN_ERROR;
	    cli->cli_Background = DOSTRUE;
	    ApplyTagChanges(defaults, tags);
	    
	    dir = AllocVec(defaults[0].ti_Data + 1, MEMF_PUBLIC | MEMF_CLEAR);
	    ENOMEM_IF(dir == NULL);

	    cli->cli_SetName = MKBADDR(dir);

	    command = AllocVec(defaults[1].ti_Data + 1,
			       MEMF_PUBLIC | MEMF_CLEAR);
	    ENOMEM_IF(command == NULL);

	    cli->cli_CommandName = MKBADDR(command);

	    file = AllocVec(defaults[2].ti_Data + 1, MEMF_PUBLIC | MEMF_CLEAR);
	    ENOMEM_IF(file == NULL);

	    cli->cli_CommandFile = MKBADDR(file);

	    prompt = AllocVec(defaults[3].ti_Data + 1,
			      MEMF_PUBLIC | MEMF_CLEAR);
	    ENOMEM_IF(prompt == NULL);

	    cli->cli_Prompt = MKBADDR(prompt);
	    
	    return cli;
	    
enomem:
	    if(cli != NULL)
		FreeMem(cli, sizeof(struct CommandLineInterface));
	    
	    FreeVec(dir);
	    FreeVec(command);
	    FreeVec(file);
	    FreeVec(prompt);
	    
	    SetIoErr(ERROR_NO_FREE_STORE);

	    return NULL;
	}
	
    case DOS_RDARGS:
	return AllocVec(sizeof(struct RDArgs), MEMF_CLEAR);
    }

    SetIoErr(ERROR_BAD_NUMBER);

    return NULL;

    AROS_LIBFUNC_EXIT
} /* AllocDosObject */
