/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Code to parse the command line options and the module config file for
    the genmodule program
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define __USE_XOPEN
#include <time.h>
#include <unistd.h>
#include <limits.h>

#include "functionhead.h"
#include "config.h"

const static char bannertemplate[] =
    "/*\n"
    "    *** Automatically generated from '%s'. Edits will be lost. ***\n"
    "    Copyright � 1995-%4u, The AROS Development Team. All rights reserved.\n"
    "*/\n";

char*
getBanner(struct config* config)
{
    static unsigned currentyear = 0;

    int bannerlength = strlen(config->conffile) + strlen(bannertemplate);
    // No additional bytes needed because
    // + 1 (NUL) + 4 (4 digit year) - strlen("%s") - strlen("%4u) = 0

    char * banner = malloc(bannerlength);

    if (currentyear == 0)
    {
        time_t rawtime;
        time(&rawtime);
        struct tm *utctm = gmtime(&rawtime);
        currentyear = utctm->tm_year + 1900;
    }

    snprintf (banner, bannerlength, bannertemplate, config->conffile, currentyear);

    return(banner);
}

void
freeBanner(char *banner)
{
    free((void *)banner);
}

const static char usage[] =
    "\n"
    "Usage: genmodule [-c conffile] [-s suffix] [-d gendir] [-v versionextra]\n"
    "       {writefiles|writemakefile|writeincludes|writelibdefs|writefunclist|writefd|writeskel} modname modtype\n"
;

static void readconfig(struct config *);
static struct classinfo *newclass(struct config *);
static struct handlerinfo *newhandler(struct config *);
static struct interfaceinfo *newinterface(struct config *);

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

    while ((c = getopt(argc, argv, ":c:s:d:v:")) != -1)
    {
        if (c == ':')
        {
            fprintf(stderr, "Option -%c needs an argument\n",optopt);
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

        case 'v':
            cfg->versionextra = optarg;
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
    else if (strcmp(argv[optind], "writefunclist") == 0)
    {
        cfg->command = WRITEFUNCLIST;
    }
    else if (strcmp(argv[optind], "writefd") == 0)
    {
        cfg->command = WRITEFD;
    }
    else if (strcmp(argv[optind], "writeskel") == 0)
    {
        cfg->command = WRITESKEL;
    }
    else
    {
        fprintf(stderr, "Unrecognized argument \"%s\"\n%s", argv[optind], usage);
        exit(20);
    }

    cfg->modulename = argv[optind+1];
    cfg->modulenameupper = strdup(cfg->modulename);
    for (s=cfg->modulenameupper; *s!='\0'; *s = toupper(*s), s++) {
        if (!isalnum(*s)) *s = '_';
    }

    if (strcmp(argv[optind+2],"library")==0)
    {
        cfg->modtype = LIBRARY;
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
        cfg->moddir = "Devs";
    }
    else if (strcmp(argv[optind+2], "resource")==0)
    {
        cfg->modtype = RESOURCE;
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
        cfg->moddir = "Classes/DataTypes";
    }
    else if (strcmp(argv[optind+2], "usbclass")==0)
    {
        cfg->modtype = USBCLASS;
        cfg->moddir = "Classes/USB";
        if(!hassuffix)
        {
            cfg->suffix = "class";
            hassuffix = 1;
        }
    }
    else if (strcmp(argv[optind+2], "hidd")==0)
    {
        cfg->modtype = HIDD;
        cfg->moddir = "Devs/Drivers";
    }
    else if (strcmp(argv[optind+2], "handler")==0)
    {
        cfg->modtype = HANDLER;
        cfg->moddir = "$(AROS_DIR_FS)";
    }
    else if (strcmp(argv[optind+2], "hook")==0)
    {
        cfg->modtype = HANDLER;
        cfg->moddir = "Devs";
    }
    else
    {
        fprintf(stderr, "Unknown modtype \"%s\" specified for second argument\n", argv[optind+2]);
        exit(20);
    }
    cfg->modtypestr = argv[optind+2];

    if (!hassuffix)
        cfg->suffix = argv[optind+2];

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
    }

    readconfig(cfg);

    /* For a device add the functions given in beginiofunc and abortiofunc to the functionlist
     * if they are provided
     */
    if (cfg->beginiofunc != NULL)
    {
        struct functionhead *funchead;

        cfg->intcfg |= CFG_NOREADFUNCS;

        /* Add beginio_func to the list of functions */
        funchead = newfunctionhead(cfg->beginiofunc, REGISTERMACRO);
        funchead->type = strdup("void");
        funchead->lvo = 5;
        funcaddarg(funchead, "struct IORequest *ioreq", "A1");

        funchead->next = cfg->funclist;
        cfg->funclist = funchead;

        /* Add abortio_func to the list of functions */
        funchead = newfunctionhead(cfg->abortiofunc, REGISTERMACRO);
        funchead->type = strdup("LONG");
        funchead->lvo = 6;
        funcaddarg(funchead, "struct IORequest *ioreq", "A1");

        funchead->next = cfg->funclist->next;
        cfg->funclist->next = funchead;
    }
    else if (cfg->modtype == DEVICE && cfg->intcfg & CFG_NOREADFUNCS)
    {
        fprintf
        (
            stderr,
            "beginio_func and abortio_func missing for a device with a non empty function list\n"
        );
        exit(20);
    }

    /* See if we have any stackcall options */
    if (cfg->funclist) {
        struct functionhead *funchead;

        for (funchead = cfg->funclist; funchead; funchead = funchead->next) {
            if (funchead->libcall == STACK) {
                cfg->options |= OPTION_STACKCALL;
                break;
            }
        }
    }

    /* Provide default version for functions that didnt have it */
    if (cfg->funclist) {
        struct functionhead *funchead;
        int defversion = cfg->majorversion; /* Assume library version is default */

        for (funchead = cfg->funclist; funchead; funchead = funchead->next) {
            if (funchead->version > -1) {
                /* There was at least one .version tag. Assume 0 is default */
                defversion = 0;
                break;
            }
        }

        for (funchead = cfg->funclist; funchead; funchead = funchead->next) {
            if (funchead->version == -1) {
                funchead->version = defversion;
            }
        }
    }

    /* Verify that a handler has a handler */
    if (cfg->modtype == HANDLER) {
        if (cfg->handlerfunc == NULL) {
            fprintf(stderr, "handler modules require a 'handler_func' ##config option\n");
            exit(20);
        }
        cfg->options |= OPTION_NOAUTOLIB | OPTION_NOEXPUNGE | OPTION_NOOPENCLOSE;
    }

    return cfg;
}

