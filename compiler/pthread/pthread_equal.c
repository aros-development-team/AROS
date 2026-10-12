/*
  Copyright (C) 2014 Szilard Biro
  Copyright (C) 2018 Harry Sintonen
  Copyright (C) 2019 Stefan "Bebbo" Franke - AmigaOS 3 port

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "pthread_intern.h"
#include "debug.h"

/****************************************************************************
    NAME */
        int pthread_equal(

/*  SYNOPSIS */
        pthread_t t1,
        pthread_t t2)

/*  FUNCTION
        Compare two thread identifier values. This implementation compares the
        numeric identifiers without looking up either thread.

    INPUTS
        t1 - First thread identifier.
        t2 - Second thread identifier.

    RESULT
        Nonzero (1) if the values are equal; 0 otherwise. No errors are reported.

    NOTES
        Include <pthread.h>; link with pthread. This call neither blocks nor
        changes thread state. Equality does not prove that a thread exists or
        is still running. An identifier may be reused after its slot is released,
        so equal values obtained at different times need not name the same
        worker lifetime.

    SEE ALSO
        pthread_self(),
        pthread_create()

*****************************************************************************/
{
    D(bug("%s(%u, %u)\n", __FUNCTION__, t1, t2));

    return (t1 == t2);
}
