/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/datatypes.h>
#include <aros/symbolsets.h>

ADD2LIBS("datatypes.library", 0, LIBSET_DATATYPES_PRI, struct Library *, DataTypesBase, NULL, NULL);
