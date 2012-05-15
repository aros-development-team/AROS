
/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/rdargs.h>
#include <dos/dosextens.h>
#include <proto/utility.h>

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
#    undef AROS_LIBFUNC_EXIT
#    define AROS_LIBFUNC_EXIT
#endif

/* Fix the end-of-buffer ReadItem() 'well known' bug
 * where it "ungets" non-terminator (' ','=','\t','\n')
 * characters at the end of a non-\n terminated buffer.
 */
#define READITEM(buff, bufflen, cs) \
    ({ LONG ret = ReadItem(buff, bufflen, cs); \
       if (ret == ITEM_UNQUOTED && (cs->CS_CurChr+1) == cs->CS_Length) \
         cs->CS_CurChr++; \
       ret; \
     })

/* Returns 0 if this is not a '?' line, otherwise
 * returns the length between the '?' and the '\n'
 */

static inline LONG is_question(BYTE * buff, LONG buffsize)
{
    LONG i, j = 0;
    BOOL escaped       = FALSE,
         quoted        = FALSE,
         seen_space    = FALSE,
         seen_question = FALSE;

    /* Reach end of line */
    for (i = 0; i < buffsize; i++)
    {
        /* Only spaces and return are allowed at the end of the line after the
         * question mark for it to lead to reprompting. BTW, AmigaOS allowed
         * only one space then... but do we need to be _that_ compatible?
         */
        switch (buff[i])
        {
        case ' ':
        case '\t':
        case '\n':
            seen_space = TRUE;
            break;
        case '?':
            break;
        default:
            seen_question = seen_space = FALSE;
        }

        switch (buff[i])
        {
        case '*':
            if (quoted)
                escaped = !escaped;
            break;
        case '"':
            if (!escaped)
                quoted = !quoted;
            break;
        case '?':
            if (quoted)
                escaped = FALSE;
            else if (seen_space)
            {
                seen_question = TRUE;
                j = i;
            }
            break;
        case ' ':
        case '\t':
            escaped = FALSE;
            break;
        case EOF:
        case '\n':
            if (seen_question)
                return (i + 1 - j);
            else
                return 0;
        default:
            escaped = seen_space = FALSE;
        }
    }
    return 0;
}

/*****************************************************************************

    NAME */
#include <proto/dos.h>

