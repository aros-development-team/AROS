#include "genmodule.h"

/* In funclist the information of all the functions of the module will be stored.
 * The list has to be sorted on the lvonum field
 */
struct functionlist *funclist = NULL;

/* In methlist the information of all the methods of the class will be 
 * stored. We (mis)use struct functionlist for this, but don't use certain
 * fields (like lvo and reg (in struct arglist)).
 */
struct functionlist *methlist;

/* In forcelist a list of basenames is present that need to be present in the
 * static link library so that certain libraries are opened by a program
 */
struct forcelist *forcelist = NULL;

/* global variables that store filename and paths derived from argv */
char *conffile, *gendir, *genincdir, *reffile;

/* global variables that store the configuration of the module */
enum modtype modtype = UNSPECIFIED;
enum libcall libcall = STACK;

char *modulename = NULL, *basename = NULL, *modulenameupper = NULL, 
     *libbase = NULL, *libbasetype = NULL, *libbasetypeextern = NULL, 
     *datestring = "00.00.0000", *superclass = NULL;
unsigned int majorversion = 0, minorversion = 0, firstlvo = 0;
struct linelist *cdeflines = NULL, *cdefprivatelines = NULL, *protolines = NULL;

int customdispatcher = 0; /* does class have custom dispatcher? */
