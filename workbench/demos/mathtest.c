#include <proto/exec.h>
#include <proto/aros.h>
#include <proto/mathffp.h>

#include <stdio.h>

#include <exec/types.h>

struct Library * MathBase;

int main(int argc, char ** argv)
{
  LONG z;
  if (!(MathBase = OpenLibrary("mathffp.library", 0L)))
  {
    fprintf(stderr, "Couldn't open mathffp.library\n");
    return (0);
  }

  printf("Basic mathffp functionality test...\n");

  /* this should set the zero-bit*/
  if ( 0 != SPAbs(0))
     printf("Error with the SPAbs-function!\n");
  else
     printf("SPAbs-function ok!\n");

  CloseLibrary(MathBase);
  return (0);
}
