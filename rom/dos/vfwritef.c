/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include "dos_intern.h"
#include <dos/bptr.h>
#include <string.h>

LONG    putNumber(CONST_STRPTR *format, IPTR **args, ULONG base, BPTR fh,
                  struct DosLibrary *DOSBase);
STRPTR  writeNumber(char *buffer, ULONG base, ULONG n, BOOL minus,
                    struct DosLibrary *DOSBase);

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(LONG, VFWritef,

/*  SYNOPSIS */
        AROS_LHA(BPTR  , fh      , D1),
        AROS_LHA(CONST_STRPTR, fmt     , D2),
        AROS_LHA(const IPTR *, argarray, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 58, Dos)

/*  FUNCTION
        Write a formatted string (with supplied values) to a specified file.
        The string may be of any length and the routine is buffered.
        The following format commands may be used (preceded by a '%') a la
        printf().

            S  - string (C style)
            Tx - writes a left justified string padding it to be (at least)
                 x bytes long
            C  - character
            Ox - octal number; maximum width x characters
            Xx - hexadecimal number; maximum width x characters
            Ix - decimal number; maximum width x chararcters
            N  - decimal number; any length
            Ux - unsigned decimal number; maximum width x characters
            $  - ignore parameter

        Note: 'x' above is the character value - '0'.

    INPUTS
        fh       - file to write the output to
        fmt      - format string
        argarray - pointer to an array of formatting values

    RESULT
        The number of bytes written or -1 if there was an error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        VFPrintf(), FPutC()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

#define  bLast  (sizeof(ULONG)*8/3 + 1)

    char    buffer[bLast + 1];

    LONG    count  = 0;      /* Number of characters written */
    CONST_STRPTR  format = fmt;
    const IPTR   *args   = argarray;

    STRPTR  string;
    STRPTR  wBuf;            /* Pointer to first number character in buffer */
    LONG    len;
    LONG    i;               /* Loop variable */
    LONG    number;
    BOOL    minus;
    BOOL    quitNow = FALSE;    /* Takes care of the case "...%" as format
                                   string */


    while (*format != 0 && !quitNow)
    {
        if (*format == '%')
        {
            format++;
            
            switch (*format)
            {
            case 'S':           /* Regular c string */
            case 's':
                string = (STRPTR)*args;
                args++;

                if (string == NULL)
                {
                    return -1;
                }
                
                while (*string != 0)
                {
                    FPutC(fh, *string++);
                    count++;
                }
                
                break;
                
            case 'T':           /* BCPL string (possibly filled out) */
            case 't':
                format++;
                len = *format - '0';
                
                if (BADDR(*args) == NULL)
                {
                    return -1;
                }

                for (i = 0; i < AROS_BSTR_strlen((BSTR)*args); i++)
                {
                    FPutC(fh, AROS_BSTR_getchar((BSTR)*args, i));
                    count++;
                }
                
                args++;
                
                /* If needed, write out spaces to fill field. */
                for(; i < len; i++)
                {
                    FPutC(fh, ' ');
                    count++;
                }

                break;
                
            case 'C':           /* Character */
            case 'c':
                FPutC(fh, (char)*args);
                count++;
                args++;
                break;
                
            case 'O':           /* Octal number */
            case 'o':
                count += putNumber(&format, (IPTR **)&args, 8, fh, DOSBase);
                break;
                
            case 'X':           /* Hexadecimal number */
            case 'x':
                count += putNumber(&format, (IPTR **)&args, 16, fh, DOSBase);
                break;
                
            case 'I':           /* Decimal number */
            case 'i':
                count += putNumber(&format, (IPTR **)&args, 10, fh, DOSBase);
                break;
                
            case 'N':           /* Decimal number (no length restriction) */
            case 'n':
                number = *args;
                args++;
                
                if (number < 0)
                {
                    number = -number;
                    minus = TRUE;
                }
                else
                {
                    minus = FALSE;
                }
                
                buffer[bLast] = 0;

                /* Write decimal number */
                wBuf = writeNumber(&buffer[bLast], 10, number, minus, DOSBase);
                
                while (*wBuf != 0)
                {
                    FPutC(fh, *wBuf++);
                    count++;
                }

                break;
                
            case 'U':           /* Unsigned decimal number */
            case 'u':
                format++;
                len = *format - '0';
    
                number = *args;
                args++;
                                
                wBuf = writeNumber(&buffer[bLast], 10, number, FALSE, DOSBase);

                for (i = 0; i < len; i++)
                {
                    FPutC(fh, *wBuf++);
                    count++;
                    
                    if (*wBuf == 0)
                    {
                        break;
                    }
                }

                break;
                
            case '$':           /* Skip argument */
                args++;
                break;
                
            case 0:             /* Stupid user... */
                quitNow = TRUE;
                break;

            default:            /* Ability to print '%':s */
                FPutC(fh, *format);
                count++;
                break;
            }
        }
        else
        {
            /* A regular character */
            FPutC(fh, *format);
            count++;
        }

        format++;
    }

    return count;
    
    AROS_LIBFUNC_EXIT
} /* VFWritef */


LONG putNumber(CONST_STRPTR *format, IPTR **args, ULONG base, BPTR fh,
               struct DosLibrary *DOSBase)
{
    char    buffer[bLast + 1];
    LONG    icount = 0;
    LONG    number;
    LONG    len;                /* Maximum width of number (ASCII) */
    BOOL    minus = FALSE;
    STRPTR  aNum;
    LONG    i;                  /* Loop variable */

    buffer[bLast] = 0;

    (*format)++;
    len = **format - '0';
    
    number = **args;
    (*args)++;

    if(number < 0)
    {
        number = -number;
        minus = TRUE;
    }
    
    aNum = writeNumber(&buffer[bLast], base, number, minus, DOSBase);
    
    /* Write the textual number to the file */
    for (i = 0; i < len; i++)
    {
        FPutC(fh, *aNum++);
        icount++;

        if(*aNum == 0)
            break;
    }

    return icount;
} /* VFWritef */
    

/* Generate a text string from a number */
STRPTR writeNumber(char *buffer, ULONG base, ULONG n, BOOL minus,
                   struct DosLibrary *DOSBase)
{
    int val;

    do
    {
        val = n % base;
        *--buffer = val < 10 ? val + '0' : val - 10 + 'A';
        n /= base;
    } while(n != 0);

    if (minus)
    {
        *--buffer = '-';
    }

    return buffer;
}

