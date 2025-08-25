/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    C99 function strtok().
*/

#if !defined(STDC_STATIC)
#include "__stdc_intbase.h"
#endif
#include <aros/symbolsets.h>

#if defined(STDC_STATIC)
static char *__strlst = NULL;
#endif

/*****************************************************************************

    NAME */
#include <string.h>

        char * strtok (

/*  SYNOPSIS */
        char       * str,
        const char * sep)

/*  FUNCTION
        Separates a string by the characters in sep.

    INPUTS
        str - The string to check or NULL if the next word in
                the last string is to be searched.
        sep - Characters which separate "words" in str.

    RESULT
        The first word in str or the next one if str is NULL.

    NOTES
        The function changes str !

    EXAMPLE
        char buffer[64];

        strcpy (buffer, "Hello, this is a test.");

        // Init. Returns "Hello"
        strtok (str, " \t,.");

        // Next word. Returns "this"
        strtok (NULL, " \t,.");

        // Next word. Returns "is"
        strtok (NULL, " \t");

        // Next word. Returns "a"
        strtok (NULL, " \t");

        // Next word. Returns "test."
        strtok (NULL, " \t");

        // Next word. Returns NULL.
        strtok (NULL, " \t");

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
#if !defined(STDC_STATIC)
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();
    char **strlst = &StdCBase->last;
#else
    char **strlst = &__strlst;
#endif

    if (str != NULL)
        *strlst = str;
    else
        str = *strlst;

    str += strspn (str, sep);

    if (*str == '\0')
        return NULL;

    *strlst = str;

    *strlst += strcspn (str, sep);

    if ((*strlst)[0] != '\0') {
        (*strlst)[0] = '\0';
        *strlst += 1;
    }

    return str;
} /* strtok */

#if !defined(STDC_STATIC)
static int __strtok_init(struct StdCIntBase *StdCBase)
{
    StdCBase->last = NULL;

    return 1;
}

ADD2OPENLIB(__strtok_init, 0);
#endif