/* Functions to read configuration from the configuration file */

#include "fileread.h"

static char *readsections(struct config *, struct classinfo *cl, struct interfaceinfo *in, int inclass);
static void readsectionconfig(struct config *, struct classinfo *cl, struct interfaceinfo *in, int inclass);
static void readsectioncdef(struct config *);
static void readsectioncdefprivate(struct config *);
static void readsectionstartup(struct config *);
static void readsectionfunctionlist(const char *type, struct functionhead **funclistptr, unsigned int firstlvo, int isattribute, enum libcall def_libcall);
static void readsectionclass_methodlist(struct classinfo *);
static void readsectionclass(struct config *);
static void readsectionhandler(struct config *);
static void readsectioninterface(struct config *);

static void readconfig(struct config *cfg)
{
    struct classinfo *mainclass = NULL;

    /* Create a classinfo structure if this module is a class */
    switch (cfg->modtype)
    {
    case LIBRARY:
    case DEVICE:
    case RESOURCE:
    case USBCLASS:
    case HANDLER:
        break;

    case MCC:
    case MUI:
    case MCP:
    case GADGET:
    case DATATYPE:
    case HIDD:
        mainclass = newclass(cfg);
        mainclass->classtype = cfg->modtype;
        break;

    default:
        fprintf(stderr, "Internal error: unsupported modtype for classinfo creation\n");
        exit(20);
    }

    switch (cfg->modtype)
    {
    case LIBRARY:
    case USBCLASS:
        cfg->firstlvo = 5;
        break;
    case DEVICE:
        cfg->firstlvo = 7;
        break;
    case MCC:
    case MUI:
    case MCP:
        cfg->firstlvo = 6;
        mainclass->boopsimprefix = muimprefix;
        break;
    case HANDLER:
    case RESOURCE:
        cfg->firstlvo = 1;
        break;
    case GADGET:
        cfg->firstlvo = 5;
        mainclass->boopsimprefix = gadgetmprefix;
        break;
    case DATATYPE:
        cfg->firstlvo = 6;
        mainclass->boopsimprefix = dtmprefix;
        break;
    case HIDD:
        cfg->firstlvo = 5;
        /* FIXME: need boopsimprefix ? */
        break;
    default:
        fprintf(stderr, "Internal error: unsupported modtype for firstlvo\n");
        exit(20);
    }

    if (!fileopen(cfg->conffile))
    {
        fprintf(stderr, "In readconfig: Could not open %s\n", cfg->conffile);
        exit(20);
    }

    /* Read all sections and see that we are at the end of the file */
    if (readsections(cfg, mainclass, NULL, 0) != NULL)
        exitfileerror(20, "Syntax error");

    fileclose();
}

/* readsections will scan through all the sections in the config file.
 * arguments:
 * struct config *cfg: The module config data which may be updated by
 *     the information in the sections
 * struct classinfo *cl: The classdata to be filled with data from the sections.
 *     This may be NULL if this is the main part of the configuration file and the
 *     type of the module is not a class
 * int inclass: Boolean to indicate if we are in a class part. If not we are in the main
 *     part of the config file.
 */
