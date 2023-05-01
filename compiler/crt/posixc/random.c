/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <aros/symbolsets.h>

#include <string.h>
#include <stdlib.h>

#include "__posixc_intbase.h"

/*
 * random.c:
 * An improved random number generation package.  In addition to the standard
 * rand()/srand() like interface, this package also has a special state info
 * interface.  The initstate() routine is called with a seed, an array of
 * bytes, and a count of how many bytes are being passed in; this array is then
 * initialized to contain information for random number generation with that
 * much state information.  Good sizes for the amount of state information are
 * 32, 64, 128, and 256 bytes.  The state can be switched by calling the
 * setstate() routine with the same array as was initiallized with initstate().
 * By default, the package runs with 128 bytes of state information and
 * generates far better random numbers than a linear congruential generator.
 * If the amount of state information is less than 32 bytes, a simple linear
 * congruential R.N.G. is used.
 * Internally, the state information is treated as an array of longs; the
 * zeroeth element of the array is the type of R.N.G. being used (small
 * integer); the remainder of the array is the state information for the
 * R.N.G.  Thus, 32 bytes of state information will give 7 longs worth of
 * state information, which will allow a degree seven polynomial.  (Note: the 
 * zeroeth word of state information also has some other information stored
 * in it -- see setstate() for details).
 * The random number generation technique is a linear feedback shift register
 * approach, employing trinomials (since there are fewer terms to sum up that
 * way).  In this approach, the least significant bit of all the numbers in
 * the state table will act as a linear feedback shift register, and will have
 * period 2^deg - 1 (where deg is the degree of the polynomial being used,
 * assuming that the polynomial is irreducible and primitive).  The higher
 * order bits will have longer periods, since their values are also influenced
 * by pseudo-random carries out of the lower bits.  The total period of the
 * generator is approximately deg*(2**deg - 1); thus doubling the amount of
 * state information has a vast influence on the period of the generator.
 * Note: the deg*(2**deg - 1) is an approximation only good for large deg,
 * when the period of the shift register is the dominant factor.  With deg
 * equal to seven, the period is actually much longer than the 7*(2**7 - 1)
 * predicted by this formula.
 */

/*
 * For each of the currently supported random number generators, we have a
 * break value on the amount of state information (you need at least this
 * many bytes of state info to support this random number generator), a degree
 * for the polynomial (actually a trinomial) that the R.N.G. is based on, and
 * the separation between the two lower order coefficients of the trinomial.
 */

#define		TYPE_0		0		/* linear congruential */
#define		BREAK_0		8
#define		DEG_0		0
#define		SEP_0		0

#define		TYPE_1		1		/* x**7 + x**3 + 1 */
#define		BREAK_1		32
#define		DEG_1		7
#define		SEP_1		3

#define		TYPE_2		2		/* x**15 + x + 1 */
#define		BREAK_2		64
#define		DEG_2		15
#define		SEP_2		1

#define		TYPE_3		3		/* x**31 + x**3 + 1 */
#define		BREAK_3		128
#define		DEG_3		31
#define		SEP_3		3

#define		TYPE_4		4		/* x**63 + x + 1 */
#define		BREAK_4		256
#define		DEG_4		63
#define		SEP_4		1

/*
 * Array versions of the above information to make code run faster -- relies
 * on fact that TYPE_i == i.
 */

#define		MAX_TYPES	5		/* max number of types above */

static  int const	_degrees[ MAX_TYPES ]	= { DEG_0, DEG_1, DEG_2,
								DEG_3, DEG_4 };

static  int const	_seps[ MAX_TYPES ]	= { SEP_0, SEP_1, SEP_2,
								SEP_3, SEP_4 };



/*
 * Initially, everything is set up as if from :
 *		initstate( 1, &randtbl, 128 );
 * Note that this initialization takes advantage of the fact that srandom()
 * advances the front and rear pointers 10*rand_deg times, and hence the
 * rear pointer which starts at 0 will also end up at zero; thus the zeroeth
 * element of the state information, which contains info about the current
 * position of the rear pointer is just
 *	MAX_TYPES*(rptr - state) + TYPE_3 == TYPE_3.
 */

static  long const	_randtbl[ DEG_3 + 1 ]	= { TYPE_3,
			    0x9a319039, 0x32d9c024, 0x9b663182, 0x5da1f342,
			    0xde3b81e0, 0xdf0a6fb5, 0xf103bc02, 0x48f340fb,
			    0x7449e56b, 0xbeb1dbb0, 0xab5c5918, 0x946554fd,
			    0x8c2e680f, 0xeb3d799f, 0xb11ee0b7, 0x2d436b86,
			    0xda672e2a, 0x1588ca88, 0xe369735d, 0x904f35f7,
			    0xd7158fd6, 0x6fa6f051, 0x616e6b96, 0xac94efdc,
			    0x36413f93, 0xc622c298, 0xf5a42ab8, 0x8a88d77b,
					0xf5ad9d0e, 0x8999220b, 0x27fb47b9 };

