/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <exec/types.h>
#include <utility/hooks.h>
#include <proto/utility.h>
#include <libraries/locale.h>
#include <aros/asmcall.h>
#include "locale_intern.h"

#include <aros/debug.h>

#if USE_QUADFMT
typedef QUAD FMTLARGESTTYPE;
typedef UQUAD UFMTLARGESTTYPE;
#else /* USE_QUADFMT */
typedef LONG FMTLARGESTTYPE;
typedef ULONG UFMTLARGESTTYPE;
#endif /* USE_QUADFMT */

static const UBYTE hexarray[] = "0123456789abcdef";
static const UBYTE HEXarray[] = "0123456789ABCDEF";

static inline APTR stream_addr(APTR *args, ULONG len)
{
    APTR ret = *args;

    /* LONG data are actually IPTR-aligned */
    *args += (len == sizeof(ULONG)) ? sizeof(IPTR) : len;
    return ret;
}

/* The stuff below is based on old gcc includes (where you can actually read how
   varargs work) and ABI documentation.
   It works in the same way as traditional va_arg() except it fetches
   pointers to arguments, not arguments themselves (since on 32-bit machines
   argument values may be larger than pointers). */

#if defined(__PPC__)

static inline APTR va_addr(va_list args, ULONG len)
{
    APTR ret;

    if (len == sizeof(UQUAD))
    {
        /* On PPC UQUAD is aligned. and occupies 2 registers (plus may waste one more for alignment) */
        if (args->gpr < 7)
        {
            ULONG *regsave = (ULONG *)args->reg_save_area;

            args->gpr += args->gpr & 1;
            ret = &regsave[args->gpr];
            args->gpr += 2;
        }
        else
        {
            args->gpr = 8;
            ret = (APTR)(((IPTR)(args->overflow_arg_area + 7)) & ~7);
            args->overflow_arg_area = ret + sizeof(UQUAD);
        }
    }
    else
    {
        if (args->gpr < 8)
        {
            ULONG *regsave = (ULONG *)args->reg_save_area;

            ret = &regsave[args->gpr++];
        }
        else
        {
            ret = args->overflow_arg_area;
            args->overflow_arg_area += sizeof(ULONG);
        }
    }
    return ret;
}

#elif defined(__x86_64__)

static inline APTR va_addr(va_list args, ULONG len)
{
    APTR ret;

    if (args->gp_offset < 48)
    {
        ret = args->reg_save_area + args->gp_offset;
        args->gp_offset += sizeof(IPTR);
    }
    else
    {
        ret = args->overflow_arg_area;
        args->overflow_arg_area += sizeof(IPTR);
    }
    return ret;
}

#elif defined(__arm__)

#define va_addr(args, len) stream_addr(&args.__ap, len)
#define is_va_list(ap) ap.__ap
#define null_va_list(ap) va_list ap = {NULL}

#else

/*
 * It is OK to reuse stream_addr() here because for C-style varargs
 * 'len' will never be 2 (see default data size condition below)
 */
#define va_addr(args, len) stream_addr((APTR *)&args, len)

#endif

#ifndef is_va_list

#define is_va_list(ap) ap
#define null_va_list(ap) void *ap = NULL

#endif

