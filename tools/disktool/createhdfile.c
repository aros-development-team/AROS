#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<dirent.h>

#include <adflib.h>

/*
void testfile ( struct Volume *vol, char * name, char *text )
{
    long count, length;
    struct File *file;

    file = adfOpenFile ( vol, name, "w" );
    if ( file == NULL )
    {
	fprintf ( stderr, "Could not write file\n" );
	exit ( -1 );
    }
    length = strlen ( text );
    count = adfWriteFile ( file, length, (unsigned char *)text );
    if ( count != length )
    {
	fprintf ( stderr, "Could only write %ld out of %ld bytes\n", count, length );
	exit ( -1 );
    }
    adfCloseFile ( file );
}
*/

void copyfile ( struct Volume * vol, char * name, char * full )
{
    FILE *fd;
    int count;
    struct File *file;
    unsigned char buffer[1024];

    file = adfOpenFile ( vol, name, "w" );
    if ( file == NULL )
    {
	fprintf ( stderr, "Could not write file\n" );
	exit ( -1 );
    }
    fd = fopen ( full, "rb" );
    do
    {
	count = fread ( buffer, 1, 1024, fd );
	if ( count != adfWriteFile ( file, count, buffer ) )
	{
	    fprintf ( stderr, "Could not finish write of file %s\n", name );
	    exit ( -1 );
	}
    } while ( count == 1024 );

    fclose ( fd );
    adfCloseFile ( file );
}

void gotodir ( struct Volume * vol, char * name, char * base )
{
    DIR * dp;
    struct dirent * dirent;
    char buffer[1024];
    char *s;

    s = name;
    while ( *s )
	s++;
    while ( *s != '/' && s>name )
	s--;
    s++;

    if ( !(dp = opendir( name )) )
    {
	copyfile ( vol, s, name );
	return;
    }
    else
    {
	if ( strcmp ( name, base ) )
	{
	    adfCreateDir ( vol, vol->curDirPtr, s );
	    adfChangeDir ( vol, s );
	}
    }

    dirent = readdir( dp );
    while( dirent )
    {
        if ( strcmp ( dirent->d_name, "." ) && strcmp ( dirent->d_name, ".." ) )
        {
          sprintf ( buffer, "%s/%s", name, dirent->d_name );
          gotodir ( vol, buffer, base );
        }
        dirent = readdir( dp );
    }
    adfParentDir ( vol );
}

int main(int argc, char* argv[])
{
    struct Device *dev;
    struct Volume *vol;

    char *dirname;
    DIR * dp;

    long cylinders, heads, sectors;
    char *name;

    if ( argc != 6 && argc != 4 )
    {
	fprintf ( stdout, "Usage: %s filename C H S directory\n", argv[0] );
	fprintf ( stdout, "       %s filename floppy1440 directory\n", argv[0] );
	exit ( 0 );
    }
    if ( argc == 4 )
    {
	if ( strcmp (argv[2], "floppy1440") == 0 )
	{
	    cylinders = 80;
	    heads     = 1;
	    sectors   = 36;
	    dirname = argv[3];
	}
	else
	{
 	    fprintf ( stderr, "%s: supported floppy types: floppy1440\n", argv[0] );
	    exit ( -1 );
	}
    }
    else
    {
        cylinders = atol ( argv[2] );
        heads     = atol ( argv[3] );
        sectors   = atol ( argv[4] );
        dirname = argv[5];
    }
    name = strdup ( argv[1] );

    adfEnvInitDefault();

    dev = adfCreateDumpDevice ( name, cylinders, heads, sectors );

    adfCreateHdFile ( dev, "AROS-Boot", 1 );

//    fprintf ( stdout, "DeviceInfo:\n-----------\n" );
//    adfDeviceInfo ( dev );

    vol = adfMount ( dev, 0, 0 );

//    adfCreateDir ( vol, vol->curDirPtr, "blah" );
//    adfChangeDir ( vol, "blah" );
//    testfile ( vol, "testfile.ext", "Hello world of Amiga Hardfiles!\n" );
//    adfParentDir ( vol );

    if ( !(dp = opendir( dirname )) )
    {
	fprintf ( stdout, "No such dir %s\n", dirname );
	exit ( -1 );
    }
    else
    {
	gotodir ( vol, dirname, dirname );
    }

//    fprintf ( stdout, "VolumeInfo:\n-----------\n" );
//    adfVolumeInfo ( vol );

    adfUnMount ( vol );
    adfUnMountDev ( dev );

    return ( 0 );
}
