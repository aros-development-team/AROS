/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROTO_RAMDRIVE_H
#define PROTO_RAMDRIVE_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/ramdrive_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/ramdrive.h>
#else
#include <defines/ramdrive.h>
#endif

#endif /* PROTO_RAMDRIVE_H */