AROS_LH3(struct RDArgs *, ReadArgs,

/*  SYNOPSIS */
         AROS_LHA(CONST_STRPTR, template, D1),
         AROS_LHA(IPTR *, array, D2),
         AROS_LHA(struct RDArgs *, rdargs, D3),

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
                      if it was really given. The number is always of type
                      LONG.
                   /A Argument is required. If it is left out ReadArgs()
                      fails.
                   /K The keyword must be given when filling the option.
                      Normally it's skipped.
                   /M Multiple strings or, when used in combination with /N,
                      numbers. The result is returned as an array of pointers
                      to strings or LONGs, and is terminated with NULL. /M
                      eats all strings that don't fit into any other option.
                      If there are unfilled /A arguments after parsing they
                      steal strings from /M. This makes it possible to, for
                      example, write a Copy command template like
                      'FROM/A/M,TO/A'. There may be only one /M option in a
                      template.
                   /F Eats the rest of the line even if there are option
                      keywords in it.
        array         - Array to be filled with the result values. The array must
                   be intialized to the default values before calling
                   ReadArgs().
        rdargs         - An optional RDArgs structure determinating the type of
                   input to process.

    RESULT
        A handle for the memory allocated by ReadArgs(). Must be freed
        with FreeArgs() later.

    SEE ALSO
        FreeArgs(), Input()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Allocated resources */
    struct DAList *dalist = NULL;
    UBYTE *flags = NULL;
    STRPTR strbuf = NULL, iline = NULL;
    STRPTR *multvec = NULL, *argbuf = NULL;
    CONST_STRPTR numstr;
    ULONG multnum = 0, multmax = 0;
    LONG strbuflen;

    /* Some variables */
    CONST_STRPTR cs1;
    STRPTR s1, s2, *newmult;
    ULONG arg, numargs, nextarg;
    LONG it, item, chars, delthis;
    struct CSource lcs, *cs;
    BOOL is_file_not_buffer;
    TEXT argbuff[256 + 1];        /* Maximum BCPL string length + injected \n + ASCIIZ */

    ASSERT_VALID_PTR(template);
    ASSERT_VALID_PTR(array);
    ASSERT_VALID_PTR_OR_NULL(rdargs);

    D(bug("[ReadArgs] Template: \"%s\"\n", template));
    /* Get pointer to process structure. */
    struct Process *me = (struct Process *) FindTask(NULL);

    ASSERT_VALID_PROCESS(me);

    /* Error recovery. C has no exceptions. This is a simple replacement. */
    LONG error;

#undef ERROR
#define ERROR(a) { error=a; goto end; }

    /* Template options */
#define REQUIRED 0x80           /* /A */
#define KEYWORD  0x40           /* /K */
#define MULTIPLE 0x20           /* /M */
#define TYPEMASK 0x07
#define NORMAL         0x00           /* No option */
#define SWITCH         0x01           /* /S, implies /K */
#define TOGGLE         0x02           /* /T, implies /K */
#define NUMERIC  0x03           /* /N */
#define REST         0x04           /* /F */

    /* Flags for each possible character. */
    static const UBYTE argflags[] =
    {
        REQUIRED, 0, 0, 0, 0, REST, 0, 0, 0, 0, KEYWORD, 0, MULTIPLE,
        NUMERIC, 0, 0, 0, 0, SWITCH | KEYWORD, TOGGLE | KEYWORD, 0, 0,
        0, 0, 0, 0
    };

    /* Allocate readargs structure (and private internal one) */
    if (!rdargs)
    {
        rdargs = (struct RDArgs *) AllocVec(sizeof(struct RDArgs),
                                       MEMF_ANY | MEMF_CLEAR);
        if (rdargs)
        {
            rdargs->RDA_Flags |= RDAF_ALLOCATED_BY_READARGS;
        }
    }

    dalist = (struct DAList *) AllocVec(sizeof(struct DAList),
                                   MEMF_ANY | MEMF_CLEAR);

    if (rdargs == NULL || dalist == NULL)
    {
        ERROR(ERROR_NO_FREE_STORE);
    }

    /* Init character source. */
    if (rdargs->RDA_Source.CS_Buffer)
    {
        cs = &rdargs->RDA_Source;
        D(bug("[ReadArgs] Buffer: \"%s\"\n", cs->CS_Buffer));
        is_file_not_buffer = FALSE;
    }
    else
    {
        BOOL notempty = TRUE;
        BPTR input = Input();

        D(bug("[ReadArgs] Input: 0x%p\n", input));
        is_file_not_buffer = TRUE;

        /*
         * Take arguments from input stream. They were injected there by either
         * runcommand.c or createnewproc.c (see vbuf_inject() routine).
         * This is described in Guru Book.
         */
        argbuff[0] = 0;
        lcs.CS_Buffer = &argbuff[0];

        /*
         * Special kludge for interactive filehandles (i. e. CLI windows).
         * Read data only if filehandle's buffer is not empty. Otherwise
         * read will cause opening CLI window and waiting for user's input.
         * As a consequence we still can use ReadArgs() on input redirected
         * from a file, even if we are started from Workbench (hypothetical
         * situation).
         * This prevents opening a CLI window if the program was started from
         * Workbench and redirected its Input() and Output() to own window,
         * but still called ReadArgs() after redirection for some reason.
         * Streams redirection is widely used in AROS startup code.
         */
        if (IsInteractive(input))
        {
            struct FileHandle *fh = BADDR(input);

            notempty = (fh->fh_Pos != fh->fh_End);
        }

        if (notempty)
            FGets(input, lcs.CS_Buffer, sizeof(argbuff));

        D(bug("[ReadArgs] Line: %s\n", argbuff));

        cs1 = lcs.CS_Buffer;

        for (; *cs1 != '\0'; ++cs1);

        lcs.CS_Length = cs1 - lcs.CS_Buffer;
        lcs.CS_CurChr = 0;

        cs = &lcs;
    }

    /* Check for optional reprompting */
    if (!(rdargs->RDA_Flags & RDAF_NOPROMPT))
    {
        if ((delthis = is_question(cs->CS_Buffer, cs->CS_Length)))
        {
            /* '?' was found on the commandline. */
            BPTR input = Input();
            BPTR output = Output();
            ULONG isize = 0, ibuf = 0;
            LONG c;
            ULONG helpdisplayed = FALSE;

            /* Prompt for more input */

            D(bug("[ReadArgs] '?' found, %d chars to be removed\n", delthis));
            D(bug("[ReadArgs] rdargs=0x%p\n", rdargs));
            D(if (rdargs) bug ("[ReadArds] rdargs->RDA_ExtHelp=0x%p\n", rdargs->RDA_ExtHelp);)

            if (FPuts(output, template) || FPuts(output, ": "))
            {
                ERROR(me->pr_Result2);
            }

            if (!Flush(output))
            {
                ERROR(me->pr_Result2);
            }

            cs->CS_Length -= delthis;
            isize = ibuf = cs->CS_Length;
            iline = (STRPTR) AllocVec(ibuf, MEMF_ANY);
            CopyMemQuick(cs->CS_Buffer, iline, isize);

            do
            {
                /* Read a line in. */
                c  = -1;
                for (;;)
                {
                    if (c == '\n')
                    {
                        iline[isize] = '\0'; /* end of string */
                        break;
                    }
                    if (isize >= ibuf)
                    {
                        /* Buffer too small. Get a new one. */
                        STRPTR newiline;

                        ibuf += 256;

                        newiline = (STRPTR) AllocVec(ibuf, MEMF_ANY);
                        if (newiline == NULL)
                        {
                            ERROR(ERROR_NO_FREE_STORE);
                        }

                        if (iline != NULL)
                            CopyMemQuick(iline, newiline, isize);

                        FreeVec(iline);

                        iline = newiline;
                    }

                    /* Read character */
                    if (is_file_not_buffer)
                    {
                        c = FGetC(input);
                    }
                    else
                    {
                        SetIoErr(0);

                        if (cs->CS_CurChr >= cs->CS_Length)
                            c = EOF;
                        else
                            c = cs->CS_Buffer[cs->CS_CurChr++];
                    }

                    /* Check and write it. */
                    if (c == EOF && me->pr_Result2)
                    {
                        ERROR(me->pr_Result2);
                    }
                
                    /* Fix short buffers to have a trailing '\n' */
                    if (c == EOF || c == '\0')
                        c = '\n';

                    iline[isize++] = c;
                }
                iline[isize] = '\0'; /* end of string */

                D(iline[isize] = 0; bug("[ReadArgs] Size %d, line: '%s'\n", isize, iline));

                /* if user entered single ? again or some string ending
                   with space and ? either display template again or
                   extended help if it's available */
                if ((delthis = is_question(iline, isize)))
                {
                    helpdisplayed = TRUE;
                    isize -= delthis;
                    if(rdargs->RDA_ExtHelp != NULL)
                    {
                        if (FPuts(output, rdargs->RDA_ExtHelp) || FPuts(output, ": "))
                            ERROR(me->pr_Result2);
                    }
                    else if (FPuts(output, template) || FPuts(output, ": "))
                    {
                        ERROR(me->pr_Result2);
                    }

                    if (!Flush(output))
                    {
                        ERROR(me->pr_Result2);
                    }
                }
                else
                    helpdisplayed = FALSE;
            } while(helpdisplayed);

            /* Prepare input source for new line. */
            cs->CS_Buffer = iline;
            cs->CS_Length = isize;
            cs->CS_CurChr = 0;
        }
    }

    /*
     * Get enough space for string buffer.
     * It's always smaller than the size of the input line+1.
     */

    strbuflen = cs->CS_Length + 1;
    strbuf = (STRPTR) AllocVec(strbuflen, MEMF_ANY);

    if (strbuf == NULL)
    {
        ERROR(ERROR_NO_FREE_STORE);
    }
    
    /* Count the number of items in the template (number of ','+1). */
    numargs = 1;
    cs1 = template;

    while (*cs1)
    {
        if (*cs1++ == ',')
        {
            numargs++;
        }
    }

    /* Use this count to get space for temporary flag array and result
     * buffer. */
    flags = (UBYTE *) AllocVec(numargs + 1, MEMF_CLEAR);

    argbuf = (STRPTR *) AllocVec((numargs + 1) * sizeof(STRPTR), MEMF_CLEAR);

    if (flags == NULL || argbuf == NULL)
    {
        ERROR(ERROR_NO_FREE_STORE);
    }

    /* Fill the flag array. */
    cs1 = template;
    s2 = flags;

    while (*cs1)
    {
        /* A ',' means: goto next item. */
        if (*cs1 == ',')
        {
            s2++;
        }
        
        /* In case of a '/' use the next character as option. */
        if (*cs1++ == '/')
        {
            UBYTE argc = ToUpper(*cs1);
            if (argc >= 'A' && argc <= 'Z')
                    *s2 |= argflags[argc - 'A'];
        }
    }

    /* Add a dummy so that the whole line is processed (see below). */
    *++s2 = MULTIPLE;

    /*
     * Now process commandline for the first time:
     * Go from left to right and fill all items that need filling.
     * If an item is given as 'OPTION=VALUE' or 'OPTION VALUE' fill
     * it out of turn.
     * NOTE: '<=' comparison is intentional here. When we allocated argbuf, we added one
     * to the number of arguments. And right above we added fictional MULTIPLE flag.
     * This is actually needed to make /S and /K working.
     */
    s1 = strbuf;

    for (arg = 0; arg <= numargs ; arg = nextarg)
    {
        nextarg = arg + 1;

        D(bug("[ReadArgs] Arg %d (0x%x) s1=&strbuf[%d], %d left\n", arg, flags[arg], s1-strbuf, strbuflen));

        /* Out of buffer space?
         * This should not have happened, some internal logic
         * must have broken.
         */
        if (strbuflen == 0) {
            D(bug("[ReadArgs] %d: INTERNAL ERROR: Ran out of buffer space.\n", arg));
            break;
        }

        /* Skip /K options and options that are already done. */
        if (flags[arg] & KEYWORD || argbuf[arg] != NULL)
        {
            continue;
        }

    #if 0 /* stegerg: if so a template of CLOSE/S,QUICK/S,COMMAND/F would
                      not work correctly if command line for example is
                      "CLOSE QUICK" it would all end up being eaten by COMMAND/F
                      argument */
                      
        /* If the current option is of type /F do not look for keywords */
        if ((flags[arg] & TYPEMASK) != REST)
    #endif
            
        {
            /* Get item. Quoted items are never keywords. */
            it = READITEM(s1, strbuflen, cs);
            D(bug("[ReadArgs] Item %s type %d\n", s1, it));

            if (it == ITEM_UNQUOTED)
            {
                /* Not quoted. Check if it's a keyword. */
                item = FindArg(template, s1);

                if (item >= 0 && item < numargs && argbuf[item] == NULL)
                {
                    D(bug("[ReadArgs] %d: Keyword \"%s\" (%d)\n", arg, s1, item));
                    /*
                     * It's a keyword. Fill it and retry the current option
                     * at the next turn
                     */
                    nextarg = arg;
                    arg = item;

                    /* /S /T may not be given as 'OPTION=VALUE'. */
                    if ((flags[item] & TYPEMASK) != SWITCH
                        && (flags[item] & TYPEMASK) != TOGGLE)
                    {
                        /* Get value. */
                        it = READITEM(s1, strbuflen, cs);

                        if (it == ITEM_EQUAL)
                        {
                            it = READITEM(s1, strbuflen, cs);
                        } else if (it != ITEM_QUOTED && it != ITEM_UNQUOTED) {
                            ERROR(ERROR_KEY_NEEDS_ARG);
                        }
                    }
                }
            }

            /* Check returncode of ReadItem(). */
            if (it == ITEM_EQUAL)
            {
                ERROR(ERROR_BAD_TEMPLATE);
            }
            else if (it == ITEM_ERROR)
            {
                ERROR(me->pr_Result2);
            }
            else if (it == ITEM_NOTHING)
            {
                break;
            }
        }

        /* /F takes all the rest, including extra spaces, =, and '"'
         * NOTE: If the item was quoted, this will strip off the
         *       next '"' mark it sees.
         */
        if ((flags[arg] & TYPEMASK) == REST)
        {
            BOOL eat_quote = (it == ITEM_QUOTED) ? TRUE : FALSE;
            argbuf[arg] = s1;

            /* Skip past what ReadItem() just read.
             */
            while (*s1 && strbuflen > 0) {
                s1++;
                strbuflen--;
            }

            /*
             * Put the rest into the buffer, including the separator
             * ReadItem() actually ungets '\n' terminator. So if CurChr points to it,
             * we don't need to adjust it. Otherwise we duplicate last character of arguments line.
             */
            if (cs->CS_Buffer[cs->CS_CurChr] != '\n')
                cs->CS_CurChr--;
            s2 = &cs->CS_Buffer[cs->CS_CurChr];
           
            while (cs->CS_CurChr < cs->CS_Length && 
                   strbuflen > 1 &&
                   *s2 &&
                   *s2 != '\n')
            {
                cs->CS_CurChr++;

                if (eat_quote && *s2 == '"')
                {
                    s2++;
                    eat_quote = FALSE;
                    continue;
                }

                *(s1++) = *(s2++);
                strbuflen--;
            }

            *(s1++) = 0;
            strbuflen--;
            D(bug("[ReadArgs] /F copy: \"%s\" left=%d, CS_CurChr=%d, CS_Length=%d\n", argbuf[arg], strbuflen, cs->CS_CurChr, cs->CS_Length));
            it = ITEM_NOTHING;
            break;
        }

        if (flags[arg] & MULTIPLE)
        {
            /* All /M arguments are stored in a buffer. */
            if (multnum >= multmax)
            {
                /* Buffer too small. Get a new one. */
                multmax += 16;

                newmult = (STRPTR *) AllocVec(multmax * sizeof(char *),
                                        MEMF_ANY);
                if (newmult == NULL)
                {
                    ERROR(ERROR_NO_FREE_STORE);
                }

                CopyMemQuick((ULONG *) multvec, (ULONG *) newmult,
                             multnum * sizeof(char *));

                FreeVec(multvec);

                multvec = newmult;
            }

            /* Put string into the buffer. */
            multvec[multnum++] = s1;

            D(bug("[ReadArgs] %d: Multiple +\"%s\"\n", arg, s1));
            while (*s1++)
                --strbuflen;
            /* Account for the \000 at the end. */
            --strbuflen;

            /* /M takes more than one argument, so retry. */
            nextarg = arg;
        }
        else if ((flags[arg] & TYPEMASK) == SWITCH
                 || (flags[arg] & TYPEMASK) == TOGGLE)
        {
            /* /S or /T just set a flag */
            argbuf[arg] = (char *) ~0;
            D(bug("[ReadArgs] %d: Toggle\n", arg));
        }
        else                    /* NORMAL || NUMERIC */
        {
            /* Put argument into argument buffer. */
            argbuf[arg] = s1;
            D(bug("[ReadArgs] %d: Normal: \"%s\"\n", arg, s1));

            while (*s1++)
                --strbuflen;
            /* Account for the \000 at the end. */
            --strbuflen;
        }

        if (cs->CS_CurChr >= cs->CS_Length)
            break; /* end of input */
    }

    /* Unfilled /A options steal Arguments from /M */
    for (arg = numargs; arg-- > 0;)
    {
        if (flags[arg] & REQUIRED && argbuf[arg] == NULL
            && !(flags[arg] & MULTIPLE))
        {
            if (flags[arg] & KEYWORD)
            {
                /* /K/A argument, which insists on keyword
                 * being used, cannot be satisfied */

                ERROR(ERROR_TOO_MANY_ARGS); /* yes, strange error number,
                                             * but it translates to "wrong
                                             * number of arguments" */

            }

            if (multnum == 0)
            {
                /* No arguments left? Oh dear! */
                ERROR(ERROR_REQUIRED_ARG_MISSING);
            }

            argbuf[arg] = multvec[--multnum];
        }
    }

    /* Put the rest of /M where it belongs */
    for (arg = 0; arg < numargs; arg++)
    {
        if (flags[arg] & MULTIPLE)
        {
            if (flags[arg] & REQUIRED && multnum == 0)
            {
                ERROR(ERROR_REQUIRED_ARG_MISSING);
            }

            if (multnum != 0)
            {
                /* NULL terminate it. */
                if (multnum >= multmax)
                {
                    multmax += 16;

                    newmult = (STRPTR *) AllocVec(multmax * sizeof(STRPTR),
                                            MEMF_ANY);

                    if (newmult == NULL)
                    {
                        ERROR(ERROR_NO_FREE_STORE);
                    }

                    CopyMemQuick((ULONG *) multvec, (ULONG *) newmult,
                                 multnum * sizeof(char *));

                    FreeVec(multvec);

                    multvec = newmult;
                }

                multvec[multnum++] = NULL;
                argbuf[arg] = (STRPTR) multvec;
            }
            else
            {
/* Shouldn't be necessary, but some buggy software relies on this */
                argbuf[arg] = NULL;
            }

            break;
        }
    }

    /* There are some arguments left? Return error. */
    if (multnum != 0 && arg == numargs)
    {
        ERROR(ERROR_TOO_MANY_ARGS);
    }

    /*
     * The commandline is processed now. Put the results in the result array.
     * Convert /N arguments by the way.
     */
    for (arg = 0; arg < numargs; arg++)
    {
        /* Just for the arguments given. */
        if (argbuf[arg] != NULL)
        {
            if (flags[arg] & MULTIPLE)
            {
                array[arg] = (IPTR) argbuf[arg];

                if ((flags[arg] & TYPEMASK) == NUMERIC)
                {
                    STRPTR *p;
                    LONG *q;

                    if (multnum * 2 > multmax)
                    {
                        multmax = multnum * 2;
                        newmult = (STRPTR *) AllocVec(multmax * sizeof(STRPTR),
                                                MEMF_ANY);

                        if (newmult == NULL)
                        {
                            ERROR(ERROR_NO_FREE_STORE);
                        }

                        CopyMemQuick((ULONG *) multvec, (ULONG *) newmult,
                            multnum * sizeof(char *));

                        FreeVec(multvec);

                        multvec = newmult;
                    }

                    array[arg] = (IPTR) multvec;
                    p = multvec;
                    q = (LONG *) (multvec + multnum);

                    while (*p)
                    {
                        /* Convert /N argument. */
                        chars = StrToLong(*p, q);

                        if (chars <= 0 || (*p)[chars])
                        {
                            /* Conversion failed. */
                            ERROR(ERROR_BAD_NUMBER);
                        }

                        /* Put the result where it belongs. */
                        *p = (STRPTR) q;
                        p++;
                        q += sizeof(IPTR) / sizeof(LONG);
                    }
                }
            }
            else
            {
                switch (flags[arg] & TYPEMASK)
                {
                    case NORMAL:
                    case REST:
                    case SWITCH:
                        /* Simple arguments are just copied. */
                        array[arg] = (IPTR) argbuf[arg];
                        break;

                    case TOGGLE:
                        /* /T logically inverts the argument. */
                        array[arg] = array[arg] ? 0 : ~0;
                        break;

                    case NUMERIC:
                        /* Convert /N argument. */
                        /* Abuse the argbuf buffer. It's not needed anymore. */
                        numstr = (CONST_STRPTR)argbuf[arg];
                        chars = StrToLong(numstr, (LONG *)&argbuf[arg]);

                        if (chars <= 0 || numstr[chars] != '\0')
                        {
                            /* Conversion failed. */
                            ERROR(ERROR_BAD_NUMBER);
                        }

                        /* Put the result where it belongs. */
                        array[arg] = (IPTR) &argbuf[arg];
                        break;
                }
            }
        }
        else
        {
            if (flags[arg] & MULTIPLE)
            {
                /* Shouldn't be necessary, but some buggy software relies on this.
                 * IBrowse's URL field isn't set to zero.
                 */
                array[arg] = (IPTR)NULL;
            }
        }
    }

    /* All OK. */
    error = 0;
end:
    /* Cleanup and return. */
    FreeVec(iline);
    FreeVec(flags);

    if (error)
    {
        /* ReadArgs() failed. Clean everything up. */
        if (rdargs)
        {
            if (rdargs->RDA_Flags & RDAF_ALLOCATED_BY_READARGS)
            {
                FreeVec(rdargs);
            }
        }

        FreeVec(dalist);
        FreeVec(argbuf);
        FreeVec(strbuf);
        FreeVec(multvec);

        me->pr_Result2 = error;

        return NULL;
    }
    else
    {
        /* All went well. Prepare result and return. */
        rdargs->RDA_DAList = (IPTR) dalist;
        dalist->ArgBuf = argbuf;
        dalist->StrBuf = strbuf;
        dalist->MultVec = multvec;
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

int
main(int argc, char **argv)
{
    struct RDArgs *rdargs;

    if ((rdargs = AllocDosObject(DOS_RDARGS, NULL)))
    {
        rdargs->RDA_ExtHelp = usage;    /* FIX: why doesn't this work? */

        if (!(ReadArgs(cmlargs, cmlvec, rdargs)))
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
}                               /* main */

#endif /* TEST */
