/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: global include for genmodule. Defines global variables and
          the function prototypes.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "functionhead.h"

struct forcelist {
    struct forcelist *next;
    char *basename;
};

struct linelist {
    struct linelist *next;
    char *line;
};

/* In forcelist a list of basenames is present that need to be present in the
 * static link library so that certain libraries are opened by a program
 */
extern struct forcelist *forcelist;

/* global variables that store filename and paths derived from argv */
extern char *conffile, *gendir, *genincdir, *reffile;

/* global variables that store the configuration of the module */
enum modtype { UNSPECIFIED, LIBRARY, MCC, MUI, MCP, DEVICE };
extern enum modtype modtype;
enum libcall { STACK, REGISTER, MIXED, REGISTERMACRO, AUTOREGISTER };
extern enum libcall libcall;

enum optionbit { BIT_NOAUTOLIB, BIT_NOEXPUNGE, BIT_NORESIDENT };
enum optionflags { OPTION_NOAUTOLIB = 1<<BIT_NOAUTOLIB, OPTION_NOEXPUNGE = 1<<BIT_NOEXPUNGE,
                   OPTION_NORESIDENT =1<<BIT_NORESIDENT };
extern int options;

extern char *modulename, *basename, *modulenameupper, *libbase, *libbasetype, 
            *libbasetypeextern, *datestring, *superclass;
extern int residentpri;
extern unsigned int majorversion, minorversion, firstlvo;
extern struct linelist *cdeflines, *cdefprivatelines, *protolines;

extern int customdispatcher; /* does class have custom dispatcher? */

struct linelist *addline(struct linelist **linelistptr, const char *line);
struct forcelist *addforcebase(struct forcelist **forcelistptr, const char *basename);

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