static char *readsections(struct config *cfg, struct classinfo *cl, struct interfaceinfo *in, int inclass)
{
    char *line, *s, *s2;
    int hasconfig = 0;

    while ((line=readline())!=NULL)
    {
        if (strncmp(line, "##", 2)==0)
        {
            static char *parts[] =
            {
                "config", "cdefprivate", "cdef", "startup", "functionlist", "methodlist", "class", "handler", "interface", "attributelist", "cfunctionlist"
            };
            const unsigned int nums = sizeof(parts)/sizeof(char *);
            unsigned int partnum;
            int i, atend = 0;

            s = line+2;
            while (isspace(*s)) s++;

            if (strncmp(s, "begin", 5)!=0)
                return line;

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
                readsectionconfig(cfg, cl, in, inclass);
                hasconfig = 1;
                break;

            case 2: /* cdefprivate */
                if (inclass)
                    exitfileerror(20, "cdefprivate section not allowed in class section\n");
                readsectioncdefprivate(cfg);
                break;

            case 3: /* cdef */
                if (inclass)
                    exitfileerror(20, "cdef section not allowed in class section\n");
                readsectioncdef(cfg);
                break;

            case 4: /* startup */
                if (inclass)
                    exitfileerror(20, "startup section not allowed in class section\n");
                readsectionstartup(cfg);
                break;

            case 5: /* functionlist */
                if (inclass)
                    exitfileerror(20, "functionlist section not allow in class section\n");
                if (cfg->basename==NULL)
                    exitfileerror(20, "section functionlist has to come after section config\n");

                readsectionfunctionlist("functionlist", &cfg->funclist, cfg->firstlvo, 0, REGISTERMACRO);
                cfg->intcfg |= CFG_NOREADFUNCS;
                break;

            case 6: /* methodlist */
                if (cl == NULL && in == NULL)
                    exitfileerror(20, "methodlist section when not in a class or interface\n");
                if (cl)
                    readsectionclass_methodlist(cl);
                else
                    readsectionfunctionlist("methodlist", &in->methodlist, 0, 0, REGISTERMACRO);
                cfg->intcfg |= CFG_NOREADFUNCS;
                break;

            case 7: /* class */
                if (inclass)
                    exitfileerror(20, "class section may not be in nested\n");
                readsectionclass(cfg);
                break;
            case 8: /* handler */
                readsectionhandler(cfg);
                break;
            case 9: /* interface */
                if (inclass)
                    exitfileerror(20, "interface section may not be nested\n");
                readsectioninterface(cfg);
                break;
            case 10: /* attributelist */
                if (!in)
                    exitfileerror(20, "attributelist only valid in interface sections\n");
                readsectionfunctionlist("attributelist", &in->attributelist, 0, 1, INVALID);
                break;
        case 11: /* cfunctionlist */
            if (inclass)
                exitfileerror(20, "cfunctionlist section not allow in class section\n");
                    if (cfg->basename==NULL)
                        exitfileerror(20, "section cfunctionlist has to come after section config\n");

            readsectionfunctionlist("cfunctionlist", &cfg->funclist, cfg->firstlvo, 0, REGISTER);
            cfg->intcfg |= CFG_NOREADFUNCS;
            break;
            }
        }
        else if (strlen(line)!=0)
            filewarning("warning line outside section ignored\n");
    }

    if(!inclass)
    {
        if (!hasconfig)
            exitfileerror(20, "No config section in conffile\n");

        /* If no indication was given for generating includes or not
           decide on module type and if there are functions
         */
        if(!((cfg->options & OPTION_INCLUDES) || (cfg->options & OPTION_NOINCLUDES)))
        {
            switch (cfg->modtype)
            {
            case LIBRARY:
            case RESOURCE:
                cfg->options |= OPTION_INCLUDES;
                break;

            case HANDLER:
            case MCC:
            case MUI:
            case MCP:
            case USBCLASS:
                cfg->options |= OPTION_NOINCLUDES;
                break;

            case DEVICE:
                cfg->options |= (
                    (cfg->funclist != NULL)
                    || (cfg->cdeflines != NULL)
                    || strcmp(cfg->libbasetypeptrextern, "struct Device *") != 0
                ) ? OPTION_INCLUDES : OPTION_NOINCLUDES;
                break;

            case GADGET:
            case DATATYPE:
            case HIDD:
                cfg->options |= (
                    (cfg->funclist != NULL)
                ) ? OPTION_INCLUDES : OPTION_NOINCLUDES;
                break;

            default:
                fprintf(stderr, "Internal error writemakefile: unhandled modtype for includes\n");
                exit(20);
                break;
            }
        }

        /* If no indication was given for not generating stubs only generate them if
         * the module has functions
         */
        if(!((cfg->options & OPTION_STUBS) || (cfg->options & OPTION_NOSTUBS)))
        {
            switch (cfg->modtype)
            {
            case LIBRARY:
                cfg->options |= (cfg->funclist != NULL) ? OPTION_STUBS : OPTION_NOSTUBS;
                break;

            case USBCLASS:
            case RESOURCE:
            case GADGET:
            case DEVICE:
            case DATATYPE:
            case MCC:
            case MUI:
            case MCP:
            case HIDD:
            case HANDLER:
                cfg->options |= OPTION_NOSTUBS;
                break;

            default:
                fprintf(stderr, "Internal error writemakefile: unhandled modtype for stubs\n");
                exit(20);
                break;
            }
        }

        /* If no indication was given for generating autoinit code or not
           decide on module type
         */
        if(!((cfg->options & OPTION_AUTOINIT) || (cfg->options & OPTION_NOAUTOINIT)))
        {
            switch (cfg->modtype)
            {
            case LIBRARY:
                cfg->options |= OPTION_AUTOINIT;
                break;

            case USBCLASS:
            case RESOURCE:
            case GADGET:
            case DEVICE:
            case DATATYPE:
            case MCC:
            case MUI:
            case MCP:
            case HIDD:
            case HANDLER:
                cfg->options |= OPTION_NOAUTOINIT;
                break;

            default:
                fprintf(stderr, "Internal error writemakefile: unhandled modtype for autoinit\n");
                exit(20);
                break;
            }
        }

        if ((cfg->modtype == RESOURCE) || (cfg->modtype == HANDLER))
            /* Enforce noopenclose for resources and handlers */
            cfg->options |= OPTION_NOOPENCLOSE;
        else if (!(cfg->options & OPTION_SELFINIT))
            /* Enforce using RTF_AUTOINIT for everything except resources */
            cfg->options |= OPTION_RESAUTOINIT;
    }

    return NULL;
}

