/*
  Copyright (C) 2014 Szilard Biro

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

#include <string.h>

#include "pthread_intern.h"
#include "debug.h"

int pthread_cond_destroy(pthread_cond_t *cond)
{
    D(bug("%s(%p)\n", __FUNCTION__, cond));

    if (cond == NULL)
        return EINVAL;

    // probably a statically allocated condition
    if (SemaphoreIsInvalid(&cond->semaphore))
        return 0;

    if (AttemptSemaphore(&cond->semaphore) == FALSE)
        return EBUSY;

    if (!IsListEmpty(&cond->waiters))
    {
        ReleaseSemaphore(&cond->semaphore);
        return EBUSY;
    }

    ReleaseSemaphore(&cond->semaphore);
    memset(cond, 0, sizeof(pthread_cond_t));

    return 0;
}
