
#define NOTNULLMASK 0x0001000

#define MAJOR_ID_MSB   30
#define MAJOR_ID_LSB   26
#define MAJOR_ID_SHIFT MAJOR_ID_LSB
#define MAJOR_ID_MASK  (((1 << (MAJOR_ID_MSB - MAJOR_ID_LSB + 1)) - 1) << MAJOR_ID_LSB)

#define MINOR_ID_MSB   25
#define MINOR_ID_LSB   20
#define MINOR_ID_SHIFT MINOR_ID_LSB
#define MINOR_ID_MASK  (((1 << (MINOR_ID_MSB - MINOR_ID_LSB + 1)) - 1) << MINOR_ID_LSB)

#define NUM2MAJORID(num) ((num)  << MAJOR_ID_SHIFT)
/*#define MAJORID2NUM(modeid) ( ((modeid) & ~NOTNULLMASK) >> MAJOR_ID_SHIFT)*/
#define MAJORID2NUM(modeid) ( ((modeid) & MAJOR_ID_MASK) >> MAJOR_ID_SHIFT)

#define NUM2MINORID(num) ((num)  << MINOR_ID_SHIFT)
/*#define MINORID2NUM(modeid) ( ((modeid) & ~NOTNULLMASK) >> MINOR_ID_SHIFT)*/
#define MINORID2NUM(modeid) ( ((modeid) & MINOR_ID_MASK) >> MINOR_ID_SHIFT)

/* stegerg: end check */


/* This macro assures that a modeid is never 0 by setting the MSB to 1.
   This is usefull because FindDisplayInfo just returns the modeid,
   and FidDisplayInfo returning 0 indicates failure
*/
   
#define GENERATE_MODEID(majoridx, minoridx)	\
	(NUM2MAJORID(majoridx) | NUM2MINORID(minoridx) | NOTNULLMASK)

/*
    ModeID construction is really private to the HIDD so
    this is a hack
*/

#define AMIGA_TO_HIDD_MODEID(modeid)		\
    ( ((modeid) == INVALID_ID) 			\
		? vHidd_ModeID_Invalid  	\
		: ( (MAJORID2NUM(modeid) << 16) | MINORID2NUM(modeid)) )

#define HIDD_TO_AMIGA_MODEID(modeid)			\
    ( ((modeid) == vHidd_ModeID_Invalid) 	\
		? INVALID_ID			\
		: (GENERATE_MODEID((modeid) >> 16, (modeid) & 0x0000FFFF)) )
