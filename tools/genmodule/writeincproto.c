/*
    Copyright � 1995-2002, The AROS Development Team. All rights reserved.

    Desc: Function to write proto/modulename.h. Part of genmodule.
*/
#include "genmodule.h"

void writeincproto(int dummy)
{
    FILE *out;
    char line[256];
    struct linelist *linelistit;
    
    snprintf(line, 255, "%s/proto/%s.h", genincdir, modulename);
    out = fopen(line, "w");
    if (out==NULL)
    {
	fprintf(stderr, "Could not write %s\n", line);
	exit(20);
    }
    fprintf(out,
	    "#ifndef PROTO_%s_H\n"
	    "#define PROTO_%s_H\n"
	    "\n"
	    "/*\n"
	    "    *** Automatically generated file. Do not edit ***\n"
	    "    Copyright � 1995-2002, The AROS Development Team. All rights reserved.\n"
	    "*/\n"
	    "\n"
	    "#include <aros/system.h>\n"
	    "\n"
	    "#include <clib/%s_protos.h>\n"
	    "\n"
	    "#if !defined(%s) && !defined(__NOLIBBASE__) && !defined(__%s_NOLIBBASE__)\n"
	    "extern %s *%s;\n"
	    "#endif\n"
	    "\n",
	    modulenameupper, modulenameupper,
	    modulename,
	    libbase, modulenameupper,
	    libbasetypeextern, libbase);
    for (linelistit = protolines; linelistit!=NULL; linelistit = linelistit->next)
	fprintf(out, "%s\n", linelistit->line);
    
    fprintf(out,
	    "#if !defined(NOLIBDEFINES) && !defined(%s_NOLIBDEFINES)\n"
	    "#   include <defines/%s.h>\n"
	    "#endif\n"
	    "\n"
	    "#endif /* PROTO_%s_H */\n",
	    modulenameupper, modulename, modulenameupper);
    fclose(out);
}
