/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#define VVER "0.1"

#ifdef API_FSDEV
#ifdef __DEBUG__
#define VDEBUG "debug/fsdev"
#else
#define VDEBUG "fsdev"
#endif
#else
#ifdef __DEBUG__
#define VDEBUG "debug"
#else
#define VDEBUG ""
#endif
#endif

#ifdef __MORPHOS__
#define VSYS "PPC/MOS"
#else
# ifdef __AROS__
# define VSYS "AROS"
# else
# define VSYS CPU
# endif
#endif

#ifndef VDATE
#define VDATE __DATE__
#endif

static const char verstring[] = "\0$VER: FATFileSystem " VVER VDEBUG " [" VSYS "] (" VDATE ") by Marek Szyprowski <marek@amiga.pl>\n\0";
