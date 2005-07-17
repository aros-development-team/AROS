/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
    
    Code to parse the command line options and the module config file for
    the genmodule program
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "functionhead.h"
#include "config.h"

static char banner[256] = "\0";


const char*
getBanner(struct config* config)
{
    if (banner[0] == '\0')
    {
        snprintf (banner, 255,
"/*\n"
"    *** Automatically generated from '%s'. Edits will be lost. ***\n"
"    Copyright © 1995-2005, The AROS Development Team. All rights reserved.\n"
"*/\n", config->conffile
        );
    }

    return(banner);
}

const static char usage[] =
    "\n"
    "Usage: genmodule [-c conffile] [-s suffix] [-d gendir] [-r reffile]\n"
    "       {writefiles|writemakefile|writeincludes|writedummy|writelibdefs|writefunclist} modname modtype\n"
;

static void readconfig(struct config *, struct functions *);


/* the method prefices for the supported classes */
static const char *muimprefix[] =
{
    "__OM_",
    "__MUIM_",
    NULL
};
static const char *gadgetmprefix[] =
{
    "__OM_",
    "__GM_",
    "__AROSM_",
    NULL
};
static const char *dtmprefix[] =
{
    "__OM_",
    "__GM_",
    "__DTM_",
    "__PDTM_",
    NULL
};

/* Create a config struct. Initialize with the values from the programs command
 * line arguments and the contents of the modules .conf file
 */
struct config *initconfig(int argc, char **argv, struct functions *functions)
{
    struct config *cfg;
    char *s, **argvit = argv + 1;
    int hassuffix = 0, c;
    
    cfg = malloc(sizeof(struct config));
    if (cfg == NULL)
    {
	fprintf(stderr, "Out of memory\n");
	exit(20);
    }

    memset(cfg, 0, sizeof(struct config));
    
    while ((c = getopt(argc, argv, ":c:s:d:r:")) != -1)
    {

	if (c == ':' || *optarg == '-')
	{
	    fprintf(stderr, "Option -%c needs an argument\n",c);
	    exit(20);
	}

	switch (c)
	{
	case 'c':
	    cfg->conffile = optarg;
	    break;
	case 's':
	    cfg->suffix = optarg;
	    hassuffix = 1;
	    break;
	case 'd':
	    /* Remove / at end if present */
	    if ((optarg)[strlen(*argvit)-1]=='/') (optarg)[strlen(optarg)-1]='\0';
	    cfg->gendir = optarg;
	    break;
	case 'r':
	    cfg->reffile = optarg;
	    break;
	default:
	    fprintf(stderr, "Internal error: Unhandled option\n");
	    exit(20);
	}
    }

    if (optind + 3 != argc)
    {
	fprintf(stderr, "Wrong number of arguments.\n%s", usage);
	exit(20);
    }
    
    if (strcmp(argv[optind], "writefiles") == 0)
    {
	cfg->command = FILES;
    }
    else if (strcmp(argv[optind], "writemakefile") == 0)
    {
	cfg->command = MAKEFILE;
    }
    else if (strcmp(argv[optind], "writeincludes") == 0)
    {
	cfg->command = INCLUDES;
    }
    else if (strcmp(argv[optind], "writelibdefs") == 0)
    {
	cfg->command = LIBDEFS;
    }
    else if (strcmp(argv[optind], "writedummy") == 0)
    {
	cfg->command = DUMMY;
    }
    else if (strcmp(argv[optind], "writefunclist") == 0)
    {
	cfg->command = WRITEFUNCLIST;
    }
    else
    {
	fprintf(stderr, "Unrecognized argument \"%s\"\n%s", argv[optind], usage);
	exit(20);
    }

    cfg->modulename = argv[optind+1];
    cfg->modulenameupper = strdup(cfg->modulename);
    for (s=cfg->modulenameupper; *s!='\0'; *s = toupper(*s), s++)
	;

