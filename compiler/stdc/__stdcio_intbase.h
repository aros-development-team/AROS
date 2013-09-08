/*
    Copyright © 2012-2013, The AROS Development Team. All rights reserved.
    $Id$

    This file defines the private part of StdCIOBase.
    This should only be used internally in stdcio.library code so
    changes can be made to this structure without breaking backwards
    compatibility.
*/
#ifndef __STDCIO_INTBASE_H
#define __STDCIO_INTBASE_H

#include <libraries/stdcio.h>

struct StdCIOIntBase
{
    struct StdCIOBase StdCIOBase;
};

#endif //__STDCIO_INTBASE_H
