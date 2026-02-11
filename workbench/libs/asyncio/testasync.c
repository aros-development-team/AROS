#include <stdio.h>

#include <libraries/asyncio.h>

#include <proto/asyncio.h>
#include <proto/dos.h>
#include <proto/exec.h>

int main(void)
{
    struct AsyncFile *write_fh = 0;
    LONG bytes = 0;
    LONG i;
    
    STRPTR line = "This is a test.";
  
    if ((write_fh = OpenAsync("testasync.txt", MODE_WRITE, 2000)))
    {
        puts("File opened.");
        for (i = 0; i < 1000; i++)
        {
            bytes = WriteLineAsync(write_fh, line);
            printf("%d bytes written.\n", (int)bytes);
            bytes = WriteCharAsync(write_fh,'\n');
            printf("%d byte written.\n", (int)bytes);
        }
        CloseAsync(write_fh);
    }
    else
    {
        puts("Can't open file");
    }
    
    return 0;
}
