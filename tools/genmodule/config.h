/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.

    Desc: Define the C structure for storing the command line options and the
          module config data
*/

#ifndef _CONFIG_H
#define _CONFIG_H

#include <assert.h>

#include "functionhead.h"
#include "stringlist.h"

enum command { CMD_UNSPECIFIED, FILES, LIBDEFS, INCLUDES, MAKEFILE, WRITEFUNCLIST, WRITEFD, WRITESKEL, WRITETHUNK };
enum modtype { UNSPECIFIED, LIBRARY, MCC, MUI, MCP, DEVICE, RESOURCE, IMAGE, GADGET,
               DATATYPE, CLASS, HIDD, USBCLASS, HANDLER
};

enum optionbit { BIT_NOAUTOLIB, BIT_NOEXPUNGE, BIT_NORESIDENT,
                 BIT_DUPBASE, BIT_PERTASKBASE, BIT_INCLUDES, BIT_NOINCLUDES,
                 BIT_STUBS, BIT_NOSTUBS, BIT_AUTOINIT, BIT_NOAUTOINIT,
                 BIT_RESAUTOINIT, BIT_NOOPENCLOSE, BIT_SELFINIT,
                 BIT_STACKCALL, BIT_RELLINKLIB
};
enum optionflags
{
    OPTION_NOAUTOLIB = 1<<BIT_NOAUTOLIB,
    OPTION_NOEXPUNGE = 1<<BIT_NOEXPUNGE,
    OPTION_NORESIDENT = 1<<BIT_NORESIDENT,
    OPTION_DUPBASE = 1<<BIT_DUPBASE,
    OPTION_PERTASKBASE = 1<<BIT_PERTASKBASE,
    OPTION_INCLUDES = 1<<BIT_INCLUDES,
    OPTION_NOINCLUDES = 1<<BIT_NOINCLUDES,
    OPTION_STUBS = 1<<BIT_STUBS,
    OPTION_NOSTUBS = 1<<BIT_NOSTUBS,
    OPTION_AUTOINIT = 1<<BIT_AUTOINIT,
    OPTION_NOAUTOINIT = 1<<BIT_NOAUTOINIT,
    OPTION_RESAUTOINIT = 1<<BIT_RESAUTOINIT,
    OPTION_NOOPENCLOSE = 1<<BIT_NOOPENCLOSE,
    OPTION_SELFINIT = 1<<BIT_SELFINIT,
    OPTION_STACKCALL = 1<<BIT_STACKCALL,
    OPTION_RELLINKLIB = 1<<BIT_RELLINKLIB,
};

enum coptionbit { CBIT_PRIVATE };
enum coptionflags { COPTION_PRIVATE = 1<<CBIT_PRIVATE };

enum intcfgbit { BIT_GENASTUBS, BIT_NOREADFUNCS };
enum intcfgflags
{
    CFG_GENASTUBS = 1<<BIT_GENASTUBS,
    CFG_NOREADFUNCS = 1<<BIT_NOREADFUNCS /* all needed functions are available */
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
    char *classid, *superclass, *superclass_field, *classptr_field, *classptr_var;
    char *dispatcher; /* == NULL when the generated dispatcher is used,
                       * otherwise it is the function name of the dispatcher */;
    char *classdatatype; /* The type of the data for every object */

    struct functionhead *methlist;

    /* Interfaces used in this class (only for HIDD classes) */
    struct stringlist *interfaces;

};

/* interfaceinfo is used to store the information of a BOOPSI class */
struct interfaceinfo
{
    struct interfaceinfo *next;

    /* id and base name of the interface */
    char *interfaceid;
    char *interfacename;
    char *methodstub;
    char *methodbase;
    char *attributebase;

    struct functionhead *methodlist;
    struct functionhead *attributelist;
};


/* DOS handlers */
struct handlerinfo {
    struct handlerinfo *next;

    enum {
        HANDLER_DOSTYPE,        /* FileSysResource registered */
        HANDLER_DOSNODE,        /* Non-bootable DOS device */
        HANDLER_RESIDENT,       /* AddSegment() registered */
    } type;
    unsigned int id;
    char *name;
    int   autodetect;           /* Autodetect priority (0 for not autodetectable) */
    /* DeviceNode overrides */
    int bootpri;                /* Boot priority */
    int priority;               /* Task priority */
    int stacksize;              /* Stacksize information */
    int startup;                /* Startup id */
};

struct config
{
    /* members that store filename and paths derived from argv */
    char *conffile, *gendir, *libgendir, *genincdir;

    /* The name and type of the module */
    char *modulename, *modulenameupper;
    enum modtype modtype;
    char *modtypestr;
    char *suffix;
    char *includename, *includenameupper;

    /* Flavour (sub-build) of the module */
    char *flavour;

    /* Extra string to include in version */
    char *versionextra;

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
    char *sysbase_field, *seglist_field, *rootbase_field, *oopbase_field;

    /* Some additional options, see optionsflags enum above */
    enum optionflags options;

    /* Internal configuration flags */
    enum intcfgflags intcfg;

    /* Further configuration data for the generated Resident struct */
    char *datestring, *copyright;
    int residentpri;
    unsigned int majorversion, minorversion;
    char *addromtag;

    /* In forcelist a list of basenames is present that need to be present in the
     * static link library so that certain libraries are opened by a program
     */
    struct stringlist *forcelist;

    /* Code to add to the generated files */
    struct stringlist *cdeflines, *cdefprivatelines, *stubprivatelines, *startuplines;

    /* device specific data */
    char *beginiofunc, *abortiofunc;

    /* The functions of this module */
    struct functionhead *funclist;

    /* The classes defined in this module */
    struct classinfo *classlist;

    /* The interface defined in this module */
    struct interfaceinfo *interfacelist;

    /* The DOS IDs and handlers for this module */
    char *handlerfunc;
    struct handlerinfo *handlerlist;

    /* Relative libraries used by this library
     */
    struct stringlist *rellibs;
};

/* Function prototypes */

struct config *initconfig(int, char **);

char* getBanner(struct config*);
void freeBanner(char*);

#endif //_CONFIG_H
