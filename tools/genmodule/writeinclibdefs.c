/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: function to write libdefs.h. Part of genmodule.
*/
#include "genmodule.h"

void writeinclibdefs(void)
{
    FILE *out;
    
    snprintf(line, slen-1, "%s/%s_libdefs.h", gendir, modulename);
    out = fopen(line, "w");
    if (out==NULL)
    {
	fprintf(stderr, "Could not write file \"%s\"\n", line);
	exit(20);
    }
    fprintf(out,
	    "#ifndef _%s_LIBDEFS_H\n"
	    "#define _%s_LIBDEFS_H\n"
	    "#define NAME_STRING      \"%s.library\"\n"
	    "#define NT_TYPE          NT_LIBRARY\n"
	    "#define LC_UNIQUE_PREFIX %s\n"
	    "#define LC_BUILDNAME(n)  %s_ ## n\n"
	    "#define LIBBASE          %s\n"
	    "#define LIBBASETYPE      %s\n"
	    "#define LIBBASETYPEPTR   %s *\n"
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
	    "#endif /* _%s_LIBDEFS_H */\n",
	    modulenameupper, modulenameupper, modulename,
	    basename, basename,
	    libbase, libbasetype, libbasetype,
	    majorversion, minorversion,
	    basename, basename,
	    modulename, majorversion, minorversion, datestring,
	    modulename, modulename,
	    modulenameupper
	   );
    fclose(out);
}
