

/****** AmigaOS Bit defines for AttnFlags (for compatability) ******************************/

/*  Processors and Co-processors: */

#define AFB_68010	0	                                /* also set for 68020 */
#define AFB_68020	1	                                /* also set for 68030 */
#define AFB_68030	2	                                /* also set for 68040 */
#define AFB_68040	3                                       /* also set for 68060 */
#define	AFB_68060	7

#define AFB_68881	4	                                /* also set for 68882 */
#define AFB_68882	5
#define	AFB_FPU40	6	                                /* Set if 68040/060 FPU */

/*
    The AFB_FPU40 bit is set when a working 040/060 FPU
    is in the system.  If this bit is set and both the
    AFB_68881 and AFB_68882 bits are not set, then the 680x0
    math emulation code has not been loaded and only 680x0
    FPU instructions are available.  This bit is valid *ONLY*
    if the AFB_68040 bit is set.
*/

#define AFB_PRIVATE	15	/* Just what it says */

#define AFF_68010	(1L<<0)
#define AFF_68020	(1L<<1)
#define AFF_68030	(1L<<2)
#define AFF_68040	(1L<<3)
#define	AFF_68060	(1L<<7)

#define AFF_68881	(1L<<4)
#define AFF_68882	(1L<<5)
#define	AFF_FPU40	(1L<<6)

#define AFF_PRIVATE	(1L<<15)

/* #define AFB_RESERVED8   8 */
/* #define AFB_RESERVED9   9 */

