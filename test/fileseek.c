#include <stdio.h>
#include <proto/dos.h>
#include <dos/dos.h>

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
  i = fread( buffer, 1, 7, fd );
  if( i != 7 )
  {
    fprintf( stderr, "Wanted to fread() %d chars, but could only get %d!\n", 11, i );
    exit(-1);
  }
  buffer[i] = 0;
  printf( "fseek%s", buffer );
  fseek( fd, 4, SEEK_CUR );
  i = fread( buffer, 1, 11, fd );
  buffer[i] = 0;
  printf( "%s", buffer );
  fclose(fd);

/* Seek() */
  file = Open( "seek.txt", MODE_OLDFILE );
  i = Read( file, buffer, 7 );
  buffer[i] = 0;
  printf( " and Seek%s", buffer );
  Seek( file, 4, OFFSET_CURRENT );
  i = Read( file, buffer, 11 );
  buffer[i] = 0;
  printf( "%s", buffer );
  
  return 0;
}
