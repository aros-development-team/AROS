/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: English
*/

/******************************************************************************

    NAME

        Eval

    SYNOPSIS

        VALUE1/A,OP,VALUE2/M,TO/K,LFORMAT/K

    LOCATION

        Workbench:C

    FUNCTION

        Evaluate an integer expression and print the result. The result is
        written to standard output if not the TO switch are used which instead
	prints the result to a file. Using the switch LFORMAT, it is
	possible to direct how to write the result. Numbers prefixed by
	0x or #x are interpreted as hexadecimal and those prefixed by # or 0
	are interpreted as octals. Alphabetical characters are indicated
	by a leading single quotation mark ('), and are evaluated as their
	ASCII equivalent.
 
    INPUTS

        VALUE1,
	OP,
	VALUE2      --  The expression to evaluate. The following operators
                        are supported

			Operator              Symbols
                        ----------------------------------
			addition              +
			subtraction           -
			multiplication        *
			division              /
			modulo                mod, M, m, %
       			bitwise and           &
			bitwise or            |
			bitwise not           ~
			left shift            lsh, L, l
			right shift           rsh, R, r
			negation               -
			exclusive or          xor, X, x
			bitwise equivalence   eqv, E, e

	TO          --  File to write the result to
	LFORMAT     --  printf-like specification of what to write.
	                The possible swiches are:
			 
			%x  --  hexadecimal output
			%o  --  octal output
			%n  --  decimal output
			%c  --  character output (the ANSI-character
			        corresponding to the result value)
				
			By specifying *n in the LFORMAT string, a newline
			is output.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

    01.01.2001  SDuvan  implemented (although a bit tired... happy new year!)

******************************************************************************/

#define   DEBUG  0
#include  <aros/debug.h>

#include  "evalParser.tab.c"

#include  <exec/types.h>
#include  <exec/memory.h>
#include  <dos/dos.h>
#include  <dos/dosextens.h>
#include  <dos/rdargs.h>
#include  <proto/dos.h>
#include  <proto/exec.h>


static const char version[] = "$VER: Eval 41.1 (01.01.2001)\n";

#define  ARG_TEMPLATE  "VALUE1/A,OP,VALUE2/M,TO/K,LFORMAT/K"
enum 
{ 
    ARG_VALUE1 = 0,
    ARG_OP,
    ARG_VALUE2,
    ARG_TO,
    ARG_LFORMAT,
    NOOFARGS
};


void printLformat(STRPTR format, int value);
STRPTR fixExpression(STRPTR val1, STRPTR op, STRPTR *vals);


int __nocommandline;

int main(void)
{
    int  retval = RETURN_FAIL;

    IPTR args[] = { (IPTR)NULL,
		    (IPTR)NULL, 
		    (IPTR)NULL,
                    (IPTR)NULL,
                    (IPTR)NULL };

    struct RDArgs *rda;
	
    rda = ReadArgs(ARG_TEMPLATE, args, NULL);
    
    if (rda != NULL)
    {
	STRPTR  toFile  = (STRPTR)args[ARG_TO];
	STRPTR  lFormat = (STRPTR)args[ARG_LFORMAT];
	STRPTR  value1  = (STRPTR)args[ARG_VALUE1];
	STRPTR  op      = (STRPTR)args[ARG_OP];
	STRPTR *value2  = (STRPTR *)args[ARG_VALUE2];

	STRPTR  argString;
	
	BPTR  oldout = NULL;	/* Former output stream if using TO */
	BPTR  file = NULL;	/* Output redirection stream */
	
	/* The Amiga Eval command is totally brain-dead and, at the same time,
	   ReadArgs() is not really suitable for these arguments. To be
	   compatible to the Amiga Eval command, we have to patch the arguments
	   together to get something useful out of this. */
	argString = fixExpression(value1, op, value2);

	text = argString;	/* Note: text is a global variable that is
				   modified, so we cannot free 'text' below */

	D(bug("Put together text: %s\n", text));
	
	if (text == NULL || yyparse() == 1)
	{
	    FreeVec(argString);
	    FreeArgs(rda);
	    
	    return RETURN_ERROR;
	}

	FreeVec(argString);

	if (toFile != NULL)
	{
	    file = Open(toFile, MODE_NEWFILE);
	    
	    if (file != NULL)
	    {
		oldout = SelectOutput(file);
	    }
	    else
	    {
		printf("Cannot open output file %s\n", toFile);
		FreeArgs(rda);

		return RETURN_FAIL;
	    }
	}
	
	if (lFormat != NULL)
	{
	    printLformat(lFormat, g_result);
	}
	else
	{
	    printf("%i\n", g_result);
	}
	
	/* Reinstall output stream if we changed it */
	if (oldout != NULL)
	{
	    SelectOutput(oldout);
	    
	    /* Close the TO/K file */
	    Close(file);
	}
    }
    else
    {
	PrintFault(IoErr(), "Eval");
    }
    
    FreeArgs(rda);
    
    return retval;
}



extern int g_result;		/* The result variable is global for now */

void printLformat(STRPTR format, int value)
{
    ULONG i;			/* Loop variable */

    /* If it weren't for *n we could use VfWriteF() */

    for (i = 0; format[i] != 0; i++)
    {
	switch (format[i])
	{
	case '*':
	    if (format[i] == 'n')
	    {
		printf("\n");
	    }
	    else
	    {
		printf("*");
	    }

	    break;

	case '%':
	    i++;
	    
	    switch (tolower(format[i]))
	    {
            /* Hexadecimal display */
	    case 'x':
		printf("%x", value);
		break;

	    /* Octal display */
	    case 'o':
		printf("%o", value);
		break;

            /* Integer display */
	    case 'n':
		printf("%i", value);
		break;

	    /* Character display */
	    case 'c':
		printf("%c", value);
		break;

	    case '%':
		printf("%%");
		break;

		/* Stupid user writes "...%" */
	    case 0:
		i--;
		break;

	    default:
		printf("%%%c", format[i]);
		break;
		
	    } /* switch(%-command) */

	    break;

	default:
	    printf("%c", format[i]);
	    break;
	} /* switch format character */
    }
}


STRPTR fixExpression(STRPTR val1, STRPTR op, STRPTR *vals)
{
    /* val1 must be != 0 as it's an /A argument */
    int    len;
    int    i;			/* Loop variable */
    STRPTR arg;

    len = strlen(val1) + 1 + 1;	     /* One extra for the 0 byte to end
					the string */

    if (op != NULL)
    {
	len += strlen(op) + 1;
    }

    if (vals) for (i = 0; vals[i] != NULL; i++)
    {
	len += strlen(vals[i]) + 1;
    }

    arg = AllocVec(len, MEMF_ANY);

    if (arg == NULL)
    {
	return NULL;
    }

    strcpy(arg, val1);
    strcat(arg, " ");

    if (op != NULL)
    {
	strcat(arg, op);
	strcat(arg, " ");
    }

    if (vals) for (i = 0; vals[i] != NULL; i++)
    {
	strcat(arg, vals[i]);
	strcat(arg, " ");
    }

    return arg;
}
