/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: function to write modulename_end.c. Part of genmodule.
*/
#include "genmodule.h"

void writeend(void)
{
    FILE *out;
    char line[256];
    
    snprintf(line, 255, "%s/%s_end.c", gendir, modulename);
    out = fopen(line, "w");
    if (out==NULL)
    {
	fprintf(stderr, "Could not write %s\n", line);
	exit(20);
    }
    fprintf(out,
	    "#include \"%s_libdefs.h\"\n"
	    "int GM_UNIQUENAME(End)(void) {return 0;}\n",
	    modulename
    );
    fclose(out);
}
