/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.

    Desc: The functions to read lines from a file
*/
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

static char *line = NULL; /* The current read file */
static char *filename = NULL; /* The name of the opened file */
static FILE *file = NULL; /* The opened file */
static unsigned int slen = 0; /* The allocation length pointed to be line */
static unsigned int lineno = 0; /* The line number, will be increased by one everytime a line is read */

int fileopen(const char *fname)
{
    if (file!=NULL)
    {
	fclose(file);
	free(filename);
	file = NULL;
	filename = NULL;
	lineno = 0;
    }
    file = fopen(fname, "r");
    if (file!=NULL)
	filename = strdup(fname);
    
    return file!=NULL;
}

void fileclose(void)
{
    if (file!=NULL)
    {
	fclose(file);
	free(filename);
	file = NULL;
	filename = NULL;
    }
}

char *readline(void)
{
    char haseol;

    if (file==NULL || feof(file))
	return NULL;

    if (slen==0)
    {
	slen = 256;
	line = malloc(slen);
    }
    if (fgets(line, slen, file))
    {
	haseol = line[strlen(line)-1]=='\n';
	if (haseol) line[strlen(line)-1]='\0';
	
	while (!(haseol || feof(file)))
	{
	    slen += 256;
	    line = (char *)realloc(line, slen);
	    fgets(line+strlen(line), slen, file);
	    haseol = line[strlen(line)-1]=='\n';
	    if (haseol) line[strlen(line)-1]='\0';
	}
    }
    else
	line[0]='\0';
    lineno++;
    
    return line;
}

void filewarning(const char *format, ...)
{
    va_list ap;

    fprintf(stderr, "%s:%d:warning ", filename, lineno);
    
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
}

void exitfileerror(int code, const char *format, ...)
{
    va_list ap;
    
    fprintf(stderr, "%s:%d:error ", filename, lineno);
    
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    
    exit(code);
}
