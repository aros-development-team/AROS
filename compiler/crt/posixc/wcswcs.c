/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function wcswcs.
*/


/*****************************************************************************

    NAME */
#include <wchar.h>

 wchar_t * wcswcs(

/*  SYNOPSIS */
      const wchar_t *haystack, const wchar_t *needle)
 
/*  FUNCTION
        Locates the first occurrence of the wide-character string 'needle'
        in the wide-character string 'haystack'. Returns a pointer to the
        start of the located substring, or NULL if not found.
 
    INPUTS
        haystack -- wide-character string to be searched.
        needle   -- wide-character string to search for.
 
    RESULT
        Pointer to the first occurrence of 'needle' in 'haystack',
        or NULL if no match is found.
 
    NOTES
        If 'needle' is an empty string, 'haystack' is returned.
 
    EXAMPLE

    BUGS

    SEE ALSO
        wcsstr(), wcschr(), wcsrchr()

    INTERNALS

******************************************************************************/
{
    const wchar_t *h, *n;

    if (*needle == L'\0')
        return (wchar_t *)haystack;

    for (; *haystack; haystack++) {
        if (*haystack != *needle)
            continue;

        h = haystack;
        n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (*n == L'\0')
            return (wchar_t *)haystack;
    }

    return NULL;
}
