/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/rdargs.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

#ifdef TEST
#    include <stdio.h>
#    include <proto/dos.h>
#    undef ReadArgs
#    undef AROS_LH3
#    define AROS_LH3(t,fn,a1,a2,a3,bt,bn,o,lib)     t fn (a1,a2,a3)
#    undef AROS_LHA
#    define AROS_LHA(t,n,r)                   t n
#    undef AROS_LIBFUNC_INIT
#    define AROS_LIBFUNC_INIT
#    undef AROS_LIBBASE_EXT_DECL
#    define AROS_LIBBASE_EXT_DECL(bt,bn)
#    undef AROS_LIBFUNC_EXIT
#    define AROS_LIBFUNC_EXIT
#endif

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(struct RDArgs *, ReadArgs,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR,    template, D1),
	AROS_LHA(IPTR *,          array,    D2),
	AROS_LHA(struct RDArgs *, rdargs,   D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 133, Dos)

/*  FUNCTION
	Parses the commandline, a given string or Input() and fills
	an argument array according to the options template given.
	The array must be initialized to the wanted defaults before
	each call to ReadArgs(). If the rdargs argument is NULL
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
		      if it was really given.
		   /A Argument is required. If it is left out ReadArgs()
		      fails.
		   /K The keyword must be given when filling the option.
		      Normally it's skipped.
		   /M Multiple strings. The result is returned as a string
		      pointer array terminated with NULL. /M eats all strings
		      that don't fit into any other option. If there are
		      unfilled /A arguments after parsing they steal strings
		      from /M. This makes it possible to e.g. write a COPY
		      template like 'FROM/A/M,TO/A'. There may be only one
		      /M option in a template.
		   /F Eats the rest of the line even if there are option
		      keywords in it.
	array	 - Array to be filled with the result values. The array must
		   be intialized to the default values before calling
		   ReadArgs().
	rdargs	 - An optional RDArgs structure determinating the type of
		   input to process.

    RESULT
	A handle for the memory allocated by ReadArgs(). Must be freed
	with FreeArgs() later.

    NOTES

    EXAMPLE
	See below.

    SEE ALSO
	FreeArgs(), Input()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Allocated resources */
    struct DAList *dalist = NULL;
    UBYTE         *flags = NULL;
    STRPTR         strbuf = NULL, iline = NULL;
    STRPTR        *multvec=NULL, *argbuf = NULL;
    ULONG          multnum = 0, multmax = 0;

    /* Some variables */
    CONST_STRPTR   cs1;
    STRPTR         s1, s2, *newmult;
    ULONG          arg, numargs, nextarg;
    LONG           it, item, chars, value;
    struct CSource lcs, *cs;

    /* Get pointer to process structure. */
    struct Process *me = (struct Process *)FindTask(NULL);

    /* Error recovery. C has no exceptions. This is a simple replacement. */
    LONG error;
#undef ERROR
#define ERROR(a) { error=a; goto end; }

    /* Template options */
