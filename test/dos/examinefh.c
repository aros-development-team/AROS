#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>

int main(void)
{
    BPTR fh = Open("__TEST__", MODE_NEWFILE);
    
    if (fh != NULL)
    {
        struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
        
        if (fib != NULL)
        {
            if (ExamineFH(fh, fib))
            {
                printf("got fib. filename = %s\n", fib->fib_FileName);
            }
            else
            {
                printf("examinefh failed, ioerr = %d\n", IoErr());
            }
            FreeDosObject(DOS_FIB, fib);
        }
        else
        {
            printf("couldn't allocate fileinfoblock\n");
        }
            
        Close(fh);
        DeleteFile("__TEST__");
    }
    else
    {
        printf("couldn't create file\n");
    }
    

    return 0;
}
