/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$
*/
#ifndef PROTO_DISKFONT_H
#define PROTO_DISKFONT_H

#ifndef AROS_SYSTEM_H
#include <aros/system.h>
#endif

#ifndef DiskfontBase
extern struct Library * DiskFontBase;
#endif

#include <clib/diskfont_protos.h>

#if defined(_AMIGA) && defined(__GNUC__)
#include <inline/diskfont.h>
#else
#include <defines/diskfont.h>
#endif

#endif /* PROTO_DISKFONT_H */