static void readsectionconfig(struct config *cfg, struct classinfo *cl, struct interfaceinfo *in, int inclass)
{
    int atend = 0, i;
    char *line, *s, *s2, *libbasetypeextern = NULL;
    struct tm date;

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
                "version", "date", "copyright", "libcall", "forcebase", "superclass",
                "superclass_field", "residentpri", "options", "sysbase_field",
                "seglist_field", "rootbase_field", "classptr_field", "classptr_var",
                "classid", "classdatatype", "beginio_func", "abortio_func", "dispatcher",
                "initpri", "type", "addromtag", "oopbase_field",
                "rellib", "interfaceid", "interfacename",
                "methodstub", "methodbase", "attributebase", "handler_func",
                "includename"
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
                if (!inclass)
                    cfg->basename = strdup(s);
                if (cl != NULL)
                    cl->basename = strdup(s);
                if (in != NULL)
                    exitfileerror(20, "basename not valid config option when in an interface section\n");
                break;

            case 2: /* libbase */
                if (inclass)
                    exitfileerror(20, "libbase not valid config option when in a class section\n");
                cfg->libbase = strdup(s);
                break;

            case 3: /* libbasetype */
                if (inclass)
                    exitfileerror(20, "libbasetype not valid config option when in a class section\n");
                cfg->libbasetype = strdup(s);
                break;

            case 4: /* libbasetypeextern */
                if (inclass)
                    exitfileerror(20, "libbasetype not valid config option when in a class section\n");
                libbasetypeextern = strdup(s);
                break;

            case 5: /* version */
                if (inclass)
                    exitfileerror(20, "version not valid config option when in a class section\n");
                if (sscanf(s, "%u.%u", &cfg->majorversion, &cfg->minorversion)!=2)
                    exitfileerror(20, "wrong version string \"%s\"\n", s);
                break;

            case 6: /* date */
                if (inclass)
                    exitfileerror(20, "date not valid config option when in a class section\n");
#ifndef _WIN32
                if (strptime(s, "%e.%m.%Y", &date) == NULL)
                {
                    exitfileerror(20, "date string has to have d.m.yyyy format\n");
                }
#endif
                cfg->datestring = strdup(s);
                break;

            case 7: /* copyright */
                if (inclass)
                    exitfileerror(20, "copyright not valid config option when in a class section\n");
                cfg->copyright = strdup(s);
                break;

            case 8: /* libcall */
                fprintf(stderr, "libcall specification is deprecated and ignored\n");
                break;

            case 9: /* forcebase */
                if (inclass)
                    exitfileerror(20, "forcebase not valid config option when in a class section\n");
                slist_append(&cfg->forcelist, s);
                break;

            case 10: /* superclass */
                if (cl == NULL)
                    exitfileerror(20, "superclass specified when not a BOOPSI class\n");
                cl->superclass = strdup(s);
                break;

            case 11: /* superclass_field */
                if (cl == NULL)
                    exitfileerror(20, "superclass_field specified when not a BOOPSI class\n");
                cl->superclass_field = strdup(s);
                break;

            case 12: /* residentpri */
                if (!inclass)
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
                else
                    exitfileerror(20, "residentpri not valid config option when in a class section\n");
                break;

            case 13: /* options */
                if (!inclass)
                {
                    static const char *optionnames[] =
                    {
                        "noautolib", "noexpunge", "noresident", "peropenerbase",
                        "pertaskbase", "includes", "noincludes", "nostubs",
                        "autoinit", "noautoinit", "resautoinit", "noopenclose",
                        "selfinit"
                    };
                    const unsigned int optionnums = sizeof(optionnames)/sizeof(char *);
                    int optionnum;

                    do
                    {
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
                        case 5: /* pertaskbase */
                            cfg->options |= OPTION_PERTASKBASE;
                            /* Fall through */
                        case 4: /* peropenerbase */
                            if (cfg->options & OPTION_DUPBASE)
                                exitfileerror(20, "Only one option peropenerbase or pertaskbase allowed\n");
                            cfg->options |= OPTION_DUPBASE;
                            break;
                        case 6: /* includes */
                            if (cfg->options & OPTION_NOINCLUDES)
                                exitfileerror(20, "option includes and noincludes are incompatible\n");
                            cfg->options |= OPTION_INCLUDES;
                            break;
                        case 7: /* noincludes */
                            if (cfg->options & OPTION_INCLUDES)
                                exitfileerror(20, "option includes and noincludes are incompatible\n");
                            cfg->options |= OPTION_NOINCLUDES;
                            break;
                        case 8: /* nostubs */
                            cfg->options |= OPTION_NOSTUBS;
                            break;
                        case 9: /* autoinit */
                            if (cfg->options & OPTION_NOAUTOINIT)
                                exitfileerror(20, "option autoinit and noautoinit are incompatible\n");
                            cfg->options |= OPTION_AUTOINIT;
                            break;
                        case 10: /* noautoinit */
                            if (cfg->options & OPTION_AUTOINIT)
                                exitfileerror(20, "option autoinit and noautoinit are incompatible\n");
                            cfg->options |= OPTION_NOAUTOINIT;
                            break;
                        case 11: /* resautoinit */
                                if (cfg->options & OPTION_SELFINIT)
                                        exitfileerror(20, "option resautoinit and selfinit are incompatible\n");
                            cfg->options |= OPTION_RESAUTOINIT;
                            break;
                        case 12:
                            cfg->options |= OPTION_NOOPENCLOSE;
                            break;
                        case 13: /* noresautoinit */
                                if (cfg->options & OPTION_RESAUTOINIT)
                                        exitfileerror(20, "option resautoinit and selfinit are incompatible\n");
                            cfg->options |= OPTION_SELFINIT;
                            break;
                        }
                        while (isspace(*s)) s++;
                    } while(*s !='\0');
                }
                else
                {
                    static const char *optionnames[] =
                    {
                        "private"
                    };
                    const unsigned int optionnums = sizeof(optionnames)/sizeof(char *);
                    int optionnum;

                    do
                    {
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
                        case 1: /* private */
                            cl->options |= COPTION_PRIVATE;
                            break;
                        }
                        while (isspace(*s)) s++;
                    } while(*s !='\0');
                }
                break;

            case 14: /* sysbase_field */
                if (inclass)
                    exitfileerror(20, "sysbase_field not valid config option when in a class section\n");
                cfg->sysbase_field = strdup(s);
                break;

            case 15: /* seglist_field */
                if (inclass)
                    exitfileerror(20, "seglist_field not valid config option when in a class section\n");
                cfg->seglist_field = strdup(s);
                break;

            case 16: /* rootbase_field */
                if (inclass)
                    exitfileerror(20, "rootbase_field not valid config option when in a class section\n");
                cfg->rootbase_field = strdup(s);
                break;

            case 17: /* classptr_field */
                if (cl == NULL)
                {
                    exitfileerror
                    (
                        20,
                        "classptr_field specified when not a BOOPSI class\n"
                    );
                }
                cl->classptr_field = strdup(s);
                break;

            case 18: /* classptr_var */
                if (cl == NULL)
                {
                    exitfileerror
                    (
                        20,
                        "classptr_var specified when not a BOOPSI class\n"
                    );
                }
                cl->classptr_var = strdup(s);
                break;

            case 19: /* classid */
                if (cl == NULL)
                    exitfileerror(20, "classid specified when not a BOOPSI class\n");
                if (cl->classid != NULL)
                    exitfileerror(20, "classid specified twice\n");
                cl->classid = strdup(s);
                if (strcmp(cl->classid, "NULL") == 0)
                    cl->options |= COPTION_PRIVATE;
                break;

            case 20: /* classdatatype */
                if (cl == NULL)
                    exitfileerror(20, "classdatatype specified when not a BOOPSI class\n");
                cl->classdatatype = strdup(s);
                break;

            case 21: /* beginio_func */
                if (inclass)
                    exitfileerror(20, "beginio_func not valid config option when in a class section\n");
                if (cfg->modtype != DEVICE)
                    exitfileerror(20, "beginio_func specified when not a device\n");
                cfg->beginiofunc = strdup(s);
                break;

            case 22: /* abortio_func */
                if (inclass)
                    exitfileerror(20, "abortio_func not valid config option when in a class section\n");
                if (cfg->modtype != DEVICE)
                    exitfileerror(20, "abortio_func specified when not a device\n");
                cfg->abortiofunc = strdup(s);
                break;

            case 23: /* dispatcher */
                if (cl == NULL)
                    exitfileerror(20, "dispatcher specified when not a BOOPSI class\n");
                cl->dispatcher = strdup(s);
                /* function references are not needed when dispatcher is specified */
                cfg->intcfg |= CFG_NOREADFUNCS;
                break;

            case 24: /* initpri */
                if (cl != NULL)
                {
                    int count;
                    char dummy;

                    count = sscanf(s, "%d%c", &cl->initpri, &dummy);
                    if (count != 1 ||
                        cl->initpri < -128 || cl->initpri > 127
                    )
                    {
                        exitfileerror(20, "initpri number format error\n");
                    }
                }
                else
                    exitfileerror(20, "initpri only valid config option for a BOOPSI class\n");
                break;

            case 25: /* type */
                if (!inclass)
                    exitfileerror(20, "type only valid config option in a class section\n");
                if (strcmp(s,"mcc")==0)
                    cl->classtype = MCC;
                else if (strcmp(s,"mui")==0)
                    cl->classtype = MUI;
                else if (strcmp(s,"mcp")==0)
                    cl->classtype = MCP;
                else if (strcmp(s, "image")==0)
                    cl->classtype = IMAGE;
                else if (strcmp(s, "gadget")==0)
                    cl->classtype = GADGET;
                else if (strcmp(s, "datatype")==0)
                    cl->classtype = DATATYPE;
                else if (strcmp(s, "usbclass")==0)
                    cl->classtype = USBCLASS;
                else if (strcmp(s, "class")==0)
                    cl->classtype = CLASS;
                else if (strcmp(s, "hidd")==0)
                    cl->classtype = HIDD;
                else
                {
                    fprintf(stderr, "Unknown type \"%s\" specified\n", s);
                    exit(20);
                }
                break;

            case 26: /* addromtag */
                cfg->addromtag = strdup(s);
                break;

            case 27: /* oopbase_field */
                cfg->oopbase_field = strdup(s);
                break;
            case 28: /* rellib */
                slist_append(&cfg->rellibs, s);
                break;
            case 29: /* interfaceid */
                if (!in)
                    exitfileerror(20, "interfaceid only valid config option for an interface\n");
                in->interfaceid = strdup(s);
                break;
            case 30: /* interfacename */
                if (!in)
                    exitfileerror(20, "interfacename only valid config option for an interface\n");
                in->interfacename = strdup(s);
                break;
            case 31: /* methodstub */
                if (!in)
                    exitfileerror(20, "methodstub only valid config option for an interface\n");
                in->methodstub = strdup(s);
                break;
            case 32: /* methodbase */
                if (!in)
                    exitfileerror(20, "methodbase only valid config option for an interface\n");
                in->methodbase = strdup(s);
                break;
            case 33: /* attributebase */
                if (!in)
                    exitfileerror(20, "attributebase only valid config option for an interface\n");
                in->attributebase = strdup(s);
                break;
            case 34: /* handler_func */
                if (cfg->modtype != HANDLER)
                    exitfileerror(20, "handler specified when not a handler\n");
                cfg->handlerfunc = strdup(s);
                break;
            case 35: /* includename */
                if (inclass)
                    exitfileerror(20, "includename not valid config option"
                        " when in a class section\n");
                cfg->includename = strdup(s);
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

    /* When not in a class section fill in default values for fields in cfg */
    if (!inclass)
    {
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
        if (cfg->libbasetype == NULL && libbasetypeextern != NULL)
            cfg->libbasetype = strdup(libbasetypeextern);
        if (cfg->sysbase_field != NULL && cfg->libbasetype == NULL)
            exitfileerror(20, "sysbase_field specified when no libbasetype is given\n");
        if (cfg->seglist_field != NULL && cfg->libbasetype == NULL)
            exitfileerror(20, "seglist_field specified when no libbasetype is given\n");
        if (cfg->oopbase_field != NULL && cfg->libbasetype == NULL)
            exitfileerror(20, "oopbase_field specified when no libbasetype is given\n");
        /* rootbase_field only allowed when duplicating base */
        if (cfg->rootbase_field != NULL && !(cfg->options & OPTION_DUPBASE))
            exitfileerror(20, "rootbasefield only valid for option peropenerbase or pertaskbase\n");

        /* Set default date to current date */
        if (cfg->datestring == NULL)
        {
            char tmpbuf[256];
            time_t now = time(NULL);
            struct tm *ltime = localtime(&now);

            snprintf(tmpbuf, sizeof(tmpbuf), "%u.%u.%u",
                ltime->tm_mday, 1 + ltime->tm_mon, 1900 + ltime->tm_year);

            cfg->datestring = strdup(tmpbuf);
        }

        if (cfg->copyright == NULL)
            cfg->copyright = "";

        if ( (cfg->beginiofunc != NULL && cfg->abortiofunc == NULL)
             || (cfg->beginiofunc == NULL && cfg->abortiofunc != NULL)
        )
            exitfileerror(20, "please specify both beginio_func and abortio_func\n");

        if (libbasetypeextern==NULL)
        {
            switch (cfg->modtype)
            {
            case DEVICE:
                cfg->libbasetypeptrextern = "struct Device *";
                break;
            case HANDLER:
            case RESOURCE:
                cfg->libbasetypeptrextern = "APTR ";
                break;
            case LIBRARY:
            case MUI:
            case MCP:
            case MCC:
            case GADGET:
            case DATATYPE:
            case USBCLASS:
            case HIDD:
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

        if (cfg->includename == NULL)
            cfg->includename = cfg->modulename;
        cfg->includenameupper = strdup(cfg->includename);
        for (s=cfg->includenameupper; *s!='\0'; *s = toupper(*s), s++)
            if (!isalnum(*s)) *s = '_';
    }

    /* When class was given too fill in some defaults when not specified */
    if (cl != NULL)
    {
        if (cl->classtype == UNSPECIFIED)
            cl->classtype = CLASS;

        if (cl->basename == NULL)
        {
            if (!inclass)
                cl->basename = cfg->basename;
            else
                exitfileerror(20, "basename has to be specified in the config section inside of a class section\n");
        }

        /* MUI classes are always private */
        if (cl->classtype == MUI || cl->classtype == MCC || cl->classtype == MCP)
            cl->options |= COPTION_PRIVATE;

        if (cl->classid == NULL
            && (cl->classtype != MUI && cl->classtype != MCC && cl->classtype != MCP)
        )
        {
            if (cl->classtype == HIDD)
            {
                cl->options &= !COPTION_PRIVATE;
            }
            else if (cl->options & COPTION_PRIVATE)
            {
                cl->classid = "NULL";
            }
            else
            {
                char s[256] = "";

                if (cl->classtype == GADGET || cl->classtype == IMAGE || cl->classtype == CLASS || cl->classtype == USBCLASS)
                {
                    sprintf(s, "\"%sclass\"", inclass ? cl->basename : cfg->modulename);
                }
                else if (cl->classtype == DATATYPE)
                {
                    sprintf(s, "\"%s.datatype\"", inclass ? cl->basename : cfg->modulename);
                }
                cl->classid = strdup(s);
            }
        }

        /* Only specify superclass or superclass_field */
        if (cl->superclass != NULL && cl->superclass_field != NULL)
            exitfileerror(20, "Only specify one of superclass or superclass_field in config section\n");

        /* Give default value to superclass if it is not specified */
        if (cl->superclass == NULL && cl->superclass == NULL)
        {
            switch (cl->classtype)
            {
            case MUI:
            case MCC:
                cl->superclass = "MUIC_Area";
                break;
            case MCP:
                cl->superclass = "MUIC_Mccprefs";
                break;
            case IMAGE:
                cl->superclass = "IMAGECLASS";
                break;
            case GADGET:
                cl->superclass = "GADGETCLASS";
                break;
            case DATATYPE:
                cl->superclass = "DATATYPESCLASS";
                break;
            case CLASS:
                cl->superclass = "ROOTCLASS";
                break;
            case HIDD:
                cl->superclass = "CLID_Root";
                break;
            default:
                exitfileerror(20, "Internal error: unhandled classtype in readsectionconfig\n");
                break;
            }
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

static void readsectionstartup(struct config *cfg)
{
    int atend = 0;
    char *line, *s;

    while (!atend)
    {
        line = readline();
        if (line==NULL)
            exitfileerror(20, "unexptected end of file in section startup\n");

        if (strncmp(line, "##", 2)!=0)
        {
            slist_append(&cfg->startuplines, line);
        }
        else
        {
            s = line+2;
            while (isspace(*s)) s++;
            if (strncmp(s, "end", 3)!=0)
                exitfileerror(20, "\"##end startup\" expected\n");

            s += 3;
            while (isspace(*s)) s++;
            if (strncmp(s, "startup", 7)!=0)
                exitfileerror(20, "\"##end startup\" expected\n");

            s += 7;
            while (isspace(*s)) s++;
            if (*s!='\0')
                exitfileerror(20, "unexpected character at position %d\n");

            atend = 1;
        }
    }
}

static void readsectionfunctionlist(const char *type, struct functionhead **funclistptr, unsigned int firstlvo, int isattribute, enum libcall def_libcall)
{
    int atend = 0, i;
    char *line, *s, *s2;
    unsigned int lvo = firstlvo;
    int minversion = -1;

    while (!atend)
    {
        line = readline();
        if (line==NULL)
            exitfileerror(20, "unexpected EOF in functionlist section\n");
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
                exitfileerror(20, "\"##end %s\" expected\n", type);

            s += 3;
            while (isspace(*s)) s++;
            if (strncmp(s, type, strlen(type))!=0)
                exitfileerror(20, "\"##end %s\" expected\n", type);

            s += strlen(type);
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
                if ((*s2 != '\0') && (*s2 != '#'))
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
            }
            else if (strncmp(s, "function", 8) == 0)
            {
                s += 8;

                if (!isspace(*s))
                    exitfileerror(20, "Syntax error\n");

                while (isspace(*s)) s++;
                if (*s == '\0' || !(isalpha(*s) || *s == '_'))
                    exitfileerror(20, "syntax is '.function name'\n");

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
                    exitfileerror(20, "syntax is '.function name'\n");

                if (*funclistptr == NULL)
                    exitfileerror(20, ".function has to come after a function declaration\n");

                funcsetinternalname(*funclistptr, s2);
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
            else if (strncmp(s, "version", 7) == 0)
            {
                /* Mark version number for the following
                 * functions, so that the automatic OpenLibrary()
                 * will know what version to use.
                 */
                char *tmp;
                int ver;

                s += 7;

                while (isspace(*s)) s++;
                ver = (int)strtol(s, &tmp, 0);

                if (s == tmp)
                    exitfileerror(20, ".version expects an integer\n");

                s = tmp;
                while (isspace(*s)) s++;

                if (*s && *s != '#')
                    exitfileerror(20, ".version has junk after the version number\n");

                minversion = ver;
            }
            else if (strncmp(s, "unusedlibbase", 13) == 0)
            {
                if (*funclistptr == NULL)
                    exitfileerror(20, ".unusedlibbase has to come after a function declaration\n");
                (*funclistptr)->unusedlibbase = 1;
            }
            else
                exitfileerror(20, "Syntax error");
        }
        else if (*line!='#') /* Ignore line that is a comment, e.g. that starts with a # */
        {
            /* The line is a function or attribute prototype.
             * A function can have one of two syntaxes:
             * type funcname(argproto1, argproto2, ...)
             * type funcname(argproto1, argproto2, ...) (reg1, reg2, ...)
             * The former is for C type function argument passing, the latter for
             * register argument passing.
             * An attribute has the following syntax:
             * type attribute
             */
            char c, *args[64], *regs[64], *funcname, *cp;
            int len, argcount = 0, regcount = 0, brcount = 0;

            cp = strchr(line,'#');
            if (cp)
                *(cp++) = 0;

            /* Parse 'type functionname' at the beginning of the line */
            if (isattribute) {
                s = line + strlen(line);
            } else {
                s = strchr(line, '(');
                if (s == NULL)
                    exitfileerror(20, "( expected at position %d\n", strlen(line) + 1);
            }

            s2 = s;
            while (isspace(*(s2-1)))
                s2--;
            *s2 = '\0';

            while (s2 > line && !isspace(*(s2-1)) && !(*(s2-1) == '*'))
                s2--;

            if (s2 == line)
                exitfileerror(20, "No type specifier before %s name\n", isattribute ? "attribute" : "function");

            if (*funclistptr != NULL)
                funclistptr = &((*funclistptr)->next);
            *funclistptr = newfunctionhead(s2, STACK);

            if (cp)
                (*funclistptr)->comment = strdup(cp);
            else
                (*funclistptr)->comment = NULL;

            while (isspace(*(s2-1)))
                s2--;
            *s2 = '\0';
            (*funclistptr)->type = strdup(line);
            (*funclistptr)->lvo = lvo;
            (*funclistptr)->version = minversion;
            lvo++;

            if (isattribute)
                continue;

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
                        if (c == '/')
                        {
                            s++;
                            if (s[0] == s[-3] && s[1] == s[-2] + 1)
                            {
                                s += 2;
                                c = *s;
                            }
                            else
                                exitfileerror(20,
                                              "wrong register specification \"%s\" for argument %u\n",
                                              regs[regcount-1], regcount
                                );

                            if (regcount > 4)
                                exitfileerror(20, "maximum four arguments passed in two registers allowed (%d, %s) \n", regcount, regs[regcount-1]);
                        }
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

                (*funclistptr)->libcall = def_libcall;
                for (i = 0; i < argcount; i++)
                    funcaddarg(*funclistptr, args[i], regs[i]);
            }
            else if (*s == '\0')
            { /* No registers specified */
                for (i = 0; i < argcount; i++)
                    funcaddarg(*funclistptr, args[i], NULL);
            }
            else
                exitfileerror(20, "wrong char '%c' at position %d\n", *s, (int)(s-line) + 1);
        }
    }
}

static void readsectionclass_methodlist(struct classinfo *cl)
{
    int atend = 0, i;
    char *line, *s, *s2;
    struct functionhead **methlistptr = &cl->methlist;
    struct stringlist *interface = NULL;

    if (cl->basename==NULL)
        exitfileerror(20, "section methodlist has to come after section config\n");

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
            }
            else if (strncmp(s, "function", 8) == 0)
            {
                s += 8;

                if (!isspace(*s))
                    exitfileerror(20, "Syntax error\n");

                while (isspace(*s)) s++;
                if (*s == '\0' || !(isalpha(*s) || *s == '_'))
                    exitfileerror(20, "syntax is '.function name'\n");

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
                    exitfileerror(20, "syntax is '.function name'\n");

                if (*methlistptr == NULL)
                    exitfileerror(20, ".function has to come after a function declaration\n");

                funcsetinternalname(*methlistptr, s2);
            }
            else if (strncmp(s, "interface", 9) == 0)
            {
                if (cl->classtype != HIDD)
                    exitfileerror(20, "interface only valid for a HIDD\n");

                s += 9;

                if (!isspace(*s))
                    exitfileerror(20, "Syntax error\n");

                while (isspace(*s)) s++;
                if (*s == '\0' || !isalpha(*s))
                    exitfileerror(20, "syntax is '.interface name'\n");

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
                    exitfileerror(20, "syntax is '.interface name'\n");

                interface = slist_append(&cl->interfaces, s2);
            }
            else
                exitfileerror(20, "Syntax error");
        }
        else if (isalpha(*line))
        {
            char stmp[256];

            for (s = line + 1; isalnum(*s) || *s == '_'; s++)
                ;

            if (cl->classtype == HIDD && interface == NULL)
                exitfileerror(20, "For a HIDD the first method has to come after an .interface line\n");

            if (*s != '\0')
                exitfileerror(20, "Only letters, digits and an underscore allowed in a methodname\n");

            if (*methlistptr != NULL)
                methlistptr = &((*methlistptr)->next);
            if (cl->classtype != HIDD)
            {
                if (snprintf(stmp, 256, "%s__%s", cl->basename, line) >= 256)
                    exitfileerror(20, "Method name too large\n");

                *methlistptr = newfunctionhead(stmp, STACK);
                (*methlistptr)->type = "IPTR";
                funcaddarg(*methlistptr, "Class *cl", NULL);
                funcaddarg(*methlistptr, "Object *o", NULL);
                funcaddarg(*methlistptr, "Msg msg", NULL);
            }
            else
            {
                if (snprintf(stmp, 256, "%s__%s__%s", cl->basename, interface->s, line) >= 256)
                    exitfileerror(20, "Method name too large\n");

                *methlistptr = newfunctionhead(stmp, STACK);
                (*methlistptr)->type = "IPTR";
                funcaddarg(*methlistptr, "OOP_Class *cl", NULL);
                funcaddarg(*methlistptr, "OOP_Object *o", NULL);
                funcaddarg(*methlistptr, "OOP_Msg msg", NULL);
                (*methlistptr)->interface = interface;
                if (snprintf(stmp, 256, "mo%s_%s", interface->s, line) >= 256)
                    exitfileerror(20, "Method name too large\n");
                (*methlistptr)->method = strdup(stmp);
            }
            slist_append(&(*methlistptr)->aliases, line);
        }
        else
            exitfileerror(20, "Methodname has to begin with a letter\n");
    }
}

