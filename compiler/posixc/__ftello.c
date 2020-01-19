/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

#define POSIXC_NOSTDIO_DECL

#include "__stdio.h"

#if (__WORDSIZE < 64)

/*
 * 32bit version of ftello. only handles <=2GB files.
 */

off_t __ftello(FILE *stream)
{
    return ftell(stream);
}
#endif

/*
 * 64bit version of ftello.  Must be able to hanlde both 32bit and 64bit filesystems !
 */

off64_t __ftello64(FILE *stream)
{
    return ftell(stream);
}

#if (__WORDSIZE==64)
/*
 * on 64bit ftello is an alias of ftello64
 */
AROS_MAKE_ALIAS(__ftello64,__ftello);
#endif
