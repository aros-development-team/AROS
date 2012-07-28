#ifndef GENMODULE_H
#define GENMODULE_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.

    Desc: global include for genmodule. Defines global variables and
          the function prototypes.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "functionhead.h"
#include "config.h"

void writemakefile(struct config *);
void writeincproto(struct config *, int);
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
void writedefinestack(FILE *out, struct functionhead *funclistit, struct config *cfg);
void writegetlibbase(struct config *cfg, int is_rel);

#endif
