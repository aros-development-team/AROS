/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/machine.h>
#include <stdio.h>

#if (AROS_BIG_ENDIAN == 0)
	#define XHIGH 4
	#define XMIDDLE 2
	#define XLOW 0
	#define AHIGH 4
	#define AMIDDLE 2
	#define ALOW 0
#else
	#define XHIGH 2
	#define XMIDDLE 4
	#define XLOW 6
	#define AHIGH 0
	#define AMIDDLE 2
	#define ALOW 4
#endif

unsigned char __Xrand_buffer[6];

unsigned char __Xrand[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

#if (AROS_BIG_ENDIAN == 0)
unsigned char __arand[6] = {0x6d, 0xe6, 0xec, 0xde, 0x05, 0x00};
#else
unsigned char __arand[6] = {0x00, 0x05, 0xde, 0xec, 0xe6, 0x6d};
#endif

unsigned short __crand = 0xb;


/*
  advance the seed to the next number
  
  Calculate  Xn+1 = (aXn+c) mod m;
  
  m = 2^48;
  
  a * X is calculated as follows:
  
  a2 = bits 32 to 47 of a
  a1 = bits 16 to 31 of a
  a0 = bits 0  to 15 of a
  
  x2 = bits 32 to 47 of x
  x1 = bits 16 to 31 of x
  x0 = bits 0  to 15 of x
  
  a* X = (a2*2^32+a1*2^16+a0) * (x2*2^32+x1*2^16+x0)
       =  a2*x2*2^64+a2*x1*2^48+a2*x0*2^32+
                     a1*x2*2^48+a1*x1*2^32+a1*x0*2^16+
                                a0*x2*2^32+a0*x1*2^16+a0*x0
  
  The relevant parts are with respect to a2 = 0:
          a2*x0*2^32
          a1*x1*2^32+a1*x0*2^16+
          a0*x2*2^32+a0*x1*2^16+a0*x0
          
*/  

void __calc_seed(unsigned short int * xsubi)
{
	unsigned long r0, r1, r2, tmp, carry1 = 0, carry2 = 0; 
	unsigned long x2,x1,x0;

	unsigned long a2 = *(unsigned short *)&__arand[AHIGH];
	unsigned long a1 = *(unsigned short *)&__arand[AMIDDLE];
	unsigned long a0 = *(unsigned short *)&__arand[ALOW];

	unsigned long c0 = __crand;


	if (NULL == xsubi) {
		x2 = *(unsigned short *)&__Xrand[XHIGH];
		x1 = *(unsigned short *)&__Xrand[XMIDDLE];
		x0 = *(unsigned short *)&__Xrand[XLOW];
	} else {
		x2 = xsubi[AHIGH/2];
		x1 = xsubi[AMIDDLE/2];
		x0 = xsubi[ALOW/2];
	}
		
	r0 = a0*x0;

	tmp = r0;
	r0 += c0;
	
	if (tmp > r0)
		carry1 = 0x10000;

	r1 = a0*x1;
	
	tmp = r1;
	r1 += a1*x0;
	
	if (tmp > r1)
		carry2 = 0x10000;
	
	tmp = r1;
	r1 += carry1;
	
	if (tmp > r1)
		carry2 += 0x1000;
 
	r2 = a0*x2+a1*x1+a2*x0+carry2;
	
	
	/* the upper 16 bits of r0 are added to r1 */
	
	tmp = r1;
	r1 += (r0 >> 16);
	if (tmp > r1)
		carry2 = 0x10000;
	else
		carry2 = 0x0;

	/* the upper 16 bits of r1 are added to r2 */	
	r2 += carry2 + (r1 >> 16);


	if (NULL == xsubi) {
		*(unsigned short *)&__Xrand[XLOW]		= r0;
		*(unsigned short *)&__Xrand[XMIDDLE] = r1;
		*(unsigned short *)&__Xrand[XHIGH]	 = r2;
	} else {
		xsubi[ALOW/2]		= r0;
		xsubi[AMIDDLE/2] = r1;
		xsubi[AHIGH/2]	 = r2;
	}

}


void __set_standardvalues(void)
{
	__crand = 0x0b; 

#if (AROS_BIG_ENDIAN == 0)
	__arand[0] = 0x6d;
	__arand[1] = 0xe6;
	__arand[2] = 0xec;
	__arand[3] = 0xde;
	__arand[4] = 0x05;
	__arand[5] = 0x00;
#else
	__arand[5] = 0x6d;
	__arand[4] = 0xe6;
	__arand[3] = 0xec;
	__arand[2] = 0xde;
	__arand[1] = 0x05;
	__arand[0] = 0x00;
#endif

}

void __copy_x_to_buffer(void)
{
	int i = 0;
	while (i < 6) {
		__Xrand_buffer[i] = __Xrand[i];
		i++;
	}
}
