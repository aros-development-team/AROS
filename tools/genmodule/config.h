/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.

    Desc: Define the C structure for storing the command line options
*/

enum command { CMD_UNSPECIFIED, DUMMY, NORMAL, LIBDEFS };
enum modtype { UNSPECIFIED, LIBRARY, MCC, MUI, MCP, DEVICE, RESOURCE };
enum libcall { STACK, REGISTER, MIXED, REGISTERMACRO, AUTOREGISTER };
enum optionbit { BIT_NOAUTOLIB, BIT_NOEXPUNGE, BIT_NORESIDENT };
enum optionflags { OPTION_NOAUTOLIB = 1<<BIT_NOAUTOLIB, OPTION_NOEXPUNGE = 1<<BIT_NOEXPUNGE,
                   OPTION_NORESIDENT =1<<BIT_NORESIDENT };

struct forcelist {
    struct forcelist *next;
    char *basename;
};

struct linelist {
    struct linelist *next;
    char *line;
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
    char *sysbase_field, *seglist_field;

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
    struct forcelist *forcelist;

    /* Code to add to the generated files */
    struct linelist *cdeflines, *cdefprivatelines;

    /* MCC specific data */
    char *superclass;
    int customdispatcher; /* does class have custom dispatcher? */
};

/* Function prototypes */

struct linelist *addline(struct linelist **, const char *);
struct forcelist *addforcebase(struct forcelist **, const char *);

struct config *initconfig(int, char **, int);