    if (strcmp(argv[optind+2],"library")==0)
    {
    	cfg->modtype = LIBRARY;
	cfg->intcfg |= CFG_GENLINKLIB;
	cfg->moddir = "Libs";
    }
    else if (strcmp(argv[optind+2],"mcc")==0)
    {
    	cfg->modtype = MCC;
	cfg->moddir = "Classes/Zune";
    }
    else if (strcmp(argv[optind+2],"mui")==0)
    {
    	cfg->modtype = MUI;
	cfg->moddir = "Classes/Zune";
    }
    else if (strcmp(argv[optind+2],"mcp")==0)
    {
    	cfg->modtype = MCP;
	cfg->moddir = "Classes/Zune";
    }
    else if (strcmp(argv[optind+2], "device")==0)
    {
	cfg->modtype = DEVICE;
	cfg->intcfg |= CFG_GENLINKLIB;
	cfg->moddir = "Devs";
    }
    else if (strcmp(argv[optind+2], "resource")==0)
    {
	cfg->modtype = RESOURCE;
	cfg->intcfg |= CFG_GENLINKLIB;
	cfg->moddir = "Devs";
    }
    else if (strcmp(argv[optind+2], "gadget")==0)
    {
	cfg->modtype = GADGET;
	cfg->moddir = "Classes/Gadgets";
    }
    else if (strcmp(argv[optind+2], "datatype")==0)
    {
	cfg->modtype = DATATYPE;
	cfg->moddir = "Classes/Datatypes";
    }
    else
    {
	fprintf(stderr, "Unknown modtype \"%s\" speficied for second argument\n", argv[2]);
	exit(20);
    }

    if (!hassuffix)
	cfg->suffix = argv[optind+2];

    cfg->boopsimprefix = NULL;
    switch (cfg->modtype)
    {
    case LIBRARY:
        cfg->firstlvo = 5;
	break;
    case DEVICE:
	cfg->firstlvo = 7;
	break;
    case MCC:
    case MUI:
    case MCP:
        cfg->firstlvo = 6;
	cfg->boopsimprefix = muimprefix;
	break;
    case RESOURCE:
	cfg->firstlvo = 1;
	break;
    case GADGET:
	cfg->firstlvo = 5;
	cfg->boopsimprefix = gadgetmprefix;
	break;
    case DATATYPE:
	cfg->firstlvo = 6;
	cfg->boopsimprefix = dtmprefix;
	break;
    default:
	fprintf(stderr, "Internal error: unsupported modtype for firstlvo\n");
	exit(20);
    }

    
    /* Fill fields with default value if not specified on the command line */
    {
	char tmpbuf[256];

	if (cfg->conffile == NULL)
	{
	    snprintf(tmpbuf, sizeof(tmpbuf), "%s.conf", cfg->modulename);
	    cfg->conffile = strdup(tmpbuf);
	}

	if (cfg->gendir == NULL)
	    cfg->gendir = ".";
	
	if (cfg->command != FILES && cfg->command != INCLUDES && cfg->command != WRITEFUNCLIST)
	{
	    if (cfg->reffile != NULL)
		fprintf(stderr, "WARNING ! Option -r ingored for %s\n", argv[optind]);
	}
	else if (cfg->command == FILES && cfg->reffile == NULL)
	{
	    snprintf(tmpbuf, sizeof(tmpbuf), "%s.ref", cfg->modulename);
	    cfg->reffile = strdup(tmpbuf);
	}
	
	/* Set default date to current date */
	{
	    time_t now = time(NULL);

	    strftime(tmpbuf, sizeof(tmpbuf), "%d.%m.%Y", localtime(&now));

	    cfg->datestring = strdup(tmpbuf);
	}
    }
    
    readconfig(cfg, functions);
    
    /* For a device add the functions given in beginiofunc and abortiofunc to the functionlist
     * if they are provided
     */
    if (cfg->beginiofunc != NULL)
    {
	struct functionhead *funchead;

	cfg->intcfg |= CFG_NOREADREF;

	/* Add beginio_func to the list of functions */
	funchead = newfunctionhead(cfg->beginiofunc, REGISTERMACRO);
	funchead->type = strdup("void");
	funchead->lvo = 5;
	funcaddarg(funchead, "struct IORequest *ioreq", "A1");
	
	funchead->next = functions->funclist;
	functions->funclist = funchead;
	
	/* Add abortio_func to the list of functions */
	funchead = newfunctionhead(cfg->abortiofunc, REGISTERMACRO);
	funchead->type = strdup("LONG");
	funchead->lvo = 6;
	funcaddarg(funchead, "struct IORequest *ioreq", "A1");
	
	funchead->next = functions->funclist->next;
	functions->funclist->next = funchead;
    }
    else if (cfg->modtype == DEVICE && cfg->intcfg & CFG_NOREADREF)
    {
	fprintf
	(
	    stderr,
	    "beginio_func and abortio_func missing for a device with a non empty function list\n"
	);
	exit(20);
    }

