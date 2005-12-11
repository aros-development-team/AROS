/*
    Copyright © 2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include file for genmodules support for oop.library classes
*/
#ifndef OOPSUPPORT_H
#define OOPSUPPORT_H

#include "config.h"
#include "functionhead.h"

#include <stdio.h>

void writeoopincludes(FILE *out);
void writeoopinit(FILE *out, struct classinfo *);

#endif /* OOPSUPPORT_H */
