#ifndef HIDD_TAP_H
#define HIDD_TAP_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: tap.h 23815 2005-12-20 14:26:33Z stegerg $

    Desc: Definitions for the Tap HIDD system.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef HIDD_HIDD_H
#   include <hidd/hidd.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#include <utility/utility.h>


#define CLID_Hidd_Tap		"hidd.network.tap"
#define CLID_Hidd_TapUnit	"hidd.network.tapunit"

#define IID_Hidd_Tap		"hidd.network.tap"
#define IID_Hidd_TapUnit	"hidd.network.tapunit"



/**** Graphics definitions ****************************************************/

#ifndef __OOP_NOATTRBASES__
extern OOP_AttrBase HiddTapUnitAB;
#endif

enum {
    aoHidd_TapUnit_Unit,
    
    num_Hidd_TapUnit_Attrs
    
};

#define aHidd_TapUnit_Unit		(HiddTapUnitAB + aoHidd_TapUnit_Unit)

#define IS_HIDDTAPUNIT_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HiddTapUnitAB, num_Hidd_TapUnit_Attrs)

enum
{
    moHidd_Tap_NewUnit = 0,       
    moHidd_Tap_DisposeUnit,
    moHidd_Tap_NumMethods		// always keep this the last one!
};




struct pHidd_Tap_NewUnit
{
    OOP_MethodID	mID;
    ULONG		unitnum;
};

struct pHidd_Tap_DisposeUnit
{
    OOP_MethodID    	mID;
    OOP_Object         	*unit;
};


/**** Tap Unit definitions ******************************************************/


enum
{
    /* Methods for a tap unit */

    moHidd_TapUnit_Init,
    moHidd_TapUnit_Write,
    moHidd_TapUnit_Start,
    moHidd_TapUnit_Stop,
    moHidd_TapUnit_GetStatus,
    moHidd_TapUnit_NumMethods	// always keep this the last one!
};


/* messages for a Tap unit */

struct pHidd_TapUnit_Init
{
    OOP_MethodID	mID;
    VOID		*DataReceived;
    VOID		*DataReceivedUserData;
    VOID		*WriteData;
    VOID		*WriteDataUserData;
};

struct pHidd_TapUnit_Write
{
    OOP_MethodID	mID;
    ULONG		Length;
    UBYTE		*Outbuffer;
};

struct pHidd_TapUnit_Start
{
    OOP_MethodID	mID;
};

struct pHidd_TapUnit_Stop
{
    OOP_MethodID	mID;
};

struct pHidd_TapUnit_GetStatus
{
    OOP_MethodID	mID;
};

/* Predeclarations of stubs in libhiddTapstubs.h */

OOP_Object * HIDD_Tap_NewUnit	(OOP_Object *obj, ULONG unitnum);
VOID     HIDD_Tap_DisposeUnit	(OOP_Object *obj, OOP_Object *unit);

BOOL     HIDD_TapUnit_Init		(OOP_Object *obj, VOID * DataReceived, VOID * DataReceivedUserData, VOID * WriteData, VOID * WriteDataUserData);
ULONG    HIDD_TapUnit_Write	(OOP_Object *obj, UBYTE * data, ULONG length);
VOID     HIDD_TapUnit_Start        (OOP_Object *obj);
VOID     HIDD_TapUnit_Stop         (OOP_Object *obj);
UWORD    HIDD_TapUnit_GetStatus    (OOP_Object *obj);

#endif /* HIDD_TAP_H */