static void
readsectioninterface(struct config *cfg)
{
    char *s;
    struct interfaceinfo *in;

    in = newinterface(cfg);
    s = readsections(cfg, NULL, in, 1);
    if (s == NULL)
        exitfileerror(20, "Unexpected end of file\n");

    if (strncmp(s, "##", 2) != 0)
        exitfileerror(20, "'##end interface' expected\n");
    s += 2;

    while (isspace(*s)) s++;

    if (strncmp(s, "end", 3) != 0)
        exitfileerror(20, "'##end interface' expected\n");
    s += 3;

    if (!isspace(*s))
        exitfileerror(20, "'##end interface' expected\n");
    while (isspace(*s)) s++;

    if (strncmp(s, "interface", 9) != 0)
        exitfileerror(20, "'##end interface' expected\n");
    s += 9;

    while (isspace(*s)) s++;
    if (*s != '\0')
        exitfileerror(20, "'##end interface' expected\n");

    if (!in->interfaceid)
        exitfileerror(20, "interface has no 'interfaceid' defined!\n");

    if (!in->interfacename)
        exitfileerror(20, "interface has no 'interfacename' defined!\n");

    if (!in->methodstub)
        in->methodstub = strdup(in->interfacename);

    if (!in->methodbase) {
        int len = strlen(in->interfacename);
        in->methodbase = malloc(len + 4 + 1);
        strcpy(in->methodbase, in->interfacename);
        strcat(in->methodbase, "Base");
    }

    if (!in->attributebase) {
        int len = strlen(in->interfacename);
        in->attributebase = malloc(len + 4 + 4 + 1);
        strcpy(in->attributebase, in->interfacename);
        strcat(in->attributebase, "AttrBase");
    }
}

