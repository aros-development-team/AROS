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
enum modtype { UNSPECIFIED, LIBRARY, MCC, MUI, MCP, DEVICE, RESOURCE, IMAGE, GADGET,
	       DATATYPE, CLASS
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

enum coptionbit { CBIT_PRIVATE };
enum coptionflags { COPTION_PRIVATE = 1<<CBIT_PRIVATE };

enum intcfgbit { BIT_GENASTUBS, BIT_GENLINKLIB, BIT_NOREADREF };
enum intcfgflags
{
    CFG_GENASTUBS = 1<<BIT_GENASTUBS,
    CFG_GENLINKLIB = 1<<BIT_GENLINKLIB,
    CFG_NOREADREF = 1<<BIT_NOREADREF
};

/* Classinfo is used to store the information of a BOOPSI class */
struct classinfo
{
    struct classinfo *next;

    /* Type and name of the class */
    enum modtype classtype;
    char *basename;

    /* Priority with which the class will be initialized */
    int initpri;

    /* Additional options for the class */
    enum coptionflags options;
    
    const char **boopsimprefix;
    char *classid, *superclass, *classptr_field, *classptr_var;
    char *dispatcher; /* == NULL when the generated dispatcher is used,
		       * otherwise it is the function name of the dispatcher */;
    char *classdatatype; /* The type of the data for every object */
    
    struct functionhead *methlist;
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
    char *sysbase_field, *seglist_field, *rootbase_field;

    /* Some additional options, see optionsflags enum above */
    enum optionflags options;

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

    /* Code to add to the generated fioles */
    struct stringlist *cdeflines, *cdefprivatelines;

    /* device specific data */
    char *beginiofunc, *abortiofunc;

    /* The functions of this module */
    struct functionhead *funclist;
    
    /* The classes defined in this module */
    struct classinfo *classlist;
};

/* Function prototypes */

struct config *initconfig(int, char **);

const char* getBanner(struct config* config);

#endif //_CONFIG_H
