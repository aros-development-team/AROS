/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: main for genmodule. A tool to generate files for building modules.
*/
#include "genmodule.h"

/* In funclist the information of all the functions of the module will be stored.
 * The list has to be sorted on the lvonum field
 */
struct functionlist *funclist = NULL;

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
struct linelist *cdeflines = NULL, *protolines = NULL;

int main(int argc, char **argv)
{
    char *s;
    
    if (argc!=5)
    {
	fprintf(stderr, "Usage: %s modname modtype conffile gendir\n", argv[0]);
	exit(20);
    }

    modulename = argv[1];
    modulenameupper = strdup(modulename);
    for (s=modulenameupper; *s!='\0'; *s = toupper(*s), s++)
	;
    
    if (strcmp(argv[2],"library")==0)
    {
	modtype = LIBRARY;
	firstlvo = 5;
    }
    else
    {
	fprintf(stderr, "Unknown modtype \"%s\" speficied for second argument\n", argv[2]);
	exit(20);
    }
    
    conffile = argv[3];

    if (strlen(argv[4])>200)
    {
	fprintf(stderr, "Ridiculously long path for gendir\n");
	exit(20);
    }
    if (argv[4][strlen(argv[4])-1]=='/') argv[2][strlen(argv[2])-1]='\0';
    genincdir = argv[4];

    readconfig();
    writeincproto(1);
    writeincclib(1);
    writeincdefines(1);
    
    return 0;
}