static void
readsectionclass(struct config *cfg)
{
    char *s;
    struct classinfo *cl;

    cl = newclass(cfg);
    s = readsections(cfg, cl, NULL, 1);
    if (s == NULL)
        exitfileerror(20, "Unexpected end of file\n");

    if (strncmp(s, "##", 2) != 0)
        exitfileerror(20, "'##end class' expected\n");
    s += 2;

    while (isspace(*s)) s++;

    if (strncmp(s, "end", 3) != 0)
        exitfileerror(20, "'##end class' expected\n");
    s += 3;

    if (!isspace(*s))
        exitfileerror(20, "'##end class' expected\n");
    while (isspace(*s)) s++;

    if (strncmp(s, "class", 5) != 0)
        exitfileerror(20, "'##end class' expected\n");
    s += 5;

    while (isspace(*s)) s++;
    if (*s != '\0')
        exitfileerror(20, "'##end class' expected\n");
}

static struct classinfo *newclass(struct config *cfg)
{
    struct classinfo *cl, *classlistit;

    cl = malloc(sizeof(struct classinfo));
    if (cl == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        exit(20);
    }
    memset(cl, 0, sizeof(struct classinfo));

    /* By default the classes are initialized with a priority of 1 so they
     * are initialized before any user added initialization with priority 1
     */
    cl->initpri = 1;