#define REQUIRED 0x80 /* /A */
#define KEYWORD  0x40 /* /K */
#define MULTIPLE 0x20 /* /M */
#define TYPEMASK 0x07
#define NORMAL	 0x00 /* No option */
#define SWITCH	 0x01 /* /S, implies /K */
#define TOGGLE	 0x02 /* /T, implies /K */
#define NUMERIC  0x03 /* /N */
#define REST	 0x04 /* /F */

    /* Flags for each possible character. */
    static const UBYTE argflags[]=
    { REQUIRED, 0, 0, 0, 0, REST, 0, 0, 0, 0, KEYWORD, 0, MULTIPLE,
      NUMERIC, 0, 0, 0, 0, SWITCH|KEYWORD, TOGGLE|KEYWORD, 0, 0, 0, 0, 0, 0 };
    
    /* Allocate readargs structure (and private internal one) */
    if (!rdargs)
    {
    	rdargs = (struct RDArgs *)AllocVec(sizeof(struct RDArgs), MEMF_ANY | MEMF_CLEAR);
	if (rdargs) rdargs->RDA_Flags |= RDAF_ALLOCATED_BY_READARGS;
    }
    dalist = (struct DAList *)AllocVec(sizeof(struct DAList), MEMF_ANY | MEMF_CLEAR);

    if(rdargs == NULL || dalist == NULL)
	ERROR(ERROR_NO_FREE_STORE);
    
    /* Init character source. */
    if(rdargs->RDA_Source.CS_Buffer)
    {
	cs = &rdargs->RDA_Source;
    }
    else
    {
	lcs.CS_Buffer = (me->pr_Arguments ? me->pr_Arguments : (UBYTE *)"");
 	cs1 = lcs.CS_Buffer;

	while(*cs1++);

	lcs.CS_Length = (IPTR)cs1 - (IPTR)lcs.CS_Buffer - 1;
	lcs.CS_CurChr = 0;
	cs = &lcs;
    }

    /* Check for optional reprompting */
    if(!(rdargs->RDA_Flags & RDAF_NOPROMPT))
    {
	/* Check commandline for a single '?' */
	cs1 = cs->CS_Buffer;

	/* Skip leading whitespace */
	while(*cs1 == ' ' || *cs1 == '\t')
	    cs1++;

	/* Check for '?' */
	if(*cs1++ == '?')
	{
	    /* Skip whitespace */
	    while(*cs1 == ' ' || *cs1 == '\t')
		cs1++;

	    /* Check for EOL */
	    if(*cs1 == '\n' || !*cs1)
	    {
		/* Only a single '?' on the commandline. */
		BPTR  input = Input();
		BPTR  output = Output();
		ULONG isize=0, ibuf=0;
		LONG c;
		/* Prompt for more input */
/* printf ("Only ? found\n");
printf ("rdargs=%p\n", rdargs);
if (rdargs)
printf ("rdargs->RDA_ExtHelp=%p\n", rdargs->RDA_ExtHelp); */

		if(rdargs->RDA_ExtHelp != NULL)
		{
		    if(FPuts(output, rdargs->RDA_ExtHelp))
			ERROR(me->pr_Result2);
		} else if(FPuts(output, template) || FPuts(output, ": "))
		    ERROR(me->pr_Result2);

		if(!Flush(output))
		    ERROR(me->pr_Result2);

		/* Read a line in. */
		for(;;)
		{
		    if(isize >= ibuf)
		    {
			/* Buffer too small. Get a new one. */
			STRPTR newiline;

			ibuf += 256;
			newiline = (STRPTR)AllocVec(ibuf, MEMF_ANY);
			if(newiline == NULL)
			    ERROR(ERROR_NO_FREE_STORE);

			CopyMemQuick((ULONG *)iline, (ULONG *)newiline, isize);
			FreeVec(iline);
			iline = newiline;
		    }

		    /* Read character */
		    c = FGetC(input);
		    /* Check and write it. */
		    if(c == EOF && me->pr_Result2)
			ERROR(me->pr_Result2);

		    if(c == EOF || c== '\n' || !c)
		    {
		    	/* stegerg: added this. Otherwise try "list ?" then enter only "l" + RETURN
			   and you will get a broken wall in FreeMem reported. This happens in
			   FreeArgs() during the FreeVec() of the StrBuf. Appending '\n' here fixes
			   this, but maybe the real bug is somewhere else. */
			
		    	iline[isize++] = '\n';
			
			/* end stegerg: */
			
			break;
    	    	    }
		    
		    iline[isize++] = c;
		}

		/* Prepare input source for new line. */
		cs->CS_Buffer = iline;
		cs->CS_Length = isize;
	    }
	}
    }

    /*
	Get enough space for string buffer.
	It's always smaller than the size of the input line+1.
    */

    strbuf = (STRPTR)AllocVec(cs->CS_Length + 1 ,MEMF_ANY);
    if(strbuf == NULL)
	ERROR(ERROR_NO_FREE_STORE);

    /* Count the number of items in the template (number of ','+1). */
    numargs = 1;
    cs1 = template;

    while(*cs1)
    {
	if(*cs1++ == ',')
	    numargs++;
    }
    
    /* Use this count to get space for temporary flag array and result
       buffer. */
    flags = (UBYTE *)AllocVec(numargs + 1, MEMF_CLEAR);
    argbuf = (STRPTR *)AllocVec((numargs + 1)*sizeof(STRPTR), MEMF_CLEAR);

    if(flags == NULL || argbuf == NULL)
	ERROR(ERROR_NO_FREE_STORE);

    /* Fill the flag array. */
    cs1 = template;
    s2 = flags;

    while(*cs1)
    {
	/* A ',' means: goto next item. */
	if(*cs1 == ',')
	    s2++;

	/* In case of a '/' use the next character as option. */
	if(*cs1++ == '/')
	    *s2 |= argflags[*cs1-'A'];
    }

    /* Add a dummy so that the whole line is processed. */
    *++s2 = MULTIPLE;

    /*
        Now process commandline for the first time:
        * Go from left to right and fill all items that need filling.
        * If an item is given as 'OPTION=VALUE' or 'OPTION VALUE' fill
          it out of turn.
    */
    s1 = strbuf;

    for(arg = 0; arg <= numargs; arg = nextarg)
    {
	nextarg = arg + 1;

	/* Skip /K options and options that are already done. */
	if(flags[arg] & KEYWORD || argbuf[arg] != NULL)
	    continue;

	/* If the current option is of type /F do not look for keywords */
	if((flags[arg] & TYPEMASK) != REST)
	{
	    /* Get item. Quoted items are no keywords. */
	    it = ReadItem(s1, ~0ul/2, cs);

	    if(it == ITEM_UNQUOTED)
	    {
		/* Not quoted. Check if it's a keyword. */
		item = FindArg(template, s1);
		if(item >= 0 && item < numargs && argbuf[item] == NULL)
		{
		    /*
			It's a keyword. Fill it and retry the current option
			at the next turn
		    */
		    nextarg = arg;
		    arg = item;

		    /* /S /T and /F may not be given as 'OPTION=VALUE'. */
		    if((flags[item]&TYPEMASK) != SWITCH &&
		       (flags[item]&TYPEMASK) != TOGGLE &&
		       (flags[item]&TYPEMASK) != REST)
		    {
			/* Get value. */
			it = ReadItem(s1,~0ul/2,cs);

			if(it == ITEM_EQUAL)
			    it = ReadItem(s1,~0ul/2,cs);
		    }
		}
	    }

	    /* Check returncode of ReadItem(). */
	    if(it == ITEM_EQUAL)
		ERROR(ERROR_BAD_TEMPLATE);
	    if(it == ITEM_ERROR)
		ERROR(me->pr_Result2);
	    if(it == ITEM_NOTHING)
		break;
	}

	/* /F takes all the rest */
	/* TODO: Take care of quoted strings(?) */
	if((flags[arg] & TYPEMASK) == REST)
	{
	    /* Skip leading whitespace */
	    while(cs->CS_CurChr < cs->CS_Length &&
		  (cs->CS_Buffer[cs->CS_CurChr] == ' ' ||
		   cs->CS_Buffer[cs->CS_CurChr] == '\t'))
		cs->CS_CurChr++;

	    /* Find the last non-whitespace character */
	    s2 = s1 - 1;
	    argbuf[arg] = s1;

	    while(cs->CS_CurChr < cs->CS_Length &&
		  cs->CS_Buffer[cs->CS_CurChr]  &&
		  cs->CS_Buffer[cs->CS_CurChr]  != '\n')
	    {
		if(cs->CS_Buffer[cs->CS_CurChr] != ' ' &&
		   cs->CS_Buffer[cs->CS_CurChr] != '\t')
		    s2 = s1;

		/* Copy string by the way. */
		*s1++ = cs->CS_Buffer[cs->CS_CurChr++];
	    }

	    /* Add terminator (1 after the character found). */
	    s2[1] = 0;
	    it = ITEM_NOTHING;
	    break;
	}
	if(flags[arg]&MULTIPLE)
	{
	    /* All /M arguments are stored in a buffer. */
	    if(multnum>=multmax)
	    {
		/* Buffer too small. Get a new one. */
		multmax+=16;
		newmult=(STRPTR *)AllocVec(multmax*sizeof(char *),MEMF_ANY);
		if(newmult==NULL)
		    ERROR(ERROR_NO_FREE_STORE);
		CopyMemQuick((ULONG *)multvec,(ULONG *)newmult,
			     multnum*sizeof(char *));
		FreeVec(multvec);
		multvec=newmult;
	    }
	    /* Put string into the buffer. */
	    multvec[multnum++]=s1;
	    while(*s1++)
		;
	    /* /M takes more than one argument, so retry. */
	    nextarg=arg;
	}
	else if((flags[arg]&TYPEMASK)==SWITCH||(flags[arg]&TYPEMASK)==TOGGLE)
	{
	    /* /S or /T just set a flag */
	    argbuf[arg]=(char *)~0;
	}else /* NORMAL || NUMERIC */
	{
	    /* Put argument into argument buffer. */
	    argbuf[arg]=s1;
	    while(*s1++)
		;
	}
    }

    /* Unfilled /A options steal Arguments from /M */
    for(arg=numargs;arg-->0;)
	if(flags[arg]&REQUIRED&&argbuf[arg]==NULL&&
	   !(flags[arg]&MULTIPLE))
	{
	    if (flags[arg]&KEYWORD)
	    {
	    	/* /K/A argument, which inisits on keyword
		   being used, cannot be satisfied */
		   
	    	ERROR(ERROR_TOO_MANY_ARGS); /* yes, strange error number,
		                               but it translates to "wrong
					       number of arguments" */
		
	    }
	    
	    if(!multnum)
		/* No arguments left? Oh dear! */
		ERROR(ERROR_REQUIRED_ARG_MISSING);
	    argbuf[arg]=multvec[--multnum];
	}

    /* Put the rest of /M where it belongs */
    for(arg=0;arg<numargs;arg++)
	if(flags[arg]&MULTIPLE)
	{
	    if(flags[arg]&REQUIRED&&!multnum)
		ERROR(ERROR_REQUIRED_ARG_MISSING);

    	    if (multnum)
	    {
		/* NULL terminate it. */
		if(multnum>=multmax)
		{
		    multmax+=16;
		    newmult=(STRPTR *)AllocVec(multmax*sizeof(STRPTR),MEMF_ANY);
		    if(newmult==NULL)
			ERROR(ERROR_NO_FREE_STORE);
		    CopyMemQuick((ULONG *)multvec,(ULONG *)newmult,multnum*sizeof(char *));
		    FreeVec(multvec);
		    multvec=newmult;
		}
		multvec[multnum++]=NULL;
		argbuf[arg]=(STRPTR)multvec;
	    }
	    else
		/* Shouldn't be necessary, but some buggy software relies on this */
		argbuf[arg]=NULL;   	    	
	    break;
	}

    /* There are some arguments left? Return error. */
    if(multnum&&arg==numargs)
	ERROR(ERROR_TOO_MANY_ARGS);

    /*
	The commandline is processed now. Put the results in the result array.
	Convert /N arguments by the way.
    */
    for(arg=0;arg<numargs;arg++)
    {
	/* Just for the arguments given. */
	if(argbuf[arg]!=NULL)
	{
	    if(flags[arg]&MULTIPLE)
	    {
		array[arg]=(IPTR)argbuf[arg];
		if((flags[arg]&TYPEMASK)==NUMERIC)
		{
		    STRPTR *p;
		    LONG *q;
		    if(multnum*2>multmax)
		    {
			multmax=multnum*2;
			newmult=(STRPTR *)AllocVec(multmax*sizeof(STRPTR),MEMF_ANY);
			if(newmult==NULL)
			    ERROR(ERROR_NO_FREE_STORE);
			CopyMemQuick((ULONG *)multvec,(ULONG *)newmult,multnum*sizeof(char *));
			FreeVec(multvec);
			multvec=newmult;
		    }
		    array[arg]=(IPTR)multvec;
		    p=multvec;
		    q=(LONG*)(multvec+multnum);

		    while (*p)
		    {
			/* Convert /N argument. */
			chars=StrToLong(*p,q);
			if(chars<=0||(*p)[chars])
			    /* Conversion failed. */
			    ERROR(ERROR_BAD_NUMBER);
			/* Put the result where it belongs. */
			*p=(STRPTR)q;
			p++;
			q++;
		    }
                }
	    } else
	    {
		switch(flags[arg]&TYPEMASK)
		{
		    case NORMAL:
		    case REST:
		    case SWITCH:
			/* Simple arguments are just copied. */
			array[arg]=(IPTR)argbuf[arg];
			break;
		    case TOGGLE:
			/* /T logically inverts the argument. */
			array[arg]=array[arg]?0:~0;
			break;
		    case NUMERIC:
			/* Convert /N argument. */
			chars=StrToLong(argbuf[arg],&value);
			if(chars<=0||argbuf[arg][chars])
			    /* Conversion failed. */
			    ERROR(ERROR_BAD_NUMBER);
			/* Put the result where it belongs. */
		    #if 0
			if(flags[arg]&REQUIRED)
			    /* Required argument. Return number. */
			    array[arg]=value;
			else
		    #endif
			{
			    /* Abuse the argbuf buffer. It's not needed anymore. */
			    argbuf[arg]=(STRPTR)value;
			    array[arg]=(IPTR)&argbuf[arg];
			}
			break;
		}
	    }
	}
	else
	{
	    if(flags[arg]&MULTIPLE)
	    {
		/* Shouldn't be necessary, but some buggy software relies on this
		 * IBrowse's URL field isn`t set to zero.
		 */
		array[arg]=NULL;
	    }
	}
    }

    /* All OK. */
    error=0;
