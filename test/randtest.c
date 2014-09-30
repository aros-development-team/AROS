/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
  int c = 0;
  int array[20];
  unsigned short param[7] = {4,5,0,1,2,1,0x2};
  unsigned short seed16v[3] = {1,2,3};
  unsigned short xsubi[3] = {1,2,3};
  unsigned short *sptr;
  srand48(0x1);

  c = 0 ;
  while (c < 20)
  {
    array[c] = lrand48();
    c++;
  }
 
  printf("---\n");

  srand48(0x1); 
  c = 0 ;
  while (c < 20)
  {
    union { 
	double d;
	int i[2];
    } tmp;
    tmp.d = drand48();
    int a = tmp.i[0];
    int b = tmp.i[1];
    
    printf("%d. %e  (%x %x) %x\n",c,tmp.d,a,b,array[c]);
    c++;
  }

  lcong48(param);
  c = 0 ;
  while (c < 30)
  {
    printf("%d. %lx\n",c,lrand48());
   
    c++;
  }

  sptr = seed48(seed16v);
  printf("---\n %x,%x,%x\n",sptr[0],sptr[1],sptr[2]);
  c = 0 ;
  while (c < 30)
  {
    printf("%d. %lx\n",c,lrand48());
    c++;
  }
  printf("---\n");

  c = 0 ;
  while (c < 10)
  {
    printf("%d. %e (%x,%x,%x)\n",c,erand48(xsubi),xsubi[0],xsubi[1],xsubi[2]);
    c++;
  }
  printf("---\n");

  c = 0 ;
  xsubi[0] = 0;
  xsubi[1] = 1;
  xsubi[2] = 0;
  while (c < 10)
  {
    printf("%d. %lx (%x,%x,%x)\n",c,nrand48(xsubi),xsubi[0],xsubi[1],xsubi[2]);
    c++;
  }
  printf("---\n");

  c = 0 ;
  while (c < 10)
  {
    printf("%d. %lx (%x,%x,%x)\n",c,jrand48(xsubi),xsubi[0],xsubi[1],xsubi[2]);
    c++;
  }
  

  return 0;
}
