/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/partition.h>
#include <aros/symbolsets.h>

ADD2LIBS("partition.library", 1, LIBSET_USER_PRI, struct Library *, PartitionBase, NULL, NULL);
