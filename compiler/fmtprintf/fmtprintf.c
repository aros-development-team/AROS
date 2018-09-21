/*
    Copyright © 2018, The AROS Development Team. All rights reserved.
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
            static const char flagc[] = { '#', '0', '-', ' ', '+' };
            size_t width=0, preci=ULONG_MAX, flags=0; /* Specifications */
            char type, subtype = 'i';
#ifdef AROS_HAVE_LONG_LONG
            char lltype=0;
#endif
            char buffer1[2];                            /* Signs and that like */
            char buffer[REQUIREDBUFFER];                /* The body */
            char *buffer2 = buffer;	                /* So we can set this to any other strings */
            size_t size1 = 0, size2 = 0;                /* How many chars in buffer? */
            const char *ptr = format + 1;               /* pointer to format string */
            size_t i, pad;		                /* Some temporary variables */
            union {			                /* floating point arguments %[aAeEfFgG] */
                double dbl;
                long double ldbl;
            } fparg;

            do                                          /* read flags */
                for (i = 0; i < sizeof(flagc); i++)
                    if (flagc[i] == *ptr)
                    {
                        flags |= 1<<i;
                        ptr++;
                        break;
                    }
            while (i < sizeof(flagc));

            if (*ptr == '*')                            /* read width from arguments */
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
                while (isdigit(*ptr))
                    width = width * 10 + (*ptr++ - '0');

            if (*ptr == '.')
            {
                ptr++;
                if (*ptr == '*')                        /* read precision from arguments */
                {
                    signed int a;
                    ptr++;
                    a = va_arg(args, signed int);
                    if (a >= 0)
                        preci = a;
                }else
                {
                    preci = 0;
                    while (isdigit(*ptr))
                        preci = preci * 10 + (*ptr++ - '0');
                }
            }

            if (*ptr == 'h' || *ptr == 'l' || *ptr == 'L' || *ptr == 'z')
                subtype=*ptr++;

            if (*ptr == 'l' || *ptr == 'q')
            {
#ifdef AROS_HAVE_LONG_LONG
                lltype = 1;
                subtype = 'l';
#endif
                ptr++;
            }
            
            type = *ptr++;

            switch(type)
            {
            case 'd':
            case 'i':
            case 'o':
            case 'p':
            case 'u':
            case 'x':
            case 'X':
            {
#ifdef AROS_HAVE_LONG_LONG
                    unsigned long long llv = 0;
#endif
                    unsigned long v = 0;
                    int base = 10;

                    if (type=='p')                      /* This is written as 0x08lx (or 0x016lx on 64 bits) */
                    {
                        subtype = 'l';
                        type = 'x';
                        if (!width)
                            width = sizeof(void *) * 2;
                        flags |= ZEROPADFLAG;
                    }

                    if (type=='d' || type=='i')         /* These are signed */
                    {
                        signed long v2;

                        if (subtype=='l')
                        {
#ifdef AROS_HAVE_LONG_LONG
                            if (lltype)
                            {
                                signed long long llv2;
                                
                                llv2 = va_arg(args, signed long long);
                                if (llv2 < 0)
                                {
                                    llv = - llv2;
                                    v2 = -1;	        /* Assign a dummy value to v2 in order to process sign below */
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
                        else if (subtype == 'z')
                            v2 = va_arg(args, size_t);
                        else
                            v2 = va_arg(args, signed int);

                        if (v2 < 0)
                        {
                            buffer1[size1++] = '-';
                            v = -v2;
                        }
                        else
                        {
                            if (flags & SIGNFLAG)
                                buffer1[size1++] = '+';
                            else if (flags & BLANKFLAG)
                                buffer1[size1++] = ' ';
                            v = v2;
                        }
                    }
                    else 		                /* These are unsigned */
                    {
                        if (subtype=='l')
                        {
#ifdef AROS_HAVE_LONG_LONG
                            if (lltype)
                                llv = va_arg(args, unsigned long long);
                            else
#endif
                                v = va_arg(args, unsigned long);
                        }
                        else if (subtype == 'z')
                            v = va_arg(args, size_t);
                        else
                            v = va_arg(args, unsigned int);

                        if (flags & ALTERNATEFLAG)
                        {
                            if (type == 'o' && preci && v)
                                buffer1[size1++] = '0';
                            if ((type == 'x' || type == 'X') && v)
                            {
                                buffer1[size1++] = '0';
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

            case 'c':
                    if (subtype=='l')
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

            case 's':
                    buffer2 = va_arg(args, char *);
                    if (!buffer2)
                        buffer2 = "(null)";
                    size2 = FMTPRINTF_STRLEN(buffer2);
                    size2 = size2 <= preci ? size2 : preci;
                    preci = 0;
                    break;

            case 'b':
                    buffer2 = BADDR(va_arg(args, BPTR));
                    if (buffer2)
#if AROS_FAST_BSTR
                        size2 = FMTPRINTF_STRLEN(buffer2);
#else
                        size2 = *(unsigned char *)buffer2++;
#endif
                    else
                    {
                        buffer2 = "(null)";
                        size2 = 6;
                    }

                    size2 = size2 <= preci ? size2 : preci;
                    preci = 0;
                    break;

#ifdef FULL_SPECIFIERS
            case 'a':
            case 'A':
            case 'f':
            case 'F':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
            {
                    char killzeros=0, sign=0;           /* some flags */
                    int ex1, ex2;                       /* Some temporary variables */
                    size_t size, dnum, dreq;
                    char *udstr = NULL;

                    if (subtype == 'L')
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
                            udstr = "+inf";
                        else
                            udstr = "-inf";
                    } else if (isnan(fparg.dbl))
                        udstr = "NaN";

                    if (udstr != NULL)
                    {
                        size2 = FMTPRINTF_STRLEN(udstr);
                        preci = 0;
                        buffer2 = udstr;
                        break; }

                    if (preci == ULONG_MAX)             /* old default */
                        preci = 6;                      /* new default */

                    if (((subtype != 'L') && (fparg.dbl < 0.0)) || ((subtype == 'L') && (fparg.ldbl < 0.0)))
                    {
                        sign = '-';
                        if (subtype == 'L')
                            fparg.ldbl = -fparg.ldbl;
                        else
                            fparg.dbl = -fparg.dbl;
                    } else {
                        if (flags & SIGNFLAG)
                            sign = '+';
                        else if (flags & BLANKFLAG)
                            sign = ' ';
                    }

                    ex1 = 0;
                    if (tolower(type) != 'a')
                    {
                        if (subtype != 'L')
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
                    if (tolower(type) == 'f')
                        ex2 += ex1;
                    if (tolower(type) == 'g')
                        ex2--;
                    if (subtype != 'L')
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

                    if (tolower(type) == 'g')           /* This changes to one of the other types */
                    {
                        if (ex1 < (signed long)preci && ex1 >= -4)
                        {
                            type = 'f';
                            preci -= ex1;
                        } else
                            type = type == 'g' ? 'e' : 'E';
                        preci--;
                        if (!(flags & ALTERNATEFLAG))
                            killzeros = 1;              /* set flag to kill trailing zeros */
                    }

                    dreq = preci + 1;                   /* Calculate number of decimal places required */
                    if (type == 'f')
                        dreq += ex1;	                /* even more before the decimal point */

                    dnum = 0;
                    while (dnum < dreq && dnum < MINFLOATSIZE) /* Calculate all decimal places needed */
                    {
                        if (subtype != 'L')
                        {
                            buffer[dnum++] = (char)fparg.dbl + '0';
                            fparg.dbl = (fparg.dbl - (double)(char)fparg.dbl) * 10.0;
                        } else {
                            buffer[dnum++] = (char)fparg.ldbl + '0';
                            fparg.ldbl = (fparg.ldbl - (long double)(char)fparg.ldbl) * 10.0;
                        }
                    }
                    if (killzeros)                      /* Kill trailing zeros if possible */
                        while(preci && (dreq-- > dnum || buffer[dreq] == '0'))
                            preci--;

                    if (tolower(type) == 'f')           /* Calculate actual size of string (without sign) */
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
                        FMTPRINTF_COUT(sign);

                    if (!(flags & LALIGNFLAG))
                        for (i = 0; i < pad; i++)
                            FMTPRINTF_COUT(flags & ZEROPADFLAG ? '0' : ' ');

                    if (sign && !(flags & ZEROPADFLAG))
                        FMTPRINTF_COUT(sign);

                    dreq = 0;
                    if (tolower(type) == 'a')
                    {
                        // TODO: Implement hexfloat literals
                        FMTPRINTF_COUT('0');
                        FMTPRINTF_COUT('x');
                        FMTPRINTF_COUT('0');
                        FMTPRINTF_COUT('.');
                        FMTPRINTF_COUT('0');
                        if (type=='A')
                            FMTPRINTF_COUT('P');
                        else
                            FMTPRINTF_COUT('p');
                        FMTPRINTF_COUT('0');
                    } else if (tolower(type) == 'f') {
                        if (ex1 < 0)
                            FMTPRINTF_COUT('0');
                        else
                            while(ex1 >= 0)
                            {
                                FMTPRINTF_COUT(dreq < dnum ? buffer[dreq++] : '0');
                                ex1--;
                            }
                        if (preci || flags & ALTERNATEFLAG)
                        {
                            FMTPRINTF_COUT(FMTPRINTF_DECIMALPOINT[0]);
                            while(preci--)
                                if (++ex1 < 0)
                                    FMTPRINTF_COUT('0');
                                else
                                    FMTPRINTF_COUT(dreq < dnum ? buffer[dreq++] : '0');
                        }
                    } else {
                        FMTPRINTF_COUT(buffer[dreq++]);
                        if (preci || flags & ALTERNATEFLAG)
                        {
                            FMTPRINTF_COUT(FMTPRINTF_DECIMALPOINT[0]);
                            while(preci--)
                                FMTPRINTF_COUT(dreq < dnum ? buffer[dreq++] : '0');
                        }
                        FMTPRINTF_COUT(type);
                        if (ex1 < 0)
                        {
                            FMTPRINTF_COUT('-');
                            ex1 = -ex1;
                        }
                        else
                            FMTPRINTF_COUT('+');
                        if (ex1 > 99)
                            FMTPRINTF_COUT(ex1 / 100 + '0');
                        FMTPRINTF_COUT(ex1 / 10 % 10 + '0');
                        FMTPRINTF_COUT(ex1 % 10 + '0');
                    }

                    if (flags & LALIGNFLAG)
                        for (i = 0; i < pad; i++)
                            FMTPRINTF_COUT(' ');

                    width = preci = 0;                  /* Everything already done */
                    break;
                }
#endif
                case '%':
                    buffer2 = "%";
                    size2 = 1;
                    preci = 0;
                    break;
                case 'n':
                    *va_arg(args, int *) = outcount;
                    width = preci = 0;
                    break;
                default:
                    if (!type)
                        ptr--;                          /* We've gone too far - step one back */
                    buffer2 = (char *)format;
                    size2 = ptr - format;
                    width = preci = 0;
                    break;
            }
            pad = size1 + (size2 >= preci ? size2 : preci); /* Calculate the number of characters */
            pad = pad >= width ? 0 : width - pad;       /* and the number of resulting pad bytes */

            if (flags & ZEROPADFLAG)                    /* print sign and that like */
                for (i = 0; i < size1; i++)
                    FMTPRINTF_COUT(buffer1[i]);

            if (!(flags & LALIGNFLAG))                  /* Pad left */
                for (i = 0; i < pad; i++)
                    FMTPRINTF_COUT(flags & ZEROPADFLAG ? '0' : ' ');

            if (!(flags & ZEROPADFLAG))                 /* print sign if not zero padded */
                for (i = 0; i < size1; i++)
                    FMTPRINTF_COUT(buffer1[i]);

            for (i = size2; i < preci; i++)             /* extend to precision */
                FMTPRINTF_COUT('0');

            for (i = 0; i < size2; i++)                 /* print body */
                FMTPRINTF_COUT(buffer2[i]);

            if (flags & LALIGNFLAG)                     /* Pad right */
                for (i = 0; i < pad; i++)
                    FMTPRINTF_COUT(' ');

            format = ptr;
        }
        else
            FMTPRINTF_COUT(*format++);
    }
