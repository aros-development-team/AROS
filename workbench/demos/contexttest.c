#include <stdio.h>
#include <exec/types.h>

int add (int a)
{
  if (1 == a)
    return 1;
  else
    return a + add(a-1);
}

int a(void)
{
  return 1;
}

void main(void)
{
  while (1)
  {
    int Res = a();
    if (Res != 1)
      printf("error!! %i\n",Res);
/*
    int Res = add(100);
    if (Res != 5050)
      printf("Error!! Result = %i\n",Res);
*/
  }
}