APTR InternalFormatString(const struct Locale * locale,
    CONST_STRPTR fmtTemplate, CONST_APTR dataStream,
    const struct Hook * putCharFunc, va_list VaListStream)
{
    enum
    { OUTPUT = 0,
        FOUND_FORMAT
    } state;

    ULONG template_pos;
    BOOL end;
    ULONG max_argpos;
    ULONG arg_counter;
    BOOL scanning;

#define INDICES 256
    IPTR indices[INDICES];

    if (!fmtTemplate)
        return (APTR) dataStream;

    template_pos = 0;           /* Current position in the template string */
    state = OUTPUT;             /* current state of parsing */
    end = FALSE;
    max_argpos = 0;
    arg_counter = 0;
    scanning = TRUE;            /* The first time I will go through
                                   and determine the width of the data in the dataStream */

    memset(indices, sizeof(APTR), sizeof(indices));

    while (!end)
    {
        /*
         ** A format description starts here?
         */
        if (fmtTemplate[template_pos] == '%')
        {
            arg_counter++;
            state = FOUND_FORMAT;
        }

        switch (state)
        {
        case OUTPUT:
            /*
             ** Call the hook for this character
             */
            if (!scanning)
            {
                AROS_UFC3NR(VOID, putCharFunc->h_Entry,
                    AROS_UFCA(const struct Hook *, putCharFunc, A0),
                    AROS_UFCA(const struct Locale *, locale, A2),
                    AROS_UFCA(UBYTE, fmtTemplate[template_pos], A1));
            }

            /*
             ** End of template string? -> End of this function.
             */
            if (fmtTemplate[template_pos] == '\0')
            {
                if (scanning)
                {
                    /*
                     ** The scanning phase is over. Next time we do the output.
                     */
                    int i;
                    scanning = FALSE;
                    template_pos = 0;
                    arg_counter = 0;

                    /*
                     ** prepare the indices array
                     */
                    if (is_va_list(VaListStream))
                    {
                        for (i = 0; i <= max_argpos; i++)
                            indices[i] =
                                (IPTR) va_addr(VaListStream, indices[i]);
                    }
                    else
                    {
                        for (i = 0; i <= max_argpos; i++)
                            indices[i] =
                                (IPTR) stream_addr((APTR *) & dataStream,
                                indices[i]);
                    }

                }
                else
                {
                    /*
                     ** We already went through the output phase. So this is
                     ** the end of it.
                     */
                    end = TRUE;
                }
            }
            else
                template_pos++;

            //kprintf("OUTPUT: template_pos: %d\n",template_pos);

            break;

        case FOUND_FORMAT:
            /*
             ** The '%' was found in the template string
             */
            template_pos++;

            //kprintf("FOUND_FORMAT: template_pos: %d\n",template_pos);
            /*
             ** Does the user want the '%' to be printed?
             */
            if (fmtTemplate[template_pos] == '%')
            {
                if (!scanning)
                {
                    AROS_UFC3NR(VOID, putCharFunc->h_Entry,
                        AROS_UFCA(const struct Hook *, putCharFunc, A0),
                        AROS_UFCA(const struct Locale *, locale, A2),
                        AROS_UFCA(UBYTE, fmtTemplate[template_pos], A1));
                }
                template_pos++;
                arg_counter--;  //stegerg
            }
            else
            {
                /*
                 ** Now parsing...
                 ** Template format: %[arg_pos$][flags][width][.limit][length]type
                 **
                 ** arg_pos specifies the position of the argument in the dataStream
                 ** flags   only '-' is allowed
                 ** width
                 ** .limit
                 ** datasize size of the datatype
                 ** type    b,d,D,u,U,x,X,s,c
                 */
                ULONG arg_pos = 1;
                BOOL left = FALSE;      // no flag was found
                UBYTE fill = ' ';
                ULONG minus;
                ULONG width = 0;
                ULONG limit = ~0;
                ULONG buflen = 0;
                ULONG datasize;
                UFMTLARGESTTYPE tmp = 0;
#define BUFFERSIZE 128
                UBYTE buf[BUFFERSIZE];
                UBYTE *buffer = buf;

                /*
                 ** arg_pos
                 */

                //kprintf("next char: %c\n",fmtTemplate[template_pos]);

                if (fmtTemplate[template_pos] >= '0' &&
                    fmtTemplate[template_pos] <= '9')
                {
                    ULONG old_template_pos = template_pos;

                    for (arg_pos = 0; (fmtTemplate[template_pos] >= '0' &&
                            fmtTemplate[template_pos] <= '9');
                        template_pos++)
                    {
                        arg_pos =
                            arg_pos * 10 + fmtTemplate[template_pos] - '0';
                    }

                    if (fmtTemplate[template_pos] == '$')
                        template_pos++;
                    else
                    {
                        arg_pos = arg_counter;
                        template_pos = old_template_pos;
                    }
                }
                else
                    arg_pos = arg_counter;

                /*
                 ** flags
                 */
                if (fmtTemplate[template_pos] == '-')
                {
                    template_pos++;
                    left = TRUE;
                }

                /*
                 ** fill character a '0'?
                 */
                if (fmtTemplate[template_pos] == '0')
                {
                    template_pos++;
                    fill = '0';
                }

                /*
                 ** width
                 */
                if (fmtTemplate[template_pos] >= '0' &&
                    fmtTemplate[template_pos] <= '9')
                {
                    for (width = 0; (fmtTemplate[template_pos] >= '0' &&
                            fmtTemplate[template_pos] <= '9');
                        template_pos++)
                    {
                        width =
                            width * 10 + fmtTemplate[template_pos] - '0';
                    }
                }

                /*
                 ** limit
                 */
                if (fmtTemplate[template_pos] == '.')
                {
                    template_pos++;

                    if (fmtTemplate[template_pos] >= '0' &&
                        fmtTemplate[template_pos] <= '9')
                    {
                        for (limit = 0; (fmtTemplate[template_pos] >= '0' &&
                                fmtTemplate[template_pos] <= '9');
                            template_pos++)
                        {
                            limit =
                                limit * 10 + fmtTemplate[template_pos] -
                                '0';
                        }
                    }
                }

                /*
                 ** Length
                 */
                switch (fmtTemplate[template_pos])
                {
#if USE_QUADFMT
                case 'L':
                    datasize = sizeof(UQUAD);
                    template_pos++;
                    break;
#endif /* USE_QUADFMT */

                case 'l':
                    template_pos++;
#if USE_QUADFMT
                    if (fmtTemplate[template_pos] == 'l')
                    {
                        datasize = sizeof(UQUAD);
                        template_pos++;
                    }
                    else
#endif /* USE_QUADFMT */
                        datasize = sizeof(ULONG);
                    break;

                default:
                    /* For C-style varargs default size is ULONG, single 'l' is effectively ignored */
                    datasize =
                        is_va_list(VaListStream) ? sizeof(ULONG) :
                        sizeof(UWORD);
                    break;
                }

                /*
                 ** Print it according to the given type info.
                 */
                switch (fmtTemplate[template_pos])
                {
                case 'b':      /* BSTR, see autodocs */
                    /*
                     ** Important parameters:
                     ** arg_pos, left, buflen, limit
                     */
                    if (!scanning)
                    {
                        BSTR s = (BSTR) * (UBYTE **) indices[arg_pos - 1];

                        if (s != (BSTR) BNULL)
                        {
                            buffer = AROS_BSTR_ADDR(s);
                            buflen = AROS_BSTR_strlen(s);
                        }
                        else
                        {
                            buffer = "";
                            buflen = 0;
                        }

#if !USE_GLOBALLIMIT
                        if (buflen > limit)
                            buflen = limit;
#endif /* !USE_GLOBALLIMIT */
                    }
                    else
                        indices[arg_pos - 1] = sizeof(BPTR);
                    break;

                case 'd':      /* signed decimal */
                case 'u':      /* unsigned decimal */

                    minus = fmtTemplate[template_pos] == 'd';

                    if (!scanning)
                    {
                        switch (datasize)
                        {
#if USE_QUADFMT
                        case 8:
                            tmp = *(UQUAD *) indices[arg_pos - 1];
                            //buffer = &buf[16+1];
                            minus *= (FMTLARGESTTYPE) tmp < 0;
                            if (minus)
                                tmp = -tmp;
                            break;
#endif /* USE_QUADFMT */

                        case 4:
                            tmp = *(ULONG *) indices[arg_pos - 1];
                            //buffer = &buf[8+1];
                            minus *= (LONG) tmp < 0;
                            if (minus)
                                tmp = (ULONG) - tmp;
                            break;

                        default:       /* 2 */
                            tmp = *(UWORD *) indices[arg_pos - 1];
                            //buffer = &buf[4+1];
                            minus *= (WORD) tmp < 0;
                            if (minus)
                                tmp = (UWORD) - tmp;
                            break;
                        }

                        buffer = &buf[BUFFERSIZE];
                        do
                        {
                            *--buffer = (tmp % 10) + '0';
                            tmp /= 10;
                            buflen++;
                        }
                        while (tmp);

                        if (minus)
                        {
                            *--buffer = '-';
                            buflen++;
                        }

                    }
                    else
                        indices[arg_pos - 1] = datasize;
                    break;

                case 'D':      /* signed decimal with locale's formatting conventions */
                case 'U':      /* unsigned decimal with locale's formatting conventions */
                    if (!scanning)
                    {
                        UBYTE groupsize;
                        ULONG group_index = 0;

                        minus = fmtTemplate[template_pos] == 'D';

                        switch (datasize)
                        {
#if USE_QUADFMT
                        case 8:
                            tmp = *(UQUAD *) indices[arg_pos - 1];
                            minus *= (FMTLARGESTTYPE) tmp < 0;
                            if (minus)
                                tmp = -tmp;
                            break;
#endif /* USE_QUADFMT */

                        case 4:
                            tmp = *(ULONG *) indices[arg_pos - 1];
                            minus *= (LONG) tmp < 0;
                            if (minus)
                                tmp = (ULONG) - tmp;
                            break;

                        default:       /* 2 */
                            tmp = *(UWORD *) indices[arg_pos - 1];
                            minus *= (WORD) tmp < 0;
                            if (minus)
                                tmp = (UWORD) - tmp;
                            break;
                        }

                        /* BUFFERSIZE should be big enough to format a string
                         ** according to locale's formatting conventions
                         */
                        buffer = &buf[BUFFERSIZE];
                        groupsize =
                            locale ? locale->
                            loc_Grouping[group_index] : 255;

                        do
                        {
                            *--buffer = (tmp % 10) + '0';
                            tmp /= 10;
                            buflen++;

                            groupsize--;

                            if (groupsize == 0 && tmp != 0)
                            {
                                /*
                                 ** Write the separator
                                 */

                                *--buffer =
                                    locale->loc_GroupSeparator[group_index];

                                groupsize =
                                    locale->loc_Grouping[group_index + 1];

                                if (groupsize == 0)
                                {
                                    /*
                                     ** Supposed to use the previous element
                                     */
                                    groupsize =
                                        locale->loc_Grouping[group_index];
                                }
                                else
                                    group_index++;

                                buflen++;
                            }
                        }
                        while (tmp);

                        if (minus)
                        {
                            *--buffer = '-';
                            buflen++;
                        }
                    }
                    else
                        indices[arg_pos - 1] = datasize;
                    break;

                case 'p':      /* lower case pointer string */
                case 'P':      /* upper case pointer string */
                    fill = '0';
                    width = sizeof(APTR) * 2;
                    /* %p is always at least natural pointer size */
                    if (datasize < sizeof(APTR))
                        datasize = sizeof(APTR);
                case 'x':      /* upper case hexadecimal string */
                case 'X':      /* lower case hexadecimal string */

                    if (!scanning)
                    {
                        const UBYTE *hexa;

                        switch (datasize)
                        {
#if USE_QUADFMT
                        case 8:
                            tmp = *(UQUAD *) indices[arg_pos - 1];
                            //buffer = &buf[16+1];
                            break;
#endif /* USE_QUADFMT */

                        case 4:
                            tmp = *(ULONG *) indices[arg_pos - 1];
                            //buffer = &buf[8+1];
                            break;

                        default:       /* 2 */
                            tmp = *(UWORD *) indices[arg_pos - 1];
                            //buffer = &buf[4+1];
                            break;
                        }

                        buffer = &buf[BUFFERSIZE];

                        /* NOTE: x/X is reverse to printf, coz orig RawDoFmt %lx for uppercase. */
                        hexa = (fmtTemplate[template_pos] == 'X' ||
                            fmtTemplate[template_pos] ==
                            'p') ? hexarray : HEXarray;
                        do
                        {
                            *--buffer = hexa[tmp & 0x0f];
                            tmp >>= 4;
                            buflen++;
                        }
                        while (tmp);
                    }
                    else
                        indices[arg_pos - 1] = datasize;
                    break;

                case 's':      /* NULL terminated string */
                    {
                        if (!scanning)
                        {
                            buffer = *(UBYTE **) indices[arg_pos - 1];

                            /*
                             * RawDoFmt() in original AmigaOS(tm) formats NULL pointers as empty strings,
                             * and not something like "(null)". Some software may rely on this behavior.
                             * %b is handled in similar manner.
                             */
                            if (!buffer)
                                buffer = "";
                            buflen = strlen(buffer);

#if !USE_GLOBALLIMIT
                            if (buflen > limit)
                                buflen = limit;
#endif /* !USE_GLOBALLIMIT */
                        }
                        else
                            indices[arg_pos - 1] = sizeof(UBYTE *);     /* the pointer has 4 bytes */
                    }
                    break;

                case 'c':      /* Character */
                    if (!scanning)
                    {
                        switch (datasize)
                        {
#if USE_QUADFMT
                        case 8:
                            buf[0] =
                                (UBYTE) * (UQUAD *) indices[arg_pos - 1];
                            break;
#endif /* USE_QUADFMT */

                        case 4:
                            buf[0] =
                                (UBYTE) * (ULONG *) indices[arg_pos - 1];
                            break;

                        default:       /* 2 */
                            buf[0] =
                                (UBYTE) * (WORD *) indices[arg_pos - 1];
                            break;
                        }

                        buflen = 1;
                    }
                    else
                        indices[arg_pos - 1] = datasize;
                    break;

                default:
                    /* Ignore the faulty '%' */

                    if (!scanning)
                    {
                        buf[0] = fmtTemplate[template_pos];
                        width = 1;
                        buflen = 1;
                    }

                    arg_pos = --arg_counter;
                    break;
                }


                if (!scanning)
                {
                    int i;

                    /*
                       Now everything I need is known:
                       buffer  - contains the string to be printed
                       buflen  - size of the string
                       fill    - the pad character
                       left    - is 1 if the string should be left aligned
                       width   - is the minimal width of the field
                       limit   - maximum number of characters to output from a string, default ~0
                     */

#if USE_GLOBALLIMIT
                    if (buflen > limit)
                        buflen = limit;
#endif /* USE_GLOBALLIMIT */

                    /* Print padding if right aligned */
                    if (!left)
                        for (i = buflen; i < width; i++)
                            AROS_UFC3NR(VOID, putCharFunc->h_Entry,
                                AROS_UFCA(const struct Hook *, putCharFunc, A0),
                                AROS_UFCA(const struct Locale *, locale, A2),
                                AROS_UFCA(UBYTE, fill, A1));

                    /* Print body up to buflen */
                    for (i = 0; i < buflen; i++)
                    {
                        AROS_UFC3NR(VOID, putCharFunc->h_Entry,
                            AROS_UFCA(const struct Hook *, putCharFunc, A0),
                            AROS_UFCA(const struct Locale *, locale, A2),
                            AROS_UFCA(UBYTE, *buffer++, A1));
                    }

                    /* Pad right if left aligned */
                    if (left)
                        for (i = buflen; i < width; i++)
                            AROS_UFC3NR(VOID, putCharFunc->h_Entry,
                                AROS_UFCA(const struct Hook *, putCharFunc, A0),
                                AROS_UFCA(const struct Locale *, locale, A2),
                                AROS_UFCA(UBYTE, fill, A1));
                }

                template_pos++;

                if (arg_pos > max_argpos)
                    max_argpos = arg_pos;

            }
            state = OUTPUT;
            break;
        }
    }

    return (APTR) indices[max_argpos];
}

/*****************************************************************************

    NAME */
#include <proto/locale.h>

        AROS_LH4(APTR, FormatString,

/*  SYNOPSIS */
        AROS_LHA(const struct Locale *, locale, A0),
        AROS_LHA(CONST_STRPTR, fmtTemplate, A1),
        AROS_LHA(CONST_APTR, dataStream, A2),
        AROS_LHA(const struct Hook *, putCharFunc, A3),

/*  LOCATION */
        struct LocaleBase *, LocaleBase, 11, Locale)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    null_va_list(vaListStream);

    return InternalFormatString(locale, fmtTemplate, dataStream,
        putCharFunc, vaListStream);

    AROS_LIBFUNC_EXIT
}
