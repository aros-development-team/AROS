#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <rexx/storage.h>

#include <stdio.h>
#include <stdlib.h>

struct RxsLib *RexxSysBase;

int main(void)
{
  UBYTE *s;
  
  RexxSysBase = (struct RxsLib *)OpenLibrary("rexxsyslib.library", 0);
  if (RexxSysBase == NULL)
  {
    puts("Error opening rexxsyslib.library");
    exit(20);
  }
  else
  {
    puts("Opening rexxsyslib.library succeeded");
  }

  puts("Creating Argstring");
  s = CreateArgstring("1234", 4);
  if (s == NULL)
  {
    puts("Error creating Argstring");
    exit(20);
  }
  puts("Getting length");
  printf("Length: %ld\n",LengthArgstring(s));
  puts("Deleting Argstring");
  DeleteArgstring(s);

  CloseLibrary((struct Library *)RexxSysBase);
  exit(0);
}
	 
