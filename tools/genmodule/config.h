/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.

    Desc: Define the C structure for storing the command line options and the
          module config data
*/

#ifndef _CONFIG_H
#define _CONFIG_H

#include "stringlist.h"

enum command { CMD_UNSPECIFIED, DUMMY, NORMAL, LIBDEFS };
enum modtype { UNSPECIFIED, LIBRARY, MCC, MUI, MCP, DEVICE, RESOURCE };
enum libcall { STACK, REGISTER, MIXED, REGISTERMACRO, AUTOREGISTER };
enum optionbit { BIT_NOAUTOLIB, BIT_NOEXPUNGE, BIT_NORESIDENT,
                 BIT_DUPBASE
};
enum optionflags
{
    OPTION_NOAUTOLIB = 1<<BIT_NOAUTOLIB,
    OPTION_NOEXPUNGE = 1<<BIT_NOEXPUNGE,
    OPTION_NORESIDENT = 1<<BIT_NORESIDENT,
    OPTION_DUPBASE = 1<<BIT_DUPBASE
};

struct conffuncinfo {
    struct conffuncinfo *next;
    char *name;
    unsigned int lvo;
    struct stringlist *regs;
    int regcount;
    struct stringlist *aliases;
};

struct config
{
    /* members that store filename and paths derived from argv */
    char *conffile, *gendir, *genincdir, *reffile;

    /* The name and type of the module */
    char *modulename, *modulenameupper;
    enum modtype modtype;
    char *suffix;
    
    /* firstlvo is the LVO number of the first user definable function
     * in the module
     */
    unsigned int firstlvo;

    /* What to do ? */
    enum command command;

    /* Name for variables and types */
    char *basename, *libbase, *libbasetype, *libbasetypeptrextern;
    
    /* Where are the sysbase and seglist fields in the libbase ? */
    char *sysbase_field, *seglist_field, *rootbase_field;

    /* How are the module functions defined */
    enum libcall libcall;

    /* Some additional options, see optionsflags enum above */
    int options;

    /* Further configuration data for the generated Resident struct */
    char *datestring;
    int residentpri;
    unsigned int majorversion, minorversion;
    
    /* In forcelist a list of basenames is present that need to be present in the
     * static link library so that certain libraries are opened by a program
     */
    struct stringlist *forcelist;

    /* Code to add to the generated files */
    struct stringlist *cdeflines, *cdefprivatelines;

    /* The function config data present in the functionlist section */
    struct conffuncinfo *conffunclist;
    
    /* MCC specific data */
    char *superclass;
    int customdispatcher; /* does class have custom dispatcher? */
};

/* Function prototypes */

struct config *initconfig(int, char **, int);

#endif //_CONFIG_H
