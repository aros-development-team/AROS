#include <aros/bootloader.h>
#include <hidd/hostio.h>
#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

struct Library *OOPBase = NULL;

char *unix_files[] = {
    "/dev/zero",
    "/dev/null",
    "/etc/motd"
};

char *windows_files[] = {
    "NUL:", /* Reading from NUL: on Windows produces EOF, but i can't invent anything better */
    "NUL:",
    "C:\\Windows\\System32\\EULA.txt"
};

int main (int argc, char **argv)
{
    struct Library *HostIOBase = NULL;
    APTR BootLoaderBase = NULL;
    STRPTR bootldr_name;
    char **files = unix_files;
    int failed = 1;
    OOP_Object *hostio = NULL;
    APTR fd = vHidd_HostIO_Invalid_Handle;
    int nbytes, ioerr;
    char buf[1024];

    if ((OOPBase = OpenLibrary("oop.library", 0)) == NULL) {
        fprintf(stderr, "can't open oop.library\n");
        goto exit;
    }
    
    HostIOBase = OpenLibrary("DEVS:Drivers/hostio.hidd", 0);
    if (!HostIOBase) {
        fprintf(stderr, "Can't open hostio.hidd\n");
	goto exit;
    }

    if ((hostio = OOP_NewObject(NULL, CLID_Hidd_HostIO, NULL)) == NULL) {
        fprintf(stderr, "can't instantiate hostio hidd\n");
        goto exit;
    }

    BootLoaderBase = OpenResource("bootloader.resource");
    if (!BootLoaderBase) {
        fprintf(stderr, "Can't open bootloader.resource\n");
	goto exit;
    }
    bootldr_name = GetBootInfo(BL_LoaderName);
    if (bootldr_name) {
        printf("Bootloader name: %s\n", bootldr_name);
	if (!strnicmp(bootldr_name, "Windows", 7))
	    files = windows_files;
    } else
        printf("Failed to obtain bootloader name, assuming UNIX host\n");

    printf("first, a trivial file read test\n\n");

    printf("opening %s for read... ", files[0]);
    fd = Hidd_HostIO_OpenFile(hostio, files[0], O_RDONLY, 0, &ioerr);
    if (ioerr != 0) {
        printf("failed (ioerr is %d)\n", ioerr);
        goto exit;
    }
    printf("ok (fd is 0x%p)\n", fd);

    printf("reading... ");
    nbytes = Hidd_HostIO_ReadFile(hostio, fd, buf, 1024, &ioerr);
    if (ioerr != 0) {
        printf("failed (ioerr is %d)\n", ioerr);
        goto exit;
    }
    printf("ok (read %d bytes)\n", nbytes);

    printf("closing file... ");
    Hidd_HostIO_CloseFile(hostio, fd, &ioerr);
    if (ioerr != 0) {
        printf("failed (ioerr is %d)\n", ioerr);
        goto exit;
    }
    printf("ok\n\n\n");


    printf("next, an equally trivial file write test\n\n");

    printf("opening %s for write... ", files[1]);
    fd = Hidd_HostIO_OpenFile(hostio, files[1], O_WRONLY, 0, &ioerr);
    if (ioerr != 0) {
        printf("failed (ioerr is %d)\n", ioerr);
        goto exit;
    }
    printf("ok (fd is 0x%p)\n", fd);

    printf("writing... ");
    nbytes = Hidd_HostIO_WriteFile(hostio, fd, buf, 1024, &ioerr);
    if (ioerr != 0) {
        printf("failed (ioerr is %d)\n", ioerr);
        goto exit;
    }
    printf("ok (wrote %d bytes)\n", nbytes);

    printf("closing file... ");
    Hidd_HostIO_CloseFile(hostio, fd, &ioerr);
    if (ioerr != 0) {
        printf("failed (ioerr is %d)\n", ioerr);
        goto exit;
    }
    printf("ok\n\n\n");


    printf("just for fun, lets read and print the contents of a file\n\n");

    printf("opening %s for read... ", files[2]);
    fd = Hidd_HostIO_OpenFile(hostio, files[2], O_RDONLY, 0, &ioerr);
    if (ioerr != 0) {
        printf("failed (ioerr is %d)\n", ioerr);
        goto exit;
    }
    printf("ok (fd is 0x%p)\n", fd);

    printf("reading... ");
    nbytes = Hidd_HostIO_ReadFile(hostio, fd, buf, 1024, &ioerr);
    if (ioerr != 0) {
        printf("failed (ioerr is %d)\n", ioerr);
        goto exit;
    }
    printf("ok (read %d bytes)\n", nbytes);

    printf("File contents:\n\n%.*s", nbytes, buf);

    printf("\nclosing file... ");
    Hidd_HostIO_CloseFile(hostio, fd, &ioerr);
    if (ioerr != 0) {
        printf("failed (ioerr is %d)\n", ioerr);
        goto exit;
    }
    printf("ok\n\n\n");

    fd = vHidd_HostIO_Invalid_Handle;

/* TODO: Windows console doesn't work with overlapped I/O, so i need to think off something else
    printf("now type something on the unix console that you\n"
           "ran aros from, then press enter. I'll wait...\n");

    Hidd_HostIO_Wait(hostio, 0, vHidd_HostIO_Read, NULL, NULL, SysBase);

    printf("reading it... ");
    nbytes = Hidd_HostIO_ReadFile(hostio, 0, buf, 1024, &ioerr);
    if (ioerr != 0) {
        printf("failed (ioerr is %d\n)", ioerr);
        goto exit;
    }
    printf("ok (read %d bytes)\n", nbytes);

    printf("you typed: %.*s\n\n", nbytes, buf);
*/

exit:
    if (fd != vHidd_HostIO_Invalid_Handle) Hidd_HostIO_CloseFile(hostio, fd, NULL);
    if (hostio != NULL)  OOP_DisposeObject(hostio);
    if (HostIOBase) CloseLibrary(HostIOBase);
    if (OOPBase != NULL) CloseLibrary(OOPBase);

    return failed ? 1 : 0;
}

