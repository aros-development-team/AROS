/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct inclist {
    struct inclist *next;
    char *text;
};

FILE * fd, * fdo;

#define _read(stream,buff,count)    fread(buff,1,count,stream)

int main ( int argc, char **argv )
{
int count;
int i, filecount;
char fbuf[50];

/* Starting sequence of a C comment: '\slash','\asterisk' */
char c_comment[3] = { 0x2f, 0x2a, 0x00 };
/* Starting sequence of a C++ comment: '\slash','\slash' */
char cpp_comment[3] = { 0x2f, 0x2f, 0x00 };

char ft[] = "zyx";
struct inclist first = { NULL, ft }, *current, *search;

char bracket;
char incname[50];
char filename[50];

    fdo = fopen ( "functions.c", "w" );
    if ( fdo == NULL )
    {
	fprintf ( stderr, "Could not open functions.c out-file!\n" );
	exit ( -1 );
    }
    fprintf ( fdo, "#include \"functions.h\"\n" );

    printf ( "Collecting functions..." );
    for ( filecount = 1 ; filecount < argc ; filecount++ )
    {
/*	printf ( "Opening %s\n", argv[filecount] ); */
	strcpy ( filename, argv[filecount] );
	strcat ( filename, ".c" );
	fd = fopen ( filename, "rb" );
	if ( fd == NULL )
	{
	    fprintf ( stderr, "\n%s - No such file !\n", argv[filecount] );
	    exit(-1);
	}

	fprintf ( fdo, "#line 1 \"%s\"\n", filename );
	count = _read ( fd, fbuf, 1 );
	while ( count == 1 )
	{
	    switch ( fbuf[0] )
	    {
		case '/':
		    count = _read( fd, &fbuf[1], 1 );
		    if ( fbuf[count] == '*' )
		    {
			/* This is a C comment, copy everything until
			   the end of the comment */
			fprintf ( fdo, c_comment );
			do
			{
			    do
			    {
				count = _read ( fd, fbuf, 1 );
				putc ( fbuf[0], fdo );
			    } while ( count == 1 && fbuf[0] != '*' );
		    	    count = _read( fd, fbuf, 1 );
			    putc ( fbuf[0], fdo );
			} while ( count == 1 && fbuf[0] != '/' );
		    }
		    else if ( fbuf[count] == '/' )
		    {
			/* This is a C++ comment, copy everything until
			   the end of the current line */
			fprintf ( fdo, cpp_comment );
			do
			{
			  count = _read( fd, fbuf, 1 );
			  putc ( fbuf[0], fdo );
			} while ( count == 1 && fbuf[0] != '\n' );
		    }
		    else
		    {
			if ( fseek ( fd, -count, SEEK_CUR ) != 0 )
			{
			    perror ( "fseek" );
			    exit ( -1 );
			}
			putc ( fbuf[0], fdo );
		    }
		    break;
		case '#':
		    count = _read ( fd, &fbuf[1], 8 );
		    fbuf[count+1] = 0;
		    if ( strcmp ( fbuf, "#include " ) == 0 )
		    {
			_read ( fd, incname, 1 );
		        if ( incname[0] == '<' || incname[0] == 0x22 )
			{
			    if ( incname[0] == '<' )
			    {
				bracket = '>';
			    }
			    else
			    {
				bracket = 0x22;
			    }
			    i = 0;
			    do {
				i++;
				_read ( fd, &incname[i], 1 );
			    } while ( incname[i] != bracket );
			    incname[i+1] = 0;
			    search = &first;
			    while ( search->next != NULL && strcmp ( search->text, incname) !=0  )
			    {
				search = search->next;
			    }
			    if ( search->next == NULL && strcmp ( search->text, incname ) != 0 )
			    {
				current = malloc ( sizeof ( struct inclist ) );
				current->next = NULL;
				current->text = strdup ( incname );
				search->next = current;
			    }
			}
			else
			{
#warning this is buggy: incname doesn't contain any proper null terminated string
			    fprintf ( fdo, "#include %s", &incname[0] );
			}
		    }
		    else
		    {
			if ( fseek ( fd, -count, SEEK_CUR ) != 0 )
			{
			    perror ( "fseek" );
			    exit ( -1 );
			}
			putc ( fbuf[0], fdo );
		    }
		    if( count > 0 )
			count = 1;
		    break;
		default:
		    putc ( fbuf[0], fdo );
		    break;
	    }
	    count = _read ( fd, fbuf, 1 );
	}
	/* If the last character of the file is not a newline '\n'
	   then append a newline character to avoid faulty concatenation
	   of files */
	if ( fbuf[0] != '\n' )
	{
	  putc ( '\n', fdo );
	}
	fclose ( fd );
    }
    fclose ( fdo );

    fdo = fopen ( "functions.h", "w" );
    if ( fdo == NULL )
    {
	fprintf ( stderr, "Could not open functions.h out-file!\n" );
	exit ( -1 );
    }

    search = first.next;
    while ( search != NULL )
    {
	current = search;
	fprintf ( fdo, "#include %s\n", current->text );
	search = current->next;
	free ( current->text );
	free ( current );
    }
    fclose ( fdo );

    printf ( "Done!\n" );

    return ( 0 );
}
