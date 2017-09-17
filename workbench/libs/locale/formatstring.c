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

#include <clib/alib_protos.h>

#include <aros/debug.h>

typedef QUAD FMTLARGESTTYPE;
typedef UQUAD UFMTLARGESTTYPE;

static const UBYTE hexarray[] = "0123456789abcdef";
static const UBYTE HEXarray[] = "0123456789ABCDEF";

APTR InternalFormatString(const struct Locale * locale,
    CONST_STRPTR fmtTemplate, CONST_APTR dataStream,
    ULONG *indexStream, const struct Hook * putCharFunc)
{
    enum
    { OUTPUT = 0,
        FOUND_FORMAT
    } state;

#define ARG(x)  ((CONST_APTR)((IPTR)dataStream + indexStream[(x) - 1]))

    ULONG template_pos;
    BOOL end;
    ULONG max_argpos, max_argpos_datasize;
    ULONG arg_counter;

    if (!fmtTemplate)
        return (APTR)dataStream;

    template_pos = 0;           /* Current position in the template string */
    state = OUTPUT;             /* current state of parsing */
    end = FALSE;
    max_argpos = 1;
    arg_counter = 0;
    max_argpos_datasize = 0;

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
            AROS_UFC3NR(VOID, putCharFunc->h_Entry,
                AROS_UFCA(const struct Hook *, putCharFunc, A0),
                AROS_UFCA(const struct Locale *, locale, A2),
                AROS_UFCA(UBYTE, fmtTemplate[template_pos], A1));

            /*
             ** End of template string? -> End of this function.
             */
            if (fmtTemplate[template_pos] == '\0')
            {
                end = TRUE;
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
                AROS_UFC3NR(VOID, putCharFunc->h_Entry,
                    AROS_UFCA(const struct Hook *, putCharFunc, A0),
                    AROS_UFCA(const struct Locale *, locale, A2),
                        AROS_UFCA(UBYTE, fmtTemplate[template_pos], A1));
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
                case 'i':
                    /* IPTR-sized type, can be mixed with %d, %u or %x */
                    datasize = sizeof(IPTR);
                    template_pos++;
                    break;
                case 'L':
                    datasize = sizeof(UQUAD);
                    template_pos++;
                    break;
                case 'l':
                    template_pos++;
                    if (fmtTemplate[template_pos] == 'l')
                    {
                        datasize = sizeof(UQUAD);
                        template_pos++;
                    }
                    else
                        datasize = sizeof(ULONG);
                    break;

                default:
                    datasize = sizeof(UWORD);
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
                    {
                        datasize = sizeof(IPTR);
                        BSTR s = (BSTR) * (UBYTE **) ARG(arg_pos);

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
                    break;

                case 'd':      /* signed decimal */
                case 'u':      /* unsigned decimal */

                    minus = fmtTemplate[template_pos] == 'd';

                    {
                        switch (datasize)
                        {
                        case 8:
                            tmp = *(UQUAD *) ARG(arg_pos);
                            //buffer = &buf[16+1];
                            minus *= (FMTLARGESTTYPE) tmp < 0;
                            if (minus)
                                tmp = -tmp;
                            break;
                        case 4:
                            tmp = *(ULONG *) ARG(arg_pos);
                            //buffer = &buf[8+1];
                            minus *= (LONG) tmp < 0;
                            if (minus)
                                tmp = (ULONG) - tmp;
                            break;

                        default:       /* 2 */
                            tmp = *(UWORD *) ARG(arg_pos);
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
                    break;

                case 'D':      /* signed decimal with locale's formatting conventions */
                case 'U':      /* unsigned decimal with locale's formatting conventions */
                    {
                        UBYTE groupsize;
                        ULONG group_index = 0;

                        minus = fmtTemplate[template_pos] == 'D';

                        switch (datasize)
                        {
                        case 8:
                            tmp = *(UQUAD *) ARG(arg_pos);
                            minus *= (FMTLARGESTTYPE) tmp < 0;
                            if (minus)
                                tmp = -tmp;
                            break;
                        case 4:
                            tmp = *(ULONG *) ARG(arg_pos);
                            minus *= (LONG) tmp < 0;
                            if (minus)
                                tmp = (ULONG) - tmp;
                            break;

                        default:       /* 2 */
                            tmp = *(UWORD *) ARG(arg_pos);
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

                    {
                        const UBYTE *hexa;

                        switch (datasize)
                        {
                        case 8:
                            tmp = *(UQUAD *) ARG(arg_pos);
                            //buffer = &buf[16+1];
                            break;
                        case 4:
                            tmp = *(ULONG *) ARG(arg_pos);
                            //buffer = &buf[8+1];
                            break;

                        default:       /* 2 */
                            tmp = *(UWORD *) ARG(arg_pos);
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
                    break;

                case 's':      /* NULL terminated string */
                    {
                        datasize = sizeof(IPTR);
                        {
                            buffer = *(UBYTE **) ARG(arg_pos);

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
                    }
                    break;

                case 'c':      /* Character */
                    {
                        switch (datasize)
                        {
                        case 8:
                            buf[0] =
                                (UBYTE) * (UQUAD *) ARG(arg_pos);
                            break;
                        case 4:
                            buf[0] =
                                (UBYTE) * (ULONG *) ARG(arg_pos);
                            break;

                        default:       /* 2 */
                            buf[0] =
                                (UBYTE) * (WORD *) ARG(arg_pos);
                            break;
                        }

                        buflen = 1;
                    }
                    break;

                default:
                    /* Ignore the faulty '%' */

                    buf[0] = fmtTemplate[template_pos];
                    width = 1;
                    buflen = 1;
                    arg_pos = --arg_counter;
                    break;
                }


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
                {
                    max_argpos = arg_pos;
                    max_argpos_datasize = datasize;
                }

            }
            state = OUTPUT;
            break;
        }
    }

    return (APTR)(ARG(max_argpos) + max_argpos_datasize);
}

/*****************************************************************************

    NAME */
#include <proto/locale.h>

        AROS_LH4(APTR, FormatString,

/*  SYNOPSIS */
        AROS_LHA(const struct Locale *, locale, A0),
        AROS_LHA(CONST_STRPTR, fmtTemplate, A1),
        AROS_LHA(RAWARG, dataStream, A2),
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
    ULONG *indices;
    ULONG indexSize = 0;
    APTR retval;
    struct Locale *def_locale = NULL;
#if defined(__arm__) || defined(__x86_64__)
    va_list nullarg = {};
#else
    va_list nullarg = 0;
#endif

    if (locale == NULL)
    {
        locale = OpenLocale(NULL);
        def_locale = (struct Locale *)locale;
    }

    /* Generate the indexes for the provided datastream */
    GetDataStreamFromFormat(fmtTemplate, nullarg, NULL, NULL, NULL, &indexSize);
    indices = alloca(indexSize);
    GetDataStreamFromFormat(fmtTemplate, nullarg, NULL, NULL, indices, &indexSize);

    retval = InternalFormatString(locale, fmtTemplate,
                                dataStream, indices, putCharFunc);

    CloseLocale(def_locale);

    return retval;

    AROS_LIBFUNC_EXIT
}
