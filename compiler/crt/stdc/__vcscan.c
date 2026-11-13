/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Function to scan a string like scanf().
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef AROS_NO_LIMITS_H
#   include <limits.h>
#else
#   define ULONG_MAX    4294967295UL
#endif
#include <ctype.h>
#include <math.h>

#ifndef STDC_STATIC
#define FULL_SPECIFIERS
#endif

/* some macros to cut this short
 * NEXT(c);         read next character
 * PREV(c);         ungetc a character
 * VAL(a)           leads to 1 if a is true and valid
 */
#define NEXT(c) ((c)=(*get_char)(data),size++,incount++)
#define PREV(c) do{if((c)!=EOF)(*unget_char)((c),data);size--;incount--;}while(0)
#define VAL(a)    ((a)&&size<=width)

extern const unsigned char * const __stdc_char_decimalpoint;

/*****************************************************************************

        NAME */

    int __vcscan (

/*  SYNOPSIS */
    void         * data,
    int        (* get_char)(void *),
    int        (* unget_char)(int,void *),
    const char * format,
    va_list            args)

/*  FUNCTION
        Scan an input stream as specified in format. The result of
        the scan will be placed in args.

    INPUTS
        data - This is passed to the usercallback getc and ungetc
        get_char - This function gets called when the routine wants to
            read the next character. It would return EOF when
            no more characters are available.
        unget_char - This function gets called when the routine wants to
            put a read character back into the stream. The next
            call to get_char() should return this character. It is possible
            that this function is called more than once before the
            next get_char().
        format - A scanf() format string.
        args - A list of arguments in which the result of the scan should
            be placed.

    RESULT
        The number of arguments converted.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    size_t blocks=0,incount=0;
    int c=0;

    while(*format)
    {
        size_t size=0;

        if(*format=='%')
        {
            size_t width=ULONG_MAX;
            char type,subtype='i',ignore=0;
            char lltype=0;
            const unsigned char *ptr=format+1;
            size_t i;

            if(*ptr=='*')
            {
                ignore=1;
                ptr++;
            }

            if(isdigit(*ptr))
            {
                width=0;
                while(isdigit(*ptr))
                    width=width*10+(*ptr++-'0');
            }

            while(*ptr=='h'||*ptr=='l'||*ptr=='L')
            {
                if (subtype=='l'&&*ptr=='l')
                    lltype=1;
                else
                    subtype=*ptr;
                ptr++;
            }

            type=*ptr++;

            if(type&&type!='%'&&type!='c'&&type!='n'&&type!='[')
            {
                do /* ignore leading whitespace characters */
                NEXT(c);
                while(isspace(c));
                size=1;
            } /* The first non-whitespace character is already read */

            switch(type)
            {
            case 'c':
            {
                unsigned char *bp;

                if(width==ULONG_MAX) /* Default */
                    width=1;

                if(!ignore)
                    bp=va_arg(args,char *);
                else
                    bp=NULL; /* Just to get the compiler happy */

                NEXT(c); /* 'c' did not skip whitespace */
                while(VAL(c!=EOF))
                {
                    if(!ignore)
                        *bp++=c;
                    NEXT(c);
                }
                PREV(c);

                if(!ignore&&size)
                    blocks++;
                break;
            }
            case '[':
            {
                unsigned char *bp;
                unsigned char tab[32],a,b;
                char circflag=0;

                if(*ptr=='^')
                {
                    circflag=1;
                    ptr++;
                }
                for(i=0;i<sizeof(tab);i++)
                    tab[i]=circflag?255:0;

                for(;;)
                {
                    if(!*ptr)
                        break;
                    a=b=*ptr++;
                    if(*ptr=='-'&&ptr[1]&&ptr[1]!=']')
                    {
                        ptr++;
                        b=*ptr++;
                    }
                    for(i=a;i<=b;i++)
                        if(circflag)
                            tab[i/8]&=~(1<<(i&7));
                        else
                            tab[i/8]|=1<<(i&7);
                    if(*ptr==']')
                    {
                        ptr++;
                        break;
                    }
                }

                if(!ignore)
                    bp=va_arg(args,char *);
                else
                    bp=NULL; /* Just to get the compiler happy */

                NEXT(c);
                while(VAL(c!=EOF&&tab[c/8]&(1<<(c&7))))
                {
                    if(!ignore)
                        *bp++=c;
                    NEXT(c);
                }
                PREV(c);

                if(!ignore&&size)
                {
                    *bp++='\0';
                    blocks++;
                }
                break;
            }
            case 's':
            {
                unsigned char *bp;

                if(!ignore)
                    bp=va_arg(args,char *);
                else
                    bp=NULL; /* Just to get the compiler happy */

                while(VAL(c!=EOF&&!isspace(c)))
                {
                    if(!ignore)
                        *bp++=c;
                    NEXT(c);
                }
                PREV(c);

                if(!ignore&&size)
                {
                    *bp++='\0';
                    blocks++;
                }
                break;
            }
#ifdef FULL_SPECIFIERS
            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G':
            case 'a':
            case 'A':
            {
                char local[128], *buf = local, *end;
                size_t used = 0, cap = sizeof(local);
                int hex = 0, dot = 0, exponent = 0, mode = 0;
                size_t sign = 0, word = 0;
                double v;

                /* Collect the longest input item, including incomplete exponent
                   prefixes, with only one character of stream lookahead. The
                   conversion itself is shared with strtod, not a second decimal
                   arithmetic implementation. Field widths exclude whitespace. */
                while (VAL(c != EOF))
                {
                    int take = 0;
                    int lower = tolower((unsigned char)c);
                    if (!used && (c == '+' || c == '-'))
                        take = 1, sign = 1;
                    else if (used == sign && (lower == 'i' || lower == 'n'))
                        mode = lower, word = 1, take = 1;
                    else if (mode == 'i')
                    {
                        if (word < 8 && lower == "infinity"[word])
                            word++, take = 1;
                    }
                    else if (mode == 'n')
                    {
                        if (word < 3 && lower == "nan"[word])
                            word++, take = 1;
                        else if (word == 3 && c == '(')
                            word = 4, take = 1;
                        else if (word == 4 && (isalnum((unsigned char)c) || c == '_'))
                            take = 1;
                        else if (word == 4 && c == ')')
                            word = 5, take = 1;
                    }
                    else if (isdigit((unsigned char)c) ||
                             (hex && !exponent && isxdigit((unsigned char)c)))
                        take = 1;
                    else if (!exponent && !dot && c == __stdc_char_decimalpoint[0])
                        dot = 1, take = 1;
                    else if (used == sign + 1 && buf[sign] == '0' && lower == 'x')
                        hex = 1, take = 1;
                    else if (!exponent && used > sign && lower == (hex ? 'p' : 'e'))
                        exponent = 1, take = 1;
                    else if (exponent && (c == '+' || c == '-') && used &&
                             tolower((unsigned char)buf[used - 1]) == (hex ? 'p' : 'e'))
                        take = 1;
                    if (!take) break;
                    if (used + 1 == cap)
                    {
                        char *next;
                        size_t newcap = cap * 2;
                        if (newcap <= cap || !(next = malloc(newcap)))
                        {
                            if (buf != local) free(buf);
                            return blocks ? (int)blocks : EOF;
                        }
                        for (i = 0; i < used; i++) next[i] = buf[i];
                        if (buf != local) free(buf);
                        buf = next;
                        cap = newcap;
                    }
                    buf[used++] = c;
                    NEXT(c);
                }
                PREV(c);
                buf[used] = '\0';
                v = strtod(buf, &end);
                if (!used || end != buf + used)
                {
                    if (buf != local) free(buf);
                    return c == EOF && !used && !blocks ? EOF : (int)blocks;
                }
                if (!ignore)
                {
                    if (subtype == 'L') *va_arg(args,long double *) = (long double)v;
                    else if (subtype == 'l') *va_arg(args,double *) = v;
                    else *va_arg(args,float *) = (float)v;
                    blocks++;
                }
                if (buf != local) free(buf);
                break;
            }
#endif
            case '%':
                NEXT(c);
                if(c!='%')
                    PREV(c); /* unget non-'%' character */
                break;
            case 'n':
                if(!ignore)
                    *va_arg(args,int *)=incount;
                size=1; /* fake a valid argument */
                break;
            default:
            {
                unsigned long v1=0;
                unsigned long long llv1=0;
                int base;
                int min=0;

                if(!type)
                    ptr--; /* unparse NUL character */

                if(type=='p')
                {
                    subtype='l'; /* This is the same as %lx */
                    type='x';
                }

                if(VAL((c=='-'&&type!='u')||c=='+'))
                {
                    min=c;
                    NEXT(c);
                }

                if(type=='i') /* which one to use ? */
                {
                    if(VAL(c=='0')) /* Could be octal or sedecimal */
                    {
                        int d;
                        NEXT(d); /* Get a look at next character */
                        if(VAL(tolower(d)=='x'))
                        {
                            int e;
                            NEXT(e); /* And the next */
                            if(VAL(isxdigit(c)))
                                type='x'; /* Is a valid x number with '0x?' */
                            PREV(e);
                        }
                        else
                            type='o';
                        PREV(d);
                    }
                    /*  deadwood: Below code is wrong and left for reference.
                        Algoritm cannot assume that string starting with A-F is
                        a hexadecimal integer. There must be '0x' sequence -
                        validated via test/clib/sscanf.c */
                    //else
                    //    if(VAL(!isdigit(c)&&isxdigit(c)))
                    //        type='x'; /* Is a valid x number without '0x' */
                }

                while(type=='x'&&VAL(c=='0')) /* sedecimal */
                {
                    int d;
                    NEXT(d);
                    if(VAL(tolower(d)=='x'))
                    {
                        int e;
                        NEXT(e);
                        if(VAL(isxdigit(e)))
                        {
                            c=e;
                            break;
                        } /* Used while just to do this ;-) */
                        PREV(e);
                    }
                    PREV(d);
                    break; /* Need no loop */
                }

                base=type=='x'||type=='X'?16:(type=='o'?8:10);
                while(VAL(isxdigit(c)&&(base!=10||isdigit(c))&&(base!=8||c<='7')))
                {
                    if(lltype==1)
                        llv1=llv1*base+(isdigit(c)?c-'0':0)+(isupper(c)?c-'A'+10:0)+(islower(c)?c-'a'+10:0);
                    else
                        v1=v1*base+(isdigit(c)?c-'0':0)+(isupper(c)?c-'A'+10:0)+(islower(c)?c-'a'+10:0);
                    NEXT(c);
                }

                if(min&&size==2) /* If there is no valid character after sign, unget last */
                {
                    PREV(c);
                    c=min;
                }

                PREV(c);

                if(ignore||!size)
                    break;

                if(type=='u')
                {
                    unsigned long long v2=v1;
                    if (lltype==1) v2=llv1;
                    switch(subtype)
                    {
                        case 'l':
                        case 'L':
                            if(lltype==1)
                                *va_arg(args,unsigned long long*)=v2;
                            else
                                *va_arg(args,unsigned long *)=v2;
                            break;
                        case 'i':
                            *va_arg(args,unsigned int *)=v2;
                            break;
                        case 'h':
                            *va_arg(args,unsigned short *)=v2;
                            break;
                    }
                }
                else
                {
                    signed long long v2=v1;
                    if(lltype==1) v2=llv1;
                    if(min=='-') v2=-v2;
                    switch(subtype)
                    {
                        case 'l':
                        case 'L':
                            if(lltype==1)
                                *va_arg(args,signed long long *)=v2;
                            else
                                *va_arg(args,signed long *)=v2;
                            break;
                        case 'i':
                            *va_arg(args,signed int *)=v2;
                            break;
                        case 'h':
                            *va_arg(args,signed short *)=v2;
                            break;
                    }
                }
                blocks++;
                break;
            }
            }
            format=ptr;
        }
        else
        {
            if(isspace(*format))
            {
                do
                NEXT(c);
                while(isspace(c));
                PREV(c);
                size=1;
            }
            else
            {
                NEXT(c);
                if(c!=*format)
                PREV(c);
            }
            
            format++;
        }
        if(!size)
            break;
    }

    if(c==EOF&&!blocks)
        return c;
    else
        return blocks;
}
