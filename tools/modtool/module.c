/*
    Copyright (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Support for reading modules.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <toollib/toollib.h>
#include <toollib/error.h>
#include <toollib/lineparser.h>
#include <toollib/filesup.h>
#include <toollib/vstring.h>
#include "module.h"

// This helps with code later on down.
#define NODE_INIT(n,t,f,l) { \
    ((struct MDNode *)(n))->n_Type = (t); \
    ((struct MDNode *)(n))->n_FileName = (f); \
    ((struct MDNode *)(n))->n_LineNo = (l); \
    }

struct {
    char *typeStr;
    char *typeExt;
} typeData[] =
{
    {	"NT_LIBRARY",	".library" },
    {	"NT_DEVICE",	".device" },
    {	"NT_RESOURCE",	".resource" },
    {	"NT_DEVICE",	".handler" },
    {	"NT_DEVICE",	".handler" },
    {	"NT_UNKNOWN",	".class" },
    {	"NT_UNKNOWN",	".class" },
    {	"NT_LIBRARY",	".gadget" },
    {	"NT_LIBRARY",	".datatype" },
    {	"NT_LIBRARY",	".image" },
    {	"NT_UNKNOWN",	".hidd" },
    {	"NT_LIBRARY",	".engine" },
    {	"NT_UNKNOWN",	"" }
};

struct ModuleConfig *ReadModuleConfig(char *file)
{
    struct ModuleConfig *mc;
    FILE *fd;
    char *line, *word, **words = NULL;
    int i, num, len, linenum = 0;

    fd = fopen(file, "rb");
    if(!fd)
    {
	StdError("could not open file %s", file);
	return NULL;
    }

    mc = xmalloc(sizeof(struct ModuleConfig));
    if(!mc)
    {
	StdError("could not read file %s", file);
	fclose(fd);
	return NULL;
    }

    /*
	This is a fairly simple kind of parser, as we only deal with
	single line input, which is supposed to be in a specific order,
	although it may not be. Basically name and type MUST come before
	anything else...
    */
    while((line = get_line(fd)))
    {
	num = get_words(line, &words);
	linenum++;

	if(num > 1)
	{
	    if(strcmp(words[0],"name") == 0)
	    {
		strlower(words[1]);
		mc->name = xstrdup(words[1]);

		if(mc->basename == NULL)
		{
		    mc->basename = xstrdup(words[1]);
		    mc->basename[0] = toupper(mc->basename[0]);
		}
	    }

	    /*
		This is the base name of the data structure for this
		type.
	    */
	    else if(strcmp(words[0],"basename") == 0)
	    {
		cfree(mc->basename);
		mc->basename = xstrdup(words[1]);
	    }

	    /*
		These are the boring ones that allow us to be 
		more specific if we have any strangeness when we
		work with base names. For example dos.library and
		graphics.library aren't quite standard.
	    */
	    else if(strcmp(words[0],"base") == 0)
	    {
		cfree(mc->base);
		mc->base = xstrdup(words[1]);
	    }
	    else if(strcmp(words[0],"basetype") == 0)
	    {
		cfree(mc->basetype);
		len = num - 1;

		for(i = 1; i < num; i++)
		    len += strlen(words[i]);

		mc->basetype = malloc(len * sizeof(char));
		strcpy(mc->basetype, words[1]);

		for(i = 2; i < num; i++)
		{
		    strcat(mc->basetype, " ");
		    strcat(mc->basetype, words[i]);
		}
	    }
	    else if(strcmp(words[0],"basetypeptr") == 0)
	    {
		cfree(mc->basetypeptr);
		len = num - 1;

		for(i = 1; i < num; i++)
		    len += strlen(words[i]);

		mc->basetypeptr = malloc(len * sizeof(char));
		strcpy(mc->basetypeptr, words[1]);

		for(i = 2; i < num; i++)
		{
		    strcat(mc->basetypeptr, " ");
		    strcat(mc->basetypeptr, words[i]);
		}
	    }
#if 0
	    /*
		For such as the workbench.library, allow us to specify
		something slightly different for the include file name.
	    */
	    else if(strcmp(words[0], "include") == 0)
	    {
		cfree(mc->include);
		mc->include = xstrdup(words[1]);
	    }
#endif
	    else if(strcmp(words[0],"version") == 0)
	    {
		i = 0;

		/* Find the decimal separating version and revision */
		while(words[1][i] && words[1][i] != '.')
		    i++;

		mc->revision = (words[1][i] == 0 ? 0 : atoi(&words[1][i+1]));

		if(i == 0)
		{
		    mc->version = 1;
		}
		else
		{
		    words[1][i] = 0;
		    mc->version = atoi(words[1]);
		}
	    }

	    else if(strcmp(words[0], "copyright") == 0)
	    {
		word = &line[9];
		while(*word && isspace(*word))
		    word++;

		cfree(mc->copyright);
		mc->copyright = xstrdup(words[1]);
	    }
	    else if(strcmp(words[0], "type") == 0)
	    {
		if(strcmp(words[1], "library") == 0)
		    mc->type = MT_LIBRARY;
		else if(strcmp(words[1], "device") == 0)
		    mc->type = MT_DEVICE;
		else if(strcmp(words[1], "resource") == 0)
		    mc->type = MT_RESOURCE;
		else if(strcmp(words[1], "filesys") == 0)
		    mc->type = MT_FILESYS;
		else if(strcmp(words[1], "handler") == 0)
		    mc->type = MT_HANDLER;
		else if(strcmp(words[1], "boopsi") == 0)
		    mc->type = MT_BOOPSI;
		else if(strcmp(words[1], "oop") == 0)
		    mc->type = MT_OOP;
		else if(strcmp(words[1], "gadget") == 0)
		    mc->type = MT_GADGET;
		else if(strcmp(words[1], "image") == 0)
		    mc->type = MT_IMAGE;
		else if(strcmp(words[1], "datatype") == 0)
		    mc->type = MT_DATATYPE;
		else if(strcmp(words[1], "hidd") == 0)
		    mc->type = MT_HIDD;
		else if(strcmp(words[1], "fontengine") == 0)
		    mc->type = MT_FONTENGINE;
		else if(strcmp(words[1], "custom") == 0)
		    mc->type = MT_CUSTOM;
		else
		    Warn("unknown type \"%s\"", words[1]);
	    }
	    else if(strcmp(words[0], "option") == 0)
	    {
		int i;
		for(i = 1; i < num; i++)
		{
		    if(strcmp(words[i], "noexpunge") == 0)
			mc->option |= MO_NOEXPUNGE;
		    else if(strcmp(words[i], "romonly") == 0)
			mc->option |= MO_ROMONLY;
		    else if(strcmp(words[i], "hasrt") == 0)
			mc->option |= MO_HASRT;
		    else if(strcmp(words[i], "noautoinit") == 0)
			mc->option |= MO_NOAUTOINIT;
		    else if(strcmp(words[i], "noresident") == 0)
			mc->option |= MO_NORESIDENT;
		    else if(strcmp(words[i], "noextension") == 0)
			mc->option |= MO_NOEXTENSION;
		    else if(strcmp(words[i], "classlib") == 0)
			mc->option |= MO_CLASSLIB;
		}
	    }
	    else if(strcmp(words[0], "nodetype") == 0)
	    {
		cfree(mc->nodetype);
		mc->nodetype = xstrdup(words[1]);
	    }
	    else if(strcmp(words[0], "extension") == 0)
	    {
		cfree(mc->extension);
 		mc->extension = xstrdup(words[1]);
	    }
	    else
	    {
		Warn("unknown line \"%s\"", line);
	    }
	}
	xfree(line);
    }

    /*
	Now we fill in all those that we couldn't before, in
	ascending order of specification.
    */
    len = strlen(mc->name);

    if(mc->define == NULL)
	mc->define = strdup("_MODDEFS_H");

    if(mc->include == NULL)
    {
	mc->include = xstrdup(mc->name);
    }	    

    if(mc->nodetype == NULL)
    {
	mc->nodetype = xstrdup(typeData[mc->type].typeStr);
    }

    if(mc->extension == NULL)
    {
	if(mc->option & MO_NOEXTENSION)
	    mc->extension = xstrdup("");
	else
    	    mc->extension = xstrdup(typeData[mc->type].typeExt);
    }

    if(mc->basename == NULL)
    {
	mc->basename = xstrdup(mc->name);
	mc->basename[0] = toupper(mc->basename[0]);
    }
	
    if(mc->base == NULL)
    {
	mc->base = malloc( (len+5) * sizeof(char) );
	sprintf( mc->base, "%sBase", mc->basename);
    }

    if(mc->basetype == NULL)
    {
	mc->basetype = malloc( (len + 12) * sizeof(char) );
	sprintf( mc->basetype, "struct %s", mc->base);
    }

    if(mc->basetypeptr == NULL)
    {
	mc->basetypeptr = malloc( (len + 14) * sizeof(char) );
	sprintf( mc->basetypeptr, "%s *", mc->basetype);
    }

    if(mc->copyright == NULL)
	mc->copyright = strdup("");

    if(mc->option & MO_ROMONLY)
	mc->option |= MO_NOEXPUNGE;

    /* All done. */
    return mc;
}

