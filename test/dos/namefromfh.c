#include <proto/dos.h>
#include <stdio.h>

int main(void)
{
    TEXT buffer[512];
    BPTR fh = Open("__TEST__", MODE_NEWFILE);
    
    if (fh != NULL)
    {
        if (NameFromFH(fh, buffer, 512))
        {
            printf("got name: %s\n", buffer);
        }
        else
        {
            printf("namefromlock failed. ioerr = %ld\n", IoErr());
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
