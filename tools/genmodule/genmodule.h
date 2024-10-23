/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc: global include for genmodule. Defines global variables and
          the function prototypes.
*/

#ifndef GENMODULE_H
#define GENMODULE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "functionhead.h"
#include "config.h"

// Used when generating AROS_LC* calls in defines, inlines, and linklib.
#define TYPE_NORMAL  0
#define TYPE_DOUBLE  1
#define TYPE_QUAD    2

void generate_argtype_name_part(FILE *, int, int);
void writemakefile(struct config *);
void writeincproto(struct config *);
void writeincclib(struct config *);
void writeincdefines(struct config *);
void writeincinline(struct config *);
void writeinclibdefs(struct config *);
void writeincinterfaces(struct config *);
void writestart(struct config *);
void writeend(struct config *);
void writeautoinit(struct config *, int);
void writestubs(struct config *, int);
void writefunclist(struct config *);
void writefd(struct config *);
void writeskel(struct config *cfg);
void writethunk(struct config *cfg);
void writegetlibbase(struct config *cfg, int is_rel);
void writelinkentries(struct config *cfg);

#endif