end:
    /* Cleanup and return. */
    FreeVec(iline);
    FreeVec(flags);
    if(error)
    {
	/* ReadArgs() failed. Clean everything up. */
	if (rdargs) if (rdargs->RDA_Flags & RDAF_ALLOCATED_BY_READARGS) FreeVec(rdargs);
	FreeVec(dalist);
	FreeVec(argbuf);
	FreeVec(strbuf);
	FreeVec(multvec);
	me->pr_Result2=error;
	return NULL;
    }else
    {
	/* All went well. Prepare result and return. */
	rdargs->RDA_DAList=(IPTR)dalist;
	dalist->ArgBuf=argbuf;
	dalist->StrBuf=strbuf;
	dalist->MultVec=multvec;
	return rdargs;
    }
    AROS_LIBFUNC_EXIT
} /* ReadArgs */

#ifdef TEST
#    include <dos/dos.h>
#    include <dos/rdargs.h>
#    include <utility/tagitem.h>

#    include <proto/dos.h>

char cmlargs[] = "TEST/A";

char usage[] =
   "This is exthelp for test\n"
   "Enter something";

#define CML_TEST 0
#define CML_END  1

LONG cmlvec[CML_END];

int main(int argc, char **argv)
{
    struct RDArgs *rdargs;

    if( (rdargs = AllocDosObject(DOS_RDARGS, NULL)))
    {
	rdargs->RDA_ExtHelp = usage; /* FIX: why doesn't this work? */

	if(!(ReadArgs(cmlargs, cmlvec, rdargs)))
	{
	    PrintFault(IoErr(), "AROS boot");
	    FreeDosObject(DOS_RDARGS, rdargs);
	    exit(RETURN_FAIL);
	}
    }
    else
    {
	PrintFault(ERROR_NO_FREE_STORE, "AROS boot");
	exit(RETURN_FAIL);
    }

    FreeArgs(rdargs);
    FreeDosObject(DOS_RDARGS, rdargs);

    return 0;
} /* main */

#endif /* TEST */
