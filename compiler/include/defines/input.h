/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEFINES_INPUT_H
#define DEFINES_INPUT_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/

#define PeekQualifier() \
	AROS_LC0(UWORD, PeekQualifier, \
	struct Device *, InputBase, 7, Input)


#endif /* DEFINES_INPUT_H */
