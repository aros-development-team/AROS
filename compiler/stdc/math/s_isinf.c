/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBM_SCCS) && !defined(lint)
__RCSID("$NetBSD: s_isinf.c,v 1.6 2003/07/26 19:25:05 salo Exp $");
#endif

/*
 * isinf(x) returns 1 is x is inf, else 0;
 * no branching!
 */

#include "math.h"
#include "math_private.h"

int
__isinf(double x)
{
	int32_t hx,lx;
	EXTRACT_WORDS(hx,lx,x);
	hx &= 0x7fffffff;
	hx ^= 0x7ff00000;
	hx |= lx;
	return (hx == 0);
}

/* isinf is now a macro in math.h, but we need to keep a proper symbol around
 * for anyone linking to us without recompiling with new headers */
AROS_MAKE_ALIAS(__isinf, isinf);
