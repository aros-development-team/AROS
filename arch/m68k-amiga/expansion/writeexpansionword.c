/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "expansion_intern.h"

/*****************************************************************************

    NAME */
#include <clib/expansion_protos.h>

	AROS_LH3(void, WriteExpansionWord,

/*  SYNOPSIS */
	AROS_LHA(APTR , board, A0),
	AROS_LHA(ULONG, offset, D0),
	AROS_LHA(ULONG, word, D1),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 27, Expansion)

{
    AROS_LIBFUNC_INIT

	volatile UBYTE *b = (UBYTE*)board;
	volatile UWORD *w = (UWORD*)board;

	b[offset * 4 + 4] = word;
	w[offset * 2 + 0] = word;

    AROS_LIBFUNC_EXIT
} /* WriteExpansionWord */
