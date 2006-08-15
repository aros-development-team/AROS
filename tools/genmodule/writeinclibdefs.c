/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write libdefs.h. Part of genmodule.
*/
#include "genmodule.h"

void writeinclibdefs(struct config *cfg)
{
    FILE *out;
    char line[1024];
    struct stringlist *linelistit;
    char *_libbasetype = (cfg->libbasetype==NULL) ? "struct Library" : cfg->libbasetype;
    char residentflags[256];
    struct classinfo *classlistit;

    residentflags[0] = 0;
    
	
    if (cfg->residentpri >= 105)
	strcpy(residentflags, "RTF_SINGLETASK");
    else if (cfg->residentpri >= -50)
	strcpy(residentflags, "RTF_COLDSTART");
    else if (cfg->residentpri < -120)
	strcpy(residentflags, "RTF_AFTERDOS");

    if (cfg->modtype != RESOURCE)
    {
	if(strlen(residentflags) > 0)
	    strcat(residentflags, "|");
	strcat(residentflags, "RTF_AUTOINIT");
    }

    if (strlen(residentflags) == 0)
	strcpy(residentflags, "0");
    
    snprintf(line, 1023, "%s/%s_libdefs.h", cfg->gendir, cfg->modulename);

    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
    	exit(20);
    }
    
    fprintf
    (
        out,
        "#ifndef _%s_LIBDEFS_H\n"
        "#define _%s_LIBDEFS_H\n"
        "\n",
        cfg->modulenameupper, cfg->modulenameupper
    );

    fprintf
    (
        out,
        "#define GM_UNIQUENAME(n) %s_ ## n\n"
        "#define LIBBASE          %s\n"
        "#define LIBBASETYPE      %s\n"
        "#define LIBBASETYPEPTR   %s *\n"
        "#define MOD_NAME_STRING  \"%s.%s\"\n"
        "#define VERSION_NUMBER   %u\n"
        "#define MAJOR_VERSION    %u\n"
        "#define REVISION_NUMBER  %u\n"
        "#define MINOR_VERSION    %u\n"
        "#define VERSION_STRING   \"$VER: %s.%s %u.%u (%s) %s\\r\\n\"\n"
        "#define COPYRIGHT_STRING \"%s\"\n"
        "#define LIBEND           GM_UNIQUENAME(End)\n"
        "#define LIBFUNCTABLE     GM_UNIQUENAME(FuncTable)\n"
        "#define RESIDENTPRI      %d\n"
        "#define RESIDENTFLAGS    %s\n",
        cfg->basename,
        cfg->libbase, _libbasetype, _libbasetype,
        cfg->modulename, cfg->suffix,
        cfg->majorversion, cfg->majorversion,
        cfg->minorversion, cfg->minorversion,
        cfg->modulename, cfg->suffix, cfg->majorversion, cfg->minorversion, cfg->datestring,
        cfg->copyright, cfg->copyright,
        cfg->residentpri,
        residentflags
    );

    for (linelistit = cfg->cdefprivatelines; linelistit!=NULL; linelistit = linelistit->next)
	fprintf(out, "%s\n", linelistit->s);

    /* Following code assumes that the input was checked to be consistent during the
     * parsing of the .conf file in config.c, no checks are done here
     */
    if (cfg->sysbase_field != NULL)
	fprintf(out,
		"#define GM_SYSBASE_FIELD(lh) (((LIBBASETYPEPTR)lh)->%s)\n",
		cfg->sysbase_field
	);
    if (cfg->seglist_field != NULL)
	fprintf(out,
		"#define GM_SEGLIST_FIELD(lh) (((LIBBASETYPEPTR)lh)->%s)\n",
		cfg->seglist_field
	);
    for (classlistit = cfg->classlist; classlistit != NULL; classlistit = classlistit->next)
    {
	int storeptr;
	    
	if (classlistit->classptr_field != NULL)
	{
	    storeptr = 1;
	    snprintf(line, 1023, "((LIBBASETYPEPTR)lh)->%s", classlistit->classptr_field);
	}
	else if (classlistit->classptr_var != NULL)
	{
	    storeptr = 1;
	    snprintf(line, 1023, "%s", classlistit->classptr_var);
	}
	else if ((classlistit->classid != NULL) && !(classlistit->options & COPTION_PRIVATE))
	{
	    storeptr = 0;
	    snprintf(line, 1023, "FindClass(%s)", classlistit->classid);
	}
	else
	    /* Don't write anything */
	    continue;

	fprintf(out,
		"#define %s_STORE_CLASSPTR %d\n",
		classlistit->basename, storeptr
	);
	    
	/* When class is the main class also define GM_CLASSPTR_FIELD for legacy */
	if (strcmp(classlistit->basename, cfg->basename) == 0)
	    fprintf(out, "#define GM_CLASSPTR_FIELD(lh) (%s)\n", line);

	fprintf(out,
		"#define %s_CLASSPTR_FIELD(lh) (%s)\n",
		classlistit->basename, line
	);
    }
	
    if ((cfg->options & OPTION_DUPBASE) && cfg->rootbase_field != NULL)
	fprintf(out,
		"#define GM_ROOTBASE_FIELD(lh) (((LIBBASETYPEPTR)lh)->%s)\n",
		cfg->rootbase_field
	);

    fprintf
    (
        out,
        "\n"
        "#endif /* _%s_LIBDEFS_H */\n",
        cfg->modulenameupper
    );
    
    if (ferror(out))
    {
	perror("Error writing libdefs.h");
	fclose(out);
	exit(20);
    }
    
    fclose(out);
}
