/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BCPL support
    Lang: english
*/
#define DEBUG 1
#include <aros/debug.h>
#include <aros/asmcall.h>

#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/rdargs.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

AROS_UFH4(ULONG, BCPL_ReadArgs,
/*  SYNOPSIS */
	AROS_UFHA(BSTR, btemplate, D1),
	AROS_UFHA(BPTR, bvec, D2),
	AROS_UFHA(ULONG, upb, D3),
	AROS_UFHA(struct DosLibrary *, DOSBase, A6))

/*  LOCATION
        BCPL Vector offset 138

    FUNCTION
	Parses the commandline, a given string or Input() and fills
	an argument array according to the options template given.
	The array must be initialized to the wanted defaults before
	each call to ReadArgs().
	ReadArgs() tries to parse the commandline and continues
	on the input channel if it just consists of a single '?',
	prompting the user for input.

    INPUTS
	template - Template string. The template string is given as
		   a number of options separated by ',' and modified
		   by '/' modifiers, e.g. 'NAME,WIDTH/N,HEIGHT/N'
		   means get a name string and two numbers (width and
		   height). The possible modifiers are:
		   /S Option is a switch. It may be either set or
		      left out.
		   /T Option is a boolean value. Requires an argument
		      which may be "ON", "YES" (setting the respective
		      argument to 1), "OFF" or "NO" (setting the
		      respective argument to 0).
		   /N Option is a number. Strings are not allowed.
		      If the option is optional, a pointer to the
		      actual number is returned. This is how you know
		      if it was really given. The number is always of type
		      LONG.
		   /A Argument is required. If it is left out ReadArgs()
		      fails.
		   /K The keyword must be given when filling the option.
		      Normally it's skipped.
		   /M Multiple strings or, when used in combination with /N,
		      numbers. The result is returned as an array of pointers
		      to strings or LONGs, and is terminated with NULL. /M
		      eats all strings that don't fit into any other option.
		      If there are unfilled /A arguments after parsing they
		      steal strings from /M. This makes it possible to, for
		      example, write a Copy command template like
		      'FROM/A/M,TO/A'. There may be only one /M option in a
		      template.
		   /F Eats the rest of the line even if there are option
		      keywords in it.
	array	 - Array to be filled with the result values. The array must
		   be intialized to the default values before calling
		   ReadArgs().
	array_wl - Maximum size of input

    RESULT
        Number of words used in argv

    SEE ALSO
	Input()

*****************************************************************************/
{
    AROS_USERFUNC_INIT

    CONST_STRPTR template = AROS_BSTR_ADDR(btemplate);
    IPTR *vec = BADDR(bvec);
    struct FileHandle *input = BADDR(Input());
    struct RDArgs *rd;
    BSTR bstr;
    CONST_STRPTR cp;
    int arg, args, seen_key, seen_enough, svec;
#define RA_FLAG(x)	((x)&0x1f)

    D(bug("-- Template \"%s\"\n", template));

    /* Count args in the template */
    for (args = 1, cp = template; *cp; cp++) {
    	if (*cp == ',')
    	    args++;
    }

    rd = AllocDosObject(DOS_RDARGS, NULL);
    if (rd == NULL)
    	return 0;

    if (input != NULL) {
    	rd->RDA_Source.CS_Buffer = input->fh_Buf;
    	rd->RDA_Source.CS_Length = input->fh_End-input->fh_Buf;
    	rd->RDA_Source.CS_CurChr = input->fh_Pos-input->fh_Buf;
    }
    rd = ReadArgs(template, vec, rd);
    if (rd == NULL)
    	return 0;

    svec = args;
    seen_enough = 0;
    for (arg = 0, seen_key = 0, cp = template; *cp; cp++) {
    	int len, left;

    	if (*cp == ',') {
    	    if (!seen_enough && vec[arg] != 0) {
		/* Ok, it's probably a string. Convert it to BCPL */
		len = strlen((STRPTR)vec[arg]);
		left = (upb - svec)*sizeof(IPTR);
		if (AROS_BSTR_MEMSIZE4LEN(len) > left) {
		    FreeArgs(rd);
		    return 0;
		}

		bstr = MKBADDR(&vec[svec]);
		D(bug("-- Convert arg %d (%p) \"%s\" to BCPL 0x%x at %p\n", arg, (APTR)vec[arg], (APTR)vec[arg], bstr, &vec[svec]));
		memcpy(AROS_BSTR_ADDR(bstr), (APTR)vec[arg], len);
		AROS_BSTR_setstrlen(bstr, len);
		vec[arg] = bstr;
		svec += AROS_ALIGN(AROS_BSTR_MEMSIZE4LEN(len))/sizeof(IPTR);
	    }
    	    arg++;
    	    seen_key = 0;
    	    continue;
    	}

    	if (!seen_key && *cp == '/') {
    	    seen_key = 1;
    	    seen_enough = 0;
    	    continue;
    	}

    	if (!seen_key)
    	    continue;

    	if (seen_enough)
    	    continue;

    	if (RA_FLAG(*cp) == RA_FLAG('N')) {
    	    seen_enough=1;
    	    continue;
    	}
    	if (RA_FLAG(*cp) == RA_FLAG('S')) {
    	    seen_enough=1;
    	    continue;
    	}
    	if (RA_FLAG(*cp) == RA_FLAG('T')) {
    	    seen_enough=1;
    	    continue;
    	}
    }

    FreeArgs(rd);
    FreeDosObject(DOS_RDARGS, rd);

    SetIoErr(0);
    return svec;

    AROS_USERFUNC_EXIT
}

