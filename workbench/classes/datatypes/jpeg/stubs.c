/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

/* There are some functions that are never used, 
 * but are required for proper linking against libjpeg.  
 */

#include <aros/system.h>

#include <stdio.h>

int ferror(FILE *stream)
{
	return -1;
}

int fflush(FILE *stream)
{
	return -1;
}

int fprintf(FILE *stream, const char *format, ...)
{
	return -1;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	return -1;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	return -1;
}
