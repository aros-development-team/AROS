/*
    Copyright (C) 2018-2025, The AROS Development Team. All rights reserved.
    $Id:$

    Common code block to format a string like printf().
*/

/*
 * This code is used by debug functions during early startup.
 * Please keep it self-contained, at least when compiled with -DSTDC_STATIC.
 */

    size_t outcount=0;

    while (*format)
    {
        if (*format == '%')
        {
            static const FMTPRINTF_TYPE flagc[] = {FMTPRINTF_STR('#'), FMTPRINTF_STR('0'), FMTPRINTF_STR('-'), FMTPRINTF_STR(' '), FMTPRINTF_STR('+') };
            size_t width=0, preci=ULONG_MAX, flags=0; /* Specifications */
            FMTPRINTF_TYPE type, subtype = FMTPRINTF_STR('i');
#ifdef AROS_HAVE_LONG_LONG
            FMTPRINTF_TYPE lltype=0;
#endif
            FMTPRINTF_TYPE buffer1[2];                            /* Signs and that like */
            FMTPRINTF_TYPE buffer[REQUIREDBUFFER];                /* The body */
            FMTPRINTF_TYPE *buffer2 = buffer;                     /* So we can set this to any other strings */
            size_t size1 = 0, size2 = 0;                /* How many chars in buffer? */
            const FMTPRINTF_TYPE *ptr = format + 1;               /* pointer to format string */
            size_t i, pad;                              /* Some temporary variables */
#if defined(FULL_SPECIFIERS)
            union {                                     /* floating point arguments %[aAeEfFgG] */
                double dbl;
                long double ldbl;
            } fparg;
#endif
            do                                          /* read flags */
                for (i = 0; i < sizeof(flagc); i++)
                    if (flagc[i] == *ptr)
                    {
                        flags |= 1<<i;
                        ptr++;
                        break;
                    }
            while (i < sizeof(flagc));

            if (*ptr == FMTPRINTF_STR('*'))                            /* read width from arguments */
            {
                signed int a;
                ptr++;
                a = va_arg(args, signed int);
                if (a < 0)
                {
                    flags |= LALIGNFLAG;
                    width = -a;
                }
                else
                    width = a;
            }else
                while (FMTPRINTF_ISDIGIT(*ptr))
                    width = width * 10 + (*ptr++ - FMTPRINTF_STR('0'));

            if (*ptr == FMTPRINTF_STR('.'))
            {
                ptr++;
                if (*ptr == FMTPRINTF_STR('*'))                        /* read precision from arguments */
                {
                    signed int a;
                    ptr++;
                    a = va_arg(args, signed int);
                    if (a >= 0)
                        preci = a;
                }else
                {
                    preci = 0;
                    while (FMTPRINTF_ISDIGIT(*ptr))
                        preci = preci * 10 + (*ptr++ - FMTPRINTF_STR('0'));
                }
            }

            if (*ptr == FMTPRINTF_STR('h') || *ptr == FMTPRINTF_STR('l') || *ptr == FMTPRINTF_STR('L') || *ptr == FMTPRINTF_STR('z'))
                subtype=*ptr++;

            if (*ptr == FMTPRINTF_STR('l') || *ptr == FMTPRINTF_STR('q'))
            {
#ifdef AROS_HAVE_LONG_LONG
                lltype = 1;
                subtype = FMTPRINTF_STR('l');
#endif
                ptr++;
            }
            
            type = *ptr++;

            switch(type)
            {
            case FMTPRINTF_STR('d'):
            case FMTPRINTF_STR('i'):
            case FMTPRINTF_STR('o'):
            case FMTPRINTF_STR('p'):
            case FMTPRINTF_STR('u'):
            case FMTPRINTF_STR('x'):
            case FMTPRINTF_STR('X'):
            {
#ifdef AROS_HAVE_LONG_LONG
                    unsigned long long llv = 0;
#endif
                    unsigned long v = 0;
                    int base = 10;

                    if (type==FMTPRINTF_STR('p'))                      /* This is written as 0x08lx (or 0x016lx on 64 bits) */
                    {
                        subtype = FMTPRINTF_STR('l');
                        type = FMTPRINTF_STR('x');
                        if (!width)
                            width = sizeof(void *) * 2;
                        flags |= ZEROPADFLAG;
                    }

                    if (type == FMTPRINTF_STR('d') || type == FMTPRINTF_STR('i'))         /* These are signed */
                    {
                        signed long v2;

                        if (subtype == FMTPRINTF_STR('l'))
                        {
#ifdef AROS_HAVE_LONG_LONG
                            if (lltype)
                            {
                                signed long long llv2;
                                
                                llv2 = va_arg(args, signed long long);
                                if (llv2 < 0)
                                {
                                    llv = - llv2;
                                    v2 = -1;            /* Assign a dummy value to v2 in order to process sign below */
                                }
                                else
                                {
                                    llv = llv2;
                                    v2    = llv2 ? 1 : 0;
                                }
                            }
                            else
#endif
                                v2=va_arg(args, signed long);
                        }
                        else if (subtype == FMTPRINTF_STR('z'))
                            v2 = va_arg(args, size_t);
                        else
                            v2 = va_arg(args, signed int);

                        if (v2 < 0)
                        {
                            buffer1[size1++] = FMTPRINTF_STR('-');
                            v = -v2;
                        }
                        else
                        {
                            if (flags & SIGNFLAG)
                                buffer1[size1++] = FMTPRINTF_STR('+');
                            else if (flags & BLANKFLAG)
                                buffer1[size1++] = FMTPRINTF_STR(' ');
                            v = v2;
                        }
                    }
                    else                                /* These are unsigned */
                    {
                        if (subtype == FMTPRINTF_STR('l'))
                        {
#ifdef AROS_HAVE_LONG_LONG
                            if (lltype)
                                llv = va_arg(args, unsigned long long);
                            else
#endif
                                v = va_arg(args, unsigned long);
                        }
                        else if (subtype == FMTPRINTF_STR('z'))
                            v = va_arg(args, size_t);
                        else
                            v = va_arg(args, unsigned int);

                        if (flags & ALTERNATEFLAG)
                        {
                            FMTPRINTF_TYPE nzero;
#ifdef AROS_HAVE_LONG_LONG
                            if (lltype)
                                nzero = (llv != 0);
                            else
#endif
                                nzero = (v != 0);

                            if (type == FMTPRINTF_STR('o') && preci && nzero)
                                buffer1[size1++] = FMTPRINTF_STR('0');
                            if ((type == FMTPRINTF_STR('x') || type == FMTPRINTF_STR('X')) && nzero)
                            {
                                buffer1[size1++] = FMTPRINTF_STR('0');
                                buffer1[size1++] = type;
                            }
                        }
                    }

                    buffer2 = &buffer[sizeof(buffer)];  /* Calculate body string */

#ifdef AROS_HAVE_LONG_LONG
                    /*
                     * For long long type we have actual value in llv.
                     * For long we have actual value in v.
                     * This avoids slow 64-bit operations on 32-bit processors
                     * when not needed.
                     */
                    if (lltype)
                        size2 = format_longlong(buffer2, type, base, llv);
                    else
#endif
                        size2 = format_long(buffer2, type, base, v);
                    /* Position to the beginning of the string */
                    buffer2 -= size2;

                    if (preci == ULONG_MAX)             /* default */
                        preci = 0;
                    else
                        flags &= ~ZEROPADFLAG;
                    break;
            }

            case FMTPRINTF_STR('c'):
                    if (subtype == FMTPRINTF_STR('l'))
                    {
#ifdef AROS_HAVE_LONG_LONG
                        if (lltype)
                            *buffer2 = va_arg(args, long long);
                        else
#endif
                            *buffer2 = va_arg(args, long);
                    }
                    else
                        *buffer2 = va_arg(args, int);

                    size2 = 1;
                    preci = 0;
                    break;

            case FMTPRINTF_STR('s'):
                    buffer2 = va_arg(args, FMTPRINTF_TYPE *);
                    if (!buffer2)
                        buffer2 = FMTPRINTF_STR("(null)");
                    size2 = FMTPRINTF_STRLEN(buffer2);
                    size2 = size2 <= preci ? size2 : preci;
                    preci = 0;
                    break;

            case FMTPRINTF_STR('b'):
                    buffer2 = BADDR(va_arg(args, BPTR));
                    if (buffer2)
#if AROS_FAST_BSTR
                        size2 = FMTPRINTF_STRLEN(buffer2);
#else
                        size2 = *(FMTPRINTF_UTYPE *)buffer2++;
#endif
                    else
                    {
                        buffer2 = FMTPRINTF_STR("(null)");
                        size2 = 6;
                    }

                    size2 = size2 <= preci ? size2 : preci;
                    preci = 0;
                    break;

#ifdef FULL_SPECIFIERS
            case FMTPRINTF_STR('a'):
            case FMTPRINTF_STR('A'):
            case FMTPRINTF_STR('f'):
            case FMTPRINTF_STR('F'):
            case FMTPRINTF_STR('e'):
            case FMTPRINTF_STR('E'):
            case FMTPRINTF_STR('g'):
            case FMTPRINTF_STR('G'):
            {
                    FMTPRINTF_TYPE killzeros=0, sign=0;           /* some flags */
                    int ex1, ex2;                       /* Some temporary variables */
                    size_t size, dnum, dreq;
                    FMTPRINTF_TYPE *udstr = NULL;

                    if (subtype == FMTPRINTF_STR('L'))
                    {
                        flags |= LDBLFLAG;
                        fparg.ldbl = va_arg(args, long double);
                    } else {
                        flags &= ~LDBLFLAG;
                        fparg.dbl = va_arg(args, double);
                    }

                    if (isinf(fparg.dbl))
                    {
                        if (fparg.dbl>0)
                            udstr = FMTPRINTF_STR("+inf");
                        else
                            udstr = FMTPRINTF_STR("-inf");
                    } else if (isnan(fparg.dbl))
                        udstr = FMTPRINTF_STR("NaN");

                    if (udstr != NULL)
                    {
                        size2 = FMTPRINTF_STRLEN(udstr);
                        preci = 0;
                        buffer2 = udstr;
                        break; }

                    if (preci == ULONG_MAX)             /* old default */
                        preci = 6;                      /* new default */

                    if (((subtype != FMTPRINTF_STR('L')) && (fparg.dbl < 0.0)) || ((subtype == FMTPRINTF_STR('L')) && (fparg.ldbl < 0.0)))
                    {
                        sign = FMTPRINTF_STR('-');
                        if (subtype == FMTPRINTF_STR('L'))
                            fparg.ldbl = -fparg.ldbl;
                        else
                            fparg.dbl = -fparg.dbl;
                    } else {
                        if (flags & SIGNFLAG)
                            sign = FMTPRINTF_STR('+');
                        else if (flags & BLANKFLAG)
                            sign = FMTPRINTF_STR(' ');
                    }

                    ex1 = 0;
                    if (FMTPRINTF_TOLOWER(type) != FMTPRINTF_STR('a'))
                    {
                        if (subtype != FMTPRINTF_STR('L'))
                        {
                            if (fparg.dbl != 0.0)
                            {
                                ex1 = log10(fparg.dbl);
                                if (fparg.dbl < 1.0)
                                    fparg.dbl = fparg.dbl * pow(10,- --ex1); /* Caution: (int)log10(.5)!=-1 */
                                else
                                    fparg.dbl=fparg.dbl / pow(10, ex1);
                                if (fparg.dbl < 1.0)    /* adjust if we are too low (log10(.1)=-.999999999) */
                                {
                                    fparg.dbl *= 10.0;  /* luckily this cannot happen with FLT_MAX and FLT_MIN */
                                    ex1--;              /* The case too high (log(10.)=.999999999) is done later */
                                }
                            }
                        } else {
                            if (fparg.ldbl != 0.0)
                            {
                                ex1 = log10l(fparg.ldbl);
                                if (fparg.ldbl < 1.0)
                                    fparg.ldbl= fparg.ldbl * powl(10,- --ex1);
                                else
                                    fparg.ldbl=fparg.ldbl / powl(10, ex1);
                                if (fparg.ldbl < 1.0)
                                {
                                    fparg.ldbl *= 10.0;
                                    ex1--;
                                }
                            }
                        }
                    }

                    ex2 = preci;
                    if (FMTPRINTF_TOLOWER(type) == FMTPRINTF_STR('f'))
                        ex2 += ex1;
                    if (FMTPRINTF_TOLOWER(type) == FMTPRINTF_STR('g'))
                        ex2--;
                    if (subtype != FMTPRINTF_STR('L'))
                    {
                        fparg.dbl += .5 / pow(10, ex2 < MINFLOATSIZE ? ex2 : MINFLOATSIZE); /* Round up */
                        if (fparg.dbl >= 10.0)          /* Adjusts log10(10.)=.999999999 too */
                        {
                            fparg.dbl /= 10.0;
                            ex1++;
                        }
                    } else {
                        fparg.ldbl += .5 / powl(10, ex2 < MINFLOATSIZE ? ex2 : MINFLOATSIZE); /* Round up */
                        if (fparg.ldbl >= 10.0)         /* Adjusts log10(10.)=.999999999 too */
                        {
                            fparg.ldbl /= 10.0;
                            ex1++;
                        }
                    }

                    if (FMTPRINTF_TOLOWER(type) == FMTPRINTF_STR('g'))           /* This changes to one of the other types */
                    {
                        if (ex1 < (signed long)preci && ex1 >= -4)
                        {
                            type = FMTPRINTF_STR('f');
                            preci -= ex1;
                        } else
                            type = type == FMTPRINTF_STR('g') ? FMTPRINTF_STR('e') : FMTPRINTF_STR('E');
                        preci--;
                        if (!(flags & ALTERNATEFLAG))
                            killzeros = 1;              /* set flag to kill trailing zeros */
                    }

                    dreq = preci + 1;                   /* Calculate number of decimal places required */
                    if (type == FMTPRINTF_STR('f'))
                        dreq += ex1;                    /* even more before the decimal point */

                    dnum = 0;
                    while (dnum < dreq && dnum < MINFLOATSIZE) /* Calculate all decimal places needed */
                    {
                        if (subtype != FMTPRINTF_STR('L'))
                        {
                            buffer[dnum++] = (FMTPRINTF_TYPE)fparg.dbl + FMTPRINTF_STR('0');
                            fparg.dbl = (fparg.dbl - (double)(FMTPRINTF_TYPE)fparg.dbl) * 10.0;
                        } else {
                            buffer[dnum++] = (FMTPRINTF_TYPE)fparg.ldbl + FMTPRINTF_STR('0');
                            fparg.ldbl = (fparg.ldbl - (long double)(FMTPRINTF_TYPE)fparg.ldbl) * 10.0;
                        }
                    }
                    if (killzeros)                      /* Kill trailing zeros if possible */
                        while(preci && (dreq-- > dnum || buffer[dreq] == FMTPRINTF_STR('0')))
                            preci--;

                    if (FMTPRINTF_TOLOWER(type) == FMTPRINTF_STR('f'))           /* Calculate actual size of string (without sign) */
                    {
                        size = preci + 1;               /* numbers after decimal point + 1 before */
                        if (ex1 > 0)
                            size += ex1;                /* numbers >= 10 */
                        if (preci || flags & ALTERNATEFLAG)
                            size++;                     /* 1 for decimal point */
                    }else
                    {
                        size = preci + 5;               /* 1 for the number before the decimal point, and 4 for the exponent */
                        if (preci || flags & ALTERNATEFLAG)
                            size++;
                        if (ex1 > 99 || ex1 < -99)
                            size++;                     /* exponent needs an extra decimal place */
                    }

                    pad = size + (sign != 0);
                    pad = pad >= width ? 0 : width - pad;

                    if (sign && flags & ZEROPADFLAG)
                        FMTPRINTF_OUT(sign, ctx);

                    if (!(flags & LALIGNFLAG))
                        for (i = 0; i < pad; i++)
                            FMTPRINTF_OUT(flags & ZEROPADFLAG ? FMTPRINTF_STR('0') : FMTPRINTF_STR(' '), ctx);

                    if (sign && !(flags & ZEROPADFLAG))
                        FMTPRINTF_OUT(sign, ctx);

                    dreq = 0;
                    if (FMTPRINTF_TOLOWER(type) == FMTPRINTF_STR('a'))
                    {
                        // TODO: Implement hexfloat literals
                        FMTPRINTF_OUT(FMTPRINTF_STR('0'), ctx);
                        FMTPRINTF_OUT(FMTPRINTF_STR('x'), ctx);
                        FMTPRINTF_OUT(FMTPRINTF_STR('0'), ctx);
                        FMTPRINTF_OUT(FMTPRINTF_STR('.'), ctx);
                        FMTPRINTF_OUT(FMTPRINTF_STR('0'), ctx);
                        if (type == FMTPRINTF_STR('A'))
                            FMTPRINTF_OUT(FMTPRINTF_STR('P'), ctx);
                        else
                            FMTPRINTF_OUT(FMTPRINTF_STR('p'), ctx);
                        FMTPRINTF_OUT(FMTPRINTF_STR('0'), ctx);
                    } else if (FMTPRINTF_TOLOWER(type) == FMTPRINTF_STR('f')) {
                        if (ex1 < 0)
                            FMTPRINTF_OUT(FMTPRINTF_STR('0'), ctx);
                        else
                            while(ex1 >= 0)
                            {
                                FMTPRINTF_OUT(dreq < dnum ? buffer[dreq++] : FMTPRINTF_STR('0'), ctx);
                                ex1--;
                            }
                        if (preci || flags & ALTERNATEFLAG)
                        {
                            FMTPRINTF_OUT(FMTPRINTF_DECIMALPOINT[0], ctx);
                            while(preci--)
                                if (++ex1 < 0)
                                    FMTPRINTF_OUT(FMTPRINTF_STR('0'), ctx);
                                else
                                    FMTPRINTF_OUT(dreq < dnum ? buffer[dreq++] : FMTPRINTF_STR('0'), ctx);
                        }
                    } else {
                        FMTPRINTF_OUT(buffer[dreq++], ctx);
                        if (preci || flags & ALTERNATEFLAG)
                        {
                            FMTPRINTF_OUT(FMTPRINTF_DECIMALPOINT[0], ctx);
                            while(preci--)
                                FMTPRINTF_OUT(dreq < dnum ? buffer[dreq++] : FMTPRINTF_STR('0'), ctx);
                        }
                        FMTPRINTF_OUT(type, ctx);
                        if (ex1 < 0)
                        {
                            FMTPRINTF_OUT(FMTPRINTF_STR('-'), ctx);
                            ex1 = -ex1;
                        }
                        else
                            FMTPRINTF_OUT(FMTPRINTF_STR('+'), ctx);
                        if (ex1 > 99)
                            FMTPRINTF_OUT(ex1 / 100 + FMTPRINTF_STR('0'), ctx);
                        FMTPRINTF_OUT(ex1 / 10 % 10 + FMTPRINTF_STR('0'), ctx);
                        FMTPRINTF_OUT(ex1 % 10 + FMTPRINTF_STR('0'), ctx);
                    }

                    if (flags & LALIGNFLAG)
                        for (i = 0; i < pad; i++)
                            FMTPRINTF_OUT(FMTPRINTF_STR(' '), ctx);

                    width = preci = 0;                  /* Everything already done */
                    break;
                }
#endif
                case FMTPRINTF_STR('%'):
                    buffer2 = FMTPRINTF_STR("%");
                    size2 = 1;
                    preci = 0;
                    break;
                case FMTPRINTF_STR('n'):
                    *va_arg(args, int *) = outcount;
                    width = preci = 0;
                    break;
                default:
                    if (!type)
                        ptr--;                          /* We've gone too far - step one back */
                    buffer2 = (FMTPRINTF_TYPE *)format;
                    size2 = ptr - format;
                    width = preci = 0;
                    break;
            }
            pad = size1 + (size2 >= preci ? size2 : preci); /* Calculate the number of characters */
            pad = pad >= width ? 0 : width - pad;       /* and the number of resulting pad bytes */

            if (flags & ZEROPADFLAG)                    /* print sign and that like */
                for (i = 0; i < size1; i++)
                    FMTPRINTF_OUT(buffer1[i], ctx);

            if (!(flags & LALIGNFLAG))                  /* Pad left */
                for (i = 0; i < pad; i++)
                    FMTPRINTF_OUT(flags & ZEROPADFLAG ? FMTPRINTF_STR('0') : FMTPRINTF_STR(' '), ctx);

            if (!(flags & ZEROPADFLAG))                 /* print sign if not zero padded */
                for (i = 0; i < size1; i++)
                    FMTPRINTF_OUT(buffer1[i], ctx);

            for (i = size2; i < preci; i++)             /* extend to precision */
                FMTPRINTF_OUT(FMTPRINTF_STR('0'), ctx);

            for (i = 0; i < size2; i++)                 /* print body */
                FMTPRINTF_OUT(buffer2[i], ctx);

            if (flags & LALIGNFLAG)                     /* Pad right */
                for (i = 0; i < pad; i++)
                    FMTPRINTF_OUT(FMTPRINTF_STR(' '), ctx);

            format = ptr;
        }
        else
            FMTPRINTF_OUT(*format++, ctx);
    }
