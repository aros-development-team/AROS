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
    "Usage: genmodule [-c conffile] [-s suffix] [-d gendir]\n"
    "       {writefiles|writemakefile|writeincludes|writedummy|writelibdefs} modname modtype\n"
;

static void readconfig(struct config *);
static struct conffuncinfo *newconffuncinfo(const char *name, unsigned int lvo);


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
struct config *initconfig(int argc, char **argv)
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
	cfg->genlinklib = 1;
	cfg->moddir = "Libs";
    }
    else if (strcmp(argv[optind+2],"mcc")==0)
    {
    	cfg->modtype = MCC;
	cfg->genlinklib = 0;
	cfg->moddir = "Classes/Zune";
    }
    else if (strcmp(argv[optind+2],"mui")==0)
    {
    	cfg->modtype = MUI;
	cfg->genlinklib = 0;
	cfg->moddir = "Classes/Zune";
    }
    else if (strcmp(argv[optind+2],"mcp")==0)
    {
    	cfg->modtype = MCP;
	cfg->genlinklib = 0;
	cfg->moddir = "Classes/Zune";
    }
    else if (strcmp(argv[optind+2], "device")==0)
    {
	cfg->modtype = DEVICE;
	cfg->genlinklib = 1;
	cfg->moddir = "Devs";
    }
    else if (strcmp(argv[optind+2], "resource")==0)
    {
	cfg->modtype = RESOURCE;
	cfg->genlinklib = 1;
	cfg->moddir = "Devs";
    }
    else if (strcmp(argv[optind+2], "gadget")==0)
    {
	cfg->modtype = GADGET;
	cfg->genlinklib = 0;
	cfg->moddir = "Classes/Gadgets";
    }
    else if (strcmp(argv[optind+2], "datatype")==0)
    {
	cfg->modtype = DATATYPE;
	cfg->genlinklib = 0;
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
	
	if (cfg->command != FILES && cfg->command != INCLUDES)
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
    
    readconfig(cfg);
    
    return cfg;
}

/* Functions to read configuration from the configuration file */

#include "fileread.h"

static void readsectionconfig(struct config *);
static void readsectioncdef(struct config *);
static void readsectioncdefprivate(struct config *);
static void readsectionfunctionlist(struct config *);

static void readconfig(struct config *cfg)
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
	    static char *parts[] = {"config", "cdefprivate", "cdef", "functionlist"};
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
		readsectionfunctionlist(cfg);
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
		"rootbase_field", "classptr_field", "classname", "classdatatype"
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
    
    /* Givw default value to superclass if it is not specified */
    if (cfg->superclass == NULL)
    {
	switch (cfg->modtype)
	{
	case MUI:
	case MCP:
	case MCC:
	    cfg->superclass = "Area.mui";
	    break;
	case GADGET:
	    cfg->superclass = "gadgetclass";
	    break;
	case DATATYPE:
	    cfg->superclass = "datatypesclass";
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

static void readsectionfunctionlist(struct config *cfg)
{
    int atend = 0;
    char *line, *s, *s2;
    unsigned int lvo = cfg->firstlvo;
    struct conffuncinfo **funclistptr = &cfg->conffunclist;
    
    if (cfg->basename==NULL)
	exitfileerror(20, "section functionlist has to come after section config\n");

    while (!atend)
    {
	line = readline();
	if (line==NULL)
	    exitfileerror(20, "unexptected EOF in functionlist section\n");
	if (strlen(line)==0)
	    lvo++;
	else if (isspace(*line))
	{
	    s = line;
	    while (isspace(*s)) s++;
	    if (*s=='\0')
		lvo++;
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
		    exitfileerror(20, "in syntax\n");
		
		n=strtol(s, &s2, 10);
		if (s2==NULL)
		    exitfileerror(20, "positive number expected\n");
		
		while (isspace(*s2)) s2++;
		if (*s2!='\0')
		    exitfileerror(20, "in syntax\n");
		lvo += n;
	    }
	}
	else if (*line!='#')
	{
	    char *sopenbracket, *sclosebracket, *scolon;
	    int len;

	    sopenbracket = strchr(line,'(');
	    sclosebracket = strchr(line,')');
	    scolon = strchr(line,':');
	    
	    /* Duplicate the function name */
	    for (len = 0;
		 line[len] != '\0' && !isspace(line[len]) && line[len] != ':' && line[len] != '(';
		 len++
	    )
		/*NOP*/;

	    line[len] = '\0';

	    *funclistptr = newconffuncinfo(line, lvo);
	    lvo++;

	    /* Parse registers specifications if available */
	    if (sopenbracket != NULL && (scolon == NULL || scolon > sopenbracket))
	    {
		(*funclistptr)->regcount = 0;
		
		if (sclosebracket == NULL)
		    exitfileerror(20, "'(' without ')'");
		
		*sopenbracket='\0';
		*sclosebracket='\0';
		
		s = sopenbracket+1;
		while (isspace(*s)) s++;
		while (*s!='\0')
		{
		    *s = toupper(*s);
		    if (memchr("AD",s[0],2)!=NULL && memchr("01234567",s[1],8)!=NULL)
		    {
			char c = s[2];

			s[2] = '\0';
			slist_append(&(*funclistptr)->regs, s);
			(*funclistptr)->regcount++;
			s[2] = c;
		    }
		    else
			exitfileerror(20,
				      "wrong register \"%s\" for argument %u\n",
				      s, (*funclistptr)->regcount+1
			);
		    
		    s += 2;
		    while (isspace(*s)) s++;
		    if (*s == ',')
			s++;
		    else if (*s != '\0')
			exitfileerror(20, "wrong char %c at position %d\n", *s, (int)(s-line) + 1);
		    
		    while(isspace(*s)) s++;
		}
	    }

	    
	    /* Parse extra specification, like aliases, vararg, ... */
	    if (scolon != NULL)
	    {
		s = scolon+1;
		while (isspace(*s)) s++;
		do
		{
		    if (strncmp(s, "alias", 5) == 0)
		    {
			char c;
			s+=5;
			while (isspace(*s)) s++;
			if (*s != '(')
			    exitfileerror(20, "Wrong format for alias: alias(name) is the right form\n");
			s++;
			while (isspace(*s)) s++;
			s2 = s;
			while (!isspace(*s2) && *s2!=')') s2++;
			
			c = *s2;
			*s2 = '\0';
			slist_append(&(*funclistptr)->aliases, s);
			*s2 = c;
			
			s = s2;
			while (isspace(*s)) s++;

			if (*s != ')')
			    exitfileerror(20, "'(' without a ')'");
			s++;
		    }
		    else
			exitfileerror(20, "Unknown option for function\n");
		    
		    while (isspace(*s)) s++;
		} while (*s != '\0');
	    }
	    
	    funclistptr = &((*funclistptr)->next);
	}
    }
}

static struct conffuncinfo *newconffuncinfo(const char *name, unsigned int lvo)
{
    struct conffuncinfo *conffuncinfo = malloc(sizeof(struct conffuncinfo));
    
    if (conffuncinfo == NULL)
    {
	fprintf(stderr, "Out of memory !\n");
	exit (20);
    }
    
    conffuncinfo->next = NULL;
    conffuncinfo->name = strdup(name);
    conffuncinfo->lvo = lvo;
    conffuncinfo->regcount = -1;
    conffuncinfo->regs = NULL;
    conffuncinfo->aliases = NULL;

    return conffuncinfo;
}