void FreeModuleConfig(struct ModuleConfig *mc)
{
    cfree(mc->name);
    cfree(mc->basename);
    cfree(mc->copyright);
    cfree(mc->base);
    cfree(mc->basetype);
    cfree(mc->basetypeptr);
    cfree(mc->define);
    cfree(mc->include);
    cfree(mc->nodetype);
    cfree(mc->extension);
    cfree(mc);
}

struct ModuleData *ModuleStart(struct ModuleConfig *mc, int flags)
{
    struct ModuleData *md;

    md = xmalloc(sizeof(struct ModuleData));
    if(md != NULL)
    {
	md->md_MemoryLink = NULL;
	md->md_Config	= mc;
	md->md_Flags	= flags;
	NewList(&md->md_Data);
    }
    return md;
}

void ModuleEnd(struct ModuleData *md)
{
    struct MDNode *mn;
    void **mem;

    /* Free the module data. This is a long process */

    while(!IsListEmpty( &md->md_Data ))
    {
	mn = (struct MDNode *)RemHead( &md->md_Data );
	switch(mn->n_Type)
	{
	    case T_Function:
	    {
		struct FunctionData *fd = (struct FunctionData *)mn;
		struct ArgData *ad, *ad2;

		ad = fd->fd_Args;
		while(ad != NULL)
		{
		    ad2 = ad->ad_Next;
		    xfree(ad);
		    ad = ad2;
		}
		cfree(fd->fd_AutoDoc);
		cfree(fd->fd_Code);
		cfree(fd->fd_Type);
		cfree(fd->fd_Name);

		xfree(fd);
	    }
	    break;

	    case T_CPP:
	    case T_Raw:
		cfree(((struct CPPData *)mn)->cpd_Data);
		break;
	}
    }

    /*
	Free the linked memory
    */
    mem = md->md_MemoryLink;
    while(mem != NULL)
    {
	void *next = *mem;
	free(mem);
	mem = next;
    }

    xfree(md);
}

