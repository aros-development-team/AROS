/*  $VER: vbcc (machine.h amiga68k) V0.4    */

/*  Here should be routines to emulate the target's data types on   */
/*  the host machine. Currently there are only macros so that the   */
/*  host's arithmetic will be used.                                 */

/*  Declaration of the data types of the target machine.            */
/*  zchar, zshort, zint and zlong are the basic signed integers.    */
/*  zuchar, zushort, zuint and zulong are the basic unsigned ints.  */
/*  zfloat and zdouble are float and double, of course.             */
/*  zpointer is a generic pointer. Currently vbcc only supports     */
/*  targets where all pointers have the same representation.        */
typedef signed char zchar;
typedef unsigned char zuchar;
typedef signed short zshort;
typedef unsigned short zushort;
typedef signed int zint;
typedef unsigned int zuint;
typedef signed long zlong;
typedef unsigned long zulong;
typedef float zfloat;
typedef double zdouble;
typedef unsigned long zpointer;

/*  These functions convert data types of the target machine.       */
/*  E.g. zl2zul takes a zlong and yields a zulong.                  */
#define zc2zl(x) ((zlong)(x))
#define zs2zl(x) ((zlong)(x))
#define zi2zl(x) ((zlong)(x))
#define zl2zc(x) ((zchar)(x))
#define zl2zs(x) ((zshort)(x))
#define zl2zi(x) ((zint)(x))
#define zl2zul(x) ((zulong)(x))
#define zuc2zul(x) ((zulong)(x))
#define zus2zul(x) ((zulong)(x))
#define zui2zul(x) ((zulong)(x))
#define zul2zuc(x) ((zuchar)(x))
#define zul2zus(x) ((zushort)(x))
#define zul2zui(x) ((zuint)(x))
#define zul2zl(x) ((zlong)(x))
#define zf2zd(x) ((zdouble)(x))
#define zd2zf(x) ((zfloat)(x))
#define zl2zd(x) ((zdouble)(x))
#define zd2zl(x) ((zlong)(x))
#define zul2zd(x) ((zdouble)(x))
#define zd2zul(x) ((zulong)(x))
/*  Note that zul2zp must yield a valid null-pointer on the target  */
/*  machine if the zulong is zero.                                  */
#define zul2zp(x) ((zpointer)(x))
#define zp2zul(x) ((zulong)(x))

/*  These functions convert data types of the host machine into     */
/*  data types of the target machine and the other way round.       */
#define l2zl(x) ((zlong)(x))
#define zl2l(x) ((long)(x))
#define ul2zul(x) ((zulong)(x))
#define zul2ul(x) ((unsigned long)(x))
#define d2zd(x) ((zdouble)(x))
#define zd2d(x) ((double)(x))

/*  These functions perform arithmetic and logical operations on    */
/*  the data types of the target machine.                           */
#define zlmult(a,b) ((a)*(b))
#define zulmult(a,b) ((a)*(b))
#define zdmult(a,b) ((a)*(b))
#define zldiv(a,b) ((a)/(b))
#define zuldiv(a,b) ((a)/(b))
#define zddiv(a,b) ((a)/(b))
#define zlmod(a,b) ((a)%(b))
#define zulmod(a,b) ((a)%(b))
#define zllshift(a,b) ((a)<<(b))
#define zullshift(a,b) ((a)<<(b))
#define zlrshift(a,b) ((a)>>(b))
#define zulrshift(a,b) ((a)>>(b))
#define zladd(a,b) ((a)+(b))
#define zuladd(a,b) ((a)+(b))
#define zdadd(a,b) ((a)+(b))
#define zlsub(a,b) ((a)-(b))
#define zulsub(a,b) ((a)-(b))
#define zdsub(a,b) ((a)-(b))
#define zlor(a,b)   ((a)|(b))
#define zulor(a,b)   ((a)|(b))
#define zland(a,b)   ((a)&(b))
#define zuland(a,b)   ((a)&(b))
#define zlxor(a,b)   ((a)^(b))
#define zulxor(a,b)   ((a)^(b))
#define zlkompl(a)  (~(a))
#define zulkompl(a)  (~(a))
#define zdkompl(a)  (~(a))

/*  Comparison functions. Must return non-zero if equation is true. */
#define zlleq(a,b) ((a)<=(b))
#define zulleq(a,b) ((a)<=(b))
#define zdleq(a,b) ((a)<=(b))
#define zdeqto(a,b)    ((a)==(b))
#define zleqto(a,b) ((a)==(b))
#define zuleqto(a,b) ((a)==(b))
#define zpleq(a,b)     ((a)<=(b))
#define zpeqto(a,b)     ((a)==(b))


/*  This struct can be used to implement machine-specific           */
/*  addressing-modes.                                               */
struct AddressingMode{
    int basereg;
    long dist;
    int skal;
    int dreg;
};

/*  The number of registers of the target machine.                  */
#define MAXR 24

/*  Number of commandline-options the code-generator accepts.       */
#define MAXGF 20

/*  If this is set to zero vbcc will not generate ICs where the     */
/*  target operand is the same as the 2nd source operand.           */
/*  This can sometimes simplify the code-generator, but usually     */
/*  the code is better if the code-generator allows it.             */
#define USEQ2ASZ 0

/*  This specifies the smallest integer type that can be added to a */
/*  pointer.                                                        */
#define MINADDI2P SHORT

/*  If the bytes of an integer are ordered most significant byte    */
/*  byte first and then decreasing set BIGENDIAN to 1.              */
#define BIGENDIAN 1

/*  If the bytes of an integer are ordered lest significant byte    */
/*  byte first and then increasing set LITTLEENDIAN to 1.           */
#define LITTLEENDIAN 0

/*  Note that BIGENDIAN and LITTLEENDIAN are mutually exclusive.    */

/*  If switch-statements should be generated as a sequence of       */
/*  SUB,TST,BEQ ICs rather than COMPARE,BEQ ICs set this to 1.      */
/*  This can yield better code on some machines.                    */
#define SWITCHSUBS 1

/*  In optimizing compilation certain library memcpy/strcpy-calls   */
/*  with length known at compile-time will be inlined using an      */
/*  ASSIGN-IC if the size is less or equal to INLINEMEMCPY.         */
/*  The type used for the ASSIGN-IC will be UNSIGNED|CHAR.          */
#define INLINEMEMCPY (1<<30)