    return cfg;
}

/* Functions to read configuration from the configuration file */

#include "fileread.h"

static void readsectionconfig(struct config *);
static void readsectioncdef(struct config *);
static void readsectioncdefprivate(struct config *);
static void readsectionfunctionlist(struct config *, struct functions *);
static void readsectionmethodlist(struct config *, struct functions *);

static void readconfig(struct config *cfg, struct functions *functions)
{
    char *line, *s, *s2;

    if (!fileopen(cfg->conffile))
    {
	fprintf(stderr, "In readconfig: Could not open %s\n", cfg->conffile);
	exit(20);
    }

    while ((line=readline())!=NULL)
    {
	if (strncmp(line, "##", 2)==0)
	{
	    static char *parts[] = {"config", "cdefprivate", "cdef", "functionlist", "methodlist"};
	    const unsigned int nums = sizeof(parts)/sizeof(char *);
	    unsigned int partnum;
	    int i, atend = 0;
	    
	    s = line+2;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "begin", 5)!=0)
		exitfileerror(20, "\"##begin\" expected\n");

	    s += 5;
	    if (!isspace(*s))
		exitfileerror(20, "space after begin expected\n");
	    while (isspace(*s)) s++;
	    
	    for (i = 0, partnum = 0; partnum==0 && i<nums; i++)
	    {
		if (strncmp(s, parts[i], strlen(parts[i]))==0)
		{
		    partnum = i+1;
		    s += strlen(parts[i]);
		    while (isspace(*s)) s++;
		    if (*s!='\0')
			exitfileerror(20, "unexpected character on position %d\n", s-line);
		}
	    }
	    if (partnum==0)
		exitfileerror(20, "unknown start of section\n");
	    switch (partnum)
	    {
	    case 1: /* config */
		readsectionconfig(cfg);
		break;
		
	    case 2: /* cdefprivate */
		readsectioncdefprivate(cfg);
		break;
		
	    case 3: /* cdef */
		readsectioncdef(cfg);
		break;

	    case 4: /* functionlist */
		readsectionfunctionlist(cfg, functions);
		break;

	    case 5: /* methodlist */
		readsectionmethodlist(cfg, functions);
		break;
	    }
	}
	else if (strlen(line)!=0)
	    filewarning("warning line outside section ignored\n");
    }
    fileclose();
}


