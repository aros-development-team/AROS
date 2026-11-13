/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function fnmatch().
*/

#include <ctype.h>
#include <string.h>
#include <fnmatch.h>

/* Is 'p' at a position where a period must be matched explicitly? */
static int leading_period(const char *string, const char *start, int flags)
{
    if (*string != '.' || !(flags & FNM_PERIOD))
        return 0;
    if (string == start)
        return 1;
    return (flags & FNM_PATHNAME) && string[-1] == '/';
}

static int fold(int c, int flags)
{
    return (flags & FNM_CASEFOLD) ? tolower((unsigned char)c) : (unsigned char)c;
}

/*
 * Match a [:class:] expression. On success *endp is set past the closing
 * ":]" and the result of the test is returned, otherwise -1.
 */
static int charclass(const char *p, char test, const char **endp)
{
    static const struct
    {
        const char *name;
        int (*test)(int);
    } classes[] =
    {
        { "alnum",  isalnum  }, { "alpha",  isalpha  }, { "blank",  isblank  },
        { "cntrl",  iscntrl  }, { "digit",  isdigit  }, { "graph",  isgraph  },
        { "lower",  islower  }, { "print",  isprint  }, { "punct",  ispunct  },
        { "space",  isspace  }, { "upper",  isupper  }, { "xdigit", isxdigit }
    };
    const char *end;
    unsigned int i;
    size_t len;

    end = strstr(p, ":]");
    if (end == NULL)
        return -1;
    len = (size_t)(end - p);

    for (i = 0; i < sizeof(classes) / sizeof(classes[0]); i++)
    {
        if (strlen(classes[i].name) == len &&
            strncmp(classes[i].name, p, len) == 0)
        {
            *endp = end + 2;
            return classes[i].test((unsigned char)test) != 0;
        }
    }
    return -1;
}

/*
 * Match a [...] bracket expression against 'test'. On a match, *endp is set
 * past the closing bracket. Returns 1 on match, 0 on mismatch and -1 if the
 * expression is unterminated, in which case '[' is an ordinary character.
 */
static int bracket(const char *pattern, char test, int flags, const char **endp)
{
    const char *p = pattern;
    int negate = 0, matched = 0;
    int c, last;

    if (*p == '!' || *p == '^')
    {
        negate = 1;
        p++;
    }

    /* A ']' right after the (negated) opening bracket is a literal */
    for (last = -1; (c = (unsigned char)*p) != '\0'; p++)
    {
        if (c == ']' && p != pattern + negate)
            break;

        /* POSIX character class, e.g. [:alpha:] */
        if (c == '[' && p[1] == ':')
        {
            const char *after;
            int ret = charclass(p + 2, test, &after);

            if (ret >= 0)
            {
                if (ret)
                    matched = 1;
                p = after - 1;
                continue;
            }
        }

        if (c == '\\' && !(flags & FNM_NOESCAPE) && p[1] != '\0')
            c = (unsigned char)*++p;

        /* A range, unless the '-' is last before the closing bracket */
        if (p[1] == '-' && p[2] != ']' && p[2] != '\0')
        {
            int hi;

            p += 2;
            hi = (unsigned char)*p;
            if (hi == '\\' && !(flags & FNM_NOESCAPE) && p[1] != '\0')
                hi = (unsigned char)*++p;
            if (fold(c, flags) <= fold(test, flags) &&
                fold(test, flags) <= fold(hi, flags))
                matched = 1;
        }
        else if (fold(c, flags) == fold(test, flags))
            matched = 1;

        last = c;
    }
    (void)last;

    if (c != ']')
        return -1;

    *endp = p + 1;
    return negate ? !matched : matched;
}

/*****************************************************************************

    NAME */
#include <fnmatch.h>

        int fnmatch (

/*  SYNOPSIS */
        const char * pattern,
        const char * string,
        int          flags
        )

/*  FUNCTION
        Matches 'string' against the shell wildcard pattern 'pattern'.

    INPUTS
        pattern - The pattern, which may contain '*', '?' and [...].
        string  - The string to match against the pattern.
        flags   - A combination of FNM_NOESCAPE, FNM_PATHNAME, FNM_PERIOD,
                  FNM_LEADING_DIR and FNM_CASEFOLD.

    RESULT
        0 if the string matches, FNM_NOMATCH if it does not.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        Backtracks by recursing on the remainder of the pattern whenever a
        '*' is encountered.

******************************************************************************/
{
    const char *start = string;
    int c;

    for (;;)
    {
        switch (c = (unsigned char)*pattern++)
        {
        case '\0':
            if ((flags & FNM_LEADING_DIR) && *string == '/')
                return 0;
            return *string == '\0' ? 0 : FNM_NOMATCH;

        case '?':
            if (*string == '\0')
                return FNM_NOMATCH;
            if (*string == '/' && (flags & FNM_PATHNAME))
                return FNM_NOMATCH;
            if (leading_period(string, start, flags))
                return FNM_NOMATCH;
            string++;
            break;

        case '*':
            /* Collapse a run of '*' into one */
            while (*pattern == '*')
                pattern++;

            if (leading_period(string, start, flags))
                return FNM_NOMATCH;

            /* A trailing '*' matches the rest, up to '/' under FNM_PATHNAME */
            if (*pattern == '\0')
            {
                if (!(flags & FNM_PATHNAME) || (flags & FNM_LEADING_DIR))
                    return 0;
                return strchr(string, '/') == NULL ? 0 : FNM_NOMATCH;
            }

            for (;;)
            {
                if (fnmatch(pattern, string, flags & ~FNM_PERIOD) == 0)
                    return 0;
                if (*string == '\0')
                    return FNM_NOMATCH;
                if (*string == '/' && (flags & FNM_PATHNAME))
                    return FNM_NOMATCH;
                string++;
            }
            /* not reached */

        case '[':
        {
            const char *end;
            int ret;

            if (*string == '\0')
                return FNM_NOMATCH;
            if (*string == '/' && (flags & FNM_PATHNAME))
                return FNM_NOMATCH;
            if (leading_period(string, start, flags))
                return FNM_NOMATCH;

            ret = bracket(pattern, *string, flags, &end);
            if (ret < 0)
            {
                /* Unterminated - '[' matches itself */
                if (*string != '[')
                    return FNM_NOMATCH;
                string++;
                break;
            }
            if (ret == 0)
                return FNM_NOMATCH;
            pattern = end;
            string++;
            break;
        }

        case '\\':
            if (!(flags & FNM_NOESCAPE))
            {
                /* A pattern ending in an escape matches nothing */
                if (*pattern == '\0')
                    return FNM_NOMATCH;
                c = (unsigned char)*pattern++;
            }
            /* fall through */

        default:
            if (fold(c, flags) != fold(*string, flags))
                return FNM_NOMATCH;
            string++;
            break;
        }
    }
}
