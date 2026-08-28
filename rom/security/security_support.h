/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/
#ifndef _SECURITY_SUPPORT_H
#define _SECURITY_SUPPORT_H

#include <exec/types.h>

struct SecurityBase;

/* Post a (localised, RawDoFmt formatted) warning to the user. args is a
 * RawDoFmt mem stream of SIPTRs (may be NULL). */
extern void Warn(struct SecurityBase *secBase, CONST_STRPTR fmt, SIPTR *args);
#define Warn0(b,f)          Warn(b, f, NULL)
#define Warn1(b,f,a)        do { SIPTR _wa[1] = { (SIPTR)(a) }; Warn(b, f, _wa); } while (0)
#define Warn2(b,f,a,c)      do { SIPTR _wa[2] = { (SIPTR)(a), (SIPTR)(c) }; Warn(b, f, _wa); } while (0)

/* Report a fatal condition. Never returns to a state where the library
 * pretends to work: the library is marked as violated and further
 * requests are refused, but the caller keeps running. */
extern void Die(struct SecurityBase *secBase, CONST_STRPTR msg, ULONG alertcode);

/* Format a RawDoFmt style string into a buffer (mem stream args) */
extern void FormatString(CONST_STRPTR fmt, SIPTR *args, STRPTR dst, ULONG dstsize);

#endif /* _SECURITY_SUPPORT_H */
