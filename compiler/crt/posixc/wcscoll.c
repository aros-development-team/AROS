/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function wcscoll.
*/


/*****************************************************************************

    NAME */
#include <wchar.h>

  int wcscoll(

/*  SYNOPSIS */
      const wchar_t *ws1, const wchar_t *ws2)
 
/*  FUNCTION
        Compares the two wide-character strings 'ws1' and 'ws2' according
        to the current locale's collation order.

    INPUTS
        ws1 -- first wide-character string
        ws2 -- second wide-character string

    RESULT
        Returns < 0 if ws1 < ws2, 0 if ws1 == ws2, > 0 if ws1 > ws2.

    NOTES
        This implementation ignores locale collation and performs a
        binary comparison using wcscmp().

    EXAMPLE

    BUGS

    SEE ALSO
        wcscmp(), wcsxfrm(), strcoll()

    INTERNALS

******************************************************************************/
{
    return wcscmp(ws1, ws2);
}
