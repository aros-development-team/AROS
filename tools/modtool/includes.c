/*
    Copyright (C) 1995-1999 AROS - The Amiga Research OS
    $Id$

    Desc: Generate some include files.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <toollib/toollib.h>
#include <toollib/lineparser.h>
#include <toollib/filesup.h>
#include <toollib/vstring.h>
#include <toollib/error.h>

#include "module.h"
#include "modtool.h"

void printHeadersTmpl(FILE *fdout, char *file, char *type)
{
    FILE *fdin;
    char *line;
    int in_header;

    fdin = fopen( file, "rb" );
    if(fdin)
    {
	in_header = 0;

	while( (line = get_line(fdin)) )
	{
	    if(	strncmp(line, "##begin ", 8) == 0
		&& strcmp(&line[8], type) == 0)
	    {
		in_header = 1;
	    }
	    else if(strncmp(line, "##end ", 6) == 0
		&& strcmp(&line[6], type) == 0 )
	    {
		in_header = 0;
	    }
	    else if(in_header == 1)
	    {
		fprintf(fdout, "%s\n", line);
	    }
	    free(line);
	}

	fclose(fdin);
	fputc('\n', fdout);
    }
}


/*
    Generate the proto/xxx.h file.

    This is the simplest include file, since it doesn't actually include
    any information from the source files. Yay!

    It does show a good method of writing the basic part of one of
    these output routines however.
*/
void genInclProto(char *file, struct ModuleData *md, char *hdrTmpl)
{
    struct ModuleConfig *mc = md->md_Config;
    FILE *fd;
    char *newfile;
    char *upperbasename, *upperinclname;

    newfile = malloc( (strlen(file) + 5) * sizeof(char) );
    sprintf(newfile, "%s.new", file);

    fd = fopen(newfile, "w");
    if(!fd)
    {
	StdError("Could not open proto file %s", newfile);
	exit(-1);
    }

    upperbasename = xstrdup(mc->basename);
    strupper(upperbasename);

    upperinclname = xstrdup(mc->include);
    strupper(upperinclname);

    fprintf(fd, "#ifndef PROTO_%s_H\n", upperinclname);
    fprintf(fd, "#define PROTO_%s_H\n\n", upperinclname);
    fputs(	"/*\n"
		"    Copyright (C) 1995-1999 AROS - The Amiga Research OS\n"
		"    *** Automatically Generated File - Do Not Edit ***\n"
		"    Lang: english\n"
		"*/\n\n"
		"#ifndef AROS_SYSTEM_H\n"
		"#   include <aros/system.h>\n"
		"#endif\n\n", fd );

    fprintf(fd, "#include <clib/%s_protos.h>\n\n", mc->include);

    fprintf(fd, "if defined(_AMIGA) && defined(__GNUC__)\n"
		"#   include <inline/%s.h>\n"
		"#else\n"
		"#   include <defines/%s.h>\n"
		"#endif\n\n",
	mc->include, mc->include
    );

    if(mc->option & MO_HASRT)
    {
	fprintf(fd, "#if defined(ENABLE_RT) && ENABLE_RT && !defined(ENABLE_RT_%s)\n"
		    "#   define ENABLE_RT_%s 1\n"
		    "#   include <aros/rt.h>\n"
		    "#endif\n\n",
    	    upperbasename, upperbasename
	);
    }

    fprintf(fd,	"#ifndef %sNAME\n"
		"#   define %sNAME      \"%s%s\"\n"
		"#endif\n\n",
	upperbasename, upperbasename, mc->name, mc->extension
    );

    fprintf(fd,	"#endif /* PROTO_%s_H */\n", upperinclname );

    fclose(fd);
    moveifchanged(file, newfile);
    free(newfile);
    free(upperbasename);
    free(upperinclname);
}

