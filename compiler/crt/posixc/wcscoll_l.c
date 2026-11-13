/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function wcscoll_l.
*/

/*****************************************************************************

    NAME */
#include <wchar.h>

  int wcscoll_l(

/*  SYNOPSIS */
      const wchar_t *ws1,
      const wchar_t *ws2,
      locale_t locale)
 
/*  FUNCTION


    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    return wcscoll(ws1, ws2);
}
