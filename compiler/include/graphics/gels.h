#ifndef	GRAPHICS_GELS_H
#define	GRAPHICS_GELS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: gels structures
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

/* VSprite flags */
/* VSprite flags set by user: */
#define VSPRITE     0x0001   
#define SAVEBACK    0x0002   
#define OVERLAY     0x0004   
#define MUSTDRAW    0x0008   
#define SUSERFLAGS  0x00FF   

/* VSprite flags set by system: */
#define BACKSAVED   0x0100   
#define BOBUPDATE   0x0200 
#define GELGONE     0x0400   
#define VSOVERFLOW  0x0800 

/* Bob flags */
/* user flag bits */
#define SAVEBOB     0x0001 
#define BOBISCOMP   0x0002    
#define BUSERFLAGS  0x00FF   

/* system flag bits */
#define BWAITING     0x0100 
#define BDRAWN	     0x0200    
#define BOBSAWAY     0x0400   
#define BOBNIX	     0x0800   
#define SAVEPRESERVE 0x1000  
#define OUTSTEP      0x2000  

/* defines for animation procedures */
#define ANFRACSIZE  6
#define RINGTRIGGER 0x0001
#define ANIMHALF    0x0020


/* UserStuff definitions */
#ifndef VUserStuff	     
#define VUserStuff WORD
#endif

#ifndef BUserStuff	      
#define BUserStuff WORD
#endif

#ifndef AUserStuff	     
#define AUserStuff WORD
#endif




/*********************** GEL STRUCTURES ***********************************/

struct VSprite
{
/* SYSTEM VARIABLES */
    struct VSprite   *NextVSprite;
    struct VSprite   *PrevVSprite;

    struct IntVSprite *IntVSprite;
    struct VSprite   *ClearPath;

    WORD OldY, OldX;

/* COMMON VARIABLES */
    WORD Flags;

/* USER VARIABLES */
    WORD Y, X;

    WORD Height;
    WORD Width;
    WORD Depth;

    WORD MeMask;
    WORD HitMask;

    WORD *ImageData;

    WORD *BorderLine;
    WORD *CollMask;

    WORD *SprColors;

    struct Bob *VSBob;	      

    BYTE PlanePick;
    BYTE PlaneOnOff;

    VUserStuff VUserExt;      /* user definable:  see note above */
};

struct Bob
/* blitter-objects */
{
/* SYSTEM VARIABLES */

/* COMMON VARIABLES */
    WORD Flags;

/* USER VARIABLES */
    WORD *SaveBuffer;
    WORD *ImageShadow;
    struct Bob *Before;
    struct Bob *After;
    struct VSprite   *BobVSprite; 
    struct AnimComp  *BobComp;
    struct DBufPacket *DBuffer;
    BUserStuff BUserExt;
};

struct AnimComp
{
/* SYSTEM VARIABLES */

/* COMMON VARIABLES */
    WORD Flags;
    WORD Timer;

/* USER VARIABLES */
    WORD TimeSet;

    struct AnimComp  *NextComp;
    struct AnimComp  *PrevComp;
    struct AnimComp  *NextSeq;
    struct AnimComp  *PrevSeq;

    WORD (*AnimCRoutine)(); 

    WORD YTrans;     
    WORD XTrans;

    struct AnimOb    *HeadOb;

    struct Bob	     *AnimBob;
};

struct AnimOb
{
/* SYSTEM VARIABLES  */
    struct AnimOb    *NextOb, *PrevOb;

    LONG Clock;

    WORD AnOldY, AnOldX;	   

/* COMMON VARIABLES */
    WORD AnY, AnX;

/* USER VARIABLES  */
    WORD YVel, XVel;
    WORD YAccel, XAccel;

    WORD RingYTrans, RingXTrans;

    WORD (*AnimORoutine)();

    struct AnimComp  *HeadComp;

    AUserStuff AUserExt;
};

struct DBufPacket
{
    WORD BufY, BufX;		    
    struct VSprite   *BufPath;

    WORD *BufBuffer;
};



/* ************************************************************************ */

/* simple GEL functions that can currently exist as a definition.  
 */
#define InitAnimate(animKey) {*(animKey) = NULL;}
#define RemBob(b) {(b)->Flags |= BOBSAWAY;}


/* ************************************************************************ */

#define B2NORM	    0
#define B2SWAP	    1
#define B2BOBBER    2

/* ************************************************************************ */

/* a structure for the 16 collision procedure addresses */
struct collTable
{
    int (*collPtrs[16])();
};

/* cxref mixes up with the function pointers in the previous definition */
extern int __cxref_bug_gels;

#endif	/* GRAPHICS_GELS_H */
