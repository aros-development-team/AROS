/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: main for genmodule. A tool to generate files for building modules.
*/
#include "genmodule.h"

/* In funclist the information of all the functions of the module will be stored.
 * The list has to be sorted on the lvonum field
 */
struct functionlist *funclist = NULL;

/* global variables that store filename and paths derived from argv */
char *conffile, *gendir, *genincdir, *reffile;

/* global variables that store the configuration of the module */
enum modtype modtype = UNSPECIFIED;
enum libcall libcall = STACK;

char *modulename = NULL, *basename = NULL, *modulenameupper = NULL, *libbase = NULL,
     *libbasetype = NULL, *libbasetypeextern = NULL, *datestring = "00.00.0000";
unsigned int majorversion = 0, minorversion = 0, firstlvo = 0;
struct linelist *cliblines = NULL, *protolines = NULL;

/* global variables for reading lines from files */
char *line; /* The current read file */
unsigned int slen; /* The allocation length pointed to be line */
unsigned int lineno; /* The line number, will be increased by one everytime a line is read */

void readline(FILE *f)
{
    char haseol;

    if (fgets(line, slen, f))
    {
	haseol = line[strlen(line)-1]=='\n';
	if (haseol) line[strlen(line)-1]='\0';
	
	while (!(haseol || feof(f)))
	{
	    slen += 256;
	    line = realloc(line, slen);
	    fgets(line+strlen(line), slen, f);
	    haseol = line[strlen(line)-1]=='\n';
	    if (haseol) line[strlen(line)-1]='\0';
	}
    }
    else
	line[0]='\0';
    lineno++;
}

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
    gendir = argv[4];
    
    line = malloc(256);
    slen = 256;

    readconfig();
    writeinclibdefs();
    
    return 0;
}
