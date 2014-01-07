#include <hidd/unixio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <stdio.h>

#define O_RDONLY    0
#define O_WRONLY    1
#define O_RDWR      2

struct Library *OOPBase = NULL;

int main (int argc, char **argv)
{
    int failed = 1;
    struct Library *UnixIOBase = NULL;
    OOP_Object *unixio = NULL;
    int fd = -1;
    int nbytes, ioerr;
    char buf[1024];

    if ((OOPBase = OpenLibrary("oop.library", 0)) == NULL) {
        fprintf(stderr, "can't open oop.library\n");
        goto exit;
    }

    UnixIOBase = OpenLibrary("DEVS:Drivers/unixio.hidd", 42);
    if (!UnixIOBase)
    {
    	fprintf(stderr, "can't open unixio.hidd\n");
    	goto exit;
    }

    if ((unixio = OOP_NewObject(NULL, CLID_Hidd_UnixIO, NULL)) == NULL) {
        fprintf(stderr, "can't instantiate unixio hidd\n");
        goto exit;
    }


    printf("first, a trivial file read test\n\n");

    printf("opening /dev/zero for read... ");
    fd = Hidd_UnixIO_OpenFile(unixio, "/dev/zero", O_RDONLY, 0, &ioerr);
    if (fd == -1) {
        printf("failed (ioerr is %d)\n)", ioerr);
        goto exit;
    }
    printf("ok (fd is %d)\n", fd);

    printf("reading... ");
    nbytes = Hidd_UnixIO_ReadFile(unixio, fd, buf, 1024, &ioerr);
    if (nbytes == -1) {
        printf("failed (ioerr is %d\n)", ioerr);
        goto exit;
    }
    printf("ok (read %d bytes)\n", nbytes);

    printf("closing file... ");
    if (Hidd_UnixIO_CloseFile(unixio, fd, &ioerr) == -1)
    {
        printf("failed (ioerr is %d\n)", ioerr);
        goto exit;
    }
    printf("ok\n\n\n");


    printf("next, an equally trivial file write test\n\n");

    printf("opening /dev/null for write... ");
    fd = Hidd_UnixIO_OpenFile(unixio, "/dev/null", O_WRONLY, 0, &ioerr);
    if (fd == -1) {
        printf("failed (ioerr is %d)\n)", ioerr);
        goto exit;
    }
    printf("ok (fd is %d)\n", fd);

    printf("writing... ");
    nbytes = Hidd_UnixIO_WriteFile(unixio, fd, buf, 1024, &ioerr);
    if (nbytes == -1) {
        printf("failed (ioerr is %d\n)", ioerr);
        goto exit;
    }
    printf("ok (wrote %d bytes)\n", nbytes);

    printf("closing file... ");
    if (Hidd_UnixIO_CloseFile(unixio, fd, &ioerr) == -1)
    {
        printf("failed (ioerr is %d\n)", ioerr);
        goto exit;
    }
    printf("ok\n\n\n");


    printf("just for fun, lets read and print the contents of a file\n\n");

    printf("opening /etc/hosts for read... ");
    fd = Hidd_UnixIO_OpenFile(unixio, "/etc/hosts", O_RDONLY, 0, &ioerr);
    if (fd == -1) {
        printf("failed (ioerr is %d)\n)", ioerr);
        goto exit;
    }
    printf("ok (fd is %d)\n", fd);

    printf("reading... ");
    nbytes = Hidd_UnixIO_ReadFile(unixio, fd, buf, 1024, &ioerr);
    if (nbytes == -1) {
        printf("failed (ioerr is %d\n)", ioerr);
        goto exit;
    }
    printf("ok (read %d bytes)\n", nbytes);

    printf("system hosts file:\n\n%.*s\n", nbytes, buf);

    printf("closing file... ");
    if (Hidd_UnixIO_CloseFile(unixio, fd, &ioerr) == -1)
    {
        printf("failed (ioerr is %d\n)", ioerr);
        goto exit;
    }
    printf("ok\n\n\n");

    fd = -1;


    printf("now type something on the unix console that you\n"
           "ran aros from, then press enter. I'll wait...\n");

    Hidd_UnixIO_Wait(unixio, 0, vHidd_UnixIO_Read);

    printf("reading it... ");
    nbytes = Hidd_UnixIO_ReadFile(unixio, 0, buf, 1024, &ioerr);
    if (nbytes == -1) {
        printf("failed (ioerr is %d\n)", ioerr);
        goto exit;
    }
    printf("ok (read %d bytes)\n", nbytes);

    printf("you typed: %.*s\n\n", nbytes, buf);


exit:
    if (fd >= 0) Hidd_UnixIO_CloseFile(unixio, fd, NULL);
    if (unixio != NULL)  OOP_DisposeObject(unixio);
    if (UnixIOBase)
    	CloseLibrary(UnixIOBase);
    if (OOPBase != NULL) CloseLibrary(OOPBase);

    return failed ? 1 : 0;
}

