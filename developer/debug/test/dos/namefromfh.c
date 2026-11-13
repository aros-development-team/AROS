/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.
*/

#include <proto/dos.h>
#include <stdio.h>

int main(void)
{
    TEXT buffer[512];
    BPTR fh = Open("__TEST__", MODE_NEWFILE);
    
    if (fh != BNULL)
    {
        if (NameFromFH(fh, buffer, 512))
        {
            printf("got name: %s\n", buffer);
        }
        else
        {
            printf("namefromfh failed. I/O error = %ld\n", IoErr());
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