/*
 * fptr and rptr are two pointers into the state info, a front and a rear
 * pointer.  These two pointers are always rand_sep places aparts, as they cycle
 * cyclically through the state information.  (Yes, this does mean we could get
 * away with just one pointer, but the code for random() is more efficient this
 * way).  The pointers are left positioned as they would be from the call
 *			initstate( 1, randtbl, 128 )
 * (The position of the rear pointer, rptr, is really 0 (as explained above
 * in the initialization of randtbl) because the state table pointer is set
 * to point to randtbl[1] (as explained below).
 */


/*
 * The following things are the pointer to the state information table,
 * the type of the current generator, the degree of the current polynomial
 * being used, and the separation between the two pointers.
 * Note that for efficiency of random(), we remember the first location of
 * the state information, not the zeroeth.  Hence it is valid to access
 * state[-1], which is used to store the type of the R.N.G.
 * Also, we remember the last location, since this is more efficient than
 * indexing every time to find the address of the last element to see if
 * the front and rear pointers have wrapped.
 */

struct random_state {
        int     degrees[ MAX_TYPES ];
        int     seps[ MAX_TYPES ];
        long    randtbl[ DEG_3 + 1 ];
        long    *fptr;
        long    *rptr;
        long    *state;
        int     rand_type;
        int     rand_deg;
        int     rand_sep;
        long    *end_ptr;
};

static void init_random_state(struct random_state *rs)
{
    memcpy(rs->degrees, _degrees, sizeof(_degrees));
    memcpy(rs->seps, _seps, sizeof(_seps));
    memcpy(rs->randtbl, _randtbl, sizeof(_randtbl));
    rs->fptr = &rs->randtbl[ SEP_3 + 1 ];
    rs->rptr = &rs->randtbl[ 1 ];
    rs->state = &rs->randtbl[ 1 ];
    rs->rand_type = TYPE_3;
    rs->rand_deg  = DEG_3;
    rs->rand_sep  = SEP_3;
    rs->end_ptr   = &rs->randtbl[ DEG_3 + 1 ];
}

static struct random_state *get_random_state(void)
{
    struct PosixCIntBase *PosixCIntBase = (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    struct random_state *rs;

    if (PosixCIntBase->rs)
        return PosixCIntBase->rs;

    if ((rs = malloc(sizeof(*rs))))
    {
        init_random_state(rs);

        PosixCIntBase->rs = rs;
        return rs;
    }

    return NULL;
}

static void free_random_state(struct PosixCIntBase *PosixCIntBase)
{
    if (PosixCIntBase->rs) {
        free(PosixCIntBase->rs);
        PosixCIntBase->rs = NULL;
    }
}

ADD2CLOSELIB(free_random_state, 0)


/*
 * srandom:
 * Initialize the random number generator based on the given seed.  If the
 * type is the trivial no-state-information type, just remember the seed.
 * Otherwise, initializes state[] based on the given "seed" via a linear
 * congruential generator.  Then, the pointers are set to known locations
 * that are exactly rand_sep places apart.  Lastly, it cycles the state
 * information a given number of times to get rid of any initial dependencies
 * introduced by the L.C.R.N.G.
 * Note that the initialization of randtbl[] for default usage relies on
 * values produced by this routine.
 */

#ifdef srandom
#error ciaooo
#endif

void srandom(unsigned x)
{
    	register  int		i;
	long random();
	struct random_state *rs;

	if (!(rs = get_random_state()))
	    return;

	if(  rs->rand_type  ==  TYPE_0  )  {
	    rs->state[ 0 ] = x;
	}
	else  {
	    rs->state[ 0 ] = x;
	    for( i = 1; i < rs->rand_deg; i++ )  {
		rs->state[i] = 1103515245*rs->state[i - 1] + 12345;
	    }
	    rs->fptr = &rs->state[ rs->rand_sep ];
	    rs->rptr = &rs->state[ 0 ];
	    for( i = 0; i < 10*rs->rand_deg; i++ )  random();
	}
}



/*
 * initstate:
 * Initialize the state information in the given array of n bytes for
 * future random number generation.  Based on the number of bytes we
 * are given, and the break values for the different R.N.G.'s, we choose
 * the best (largest) one we can and set things up for it.  srandom() is
 * then called to initialize the state information.
 * Note that on return from srandom(), we set state[-1] to be the type
 * multiplexed with the current value of the rear pointer; this is so
 * successive calls to initstate() won't lose this information and will
 * be able to restart with setstate().
 * Note: the first thing we do is save the current state, if any, just like
 * setstate() so that it doesn't matter when initstate is called.
 * Returns a pointer to the old state.
 */

char  *
initstate( seed, arg_state, n )

