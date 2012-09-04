/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

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

#define FULL_SPECIFIERS

/* some macros to cut this short
 * NEXT(c);         read next character
 * PREV(c);         ungetc a character
 * VAL(a)           leads to 1 if a is true and valid
 */
#define NEXT(c) ((c)=(*get_char)(data),size++,incount++)
#define PREV(c) do{if((c)!=EOF)(*unget_char)((c),data);size--;incount--;}while(0)
#define VAL(a)    ((a)&&size<=width)

extern unsigned char *__decimalpoint;

#ifdef FULL_SPECIFIERS
const static unsigned char undef[3][sizeof(double)]= /* Undefined numeric values, IEEE */
{ 
    { 0x7f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00 }, /* +inf */
    { 0xff,0xf0,0x00,0x00,0x00,0x00,0x00,0x00 }, /* -inf */
    { 0x7f,0xf1,0x00,0x00,0x00,0x00,0x00,0x00 }  /* NaN */
};
#endif


/*****************************************************************************

        NAME */

    int __vcscan (

/*    SYNOPSIS */
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
            read the next character. It whould return EOF when
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
            const unsigned char *ptr=format+1;
            size_t i;

            if(isdigit(*ptr))
            { 
                width=0;
                while(isdigit(*ptr))
                    width=width*10+(*ptr++-'0'); 
            }

            while(*ptr=='h'||*ptr=='l'||*ptr=='L'||*ptr=='*')
            { 
                if(*ptr=='*')
                    ignore=1;
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
            case 'f':
            case 'g':
            { 
                double v;
                int ex=0;
                int min=0,mine=0; /* This is a workaround for gcc 2.3.3: should be char */

                do /* This is there just to be able to break out */
                {
                    if(VAL(c=='-'||c=='+'))
                    { 
                        min=c;
                        NEXT(c); 
                    }

                    if(VAL(tolower(c)=='i')) /* +- inf */
                    {
                        int d;
                        NEXT(d);
                        if(VAL(tolower(d)=='n'))
                        { 
                            int e;
                            NEXT(e);
                            if(VAL(tolower(e)=='f'))
                            {
                                v=*(double *)&undef[min=='-'];
                                break; 
                            } /* break out */
                            PREV(e);
                        }
                        PREV(d);
                    }
                    else if(VAL(toupper(c)=='N')) /* NaN */
                    { 
                        int d;
                        NEXT(d);
                        if(VAL(tolower(d)=='a'))
                        { 
                            int e;
                            NEXT(e);
                            if(VAL(toupper(e)=='N'))
                            {
                                v=*(double *)&undef[2];
                                break; 
                            }
                            PREV(e);
                        }
                        PREV(d);
                    }

                    v=0.0;
                    while(VAL(isdigit(c)))
                    {
                        v=v*10.0+(c-'0');
                        NEXT(c);
                    }

                    if(VAL(c==__decimalpoint[0]))
                    { 
                        double dp=0.1;
                        NEXT(c);
                        while(VAL(isdigit(c)))
                        { 
                            v=v+dp*(c-'0');
                            dp=dp/10.0;
                            NEXT(c); 
                        }
                        if(size==2+(min!=0)) /* No number read till now -> malformatted */
                        { 
                            PREV(c);
                            c=__decimalpoint[0]; 
                        }
                    }

                    if(min&&size==2) /* No number read till now -> malformatted */
                    { 
                        PREV(c);
                        c=min;
                    }
                    if(size==1) {
                        PREV(c);
                        break;
                    }

                    if(VAL(tolower(c)=='e'))
                    {
                        int d;
                        NEXT(d);
                        if(VAL(d=='-'||d=='+'))
                        {
                            mine=d;
                            NEXT(d);
                        }

                        if(VAL(isdigit(d)))
                        {
                            do
                            {
                                ex=ex*10+(d-'0');
                                NEXT(d);
                            }while(VAL(isdigit(d)&&ex<100));
                            c=d;
                        }
                        else
                        { 
                            PREV(d);
                            if(mine)
                                PREV(mine);
                        }
                    }
                    PREV(c);

                    if(mine=='-')
                        v=v/pow(10.0,ex);
                    else
                        v=v*pow(10.0,ex);

                    if(min=='-')
                        v=-v;

                }while(0);

                if(!ignore&&size)
                {
                    switch(subtype)
                    { 
                    case 'l':
                    case 'L':
                        *va_arg(args,double *)=v;
                        break;
                    case 'i':
                        *va_arg(args,float *)=v;
                        break;
                    }
                    blocks++;
                }
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
                unsigned long v=0;
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
                    v=v*base+(isdigit(c)?c-'0':0)+(isupper(c)?c-'A'+10:0)+(islower(c)?c-'a'+10:0);
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
                    switch(subtype)
                    { 
                        case 'l':
                        case 'L':
                            *va_arg(args,unsigned long *)=v;
                            break;
                        case 'i':
                            *va_arg(args,unsigned int *)=v;
                            break;
                        case 'h':
                            *va_arg(args,unsigned short *)=v;
                            break;
                    }
                }
                else
                { 
                    signed long v2;
                    if(min=='-')
                        v2=-v;
                    else
                        v2=v;
                    switch(subtype)
                    {
                        case 'l':
                        case 'L':
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