    if (cfg->classlist == NULL)
        cfg->classlist = cl;
    else
    {
        for
        (
            classlistit = cfg->classlist;
            classlistit->next != NULL;
            classlistit = classlistit->next
        )
            ;
        classlistit->next = cl;
    }

    return cl;
}

static struct handlerinfo *newhandler(struct config *cfg)
{
    struct handlerinfo *hl;

    hl = calloc(1,sizeof(*hl));
    hl->next = cfg->handlerlist;
    cfg->handlerlist = hl;
    return hl;
}

static struct interfaceinfo *newinterface(struct config *cfg)
{
    struct interfaceinfo *in, *interfacelistit;

    in = malloc(sizeof(struct interfaceinfo));
    if (in == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        exit(20);
    }
    memset(in, 0, sizeof(struct interfaceinfo));

    if (cfg->interfacelist == NULL)
        cfg->interfacelist = in;
    else
    {
        for
        (
            interfacelistit = cfg->interfacelist;
            interfacelistit->next != NULL;
            interfacelistit = interfacelistit->next
        )
            ;
        interfacelistit->next = in;
    }

    return in;
}


static int getdirective(char *s, const char *directive, int range_min, int range_max, int *val)
{
    char *tmp;
    int newval;

    if (strncmp(s, directive, strlen(directive)) != 0)
        return 0;

    s += strlen(directive);
    if (*s && !isspace(*s))
        exitfileerror(20, "Unrecognized directive \".%s\"\n", directive);

    while (isspace(*s)) s++;
    if (!*s)
        exitfileerror(20, "No .%s value specified\n", directive);

    newval = strtol(s, &tmp, 0);
    if (s == tmp || !(newval >= range_min && newval <= range_max)) {
        tmp = s;
        while (*tmp && !isspace(*tmp)) tmp++;
        exitfileerror(20, "Invalid .%s value of %.*s\n", directive, tmp - s, s);
    }

    *val = newval;
    return 1;
}