static void readsectionconfig(struct config *cfg)
{
    int atend = 0, i;
    char *line, *s, *s2, *libbasetypeextern = NULL;
    
    while (!atend)
    {
	line = readline();
	if (line==NULL)
	    exitfileerror(20, "unexpected end of file in section config\n");

	if (strncmp(line, "##", 2)!=0)
	{
	    const char *names[] = 
            {
                "basename", "libbase", "libbasetype", "libbasetypeextern", 
                "version", "date", "libcall", "forcebase", "superclass",
		"residentpri", "options", "sysbase_field", "seglist_field",
		"rootbase_field", "classptr_field", "classname", "classdatatype",
		"beginio_func", "abortio_func"
            };
	    const unsigned int namenums = sizeof(names)/sizeof(char *);
	    unsigned int namenum;
	    
	    for (i = 0, namenum = 0; namenum==0 && i<namenums; i++)
	    {
		if 
                (
                       strncmp(line, names[i], strlen(names[i]))==0 
                    && isspace(*(line+strlen(names[i])))
                )
		    namenum = i+1;
	    }
	    if (namenum==0)
		exitfileerror(20, "unrecognized configuration option\n");

	    s = line + strlen(names[namenum-1]);
	    if (!isspace(*s))
		exitfileerror(20, "space character expected after \"%s\"\n", names[namenum-1]);

	    while (isspace(*s)) s++;
	    if (*s=='\0')
		exitfileerror(20, "unexpected end of line\n");
	    
	    s2 = s + strlen(s);
	    while (isspace(*(s2-1))) s2--;
	    *s2 = '\0';
	    
	    switch (namenum)
	    {
	    case 1: /* basename */
		cfg->basename = strdup(s);
		break;
		
	    case 2: /* libbase */
		cfg->libbase = strdup(s);
		break;
		
	    case 3: /* libbasetype */
		cfg->libbasetype = strdup(s);
		break;
		
	    case 4: /* libbasetypeextern */
		libbasetypeextern = strdup(s);
		break;
		
	    case 5: /* version */
		if (sscanf(s, "%u.%u", &cfg->majorversion, &cfg->minorversion)!=2)
		    exitfileerror(20, "wrong version string \"%s\"\n", s);
		break;
		
	    case 6: /* date */
		if (!(strlen(s)==10 && isdigit(s[0]) && isdigit(s[1]) &&
		      s[2]=='.' && isdigit(s[3]) && isdigit(s[4]) &&
		      s[5]=='.' && isdigit(s[6]) && isdigit(s[7]) &&
		      isdigit(s[8]) && isdigit(s[9])))
		{
		    exitfileerror(20, "date string has have dd.mm.yyyy format\n");
		}
		cfg->datestring = strdup(s);
		break;
		
	    case 7: /* libcall */
		fprintf(stderr, "libcall specification is deprecated and ignored\n");
		break;
		
	    case 8: /* forcebase */
		slist_append(&cfg->forcelist, s);
		break;
                
            case 9: /* superclass */
                cfg->superclass = strdup(s);
                break;
		
	    case 10: /* residentpri */
		{
		    int count;
		    char dummy;
		    
		    count = sscanf(s, "%d%c", &cfg->residentpri, &dummy);
		    if (count != 1 ||
			cfg->residentpri < -128 || cfg->residentpri > 127
		    )
		    {
			exitfileerror(20, "residentpri number format error\n");
		    }
		}
		break;

	    case 11: /* options */
		do {
		    static const char *optionnames[] =
		    {
			"noautolib", "noexpunge", "noresident", "peropenerbase"
		    };
		    const unsigned int optionnums = sizeof(optionnames)/sizeof(char *);
		    int optionnum;

		    for (i = 0, optionnum = 0; optionnum==0 && i<optionnums; i++)
		    {
			if (strncmp(s, optionnames[i], strlen(optionnames[i]))==0)
			{
			    optionnum = i + 1;
			    s += strlen(optionnames[i]);
			    while (isspace(*s)) s++;
			    if (*s == ',')
				s++;
			    else if (*s != '\0')
				exitfileerror(20, "Unrecognized option\n");
			}
		    }
		    if (optionnum == 0)
			exitfileerror(20, "Unrecognized option\n");
		    switch (optionnum)
		    {
		    case 1: /* noautolib */
			cfg->options |= OPTION_NOAUTOLIB;
			break;
		    case 2: /* noexpunge */
			cfg->options |= OPTION_NOEXPUNGE;
			break;
		    case 3: /* noresident */
			cfg->options |= OPTION_NORESIDENT;
			cfg->firstlvo = 1;
			break;
		    case 4: /* peropenerbase */
			cfg->options |= OPTION_DUPBASE;
			break;
		    }
		    while (isspace(*s)) s++;
		} while(*s !='\0');
		break;

	    case 12: /* sysbase_field */
		cfg->sysbase_field = strdup(s);
		break;
		
	    case 13: /* seglist_field */
		cfg->seglist_field = strdup(s);
		break;
		
	    case 14: /* rootbase_field */
		cfg->rootbase_field = strdup(s);
		break;
		
	    case 15: /* classptr_field */
		if (cfg->boopsimprefix == NULL)
		{
		    exitfileerror
		    (
		        20,
		        "classptr_field specified for a module that is not a BOOPSI class\n"
		    );
		}
		cfg->classptr_field = strdup(s);
		break;
		
	    case 16: /* classname */
		if (cfg->modtype != GADGET && cfg->modtype != DATATYPE)
		    exitfileerror(20, "classname specified when not a BOOPSI class\n");
		cfg->classname = strdup(s);
		break;
		
	    case 17: /* classdatatype */
		if (cfg->boopsimprefix == NULL)
		    exitfileerror(20, "classdatatype specified when not a BOOPSI class\n");
		cfg->classdatatype = strdup(s);
		break;
		
	    case 18: /* beginio_func */
		if (cfg->modtype != DEVICE)
		    exitfileerror(20, "beginio_func specified when not a device\n");
		cfg->beginiofunc = strdup(s);
		break;
		
	    case 19: /* abortio_func */
		if (cfg->modtype != DEVICE)
		    exitfileerror(20, "abortio_func specified when not a device\n");
		cfg->abortiofunc = strdup(s);
		break;
	    }
	}
	else /* Line starts with ## */
	{
	    s = line+2;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "end", 3)!=0)
		exitfileerror(20, "\"##end config\" expected\n");

	    s += 3;
	    if (!isspace(*s))	
		exitfileerror(20, "\"##end config\" expected\n");

	    while (isspace(*s)) s++;
	    if (strncmp(s, "config", 6)!=0)
		exitfileerror(20, "\"##end config\" expected\n");

	    s += 6;
	    while (isspace(*s)) s++;
	    if (*s!='\0')
		exitfileerror(20, "\"##end config\" expected\n");

	    atend = 1;
	}
    }
    
    if (cfg->basename==NULL)
    {
	cfg->basename = strdup(cfg->modulename);
	*cfg->basename = toupper(*cfg->basename);
    }
    if (cfg->libbase==NULL)
    {
	unsigned int len = strlen(cfg->basename)+5;
	cfg->libbase = malloc(len);
	snprintf(cfg->libbase, len, "%sBase", cfg->basename);
    }
    if (cfg->sysbase_field != NULL && cfg->libbasetype == NULL)
	exitfileerror(20, "sysbase_field specified when no libbasetype is given\n");
    if (cfg->seglist_field != NULL && cfg->libbasetype == NULL)
	exitfileerror(20, "seglist_field specified when no libbasetype is given\n");

    if ( (cfg->beginiofunc != NULL && cfg->abortiofunc == NULL)
	 || (cfg->beginiofunc == NULL && cfg->abortiofunc != NULL)
    )
	exitfileerror(20, "please specify both beginio_func and abortio_func\n");

    if (cfg->classname == NULL)
    {
	if (cfg->modtype == GADGET)
	{
	    char s[256];
	    sprintf(s, "%sclass", cfg->modulename);
	    cfg->classname = strdup(s);
	}
	else if (cfg->modtype == DATATYPE)
	{
	    char s[256];
	    sprintf(s, "%s.datatype", cfg->modulename);
	    cfg->classname = strdup(s);
	}
    }

    if (libbasetypeextern==NULL)
    {
	switch (cfg->modtype)
	{
	case DEVICE:
	    cfg->libbasetypeptrextern = "struct Device *";
	    break;
	case RESOURCE:
	    cfg->libbasetypeptrextern = "APTR ";
	    break;
	case LIBRARY:
	case MUI:
	case MCP:
	case MCC:
	case GADGET:
	case DATATYPE:
	    cfg->libbasetypeptrextern = "struct Library *";
	    break;
	default:
	    fprintf(stderr, "Internal error: Unsupported modtype for libbasetypeptrextern\n");
	    exit(20);
	}
    }
    else
    {
	cfg->libbasetypeptrextern = malloc(strlen(libbasetypeextern)+3);
	strcpy(cfg->libbasetypeptrextern, libbasetypeextern);
	strcat(cfg->libbasetypeptrextern, " *");
	free(libbasetypeextern);
    }
    
    /* Give default value to superclass if it is not specified */
    if (cfg->superclass == NULL)
    {
	switch (cfg->modtype)
	{
	case MUI:
	case MCP:
	case MCC:
	    cfg->superclass = "MUIC_Area";
	    break;
	case GADGET:
	    cfg->superclass = "GADGETCLASS";
	    break;
	case DATATYPE:
	    cfg->superclass = "DATATYPESCLASS";
	    break;
	default:
	    break;
	}
    }
}

