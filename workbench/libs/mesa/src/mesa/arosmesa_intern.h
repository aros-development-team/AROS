/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROSMESA_INTERN_H
#define AROSMESA_INTERN_H

#include <proto/exec.h>

/****************************************************************************************/

/* Constants */

struct MesaBase
{
    struct Library  mglb_Lib;
    APTR            mglb_CurrentContext;
    APTR            mglb_Dispatch;
};

#endif /* AROSMESA_INTERN_H */
