/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
*/
#ifndef PROTO_WB_H
#define PROTO_WB_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#include <clib/wb_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/wb.h>
#else
#include <defines/wb.h>
#endif

#endif /* PROTO_WB_H */
