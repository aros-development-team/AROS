#include "genmodule.h"

/* In forcelist a list of basenames is present that need to be present in the
 * static link library so that certain libraries are opened by a program
 */
struct forcelist *forcelist = NULL;

/* global variables that store filename and paths derived from argv */
char *conffile, *gendir, *genincdir, *reffile;

/* global variables that store the configuration of the module */
enum modtype modtype = UNSPECIFIED;
enum libcall libcall = STACK;
int options = 0;

char *modulename = NULL, *basename = NULL, *modulenameupper = NULL, 
     *libbase = NULL, *libbasetype = NULL, *libbasetypeextern = NULL, 
     *datestring = "00.00.0000", *superclass = NULL,
     *sysbase_field = NULL, *seglist_field = NULL;
int residentpri = -128;
unsigned int majorversion = 0, minorversion = 0, firstlvo = 0;
struct linelist *cdeflines = NULL, *cdefprivatelines = NULL, *protolines = NULL;

int customdispatcher = 0; /* does class have custom dispatcher? */

/* Help functions to handle the variables and the types */
struct linelist *addline(struct linelist **linelistptr, const char *line)
{
    while(*linelistptr != NULL) linelistptr = &(*linelistptr)->next;
    
    *linelistptr = malloc(sizeof(struct linelist));
    if (*linelistptr != NULL)
    {
	(*linelistptr)->next = NULL;
	(*linelistptr)->line = strdup(line);
    }
    else
    {
	puts("Out of memory !");
	exit(20);
    }

    return *linelistptr;
}

struct forcelist *addforcebase(struct forcelist **forcelistptr, const char *basename)
{
    while(*forcelistptr != NULL) forcelistptr = &(*forcelistptr)->next;
    
    *forcelistptr = malloc(sizeof(struct forcelist));
    if (*forcelistptr != NULL)
    {
	(*forcelistptr)->next = NULL;
	(*forcelistptr)->basename = strdup(basename);
    }
    else
    {
	puts("Out of memory !");
	exit(20);
    }

    return *forcelistptr;
}
	
