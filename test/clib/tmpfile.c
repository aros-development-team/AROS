#include <stdio.h>
#include <string.h>
#include "test.h"

int main() 
{

  int i;
  FILE * fp;

  for(i = 0; i<3; i++) /* repeat test to catch problems */
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
