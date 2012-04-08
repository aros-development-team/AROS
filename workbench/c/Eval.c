/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Eval CLI command
    Lang: English
*/

/******************************************************************************

    NAME

        Eval

    SYNOPSIS

        VALUE1/A,OP,VALUE2/M,TO/K,LFORMAT/K

    LOCATION

        C:

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
                        negation              -
                        exclusive or          xor, X, x
                        bitwise equivalence   eqv, E, e

        TO          --  File to write the result to
        LFORMAT     --  printf-like specification of what to write.
                        The possible swiches are:
                         
                        %xd --  hexadecimal output, width digit d
                        %od --  octal output, width digit d
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

    04.05.2011  polluks width digit was missing
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
#include  <proto/alib.h>

const TEXT version[] = "$VER: Eval 41.2 (14.7.2011)\n";

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
    int  retval = RETURN_OK;

    IPTR args[] = { (IPTR)NULL,
                    (IPTR)NULL, 
                    (IPTR)NULL,
                    (IPTR)NULL,
                    (IPTR)NULL };

    struct RDArgs *rda;
        
    rda = ReadArgs(ARG_TEMPLATE, args, NULL);
    
    if (rda == NULL)
    {
        PrintFault(IoErr(), "Eval");
        retval = RETURN_FAIL;
    }
    else
    {
        STRPTR  toFile  = (STRPTR)args[ARG_TO];
        STRPTR  lFormat = (STRPTR)args[ARG_LFORMAT];
        STRPTR  value1  = (STRPTR)args[ARG_VALUE1];
        STRPTR  op      = (STRPTR)args[ARG_OP];
        STRPTR *value2  = (STRPTR *)args[ARG_VALUE2];

        STRPTR  argString;
        
        BPTR  oldout = BNULL; /* Former output stream if using TO */
        BPTR  file   = BNULL; /* Output redirection stream */
        
        /* The developer to use fixExpression had no high regard
           of the Amiga Eval command, and at the same time considered
           ReadArgs() not to be really suitable for these arguments.
           To be compatible to the Amiga Eval command, the developer
           felt it was necessary to patch the arguments back together. */
        argString = fixExpression(value1, op, value2);

        text = argString; /* Note: text is a global variable that is
                                   modified, so we cannot free 'text' below */

        D(bug("Put together text: %s\n", text));
        
        if (text == NULL || yyparse() == 1)
        {
            retval= RETURN_ERROR;
        }
        else {
            if (toFile != NULL)
            {
                file = Open(toFile, MODE_NEWFILE);
            
                if (file == BNULL)
                {
                    Printf("Cannot open output file %s\n", toFile);

                    retval= RETURN_FAIL;
                }
                else
                {
                    oldout = SelectOutput(file);
                }
            }

            if (toFile == NULL || file != BNULL) {                    
                if (lFormat != NULL)
                {
                    printLformat(lFormat, g_result);
                }
                else
                {
                    Printf("%ld\n", g_result);
                }
        
                /* Reinstall output stream if we changed it */
                if (oldout != BNULL)
                {
                    SelectOutput(oldout);
            
                    /* Close the TO/K file */
                    Close(file);
                }
            }
        }
        FreeVec(argString);
    }
    
    FreeArgs(rda);
    
    return retval;
}



extern int g_result;                /* The result variable is global for now */

void printLformat(STRPTR format, int value)
{
    ULONG i;                        /* Loop variable */
    char s[10];

    /* If it weren't for *n we could use VfWriteF() */

    for (i = 0; format[i] != 0; i++)
    {
        switch (format[i])
        {
        case '*':
            if (format[i] == 'n')
            {
                Printf("\n");
            }
            else
            {
                Printf("*");
            }

            break;

        case '%':
            i++;
            
            switch (tolower(format[i]))
            {
            /* Hexadecimal display */
            case 'x':
                __sprintf(s, "%X", value);
                s[format[++i] - '0'] = 0;
                Printf("%s", s);
                break;

            /* Octal display */
            case 'o':
                __sprintf(s, "%o", value);
                s[format[++i] - '0'] = 0;
                Printf("%s", s);
                break;

            /* Integer display */
            case 'n':
                Printf("%ld", value);
                break;

            /* Character display */
            case 'c':
                Printf("%c", value);
                break;

            case '%':
                Printf("%%");       /* AROS extension */
                break;

                /* Stupid user writes "...%" */
            case 0:
                i--;
                break;

            default:
                Printf("%%%c", format[i]);
                break;
                
            } /* switch(%-command) */

            break;

        default:
            Printf("%c", format[i]);
            break;
        } /* switch format character */
    }
}


STRPTR fixExpression(STRPTR val1, STRPTR op, STRPTR *vals)
{
    /* val1 must be != 0 as it's an /A argument */
    int    len;
    int    i;                        /* Loop variable */
    STRPTR arg;

    len = strlen(val1) + 1 + 1;             /* One extra for the 0 byte to end
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

/* For linking with the evalParser.y output
 */
void *malloc(YYSIZE_T size)
{
    return AllocVec(size, MEMF_ANY);
}

void free(void *ptr)
{
    FreeVec(ptr);
}

int puts(const char *s)
{
    PutStr(s);
    return 0;
}