void genInclClib(char *file, struct ModuleData *md, char *hdrTmpl)
{
    struct ModuleConfig *mc = md->md_Config;
    FILE *fd;
    char *newfile;
    char *upperbasename, *upperinclname, *capsbasename;
    struct FunctionData *thisFunc;
    struct ArgData *thisArg;

    newfile = malloc( (strlen(file) + 5) * sizeof(char) );
    sprintf(newfile, "%s.new", file);

    fd = fopen(newfile, "w");
    if(!fd)
    {
	StdError("Could not open proto file %s", newfile);
	exit(-1);
    }

    upperbasename = xstrdup(mc->basename);
    strupper(upperbasename);

    upperinclname = xstrdup(mc->include);
    strupper(upperinclname);

    capsbasename = xstrdup(mc->basename);
    capsbasename[0] = toupper(capsbasename[0]);

    fprintf(fd, "#ifndef CLIB_%s_PROTOS_H\n", upperinclname);
    fprintf(fd, "#define CLIB_%s_PROTOS_H\n\n", upperinclname);
    fputs(	"/*\n"
		"    Copyright (C) 1995-1999 AROS - The Amiga Research OS\n"
		"    *** Automatically Generated File - Do Not Edit ***\n"
		"    Lang: english\n"
		"*/\n\n"
		"#ifndef AROS_LIBCALL_H\n"
		"#   include <aros/libcall.h>\n"
		"#endif\n\n"
		"#ifndef EXEC_TYPES_H\n"
		"#   include <exec/types.h>\n"
		"#endif\n\n",
		fd );

    if(hdrTmpl)
	printHeadersTmpl(fd, hdrTmpl, "clib");

    fputs("/* Prototypes */\n", fd);

    ForeachNode(&md->md_Data, thisFunc)
    {
	if(thisFunc->fd_Link.n_Type == T_Function)
	{
	    fprintf(fd, "AROS_LP%s%d%s(%s, %s, \n",
		((thisFunc->fd_Options & FO_HasQuad)  ? "QUAD" : ""),
		thisFunc->fd_NumArgs,
		((thisFunc->fd_Options & FO_UseBase) ? "" : "I" ),
		thisFunc->fd_Type, thisFunc->fd_Name
	    );

	    thisArg = (struct ArgData *)&(thisFunc->fd_Args);
	    while(( thisArg = thisArg->ad_Next ))
	    {
		if(thisArg->ad_Reg2 != NULL)
		{
		    fprintf(fd, "\tAROS_LPAQUAD(%s, %s, %s, %s), \n",
			thisArg->ad_Type, thisArg->ad_Name,
			thisArg->ad_Reg1, thisArg->ad_Reg2
		    );
		}
		else
		{
		    fprintf(fd, "\tAROS_LPA(%s, %s, %s), \n",
			thisArg->ad_Type, thisArg->ad_Name, thisArg->ad_Reg1
		    );
		}
	    }

	    fprintf(fd, "\t%s, %s, %d, %s)\n\n", 
		mc->basetypeptr, mc->base, thisFunc->fd_Offset, capsbasename
	    );
	}
    }

    fprintf(fd,	"#endif /* CLIB_%s_PROTOS_H */\n", upperinclname );

    fclose(fd);
    moveifchanged(file, newfile);
    free(newfile);
    free(capsbasename);
    free(upperbasename);
    free(upperinclname);
}

void genInclDefine(char *file, struct ModuleData *md, char *hdrTmpl)
{
    struct ModuleConfig *mc = md->md_Config;
    FILE *fd;
    char *newfile;
    char *upperbasename, *upperinclname, *capsbasename;
    struct FunctionData *thisFunc;
    struct ArgData *thisArg;

    newfile = malloc( (strlen(file) + 5) * sizeof(char) );
    sprintf(newfile, "%s.new", file);

    fd = fopen(newfile, "w");
    if(!fd)
    {
	StdError("Could not open proto file %s", newfile);
	exit(-1);
    }

    upperbasename = xstrdup(mc->basename);
    strupper(upperbasename);

    upperinclname = xstrdup(mc->include);
    strupper(upperinclname);

    capsbasename = xstrdup(mc->basename);
    capsbasename[0] = toupper(capsbasename[0]);

    fprintf(fd, "#ifndef DEFINES_%s_H\n", upperinclname);
    fprintf(fd, "#define DEFINES_%s_H\n\n", upperinclname);
    fputs(	"/*\n"
		"    Copyright (C) 1995-1999 AROS - The Amiga Research OS\n"
		"    *** Automatically Generated File - Do Not Edit ***\n"
		"    Lang: english\n"
		"*/\n\n"
		"#ifndef AROS_LIBCALL_H\n"
		"#   include <aros/libcall.h>\n"
		"#endif\n\n"
		"#ifndef EXEC_TYPES_H\n"
		"#   include <exec/types.h>\n"
		"#endif\n\n",
		fd );

    if(hdrTmpl)
	printHeadersTmpl(fd, hdrTmpl, "defines");

    fputs("/* Defines */\n", fd);

    ForeachNode(&md->md_Data, thisFunc)
    {
	if(thisFunc->fd_Link.n_Type == T_Function)
	{
	    fprintf(fd, "#define %s(", thisFunc->fd_Name);
	    thisArg = (struct ArgData *)&(thisFunc->fd_Args);
	    while(( thisArg = thisArg->ad_Next ))
	    {
		fprintf(fd, "%s%s", thisArg->ad_Name,
		    ((thisArg->ad_Next != NULL) ? ", " : "), \n")
		);
	    }

	    fprintf(fd, "\tAROS_LC%s%d%s(%s, %s \n",
		((thisFunc->fd_Options & FO_HasQuad) ? "QUAD" : ""),
		thisFunc->fd_NumArgs,
		((thisFunc->fd_Options & FO_UseBase) ? "" : "I" ),
		thisFunc->fd_Type, thisFunc->fd_Name
	    );

	    thisArg = (struct ArgData *)&(thisFunc->fd_Args);
	    while(( thisArg = thisArg->ad_Next ))
	    {
		if(thisArg->ad_Reg2 != NULL)
		{
		    fprintf(fd, "\t\tAROS_LCAQUAD(%s, %s, %s, %s), \n",
			thisArg->ad_Type, thisArg->ad_Name,
			thisArg->ad_Reg1, thisArg->ad_Reg2
		    );
		}
		else
		{
		    fprintf(fd, "\t\tAROS_LCA(%s, %s, %s), \n",
			thisArg->ad_Type, thisArg->ad_Name, thisArg->ad_Reg1
		    );
		}
	    }

	    fprintf(fd, "\t%s, %s, %d, %s)\n\n", 
		mc->basetypeptr, mc->base, thisFunc->fd_Offset, capsbasename
	    );
	}
    }

    fprintf(fd,	"#endif /* DEFINES_%s_H */\n", upperinclname );

    fclose(fd);
    moveifchanged(file, newfile);
    free(newfile);
    free(capsbasename);
    free(upperbasename);
    free(upperinclname);
}

