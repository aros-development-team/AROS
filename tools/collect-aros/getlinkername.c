/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>

void fatalerror(int doexit)
{
    if (doexit)
    {
        if (errno) perror("getlinkername: Internal error");
        exit(1);
    }
}

#define BUFSIZE 200
char *safegets(void)
{
    static char *buf=NULL;
    static int size = 0;
    int pos = 0;
    int c = EOF;

    do
    {
	size += BUFSIZE;
	fatalerror(!(buf = realloc(buf, size)));

	for (; pos < size && (c=fgetc(stdin))!=EOF && c!='\n'; pos++)
	{
	    //printf(">>>%d\n", c);
	    buf[pos] = c;
        }
    } while (c != EOF && c!='\n');

    fatalerror(ferror(stdin));

    if (feof(stdin) && pos == 0)
    	return NULL;

    buf[pos] = '\0';
    return buf;
}


int main(void)
{
    char *line;

    while ((line=safegets()))
    	if (strcmp(line, "*linker:") == 0)
	    break;

    line = safegets();
    if (line)
    	printf("%s\n", line);

    return 0;
}




