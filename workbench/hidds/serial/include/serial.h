#ifndef HIDD_SERIAL_H
#define HIDD_SERIAL_H

/*
    Copyright (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Definitions for the Serial HIDD system.
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


#define CLID_Hidd_Serial	"hidd.serial.serial"
#define CLID_Hidd_SerialUnit	"hidd.serial.serialunit"

#define IID_Hidd_Serial		"hidd.serial.serial"
#define IID_Hidd_SerialUnit	"hidd.serial.serialunit"


/**** Some tags for taglists  ***********************************************/

#define HIDDA_SerialUnit_BPSRate	0x0001
#define HIDDA_SerialUnit_DataLength	0x0002

/**** serial definitions ****************************************************/

extern OOP_AttrBase HiddSerialUnitAB;

enum {
    aoHidd_SerialUnit_Unit,
    aoHidd_SerialUnit_BPSRate,
    aoHidd_SerialUnit_DataLength,
    
    num_Hidd_SerialUnit_Attrs
    
};

#define aHidd_SerialUnit_Unit		(HiddSerialUnitAB + aoHidd_SerialUnit_Unit)
#define aHidd_SerialUnit_BPSRate	(HiddSerialUnitAB + aoHidd_SerialUnit_BPSRate)
#define aHidd_SerialUnit_DataLength	(HiddSerialUnitAB + aoHidd_SerialUnit_DataLength)

#define IS_HIDDSERIALUNIT_ATTR(attr, idx) IS_IF_ATTR(attr, idx, HiddSerialUnitAB, num_Hidd_SerialUnit_Attrs)

enum
{
    /* Methods for a serial hidd */

    moHidd_Serial_NewUnit = 0,       
    moHidd_Serial_DisposeUnit,
    moHidd_Serial_NumMethods		// always keep this the last one!
};




/* messages for a serial hidd */

struct pHidd_Serial_NewUnit
{
    OOP_MethodID	mID;
    ULONG		unitnum;
};

struct pHidd_Serial_DisposeUnit
{
    OOP_MethodID    	mID;
    OOP_Object      	*unit;
};


/**** Serial Unit definitions ******************************************************/


enum
{
    /* Methods for a serial unit */

    moHidd_SerialUnit_Init,
    moHidd_SerialUnit_Write,
    moHidd_SerialUnit_SetBaudrate,
    moHidd_SerialUnit_SetParameters,
    moHidd_SerialUnit_SendBreak,
    moHidd_SerialUnit_Start,
    moHidd_SerialUnit_Stop,
    moHidd_SerialUnit_GetCapabilities,
    moHidd_SerialUnit_GetStatus,
    moHidd_SerialUnit_NumMethods	// always keep this the last one!
};


/* messages for a serial unit */

struct pHidd_SerialUnit_Init
{
    OOP_MethodID	mID;
    VOID		*DataReceived;
    VOID		*DataReceivedUserData;
    VOID		*WriteData;
    VOID		*WriteDataUserData;
};

struct pHidd_SerialUnit_Write
{
    OOP_MethodID	mID;
    ULONG		Length;
    UBYTE		*Outbuffer;
};

struct pHidd_SerialUnit_SetBaudrate
{
    OOP_MethodID	mID;
    ULONG		baudrate;
};

struct pHidd_SerialUnit_SetParameters
{
    OOP_MethodID	mID;
    struct TagItem     *tags;
};

struct pHidd_SerialUnit_SendBreak
{
    OOP_MethodID	mID;
    int			duration;
};

struct pHidd_SerialUnit_GetCapabilities
{
    OOP_MethodID        mID;
    struct TagItem 	* taglist;
};

struct pHidd_SerialUnit_Start
{
    OOP_MethodID        mID;
};

struct pHidd_SerialUnit_Stop
{
    OOP_MethodID        mID;
};

struct pHidd_SerialUnit_GetStatus
{
    OOP_MethodID        mID;
};


/* some tags for HIDD_SerialUnit_SetParameters() */

#define TAG_PARITY	0x1001
#define TAG_PARITY_OFF	0x1002
#define TAG_STOP_BITS	0x1003
#define TAG_DATALENGTH	0x1004
#define TAG_SET_MCR	0x1005

/* some values for parities */

#define PARITY_1	0x01
#define PARITY_0	0x02
#define PARITY_EVEN	0x03
#define PARITY_ODD	0x04


/* for range values - for example on baudrates */
#define LIMIT_LOWER_BOUND	0x40000000
#define LIMIT_UPPER_BOUND	0x80000000



/* Predeclarations of stubs in libhiddserialstubs */

OOP_Object * HIDD_Serial_NewUnit(OOP_Object *obj, ULONG unitnum);
VOID     HIDD_Serial_DisposeUnit(OOP_Object *obj, OOP_Object *unit);

BOOL     HIDD_SerialUnit_Init	(OOP_Object *obj, VOID * DataReceived, VOID * DataReceivedUserData, VOID * WriteData, VOID * WriteDataUserData);
ULONG    HIDD_SerialUnit_Write	(OOP_Object *obj, UBYTE * data, ULONG length);
BOOL     HIDD_SerialUnit_SetBaudrate(OOP_Object *obj, ULONG baudrate);
BOOL     HIDD_SerialUnit_SetParameters(OOP_Object *obj, struct TagItem *tags);
BYTE     HIDD_SerialUnit_SendBreak(OOP_Object *obj, int duration);
VOID     HIDD_SerialUnit_Stop(OOP_Object * obj);
VOID     HIDD_SerialUnit_Start(OOP_Object * obj);
VOID     HIDD_SerialUnit_GetCapabilities(OOP_Object *obj, struct TagItem *tags);
UWORD    HIDD_SerialUnit_GetStatus(OOP_Object *obj);

#endif /* HIDD_SERIAL_H */
