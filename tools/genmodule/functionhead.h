/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.

    Desc: global include for genmodule. Defines global variables and
          the function prototypes.
*/
#ifndef FUNCTIONHEAD_H
#define FUNCTIONHEAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "stringlist.h"

enum libcall { STACK, REGISTER, MIXED, REGISTERMACRO, AUTOREGISTER };

struct functionarg {
    struct functionarg *next;
    char *type;
    char *name;
    char *reg;
};

struct functionhead {
    struct functionhead *next;
    char *name;
    char *type;
    enum libcall libcall;
    unsigned int argcount;
    struct functionarg *arguments;
    struct stringlist *aliases;
    unsigned int lvo;
    int novararg : 1; /* Are varargs allowed for this function ? */
    int priv     : 1; /* Is function private */
};

struct functions {
    /* In funclist the information of all the functions of the module will be stored.
     * The list has to be sorted on the lvonum field
     */
    struct functionhead *funclist;

    /* In methlist the information of all the methods of the class will be 
     * stored. We (mis)use struct functionlist for this, but don't use certain
     * fields (like lvo and reg (in struct arglist)).
     */
    struct functionhead *methlist;
};

struct functionhead *newfunctionhead(const char *name, enum libcall libcall);
struct functionarg *funcaddarg(
    struct functionhead *funchead,
    const char *name, const char *type, const char *reg
);
struct stringlist *funcaddalias(struct functionhead *funchead, const char *alias);

struct functions *functionsinit(void);

#endif //FUNCTIONHEAD_H
