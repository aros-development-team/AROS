/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: global include for genmodule. Defines global variables and
          the function prototypes.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct arglist {
    struct arglist *next;
    char *type;
    char *name;
    char *reg;
};

struct functionlist {
    struct functionlist *next;
    char *name;
    char *type;
    unsigned int argcount;
    struct arglist *arguments;
    unsigned int lvo;
};

struct forcelist {
    struct forcelist *next;
    char *basename;
};

struct linelist {
    struct linelist *next;
    char *line;
};

/* In funclist the information of all the functions of the module will be stored.
 * The list has to be sorted on the lvonum field
 */
extern struct functionlist *funclist;

/* In methlist the information of all the methods of the class will be 
 * stored. We (mis)use struct functionlist for this, but don't use certain
 * fields (like lvo and reg (in struct arglist)).
 */
extern struct functionlist *methlist;

/* In forcelist a list of basenames is present that need to be present in the
 * static link library so that certain libraries are opened by a program
 */
extern struct forcelist *forcelist;

/* global variables that store filename and paths derived from argv */
extern char *conffile, *gendir, *genincdir, *reffile;

/* global variables that store the configuration of the module */
enum modtype { UNSPECIFIED, LIBRARY, MCC, MUI, MCP };
extern enum modtype modtype;
enum libcall { STACK, REGISTER, MIXED, REGISTERMACRO, AUTOREGISTER };
extern enum libcall libcall;

extern char *modulename, *basename, *modulenameupper, *libbase, *libbasetype, 
            *libbasetypeextern, *datestring, *superclass;
extern int residentpri;
extern unsigned int majorversion, minorversion, firstlvo;
extern struct linelist *cdeflines, *cdefprivatelines, *protolines;

extern int customdispatcher; /* does class have custom dispatcher? */

void readconfig(void);
void readref(void);
void writeincproto(int dummy);
void writeincclib(int dummy);
void writeincdefines(int dummy);
void writeinclibdefs(void);
void writefunctable(void);
void writestart(void);
void writeend(void);
void writeautoinit(void);
void writestubs(void);
void writemccinit(void);
void writemccquery(void);
