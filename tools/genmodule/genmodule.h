/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.

    Desc: global include for genmodule. Defines global variables and
          the function prototypes.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "functionhead.h"
#include "config.h"

void readref(struct config *, struct functions *);
void writeincproto(struct config *);
void writeincclib(struct config *, struct functions *);
void writeincdefines(struct config *, struct functions *);
void writeinclibdefs(struct config *);
void writestart(struct config *, struct functions *);
void writeend(struct config *);
void writeautoinit(struct config *);
void writestubs(struct config *, struct functions *);
void writemccinit(FILE *, struct config *, struct functions *);
void writemccquery(FILE *, struct config *);
