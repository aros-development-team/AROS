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
};

struct linelist {
    struct linelist *next;
    char *line;
};

/* In funclist the information of all the functions of the module will be stored.
 * The list has to be sorted on the lvonum field
 */
extern struct functionlist *funclist;

/* global variables that store filename and paths derived from argv */
extern char *conffile, *gendir, *genincdir, *reffile;

/* global variables that store the configuration of the module */
enum modtype { UNSPECIFIED, LIBRARY };
extern enum modtype modtype;
enum libcall { STACK, REGISTER, MIXED, REGISTERMACRO, AUTOREGISTER };
extern enum libcall libcall;

extern char *modulename, *basename, *modulenameupper, *libbase, *libbasetype, *libbasetypeextern, *datestring;
extern unsigned int majorversion, minorversion, firstlvo;
extern struct linelist *cliblines, *protolines;

/* The next variable are bools */
extern int hasinit, hasopen, hasclose, hasexpunge;

/* global variables for reading lines from files */
extern char *line; /* The current read file */
extern unsigned int slen; /* The allocation length pointed to be line */
extern unsigned int lineno; /* The line number, will be increased by one everytime a line is read */

void readline(FILE *);
void readconfig(void);
void readref(void);
void writeincproto(void);
void writeincclib(void);
void writeincdefines(void);
void writeinclibdefs(void);
void writefunctable(void);
void writestart(void);
void writeend(void);
void writeautoinit(void);
void writestubs(void);
