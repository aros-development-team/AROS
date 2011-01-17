
//#include "addroutines.h"

extern long long int c, d, e, f;
extern int i, j;

//long long
//test( ADDARGS )
//{
//  return Samples + ScaleLeft + ScaleRight + (int) StartPointLeft + (int) StartPointRight
//         + (int) Src + (int) Dst + FirstOffsetI + Add + (int) Offset + (int) StopAtZero;
//}

long long
 test2( long long* offset, long long add, unsigned long samples )
{
  *offset += add * samples;
  return offset;
}

int
compare( int a, int b )
{
  return( a < 0 && b >= 0 || a > 0 && b <= 0 );
}

int
main( void )
{
  return test( 0x00000, 0x10000, 0x20000, 0x30000, 0x40000, 0x50000,
               0x60000, 0x70000, 0x800808000, 0x90000, 0x1000 );
}

/* m68k:

 4(sp) long       0x00000000           Samples
 8(sp) long       0x00010000           ScaleLeft
12(sp) long       0x00020000           ScaleRight
16(sp) long       0x00030000           StartPointLeft
20(sp) long       0x00040000           StartPointRight
24(sp) long       0x00050000           Src
28(sp) long       0x00060000           Dst
32(sp) long       0x00070000           FirstOffsetI
36(sp) long long  0x0000000800808000   Add
44(sp) long       0x00090000           Offset
50(sp) word       0x1000               StopAtZero

** ppc

 8(r1)            0x00000008           Add
12(r1)            0x00808000
16(r1)            0x00090000           Offset
20(r1) word       0x1000               StopAtZero
r3     long       0x00000000           Samples
r4     long       0x00010000           ScaleLeft
r5     long       0x00020000           ScaleRight
r6     long       0x00030000           StartPointLeft
r7     long       0x00040000           StartPointRight
r8     long       0x00050000           Src
r9     long       0x00060000           Dst
r10    long       0x00070000           FirstOffsetI
       
*/