static void readsectioncdef(struct config *cfg)
{
    int atend = 0;
    char *line, *s;
    
    while (!atend)
    {
	line = readline();
	if (line==NULL)
	    exitfileerror(20, "unexptected end of file in section cdef\n");

	if (strncmp(line, "##", 2)!=0)
	{
	    slist_append(&cfg->cdeflines, line);
	}
	else
	{
	    s = line+2;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "end", 3)!=0)
		exitfileerror(20, "\"##end cdef\" expected\n");

	    s += 3;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "cdef", 4)!=0)
		exitfileerror(20, "\"##end cdef\" expected\n");

	    s += 5;
	    while (isspace(*s)) s++;
	    if (*s!='\0')
		exitfileerror(20, "unexpected character at position %d\n");

	    atend = 1;
	}
    }
}

static void readsectioncdefprivate(struct config *cfg)
{
    int atend = 0;
    char *line, *s;
    
    while (!atend)
    {
	line = readline();
	if (line==NULL)
	    exitfileerror(20, "unexptected end of file in section cdef\n");

	if (strncmp(line, "##", 2)!=0)
	{
	    slist_append(&cfg->cdefprivatelines, line);
	}
	else
	{
	    s = line+2;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "end", 3)!=0)
		exitfileerror(20, "\"##end cdefprivate\" expected\n");

	    s += 3;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "cdefprivate", 11)!=0)
		exitfileerror(20, "\"##end cdefprivate\" expected\n");

	    s += 11;
	    while (isspace(*s)) s++;
	    if (*s!='\0')
		exitfileerror(20, "unexpected character at position %d\n");

	    atend = 1;
	}
    }
}

