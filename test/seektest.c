#include <proto/dos.h>
#include <stdlib.h>
#include <stdio.h>

int main()
{
    BPTR fh = NULL;
    int maxsize = 524288;
    char * buffer = NULL;
    int i = 0;
    int filepos = 0;
    int writesize = 0;

    buffer = malloc(maxsize);

    printf("Enter writesize (< 524288)\n");
    scanf("%d", &writesize);

    fh = Open("seek_test_file", MODE_NEWFILE);
    FWrite(fh, (STRPTR)buffer, writesize, 1);
    Flush(fh);
    printf("File pos: %d\n", Seek(fh, 0, OFFSET_CURRENT));
    filepos = Seek(fh, writesize, OFFSET_BEGINNING);

    if (filepos == -1)
        printf("ERROR for size: %d, IoErr: %d\n", writesize, IoErr());
    else
        printf("OK\n");

    free(buffer);

    Close(fh);
    return 0;
}

