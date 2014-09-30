/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <stdlib.h>

int main()
{
FILE *fd;
char buffer[32];
int i;
BPTR file;

  fd = fopen( "seek.txt", "wb" );
  if ( !fd )
  {
    fprintf( stderr, "Could not write test file seek.txt\n" );
    exit(-1);
  }
  fprintf( fd, "() does not work!\n" );
  fclose(fd);

/* fseek() */
  fd = fopen( "seek.txt", "rb" );
  if ( !fd )
  {
    fprintf( stderr, "Could not open test file seek.txt\n" );
    exit(-1);
  }
  i = fread( buffer, 1, 1, fd );
//printf("pos=%ld\n",ftell(fd));
  i += fread( &buffer[1], 1, 6, fd );
  if( i != 7 )
  {
    fprintf( stderr, "Wanted to fread() %d chars, but could only get %d!\n", 6, i-1 );
    exit(-1);
  }
  fseek( fd, 4, SEEK_CUR );
  i = fread( &buffer[7], 1, 11, fd );
  buffer[7+i]=0;
  printf( "fseek%s", buffer );
  fclose(fd);

/* Seek() */
  file = Open( "seek.txt", MODE_OLDFILE );
  i = Read( file, buffer, 7 );
  Seek( file, 4, OFFSET_CURRENT );
  i += Read( file, &buffer[7], 11 );
  buffer[i] = 0;
  printf( "\nSeek%s", buffer );
  
  return 0;
}
