/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.

    Desc: Define the C structure for storing the command line options and the
          module config data
*/

#ifndef _CONFIG_H
#define _CONFIG_H

#include <assert.h>

#include "functionhead.h"
#include "stringlist.h"

enum command { CMD_UNSPECIFIED, DUMMY, FILES, LIBDEFS, INCLUDES, MAKEFILE, WRITEFUNCLIST };
enum modtype { UNSPECIFIED, LIBRARY, MCC, MUI, MCP, DEVICE, RESOURCE, GADGET,
	       DATATYPE
};
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
enum intcfgbit { BIT_GENASTUBS, BIT_GENLINKLIB, BIT_NOREADREF };
enum intcfgflags
{
    CFG_GENASTUBS = 1<<BIT_GENASTUBS,
    CFG_GENLINKLIB = 1<<BIT_GENLINKLIB,
    CFG_NOREADREF = 1<<BIT_NOREADREF
};

struct conffuncinfo {
    struct conffuncinfo *next;
    char *name;
    unsigned int lvo;
    struct stringlist *regs;
    int regcount;
    /* regcount <  0 => stack based calling
     * regcount == 0 => register based calling, 0 arguments, libbase in A6
     * regcount >  0 => register based calling, regcount argument,s libbase in A6
     */
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


    /* The default path to put the module relative to SYS: */
    char *moddir;
    
    /* The names of the fields in the custom library base for storing internal
     * information
     */
    char *sysbase_field, *seglist_field, *rootbase_field, *classptr_field;

    /* Some additional options, see optionsflags enum above */
    int options;

    /* Internal configuration flags */
    enum intcfgflags intcfg;

    /* Further configuration data for the generated Resident struct */
    char *datestring, *copyright;
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

    /* device specific data */
    char *beginiofunc, *abortiofunc;
    
    /* BOOPSI specific data */
    const char **boopsimprefix;
    char *classname, *superclass;
    char *dispatcher; /* == NULL when the generated dispatcher is used,
		       * otherwise it is the function name of the dispatcher */;
    char *classdatatype; /* The type of the data for every object */
};

/* Function prototypes */

struct config *initconfig(int, char **, struct functions *);

const char* getBanner(struct config* config);

#endif //_CONFIG_H