static void readsectionfunctionlist(struct config *cfg, struct functions *functions)
{
    int atend = 0, i;
    char *line, *s, *s2;
    unsigned int lvo = cfg->firstlvo;
    struct functionhead **funclistptr = &functions->funclist;
    
    if (cfg->basename==NULL)
	exitfileerror(20, "section functionlist has to come after section config\n");

    cfg->intcfg |= CFG_NOREADREF;
    
    while (!atend)
    {
	line = readline();
	if (line==NULL)
	    exitfileerror(20, "unexptected EOF in functionlist section\n");
	if (strlen(line)==0)
	{
	    if (*funclistptr != NULL)
		funclistptr = &((*funclistptr)->next);
	    lvo++;
	}
	else if (isspace(*line))
	{
	    s = line;
	    while (isspace(*s)) s++;
	    if (*s=='\0')
	    {
		if (*funclistptr != NULL)
		    funclistptr = &((*funclistptr)->next);
		lvo++;
	    }
	    else
		exitfileerror(20, "no space allowed before functionname\n");
	}
	else if (strncmp(line, "##", 2)==0)
	{
	    s = line+2;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "end", 3)!=0)
		exitfileerror(20, "\"##end functionlist\" expected\n");

	    s += 3;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "functionlist", 12)!=0)
		exitfileerror(20, "\"##end functionlist\" expected\n");

	    s += 12;
	    while (isspace(*s)) s++;
	    if (*s!='\0')
		exitfileerror(20, "unexpected character on position %d\n", s-line);

	    atend = 1;
	}
	else if (*line=='.')
	{
	    s = line+1;
	    if (strncmp(s, "skip", 4)==0)
	    {
		int n;
		
		s += 4;
		if (!isspace(*s))
		    exitfileerror(20, "syntax is '.skip n'\n");
		
		n=strtol(s, &s2, 10);
		if (s2==NULL)
		    exitfileerror(20, "positive number expected\n");
		
		while (isspace(*s2)) s2++;
		if (*s2!='\0')
		    exitfileerror(20, "syntax is '.skip n'\n");
		if (*funclistptr != NULL)
		    funclistptr = &((*funclistptr)->next);
		lvo += n;
	    }
	    else if (strncmp(s, "alias", 5)==0)
	    {
		s += 5;
		
		if (!isspace(*s))
		    exitfileerror(20, "syntax is '.alias name'\n");

		while (isspace(*s)) s++;
		if (*s == '\0' || !(isalpha(*s) || *s == '_'))
		    exitfileerror(20, "syntax is '.alias name'\n");

		s2 = s;
		s++;
		while (isalnum(*s) || *s == '_') s++;

		if (isspace(*s))
		{
		    *s = '\0';
		    do {
			s++;
		    } while (isspace(*s));
		}

		if (*s != '\0')
		    exitfileerror(20, "syntax is '.alias name'\n");
		
		if (*funclistptr == NULL)
		    exitfileerror(20, ".alias has to come after a function declaration\n");
		
		slist_append(&(*funclistptr)->aliases, s2);
		cfg->intcfg |= CFG_GENASTUBS;
	    }
	    else if (strncmp(s, "cfunction", 9)==0)
	    {
		if (*funclistptr == NULL)
		    exitfileerror(20, ".cfunction has to come after a function declaration\n");
		
		(*funclistptr)->libcall = REGISTER;
	    }
	    else if (strncmp(s, "private", 7)==0)
	    {
		if (*funclistptr == NULL)
		    exitfileerror(20, ".private has to come after a function declaration\n");
		
		(*funclistptr)->priv = 1;
	    }
	    else if (strncmp(s, "novararg", 8)==0)
	    {
		if (*funclistptr == NULL)
		    exitfileerror(20, ".novararg has to come after a function declaration\n");
		
		(*funclistptr)->novararg = 1;
	    }
	    else
		exitfileerror(20, "Syntax error");
	}
	else if (*line!='#') /* Ignore line that is a comment, e.g. that starts with a # */
	{
	    /* The line is a function prototype. It can have two syntax
	     * type funcname(argproto1, argproto2, ...)
	     * type funcname(argproto1, argproto2, ...) (reg1, reg2, ...)
	     * The former is for C type function argument passing, the latter for
	     * register argument passing.
	     */
	    char c, *args[64], *regs[64], *funcname;
	    int len, argcount = 0, regcount = 0, brcount = 0;

	    /* Parse 'type functionname' at the beginning of the line */
	    s = strchr(line, '(');
	    if (s == NULL)
		exitfileerror(20, "( expected at position %d\n", strlen(line) + 1);
	    
	    s2 = s;
	    while (isspace(*(s2-1)))
		s2--;
	    *s2 = '\0';
	    
	    while (s2 > line && !isspace(*(s2-1)) && !(*(s2-1) == '*'))
		s2--;
	    
	    if (s2 == line)
		exitfileerror(20, "No type specifier before function name\n");
	    
	    if (*funclistptr != NULL)
		funclistptr = &((*funclistptr)->next);
	    *funclistptr = newfunctionhead(s2, STACK);
	    
	    while (isspace(*(s2-1)))
		s2--;
	    *s2 = '\0';
	    (*funclistptr)->type = strdup(line);
	    (*funclistptr)->lvo = lvo;
	    lvo++;

	    /* Parse function prototype */
	    s++;
	    while (isspace(*s))
		s++;
	    c = *s;
		
	    while (c != ')')
	    {
		while (isspace(*s))
		    s++;
		
		args[argcount] = s;
		argcount++;
		
		while
		(
		    *s != '\0'
		    && !(brcount == 0 && (*s == ',' || *s == ')'))
		)
		{
		    if (*s == '(')
			brcount++;
		    if (*s == ')')
		    {
			if (brcount > 0)
			    brcount--;
			else
			    exitfileerror(20, "Unexected ')' at position %d\n", s-line+1);
		    }
		    s++;
		}

		c = *s;
		if (c == '\0')
		    exitfileerror(20, "'(' without ')'");
			
		s2 = s;
		while (isspace(*(s2-1)))
		    s2--;
		*s2 = '\0';

		if (!(s2 > args[argcount - 1]))
		    exitfileerror(20, "Syntax error in function prototype\n");

		s++;
	    }

	    s++;
	    while (*s != '\0' && isspace(*s))
		s++;

	    if (*s == '(')
	    {
		/* Parse registers specifications if available otherwise this prototype for C type argument passing */

		/* There may be no register specified with () so be sure then c is == ')' */
		s++;
		while(isspace(*s))
		    s++;
		
		c = *s;

		while (c != ')')
		{
		    while (isspace(*s))
			s++;

		    regs[regcount] = s;
		    regcount++;
		    
		    if (memchr("AD",s[0],2)!=NULL && memchr("01234567",s[1],8)!=NULL)
		    {
			s += 2;
			c = *s;
			*s = '\0';
		    }
		    else
			exitfileerror(20,
				      "wrong register \"%s\" for argument %u\n",
				      regs[regcount-1], regcount
			);

		    while (isspace(c))
		    {
			s++;
			c = *s;
		    }
		    if (c == '\0')
			exitfileerror(20, "'(' without ')'\n");
		    if (c != ',' && c != ')')
			exitfileerror(20, "',' or ')' expected at position %d\n", s-line+1);
		    
		    s++;
		}

		s++;
		while (isspace(*s)) s++;
		if (*s!='\0')
		    exitfileerror(20, "wrong char '%c' at position %d\n", *s, (int)(s-line) + 1);

		if (argcount != regcount)
		    exitfileerror(20, "Number of arguments (%d) and registers (%d) mismatch\n",
				  argcount, regcount
		    );

		(*funclistptr)->libcall = REGISTERMACRO;
		for (i = 0; i < argcount; i++)
		    funcaddarg(*funclistptr, args[i], regs[i]);
	    } 
	    else if (*s == '\0')
	    { /* No registers specified */
		for (i = 0; i < argcount; i++)
		    funcaddarg(*funclistptr, args[i], NULL);
		cfg->intcfg |= CFG_GENASTUBS;
	    }
	    else
		exitfileerror(20, "wrong char '%c' at position %d\n", *s, (int)(s-line) + 1);
	}
    }
}

