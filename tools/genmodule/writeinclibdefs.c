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
    struct linelist *linelistit;
    char *_libbasetype = (libbasetype==NULL) ? "struct GM_LibHeader" : libbasetype;
    char *residentflags, *suffix;
	
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
        "#define _%s_LIBDEFS_H\n"
        "\n",
        modulenameupper, modulenameupper
    );

    fprintf
    (
        out,
        "#define GM_UNIQUENAME(n) %s_ ## n\n"
        "#define LIBBASE          %s\n"
        "#define LIBBASETYPE      %s\n"
        "#define LIBBASETYPEPTR   %s *\n"
        "#define NAME_STRING      \"%s.%s\"\n"
        "#define VERSION_NUMBER   %u\n"
        "#define MAJOR_VERSION    %u\n"
        "#define REVISION_NUMBER  %u\n"
        "#define MINOR_VERSION    %u\n"
        "#define VERSION_STRING   \"$VER: %s.%s %u.%u (%s)\\r\\n\"\n"
        "#define COPYRIGHT_STRING \"\"\n"
        "#define LIBEND           GM_UNIQUENAME(End)\n"
        "#define LIBFUNCTABLE     GM_UNIQUENAME(FuncTable)\n"
        "#define RESIDENTPRI      %d\n"
        "#define RESIDENTFLAGS    %s\n",
        basename,
        libbase, _libbasetype, _libbasetype,
        modulename, suffix,
        majorversion, majorversion,
        minorversion, minorversion,
        modulename, suffix, majorversion, minorversion, datestring,
        residentpri,
        residentflags
    );

    for (linelistit = cdefprivatelines; linelistit!=NULL; linelistit = linelistit->next)
	fprintf(out, "%s\n", linelistit->line);
    
    if (libbasetype == NULL && !(options & OPTION_NORESIDENT))
    {
	fprintf(out,
		"\n"
		"#include <exec/libraries.h>\n"
		"#include <dos/dos.h>\n"
		"\n"
		"struct GM_LibHeader\n"
		"{\n"
		"    struct Library   lh_LibNode;\n"
		"    BPTR             lh_SegList;\n"
		"    struct ExecBase *lh_SysBase;\n"
		"};\n"
		"#define GM_SYSBASE_FIELD(lh) ((lh)->lh_SysBase)\n"
		"#define GM_SEGLIST_FIELD(lh) ((lh)->lh_SegList)\n"
	);
    }
    else
    {
	fprintf(out,
		"\n"
		"#ifdef LC_SYSBASE_FIELD\n"
		"#define GM_SYSBASE_FIELD LC_SYSBASE_FIELD\n"
		"#endif\n"
		"#ifdef LC_SEGLIST_FIELD\n"
		"#define GM_SEGLIST_FIELD LC_SEGLIST_FIELD\n"
		"#endif\n"
	);
	if (sysbase_field != NULL)
	    fprintf(out,
		    "#define GM_SYSBASE_FIELD(lh) (((LIBBASETYPEPTR)lh)->%s)\n",
		    sysbase_field
	    );
	if (seglist_field != NULL)
	    fprintf(out,
		    "#define GM_SEGLIST_FIELD(lh) (((LIBBASETYPEPTR)lh)->%s)\n",
		    seglist_field
	    );
    }

    fprintf
    (
        out,
        "\n"
        "#endif /* _%s_LIBDEFS_H */\n",
        modulenameupper
    );
    
    fclose(out);
}