static void
readsectionhandler(struct config *cfg)
{
    char *line = NULL, *s;
    struct handlerinfo *hl;
    unsigned char autolevel = 0;
    unsigned int stacksize = 0;
    int startup = 0;
    char priority = 10;
    int bootpri = -128;
    int has_filesystem = 0;

    for (;;)
    {
        char *function;
        int function_len;
        char *tmp;

        s = line = readline();

        if (s==NULL)
            exitfileerror(20, "unexpected end of file in section hanlder\n");

        if (strncmp(s, "##", 2)==0)
            break;

        /* Ignore comments */
        if (strncmp(s, "#", 1)==0)
            continue;

        /* Skip ahead to function name */
        while (*s && isspace(*s)) s++;

        /* Permit blank lines */
        if (!*s)
            continue;

        if (*s == '.') {
            int val;
            s++;

            if (getdirective(s, "autodetect", 0, 127, &val)) {
                autolevel = val;
            } else if (getdirective(s, "stacksize", 0, INT_MAX, &val)) {
                stacksize = val;
            } else if (getdirective(s, "priority", -128, 127, &val)) {
                priority = val;
            } else if (getdirective(s, "bootpri", -128, 127, &val)) {
                bootpri = val;
            } else if (getdirective(s, "startup", INT_MIN, INT_MAX, &val)) {
                startup = val;
            } else {
                exitfileerror(20, "Unrecognized directive \"%s\"\n", line);
            }
            continue;
        }

        do {
            unsigned int id = 0;

            if (strncasecmp(s,"resident=",9)==0) {
                char *res;

                s = strchr(s, '=') + 1;
                res = s;
                while (*s && !isspace(*s)) s++;
                if (res == s)
                    exitfileerror(20, "Empty resident= is not permitted\n");

                if (*s)
                    *(s++) = 0;

                hl = newhandler(cfg);
                hl->type = HANDLER_RESIDENT;
                hl->id = 0;
                hl->name = strdup(res);
                hl->autodetect = autolevel--;
                hl->stacksize = stacksize;
                hl->priority = priority;
                hl->startup = startup;
            } else if (strncasecmp(s,"dosnode=",8)==0) {
                char *dev;

                s = strchr(s, '=') + 1;
                dev = s;
                while (*s && !isspace(*s)) s++;
                if (dev == s)
                    exitfileerror(20, "Empty dosnode= is not permitted\n");

                if (*s)
                    *(s++) = 0;

                hl = newhandler(cfg);
                hl->type = HANDLER_DOSNODE;
                hl->id = 0;
                hl->name = strdup(dev);
                hl->autodetect = autolevel ? autolevel-- : 0;
                hl->stacksize = stacksize;
                hl->priority = priority;
                hl->startup = startup;
                hl->bootpri = bootpri;
            } else if (strncasecmp(s,"dostype=",8) == 0) {
                s = strchr(s, '=') + 1;

                id = (unsigned int)strtoul(s, &tmp, 0);

                if (s == tmp) {
                    while (*tmp && !isspace(*tmp))
                        tmp++;
                    exitfileerror(20, "\"%.*s\" is not a numerical DOS ID\n", (tmp -s), s);
                }
                s = tmp;

                if (id == 0 || id == ~0) {
                    exitfileerror(20, "DOS ID 0x%08x is not permitted\n", id);
                }

                hl = newhandler(cfg);
                hl->type = HANDLER_DOSTYPE;
                hl->id = id;
                hl->name = NULL;
                hl->autodetect = autolevel ? autolevel-- : 0;
                hl->stacksize = stacksize;
                hl->priority = priority;
                hl->startup = startup;
            } else {
                for (tmp = s; !isspace(*tmp); tmp++);
                exitfileerror(20, "Unknown option \"%.*s\"\n", tmp - s, s);
            }

            /* Advance to next ID */
            while (*s && isspace(*s)) s++;

        } while (*s);
    }

    if (s == NULL)
        exitfileerror(20, "Unexpected end of file\n");

    if (strncmp(s, "##", 2) != 0)
        exitfileerror(20, "'##end handler' expected\n");
    s += 2;

    while (isspace(*s)) s++;

    if (strncmp(s, "end", 3) != 0)
        exitfileerror(20, "'##end handler' expected\n");
    s += 3;

    while (isspace(*s)) s++;

    if (strncmp(s, "handler", 7) != 0)
        exitfileerror(20, "'##end handler' expected\n");
    s += 7;

    while (isspace(*s)) s++;
    if (*s != '\0')
        exitfileerror(20, "'##end handler' expected\n");
}


