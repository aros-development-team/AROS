/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write libdefs.h. Part of genmodule.
*/
#include "genmodule.h"

void writeinclibdefs(void)
{
    FILE *out;
    char line[1024];
    char *suffix;
    struct linelist *linelistit;
    char *residentflags;
    
    snprintf(line, 1023, "%s/%s_libdefs.h", gendir, modulename);
    out = fopen(line, "w");
    if (out==NULL)
    {
	fprintf(stderr, "Could not write file \"%s\"\n", line);
	exit(20);
    }
    
    fprintf
    (
        out,
        "#ifndef _%s_LIBDEFS_H\n"
        "#define _%s_LIBDEFS_H\n",
        modulenameupper, modulenameupper
    );
    
    switch(modtype)
    {
        case LIBRARY: suffix = "library"; break;
        case MCC:     suffix = "mcc"; break;
        case MUI:     suffix = "mui"; break;
        case MCP:     suffix = "mcp"; break;
        default:
            fprintf(stderr, "Unhandled modtype %d\n", modtype);
            fclose(out);
            exit(20);
    }
    
    if (residentpri >= 105)
	residentflags = "RTF_AUTOINIT|RTF_SINGLETASK";
    else if (residentpri >= -50)
	residentflags = "RTF_AUTOINIT|RTF_COLDSTART";
    else if (residentpri < -120)
	residentflags = "RTF_AUTOINIT|RTF_AFTERDOS";
    else
	residentflags = "RTF_AUTOINIT";

    fprintf
    (
        out,
        "#define NAME_STRING      \"%s.%s\"\n"
        "#define NT_TYPE          NT_LIBRARY\n"
        "#define LC_UNIQUE_PREFIX %s\n"
        "#define LC_BUILDNAME(n)  %s_ ## n\n"
        "#define LIBBASE          %s\n"
        "#define LIBBASETYPE      %s\n"
        "#define LIBBASETYPEPTR   %s *\n"
        "#define LC_LIBBASESIZE   sizeof(%s)\n"
        "#define VERSION_NUMBER   %u\n"
        "#define REVISION_NUMBER  %u\n"
        "#define BASENAME         %s\n"
        "#define BASENAME_STRING  \"%s\"\n"
        "#define VERSION_STRING   \"$VER: %s %u.%u (%s)\\r\\n\"\n"
        "#define LIBEND           %s_end\n"
        "#define LIBFUNCTABLE     %s_functable\n"
        "#define AROS_LC_SETFUNCS\n"
        "#define COPYRIGHT_STRING \"\"\n"
        "#define LC_NO_INITLIB\n"
        "#define LC_NO_OPENLIB\n"
        "#define LC_NO_CLOSELIB\n"
        "#define LC_NO_EXPUNGELIB\n"
        "#define LC_RESIDENTFLAGS %s\n"
	"#define LC_RESIDENTPRI   %d\n",
        modulename, suffix,
        basename, basename,
        libbase, libbasetype, libbasetype, libbasetype,
        majorversion, minorversion,
        basename, basename,
        modulename, majorversion, minorversion, datestring,
        modulename, modulename,
        residentflags, residentpri
    );

    if (options & OPTION_NOAUTOLIB)
	fputs("#define AROS_LC_SET_NOLIBS\n", out);
    
    if (options & OPTION_NOEXPUNGE)
	fputs("#define NOEXPUNGE\n", out);
    
    fprintf
    (
        out,
        "#endif /* _%s_LIBDEFS_H */\n",
        modulenameupper
    );
    
    for (linelistit = cdefprivatelines; linelistit!=NULL; linelistit = linelistit->next)
	fprintf(out, "%s\n", linelistit->line);
    
    fclose(out);
}
