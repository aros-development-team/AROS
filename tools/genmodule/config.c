/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
    
    Code to parse the command line options for the genmodule program
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"

struct config *initconfig(int argc, char **argv, int command)
{
    struct config *cfg;
    char *s;

    switch (command)
    {
    case NORMAL:
	if (argc!=7)
	{
	    fprintf(stderr, "Usage: %s modname modtype conffile gendir genincdir reffile\n", argv[0]);
	    exit(20);
	}
	break;
    case LIBDEFS:
    case DUMMY:
	if (argc!=5)
	{
	    fprintf(stderr, "Usage: %s modname modtype conffile gendir\n", argv[0]);
	    exit(20);
	}
	break;
    default:
	fprintf(stderr, "Unknown command in initconfig\n");
	exit(20);
    }
	

    cfg = malloc(sizeof(struct config));
    if (cfg == NULL)
    {
	fprintf(stderr, "Out of memory\n");
	exit(20);
    }
    memset(cfg, 0, sizeof(struct config));
    cfg->datestring = "00.00.0000";

    cfg->command = command;
    
    cfg->modulename = argv[1];
    cfg->modulenameupper = strdup(cfg->modulename);
    for (s=cfg->modulenameupper; *s!='\0'; *s = toupper(*s), s++)
	;

    if (strcmp(argv[2],"library")==0)
    {
    	cfg->modtype = LIBRARY;
	cfg->suffix = "library";
    }
    else if (strcmp(argv[2],"mcc")==0)
    {
    	cfg->modtype = MCC;
	cfg->suffix = "mcc";
    }
    else if (strcmp(argv[2],"mui")==0)
    {
    	cfg->modtype = MUI;
	cfg->suffix = ".mui";
    }
    else if (strcmp(argv[2],"mcp")==0)
    {
    	cfg->modtype = MCP;
	cfg->suffix = "mcp";
    }
    else if (strcmp(argv[2], "device")==0)
    {
	cfg->modtype = DEVICE;
	cfg->suffix = "device";
    }
    else
    {
	fprintf(stderr, "Unknown modtype \"%s\" speficied for second argument\n", argv[2]);
	exit(20);
    }

    if (cfg->modtype == LIBRARY)
        cfg->firstlvo = 5;
    else if (cfg->modtype == DEVICE)
	cfg->firstlvo = 7;
    else if (cfg->modtype == MCC || cfg->modtype == MUI || cfg->modtype == MCP)
        cfg->firstlvo = 6;
   
    cfg->conffile = argv[3];

    if (strlen(argv[4])>200)
    {
	fprintf(stderr, "Ridiculously long path for gendir\n");
	exit(20);
    }
    if (argv[4][strlen(argv[4])-1]=='/') argv[2][strlen(argv[2])-1]='\0';
    cfg->gendir = argv[4];

    if (command == NORMAL)
    {
	if (strlen(argv[5])>200)
	{
	    fprintf(stderr, "Ridiculously long path for genincdir\n");
	    exit(20);
	}
	if (argv[5][strlen(argv[5])-1]=='/') argv[5][strlen(argv[5])-1]='\0';
	cfg->genincdir = argv[5];

	cfg->reffile = argv[6];
    }
    else if (command == DUMMY)
    {
	cfg->genincdir = cfg->gendir;
    }

    return cfg;
}

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
