/* $Id$
 *
 * Code taken from libnix
 */

unsigned long __fixunssfsi(float x)
{ if(x<0)
    return 0;
  if(x>=0x80000000u)
    return (signed long)(x-0x80000000u)+0x80000000u;
  return (signed long)x;
}
