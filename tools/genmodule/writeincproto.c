/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: Function to write proto/modulename.h. Part of genmodule.
*/
#include "genmodule.h"

void writeincproto(void)
{
    FILE *out;
    struct linelist *linelistit;
    
    snprintf(line, slen-1, "%s/proto/%s.h", genincdir, modulename);
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
	    "    Copyright © 1995-2002, The AROS Development Team. All rights reserved.\n"
	    "*/\n"
	    "\n"
	    "#include <aros/system.h>\n"
	    "\n"
	    "extern %s *%s;\n"
	    "\n",
	    modulenameupper, modulenameupper,
	    libbasetypeextern, libbase);
    for (linelistit = protolines; linelistit!=NULL; linelistit = linelistit->next)
	fprintf(out, "%s\n", linelistit->line);
    fprintf(out,
	    "#include <clib/%s_protos.h>\n"
	    "#if !defined(NOLIBDEFINES) && !defined(%s_NOLIBDEFINES)\n"
	    "#   include <defines/%s.h>\n"
	    "#endif\n"
	    "\n"
	    "#endif /* PROTO_%s_H */\n",
	    modulename, modulenameupper, modulename, modulenameupper);
    fclose(out);
}
