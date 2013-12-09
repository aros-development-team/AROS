/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: A very simple line parser. Basically from archtool.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <toollib/lineparser.h>

char *get_line(FILE *fd)
{
    int count, len;
    char *line;
    char buffer;

    len = 0;
    do
    {
	count = fread(&buffer, 1, 1, fd);
	len += count;
    } while(count != 0 && buffer != '\n');

    if(len == 0 && count == 0)
	return NULL;

    fseek(fd, -len, SEEK_CUR);
    line = malloc( (len+1) * sizeof(char) );
    count = fread(line, 1, len, fd);
    if (count != len) {
        free(line);
        return NULL;
    }
    line[len] = 0;
    len--;

    while(isspace(line[len]) && len >= 0)
    {
	line[len] = 0;
	len--;
    }

    return line;
}

char *keyword(char *line)
{
    char *key = NULL;
    int len;

    if(line[0] == '#')
    {
	len = 1;
	while(line[len] && !isspace(line[len]))
	    len++;

	key = malloc(len * sizeof(char));
	strncpy(key, &line[1], len - 1);
	key[len - 1] = 0;
    }
    return key;
}

int get_words(char *line, char ***outarray)
{
    char **array;
    char *word;
    int num, len;

    /* Make sure that we have valid input */
    if(!outarray || !line)
    {
	fprintf(stderr, "Passed NULL pointer to get_words()!\n");
	exit(-1);
    }

    /* Free the old array */
    array = *outarray;
    if(array)
    {
	while(*array)
	{
	    free(*array);
	    array++;
	}
	free(*outarray);
    }
    array = NULL;

    /* Now scan the list of words */
    num = 0;
    word = line;
    while(*word != 0)
    {
	/* Find the start of this word */
	while(*word && isspace(*word) )
	    word++;

	/* Find the end of this word */
	len = 0;
	while( word[len] && !isspace(word[len]) )
	    len++;

	/*
	    Copy this word into the array, making the array larger
	    in order to fit the pointer to the string.
	*/
	if(len)
	{
	    num++;
	    array = realloc(array, num * sizeof(char *));
	    array[num-1] = malloc((len+1) * sizeof(char));
	    strncpy( array[num-1], word, len);
	    array[num-1][len] = 0;
	    word = &word[len];
	}
    }

    array = realloc(array, (num+1) * sizeof(char *) );
    array[num] = NULL;

    *outarray = array;
    return num;
}
