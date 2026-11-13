/*
    Copyright © 2004, Martin Gierich. All rights reserved.
    Licensed under the terms of the AROS Public License (APL)
    $Id$

    Desc: Console only test application for HTML parser and layout engine
*/

#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "main.h"

/*******************************************************************************************/
/* Main */

int main( int argc, char **argv )
{
	FILE		*infp;
	string		infile;
	int		filelen, bytes;
	string		inbuf;
	int		ret;
	int		width, height, xsize;
	page_struct	*page;

	if(argc != 3)
	{
		printf("Usage: %s <html-doc> <xsize>\n", argv[0]);
		return 0;
	}
	infile = argv[1];
	xsize = atoi( argv[2] );

	infp = fopen( infile, "r" );
	if( infp == NULL )
	{
		printf("Cannot open %s\n", infile);
		return 10;
	}
	fseek( infp, 0, SEEK_END );
	filelen = ftell( infp );
	fseek( infp, 0, SEEK_SET );
	printf("File length: %d\n", filelen);
	if( filelen <= 0 )
		return 10;

	inbuf = malloc( filelen );
	if( !inbuf )
		return 10;

	bytes = fread(inbuf, 1, filelen, infp);
	printf("Bytes read: %d\n", bytes);
	if( bytes <= 0 )
		return 10;
	
	page = parse_init( NULL );
	if( !page )
		return 10;
	printf("parse_init done, page %p\n", page);
	ret = parse_do( page, inbuf, bytes );
	if( !ret )
		return 10;
	printf("parse_do done\n");
	ret = parse_end( page );
	if( !ret )
		return 10;
	printf("parse_end done\n");

	free( inbuf );

	ret = layout_init( page );
	if( !ret )
		return 10;
	printf("layout_init done\n");

	ret = layout_do( page, xsize, &width, &height );
	if( !ret )
		return 10;
	printf("layout_do done\n");
	printf("Result: width=%d height=%d\n", width, height);

	layout_free( page );
	printf("layout_free done\n");

	parse_free( page );
	printf("parse_free done\n");

	return 0;
}