    unsigned		seed;			/* seed for R. N. G. */
    char		*arg_state;		/* pointer to state array */
    int			n;			/* # bytes of state info */
{
	struct random_state *rs;
	register  char		*ostate;

	if (!(rs = get_random_state()))
	    return NULL;
	
	ostate = (char *)( &rs->state[ -1 ] );

	if(  rs->rand_type  ==  TYPE_0  )  rs->state[ -1 ] = rs->rand_type;
	else  rs->state[ -1 ] = MAX_TYPES*(rs->rptr - rs->state) + rs->rand_type;
	if(  n  <  BREAK_1  )  {
	    if(  n  <  BREAK_0  )  {
		return 0;
	    }
	    rs->rand_type = TYPE_0;
	    rs->rand_deg = DEG_0;
	    rs->rand_sep = SEP_0;
	}
	else  {
	    if(  n  <  BREAK_2  )  {
		rs->rand_type = TYPE_1;
		rs->rand_deg = DEG_1;
		rs->rand_sep = SEP_1;
	    }
	    else  {
		if(  n  <  BREAK_3  )  {
		    rs->rand_type = TYPE_2;
		    rs->rand_deg = DEG_2;
		    rs->rand_sep = SEP_2;
		}
		else  {
		    if(  n  <  BREAK_4  )  {
			rs->rand_type = TYPE_3;
			rs->rand_deg = DEG_3;
			rs->rand_sep = SEP_3;
		    }
		    else  {
			rs->rand_type = TYPE_4;
			rs->rand_deg = DEG_4;
			rs->rand_sep = SEP_4;
		    }
		}
	    }
	}
	rs->state = &(  ( (long *)arg_state )[1]  );	/* first location */
	rs->end_ptr = &rs->state[ rs->rand_deg ];	/* must set end_ptr before srandom */
	srandom( seed );
	if(  rs->rand_type  ==  TYPE_0  )  rs->state[ -1 ] = rs->rand_type;
	else  rs->state[ -1 ] = MAX_TYPES*(rs->rptr - rs->state) + rs->rand_type;
	return( ostate );
}



/*
 * setstate:
 * Restore the state from the given state array.
 * Note: it is important that we also remember the locations of the pointers
 * in the current state information, and restore the locations of the pointers
 * from the old state information.  This is done by multiplexing the pointer
 * location into the zeroeth word of the state information.
 * Note that due to the order in which things are done, it is OK to call
 * setstate() with the same state as the current state.
 * Returns a pointer to the old state information.
 */

char *setstate(char *arg_state)
{
    struct random_state *rs;
    register  long *new_state;
    register  int  type;
    register  int  rear;
    char           *ostate;

    if (!(rs = get_random_state()) || arg_state == NULL)
        return NULL;

    new_state = (long *)arg_state;
    type	      = new_state[0]%MAX_TYPES;
    rear       = new_state[0]/MAX_TYPES;
    ostate    = (char *)(&rs->state[-1]);


    if (rs->rand_type == TYPE_0) rs->state[-1] = rs->rand_type;
    else  rs->state[ -1 ] = MAX_TYPES*(rs->rptr - rs->state) + rs->rand_type;

    switch (type)
    {
        case  TYPE_0:
	case  TYPE_1:
	case  TYPE_2:
	case  TYPE_3:
	case  TYPE_4:
	    rs->rand_type = type;
	    rs->rand_deg = rs->degrees[type];
	    rs->rand_sep = rs->seps[type];
	    break;
    }
    rs->state = &new_state[1];
    if (rs->rand_type != TYPE_0)
    {
	rs->rptr = &rs->state[rear];
	rs->fptr = &rs->state[(rear + rs->rand_sep)%rs->rand_deg];
    }
    rs->end_ptr = &rs->state[rs->rand_deg];       /* set end_ptr too */

    return ostate;
}



/*
 * random:
 * If we are using the trivial TYPE_0 R.N.G., just do the old linear
 * congruential bit.  Otherwise, we do our fancy trinomial stuff, which is the
 * same in all ther other cases due to all the global variables that have been
 * set up.  The basic operation is to add the number at the rear pointer into
 * the one at the front pointer.  Then both pointers are advanced to the next
 * location cyclically in the table.  The value returned is the sum generated,
 * reduced to 31 bits by throwing away the "least random" low bit.
 * Note: the code takes advantage of the fact that both the front and
 * rear pointers can't wrap on the same call by not testing the rear
 * pointer if the front one has wrapped.
 * Returns a 31-bit random number.
 */

long random()
{
    long i;
    struct random_state *rs;
    int rand(void);

    if (!(rs = get_random_state()))
        return rand();

    if (rs->rand_type == TYPE_0)
    {
	i = rs->state[0] = (rs->state[0]*1103515245 + 12345)&0x7fffffff;
    }
    else
    {
	*rs->fptr += *rs->rptr;
	i = (*rs->fptr >> 1)&0x7fffffff;	/* chucking least random bit */
	if (++rs->fptr >= rs->end_ptr)
	{
	    rs->fptr = rs->state;
	    ++rs->rptr;
	}
	else
	{
	    if (++rs->rptr >= rs->end_ptr) rs->rptr = rs->state;
	}
    }
    return i;
}

