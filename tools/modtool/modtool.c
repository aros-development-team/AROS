/*
    Copyright (C) 1995-1999 AROS - The Amiga Research OS
    $Id$

    Desc: Main program for modtool.

    NB: This program probably has memory leaks. I do not free()
        everything I explicitly malloc(), but this is only
        occurring in main(), so it shouldn't be too bad.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <toollib/toollib.h>
#include <toollib/error.h>

#include "module.h"
#include "modtool.h"

int main(int argc, char **argv)
{
    char *modConfFile = NULL,
	 *modDefsFile = NULL,
	 *includePath = NULL,
	 *endTagPath = NULL,
	 *clibIncFile = NULL,
	 *protoIncFile = NULL,
	 *defineIncFile = NULL,
	 *inlineIncFile = NULL,
	 *pragmaIncFile = NULL,
	 *headersTmpl = NULL;

    char **fileArray = NULL;

    struct ModuleData *md = NULL;
    struct ModuleConfig *mc = NULL;
    int i, fileCount = 0, modFlags = 0;

    /* Parse command line arguments */
    for(i=1; i < argc; i++)
    {
	char *arg = argv[i];

	/*
	    Neat little trick, makes something of an assumption that
	    we will never have filenames beginning with -- though :-)
	*/
	if(strncmp(arg, "--", 2) == 0 && strncmp(argv[i+1], "--", 2) == 0)
	{
	    Error("No argument specified for %s.", arg);
	    return -1;
	}

	/* Important the main configuration file. */
	if(strcmp(arg, "--conf") == 0)
	{
	    modConfFile = argv[i+1];
	    i++;
	}

	/*
	    Generate the output of the libdefs.h file to here.
	    This acts both as a switch to enable the output and the
	    output file name, which is always required.
	*/
	else if(strcmp(arg, "--defs") == 0)
	{
	    modDefsFile = argv[i+1];
	    i++;
	}

	/*
	    Tell us to generate the includes. This always requires
	    an argument. The individual file names can be changed
	    with the arguments below.
	*/
	else if(strcmp(arg, "--include") == 0)
	{
	    modFlags = MD_ScanHeader;
	    includePath = argv[i+1];
	    i++;
	}

#if 0
	/*
	    Generate the EndTag of the library in a separate
	    file. Really there is little point to this, but I
	    might as well be complete.
	*/
	else if(strcmp(arg, "--endtag") == 0)
	{
	    endTagPath = argv[i+1];
	    i++;
	}
#endif

	/*
	    headers.tmpl contains some extra information to stuff into
	    the headers. We have to be told where the file is.
	*/
	else if(strcmp(arg, "--hdrtmpl") == 0)
	{
	    headersTmpl = argv[i+1];
	    i++;
	}

	/*
	    Must be a file that we read source code from.
	*/
	else
	{
	    /* Add another entry... */
	    fileCount++;
	    fileArray = realloc(fileArray, fileCount * sizeof(char *));
	    fileArray[fileCount-1] = argv[i];
	}
    }

    if(modConfFile == NULL)
    {
	Error("No value given for --conf argument.");
	return -1;
    }

    /*
	Start by reading the module.conf file.
    */
    mc = ReadModuleConfig(modConfFile);

    /*
	Generate moddefs.h, but only if we are told to...
    */
    if(modDefsFile != NULL)
    {
	writeModDefs(modDefsFile, mc);
    }

    /*
	Read in all the functions from the rest of the files.
    */
    if(modFlags != 0)
    {
	md = ModuleStart(mc, modFlags);

	/*
	    Read in and parse the files. The amount read depends
	    upon the value of the modFlags variable. This is set
	    during command line argument parsing.
	*/
	for(i = 0; i < fileCount; i++)
	{
	    ModuleNext(md, fileArray[i]);
	}
    }

    /*
	Next step, generate the include files, if we have been asked
    */
    if(includePath != NULL)
    {
	int nameCount = strlen(mc->include) + strlen(includePath);

	/*
	    Create the default values of the names, all these are
	    relative to include path. Which should NOT have a trailing
	    path separator, whatever that may be...

	    Note, we are quite able to generate all these at once,
	    since if one of them changes, then the likelyhood is that
	    they will all change. This is because they all rely on
	    common data.
	*/
	if(clibIncFile == NULL)
	{
	    clibIncFile = malloc( (nameCount + 16) * sizeof(char));
	    sprintf(clibIncFile, "%s/clib/%s_protos.h",
		includePath, mc->include);
	}
	if(protoIncFile == NULL)
	{
	    protoIncFile = malloc( (nameCount + 10) * sizeof(char));
	    sprintf(protoIncFile, "%s/proto/%s.h",
		includePath, mc->include);
	}
	if(defineIncFile == NULL)
	{
	    defineIncFile = malloc( (nameCount + 12) * sizeof(char));
	    sprintf(defineIncFile, "%s/defines/%s.h",
		includePath, mc->include);
	}
	if(inlineIncFile == NULL)
	{
	    inlineIncFile = malloc( (nameCount + 11) * sizeof(char));
	    sprintf(inlineIncFile, "%s/inline/%s.h",
		includePath, mc->include);
	}

	/*
	if(pragmaIncFile == NULL)
	{
	    pragmaIncFile = malloc( (nameCount + 20) * sizeof(char));
	    sprintf(pragmaIncFile, "%s/pragmas/%s_pragmas.h",
		includePath, mc->include);
	}
	*/

	/* So, now we have the names, lets create some files. */
	genInclClib(clibIncFile, md, headersTmpl);
	genInclProto(protoIncFile, md, headersTmpl);
	genInclDefine(defineIncFile, md, headersTmpl);
	genInclInline(inlineIncFile, md, headersTmpl);

    }
    FreeModuleConfig(mc);
    ModuleEnd(md);
    return 0;
}
