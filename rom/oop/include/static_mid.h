#ifndef OOP_STATIC_MID_H
#define OOP_STATIC_MID_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef OOP_OOP_H
#include <oop/oop.h>
#endif

#ifndef AROS_CREATE_ROM
#  define STATIC_MID static OOP_MethodID static_mid = 0;
#else
#  define STATIC_MID OOP_MethodID static_mid = 0;
#endif

#endif /* OOP_STATIC_MID_H */
