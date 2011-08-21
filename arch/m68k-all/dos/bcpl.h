/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BCPL support
    Lang: english
*/

#define BCPL_GlobVec_NegSize	0xb0
#define BCPL_GlobVec_PosSize	0x21c

/* Our BCPL stub private data */

#define BCPL_DOSBase	-0xb0

/* We can add private data up to -0x88 */

#define BCPL_CLIArgument	0x214
