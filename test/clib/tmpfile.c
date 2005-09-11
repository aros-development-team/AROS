#include <stdio.h>
#include <string.h>
#include "test.h"

int main() 
{

  int i;
  FILE * fp;

  /* FIXME: loop over about 30 to test that tmpfile() copes with
     more temp files than there are letters in the alphabet. This
     is known to be a bug as at 11-Sep-2005
     */
  for(i = 0; i<20; i++) /* repeat test to catch problems */
    {
      fp = tmpfile();
      TEST((fp != NULL));
      fprintf(fp, "test text %d", i);
    }


    return OK;
}

void cleanup() 
{
    /* Nothing to clean up */
}
