#ifndef _SYS_CDEFS_H_
#define _SYS_CDEFS_H_
/*
 * Copyright 1995-2002, The AROS Development Team. All rights reserved.
 * $Id$
 *
 * <sys/cdefs.h> header file, would you believe it's mostly the same as
 * <aros/system.h>?
 */

#include <aros/system.h>

#if defined(__STDC__) || defined(__cplusplus)
#define __P(protos)	protos
#else
#define __P(protos)	()
#endif

/* FreeBSD's code use this, but we don't need it, so define it to expand to
   nothing.  */
#define __FBSDID(x)

#endif /* _SYS_CDEFS_H_ */
