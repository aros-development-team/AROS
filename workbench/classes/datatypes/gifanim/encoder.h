
#ifndef ENCODER_H
#define ENCODER_H 1

/*
**
**  $VER: encoder.h 2.2 (13.4.98)
**  gifanim.datatype 2.2
**
**  GIF Encoder header of gifanim.datatype
**
**  Written 1997/1998 by Roland 'Gizzy' Mainz
**  Original example source from David N. Junod
**
*/

/* project includes */
#include "classbase.h"

/* ansi includes */
#include <limits.h>
#include <ctype.h>

/*****************************************************************************/

/* General DEFINEs */
#define HSIZE  (5003)            /* 80% occupancy */

/*****************************************************************************/

/* a code_int must be able to hold 2**BITS values of type int, and also -1 */
typedef int      code_int;
typedef long int count_int;

/*****************************************************************************/

/* encoder context data */
struct GIFEncoder
{
    struct ClassBase   *classbase;

    Object             *object;
    struct adtFrame     loadmsg;
    struct BitMap      *srcbm;
    UBYTE              *srcchunkymap[ 2 ],
                       *currchunkymap;
    struct RastPort     rpa8tmprp;
    struct RastPort     rp;
    UWORD               whichbm; /* which source bm ? */

    /* prefs */
    BOOL                interlace;
    WORD                backgroundpen;
    WORD                transparentpen;

    /* attrs */

    ULONG               animwidth,
                        animheight,
                        animdepth,
                        numcolors;
    ULONG               tpf;
    BPTR                outfile;

    int                 Width,
                        Height;
    int                 curx,
                        cury;
    long                CountDown;
    int                 Pass/* = 0*/;
    int                 Interlace;

    int                 n_bits;             /* number of bits/code */
    code_int            maxcode;            /* maximum code, given n_bits */

    count_int           htab[ HSIZE ];
    unsigned short      codetab[ HSIZE ];

    code_int            free_ent /* = 0*/;  /* first unused entry */

    /* block compression parameters -- after all codes are used up, and compression rate changes, start over. */
    BOOL                clear_flg   /*  = 0*/;
    int                 g_init_bits;
    int                 ClearCode;
    int                 EOFCode;

    unsigned long       cur_accum/* = 0*/;
             int        cur_bits/*  = 0*/;

    char                accum[ 256 ]; /* Define the storage for the packet accumulator */
    int                 a_count;      /* Number of characters so far in this 'packet'  */
};

#endif /* !ENCODER_H */