static void readsectionmethodlist(struct config *cfg, struct functions *functions)
{
    int atend = 0, i;
    char *line, *s, *s2;
    unsigned int lvo = cfg->firstlvo;
    struct functionhead **methlistptr = &functions->methlist;
    
    if (cfg->basename==NULL)
	exitfileerror(20, "section methodlist has to come after section config\n");

    cfg->intcfg |= CFG_NOREADREF;
    
    while (!atend)
    {
	line = readline();
	if (line==NULL)
	    exitfileerror(20, "unexptected EOF in methodlist section\n");
	
	/* Ignore empty lines or lines that qre a comment, e.g. that starts with a # */
	if (strlen(line)==0 || (line[0] == '#' && line[1] != '#'))
	    continue;

	if (isspace(*line))
	    exitfileerror(20, "No space allowed at start of the line\n");

	if (strncmp(line, "##", 2)==0) /* Is this the end ? */
	{
	    s = line+2;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "end", 3)!=0)
		exitfileerror(20, "\"##end methodlist\" expected\n");

	    s += 3;
	    while (isspace(*s)) s++;
	    if (strncmp(s, "methodlist", 10)!=0)
		exitfileerror(20, "\"##end methodlist\" expected\n");

	    s += 10;
	    while (isspace(*s)) s++;
	    if (*s!='\0')
		exitfileerror(20, "unexpected character on position %d\n", s-line);

	    atend = 1;
	    
	    continue;
	}

	if (*line=='.')
	{
	    s = line+1;
	    if (strncmp(s, "alias", 5)==0)
	    {
		s += 5;
		
		if (!isspace(*s))
		    exitfileerror(20, "syntax is '.alias name'\n");

		while (isspace(*s)) s++;
		if (*s == '\0' || !(isalpha(*s) || *s == '_'))
		    exitfileerror(20, "syntax is '.alias name'\n");

		s2 = s;
		s++;
		while (isalnum(*s) || *s == '_') s++;

		if (isspace(*s))
		{
		    *s = '\0';
		    do {
			s++;
		    } while (isspace(*s));
		}

		if (*s != '\0')
		    exitfileerror(20, "syntax is '.alias name'\n");
		
		if (*methlistptr == NULL)
		    exitfileerror(20, ".alias has to come after a function declaration\n");
		
		slist_append(&(*methlistptr)->aliases, s2);
		cfg->intcfg |= CFG_GENASTUBS;
	    }
	    else
		exitfileerror(20, "Syntax error");
	}
	else if (isalpha(*line))
	{
	    for (s = line + 1; isalnum(*s) || *s == '_'; s++)
		;
	
	    if (*s != '\0')
		exitfileerror(20, "Only letters, digits and an underscore allowed in a methodname\n");

	    if (*methlistptr != NULL)
		methlistptr = &((*methlistptr)->next);
	    *methlistptr = newfunctionhead(line, STACK);
	    (*methlistptr)->type = "IPTR";
	    funcaddarg(*methlistptr, "Class *cl", NULL);
	    funcaddarg(*methlistptr, "Object *o", NULL);
	    funcaddarg(*methlistptr, "Msg msg", NULL);
	}
	else
	    exitfileerror(20, "Methodname has to begin with a letter\n");
    }
}
