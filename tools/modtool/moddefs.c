/*
    Copyright (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Create the moddefs.h include file from module.conf
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <toollib/toollib.h>
#include <toollib/error.h>
#include <toollib/filesup.h>

#include "module.h"

char *optionStr[] =
{
    "NOEXPUNGE",
    "ROMONLY",
    "HASRT",
    "NOAUTOINIT",
    "NORESIDENT",
    "NOEXTENSION",
    "CLASSLIB"
};

int writeModDefs(char *file, struct ModuleConfig *mc)
{
    FILE *fd;
    char *newfile;
    int i;

    newfile = xmalloc((strlen(file) + 4) * sizeof(char));
    strcpy(newfile, file);
    strcat(newfile, ".new");

    fd = fopen(newfile, "w");
    if(!fd)
    {
	StdError("couldn't open file %s", file);
	return -1;
    }

    /*
	Ok, format the output file. Currently this is quite simply
	done, but we could change to some kind of template mechanism.
    */
    fprintf(fd,
	"#ifndef %s\n"
	"#define %s\n\n"
	"#define NAME_STRING      \"%s%s\"\n"
	"#define NT_TYPE          %s\n\n",
	mc->define, mc->define, mc->name, mc->extension, mc->nodetype
    );

    /*
	Print out the options. To add an option, simply modify
	the array optionStr above. Remember to include it in
	toollib/module.h
    */
    for(i = 0; i < (sizeof(optionStr) / sizeof(char *)); i++)
    {
	if(mc->option & (1L<<i))
	    fprintf(fd, "#define %s\n", optionStr[i]);
    }

    fprintf(fd,
	"#define MODBASE          %s\n"
	"#define MODBASETYPE      %s\n"
	"#define MODBASETYPEPTR   %s\n"
	"#define VERSION_NUMBER   %d\n"
	"#define REVISION_NUMBER  %d\n",
	mc->base, mc->basetype, mc->basetypeptr, mc->version, mc->revision
    );

    {
	time_t t;
	struct tm *tm;

	time(&t);
	tm = localtime(&t);
	fprintf(fd,
	    "#define VERSION_STRING   \"$VER: %s%s %d.%d (%02d.%02d.%04d)\\r\\n\"\n",
	    mc->name, mc->extension, mc->version, mc->revision,
	    tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900
	);
    }

    fprintf(fd,
	"#define MODTAG           %s_resident\n"
	"#define MODEND           %s_end\n"
	"#define MODFUNCTABLE     %s_functable\n"
	"#define COPYRIGHT_STRING \"%s\"\n\n"
	"#endif /* %s */\n",
	mc->basename, mc->basename, mc->basename, mc->copyright, mc->define
    );

    fclose(fd);
    return moveifchanged(file, newfile);
}
