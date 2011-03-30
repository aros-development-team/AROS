#include <proto/dos.h>

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    BPTR oldlock = CurrentDir(BNULL);
    BPTR lock = DupLock(oldlock);
    CurrentDir(oldlock);
  
    system("dir");
   
    lock = CurrentDir(lock);
    UnLock(lock);
  
    return 0;
}