void ModuleNext(struct ModuleData *md, char *file)
{
    struct FunctionData *newFunc = NULL;
    struct ArgData *arg = NULL, *newArg = NULL;
    char *line, **words = NULL, *fileCopy;
    int num, linenum = 0;
    FILE *fd;

    fileCopy = xmalloc(strlen(file) + sizeof(size_t));
    strcpy(&fileCopy[4], file);
    *((void **)fileCopy) = md->md_MemoryLink;
    md->md_MemoryLink = fileCopy;
    fileCopy = &fileCopy[4];

    fd = fopen(file, "rb");
    if(!fd)
    {
	StdError("could not open file %s\n", file);
	return;
    }

    while((line = get_line(fd)))
    {
	num = get_words(line, &words);
	linenum++;

	/* Blank lines are valid */
	if(num == 0)
	    continue;

	/*
	    This is the start of a function:

	    #Function <rettype> <name>
	*/
	if( strcmp("#Function", words[0]) == 0 )
	{
	    newFunc = xcalloc(1, sizeof(struct FunctionData));
	    NODE_INIT(newFunc,T_Function,fileCopy,linenum);

	    newFunc->fd_Type = xstrdup(words[1]);
	    newFunc->fd_Name = xstrdup(words[2]);
	    newFunc->fd_Options |= FO_UseBase;

	    /* Have to reset arguments */
	    newArg = arg = NULL;
	}

	/*
	    This is the end of a function definition

	    #/Function
	*/
	else if( strcmp("#/Function", words[0]) == 0)
    	{
	    AddTail(&md->md_Data, (Node *)newFunc);
	}

	/*
	    A Parameter, may have an option second register.
	    If it does, the whole function is quad matched.

	    #Arg    <type> <name> <reg1> [<reg2>]
	*/
	else if(strcmp("#Arg", words[0]) == 0)
	{
	    if(newFunc != NULL)
	    {
		newArg = xcalloc(1, sizeof(struct ArgData));

		newArg->ad_Type = xstrdup(words[1]);
		newArg->ad_Name = xstrdup(words[2]);
		newArg->ad_Reg1 = xstrdup(words[3]);
		newFunc->fd_NumArgs++;

		if(num == 5)
		{
		    newArg->ad_Reg2 = xstrdup(words[4]);
		    newFunc->fd_Options |= FO_HasQuad;
		}
		else
		    newArg->ad_Reg2 = NULL;

		if(newFunc->fd_Args == NULL)
		    newFunc->fd_Args = newArg;

		if(arg != NULL)	arg->ad_Next = newArg;
		arg = newArg;
	    }
	    else
	    {
		Error("Argument outside of function %s:%d\n", file, linenum);
	    }
	}

	/*
	    Offset in the library.

	    #LibOffset <offset>
	*/
	else if(strcmp("#LibOffset", words[0]) == 0)
	{
	    if(newFunc != NULL)
		newFunc->fd_Offset = strtoul(words[1], NULL, 10);
	    else
		Error("#LibOffset outside of function %s:%d\n", file, linenum);
	}

	/*
	    Options for the library function

	    #Options ...
	*/	    
	else if(strcmp("#Options", words[0]) == 0)
	{
	}

	else if(strcmp("#Code", words[0]) == 0)
	{
	    if(newFunc != NULL)
	    {
	    }
	    else
	    {
		Error("#Code outside of #Function %s:%d\n", file, linenum);
	    }
	}

	else if(strcmp("#/Code", words[0]) == 0)
	{
	}

	else if(strcmp("#AutoDoc", words[0]) == 0)
	{
	    if(newFunc != NULL)
	    {
	    }
	    else
	    {
		Error("#Code outside of #Function %s:%d\n", file, linenum);
	    }
	}

	else if(strcmp("#/AutoDoc", words[0]) == 0)
	{
	}

	/* Special, we must keep a track of includes and defines */
	else if(
	    (strcmp("#include", words[0]) == 0)
	 || (strcmp("#define", words[0]) == 0)
	 || (strncmp("#if", words[0], 3) == 0)
	 || (strcmp("#endif", words[0]) == 0)
	 || (strcmp("#undef", words[0]) == 0)
	 || (strncmp("#el", words[0], 3) == 0) )
	{
	    /*
		Just add the line as itself to the module.
		These are supposed to come in the correct order in
		the files...
	    */
	    struct CPPData *cpd;

	    cpd = xcalloc(1, sizeof(struct CPPData));
	    NODE_INIT(cpd,T_CPP,fileCopy,linenum);

	    cpd->cpd_Data = xstrdup(line);
	    AddTail(&md->md_Data, (Node *)cpd);
	}
#if 1
	else if(*line == '#')
	{
	    Warn("Unknown command: %s at %s:%d\n", words[0], file, linenum);
	}
#endif
	/*  RAW DATA - Just copy it through. */
	else if((md->md_Flags & MD_ScanCode))
	{
	    struct CPPData *cpd;

	    cpd = xcalloc(1, sizeof(struct CPPData));
	    NODE_INIT(cpd,T_Raw,fileCopy,linenum);
	    cpd->cpd_Data = xstrdup(line);
	    AddTail(&md->md_Data, (Node *)cpd);
	}

	cfree(line);
    }
    fclose(fd);
}