void genInclInline(char *file, struct ModuleData *md, char *hdrTmpl)
{
    struct ModuleConfig *mc = md->md_Config;
    FILE *fd;
    char *newfile;
    char *upperbasename, *upperinclname, *capsbasename;
    struct FunctionData *thisFunc;
    struct ArgData *thisArg;

    newfile = malloc( (strlen(file) + 5) * sizeof(char) );
    sprintf(newfile, "%s.new", file);

    fd = fopen(newfile, "w");
    if(!fd)
    {
	StdError("Could not open inline file %s", newfile);
	exit(-1);
    }

    upperbasename = xstrdup(mc->basename);
    strupper(upperbasename);

    upperinclname = xstrdup(mc->include);
    strupper(upperinclname);

    capsbasename = xstrdup(mc->basename);
    capsbasename[0] = toupper(capsbasename[0]);

    fprintf(fd, "#ifndef _INLINE_%s_H\n", upperinclname);
    fprintf(fd, "#define _INLINE_%s_H\n\n", upperinclname);
    fputs(	"/*\n"
		"    Copyright (C) 1995-1999 AROS - The Amiga Research OS\n"
		"    *** Automatically Generated File - Do Not Edit ***\n"
		"    Lang: english\n"
		"*/\n\n"
		"#ifndef __INLINE_MACROS_H\n"
		"#include <inline/macros.h>\n"
		"#endif\n\n",
		fd );
    fprintf(fd, "#ifndef %s_BASE_NAME\n#define %s_BASE_NAME %s\n#endif",
	upperbasename, upperbasename, mc->base
    );

    fputs("\n\n/* Inline */\n", fd);

        ForeachNode(&md->md_Data, thisFunc)
    {
	if(thisFunc->fd_Link.n_Type == T_Function)
	{
	    fprintf(fd, "#define %s(", thisFunc->fd_Name);
	    thisArg = (struct ArgData *)&(thisFunc->fd_Args);
	    while(( thisArg = thisArg->ad_Next ))
	    {
		fprintf(fd, "%s%s", thisArg->ad_Name,
		    ((thisArg->ad_Next != NULL) ? ", " : "), \n" )
		);
	    }

	    if(strcmp("void", thisFunc->fd_Type) == 0)
	    {
		fprintf(fd, "\tLP%dNR(0x%x, %s, ",
		    thisFunc->fd_NumArgs, thisFunc->fd_Offset * 6,
		    thisFunc->fd_Name
		);
	    }
	    else
	    {
		fprintf(fd, "\tLP%d(0x%x, %s, %s, ",
		    thisFunc->fd_NumArgs, thisFunc->fd_Offset * 6,
		    thisFunc->fd_Type, thisFunc->fd_Name
		);
	    }

	    thisArg = (struct ArgData *)&(thisFunc->fd_Args);
	    while(( thisArg = thisArg->ad_Next ))
	    {
		if(thisArg->ad_Reg2 != NULL)
		{
		    fprintf(fd, "%s, %s, %c%c, %c%c, ",
			thisArg->ad_Type, thisArg->ad_Name,
			tolower(thisArg->ad_Reg1[0]), thisArg->ad_Reg1[1],
			tolower(thisArg->ad_Reg2[0]), thisArg->ad_Reg2[1]
		    );
		}
		else
		{
		    fprintf(fd, "%s, %s, %c%c, ",
			thisArg->ad_Type,
			thisArg->ad_Name,
			tolower(thisArg->ad_Reg1[0]), thisArg->ad_Reg1[1]
		    );
		}
	    }

	    fprintf(fd, " \n\t %s_BASE_NAME)\n\n", 
		upperbasename
	    );
	}
    }
    fprintf(fd,	"#endif /* _INLINE_%s_H */\n", upperinclname );

    fclose(fd);
    moveifchanged(file, newfile);
    free(newfile);
    free(capsbasename);
    free(upperbasename);
    free(upperinclname);
}
