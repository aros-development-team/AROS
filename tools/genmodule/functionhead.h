/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.

    Desc: global include for genmodule. Defines global variables and
          the function prototypes.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct functionarg {
    struct functionarg *next;
    char *type;
    char *name;
    char *reg;
};

struct functionalias {
    struct functionalias *next;
    char *alias;
};

struct functionhead {
    struct functionhead *next;
    char *name;
    char *type;
    unsigned int argcount;
    struct functionarg *arguments;
    struct functionalias *aliases;
    unsigned int lvo;
    int novararg;
};

/* In funclist the information of all the functions of the module will be stored.
 * The list has to be sorted on the lvonum field
 */
extern struct functionhead *funclist;

/* In methlist the information of all the methods of the class will be 
 * stored. We (mis)use struct functionlist for this, but don't use certain
 * fields (like lvo and reg (in struct arglist)).
 */
extern struct functionhead *methlist;

struct functionhead *newfunctionhead(const char *name, const char *type, unsigned int lvo);
struct functionarg *funcaddarg(
    struct functionhead *funchead,
    const char *name, const char *type, const char *reg
);
struct functionalias *funcaddalias(struct functionhead *funchead, const char *alias);
