/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "parsendkoffsets.h"
#include "header.h"

static char *skipspaces(char *start)
{
	while (start[0] == ' ')
		start++;
	return start;
}

static inline char *escapematch(char *p)
{
	while (p[0] != '\0')
	{
		if (p[0] == ':')
			return p;
		else if (p[0] == '/')
			return p;
		else if (p[0] == '.')
			return p;
		else
			p++;
	}
	return NULL;
}

static void escapeheader(char *buffer)
{
	char* p;
    for (p = buffer; (p = escapematch(p)); ++p) {
        *p = '_';
    }
}

void makefullpath(char *path)
{
    struct stat statBuf;
    char *str, *s;

    s = path;
 	while ((str = strtok (s, "/")) != NULL) {
        if (str != s) {
            str[-1] = '/';
        }
        if (stat (path, &statBuf) == -1) {
            mkdir (path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        } else {
            if (! S_ISDIR (statBuf.st_mode)) {
                fprintf (stderr, "cannot create directory %s\n", path);
                exit (1);
            }
        }
        s = NULL;
    }
}

static void writePrologue(FILE *structfile, char *header, char *structname)
{
	printBanner(structfile, "//");
	fprintf(structfile, "\n#include <stddef.h>\n#include <stdio.h>\n");
	fprintf(structfile, "#include <%s>\n", header);
	fprintf(structfile, "\nint\nmain (int argc, char ** argv)\n{\n\tstruct %s strc_%s;\n\tint retval = 0;\n", structname, structname);
	fprintf(structfile, "\n\t\tprintf(\"Validating 'struct %s'\\n\");\n", structname);
}

static void writeEpilogue(FILE *structfile)
{
	fprintf(structfile, "\n\treturn retval;\n}\n");
}

int
parsendkoffsets (char *offfile, char *sdkdir, char *gendir, char *bindir)
{
	FILE *srcfile = NULL;
	FILE *structfile = NULL;
	char *structName, *structFileName, *headerName, *headerTestName, *line = NULL;
	int retval = 0;

    if (offfile == NULL)
		return retval;

	srcfile = fopen(offfile, "r");
    if (srcfile != NULL)
	{
		makefullpath(gendir);

		line = malloc(LINELENGTH);
		structName = malloc(LINELENGTH);
		structFileName = malloc(LINELENGTH);

		while (fgets(line, LINELENGTH, srcfile) != NULL)
		{       
			if (line[0] != ' ')
			{
				// new struct
				line[strlen(line) - 2] = '\0';
				sprintf(structName, "%s", line);
				if (structfile)
				{
					writeEpilogue(structfile);
					fclose(structfile);
					if (headerTestName)
					{
						headerTestName = NULL;
					}
					structfile = NULL;
					headerName = NULL;
				}
				headerName = findStructHeader(sdkdir, structName);
				if (headerName)
				{
					headerTestName = malloc(strlen(headerName) + 1);
					memcpy(headerTestName, headerName, strlen(headerName));
					headerTestName[strlen(headerName)] = '\0';
					escapeheader(headerTestName);

					sprintf(structFileName, "%s/test-struct_%s.c", gendir, structName);

					if (verbose)
					{
						printf ("Creating '%s'\n", structFileName);
					}
					structfile = fopen(structFileName, "w");
					if (!structfile)
					{
						printf ("Failed to open '%s'\n", offfile);
						free(structName);
						free(structFileName);
						return 1;
					}
					writePrologue(structfile, headerName, structName);
					addSDKHeaderStruct(headerTestName, structName);
				}
				else if (verbose)
				{
					// Skip the structure
					printf ("Failed to locate header file for struct %s\n", structName);
				}
			}
			else if (structfile && (!strncmp(&line[15], "sizeof(", 7)))
			{
				// struct size
				int structSize = atoi(skipspaces(&line[7]));
				line[strlen(line) - 2] = '\0';
				fprintf(structfile, "\n\tif (sizeof(strc_%s) != %d)\n\t{\n\t\tprintf(\"%s->%s: sizeof(struct %s) != %d\\n\");\n\t}\n", &line[22], structSize, headerName, structName, &line[22], structSize);
			}
			else if (structfile)
			{
				// offset line
				char *elementName = skipspaces(&line[19]);
				int elementOffset = atoi(skipspaces(&line[7])), elementSize = atoi(skipspaces(&line[13]));
				elementName[strlen(elementName) - 1] = '\0';
				fprintf(structfile, "\n\tif (offsetof(struct %s,%s) != %d)\n\t{\n\t\tprintf(\"%s->%s: -   offsetof(struct %s,%s) != %d\\n\");\n\t}\n", structName, elementName, elementOffset, headerName, structName, structName, elementName, elementOffset);
				fprintf(structfile, "\n\tif (sizeof(strc_%s.%s) != %d)\n\t{\n\t\tprintf(\"%s->%s:  -  sizeof(%s.%s) != %d\\n\");\n\t}\n", structName, elementName, elementSize, headerName, structName, structName, elementName, elementSize);
			}
		}
		if (structfile)
		{
			writeEpilogue(structfile);
			fclose(structfile);
		}
		writeHeaderMakefile(gendir, bindir);
		fclose(srcfile);
		free(structName);
		free(structFileName);
	}
	else
	{
		printf ("Failed to open '%s'\n", offfile);
		retval = 1;
	}

	return retval;
}
