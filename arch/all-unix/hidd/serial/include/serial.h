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



/**** Graphics definitions ****************************************************/

enum
{
    /* Methods for a graphics hidd */

    moHidd_Serial_NewUnit = 0,       
    moHidd_Serial_DisposeUnit
};




/* messages for a graphics hidd */

struct pHidd_Serial_NewUnit
{
    MethodID	mID;
    ULONG	unitnum;
};

struct pHidd_Serial_DisposeUnit
{
    MethodID    mID;
    Object      *unit;
};


/**** Serial Unit definitions ******************************************************/


enum
{
    /* Methods for a serial unit */

    moHidd_SerialUnit_Init,
    moHidd_SerialUnit_Write,
    moHidd_SerialUnit_SetBaudrate,
    moHidd_SerialUnit_SendBreak
};


/* messages for a serial unit */

struct pHidd_SerialUnit_Init
{
    MethodID	mID;
    VOID	*DataReceived;
    VOID	*WriteData;
};

struct pHidd_SerialUnit_Write
{
    MethodID	mID;
    ULONG	Length;
    UBYTE	*Outbuffer;
};

struct pHidd_SerialUnit_SetBaudrate
{
    MethodID	mID;
    ULONG	baudrate;
};

struct pHidd_SerialUnit_SendBreak
{
    MethodID	mID;
    int		duration;
};


/* Predeclarations of stubs in libhiddserialstubs.h */

Object * HIDD_Serial_NewUnit		(Object *obj, ULONG unitnum);
VOID     HIDD_Serial_DisposeUnit	(Object *obj, Object *unit);

BOOL     HIDD_SerialUnit_Init	(Object *obj, VOID * DataReceived, VOID * WriteData);
ULONG    HIDD_SerialUnit_Write	(Object *obj, UBYTE * data, ULONG length);
BOOL     HIDD_SerialUnit_SetBaudrate(Object *obj, ULONG baudrate);
VOID     HIDD_SerialUnit_SendBreak(Object *obj, int duration);


#endif /* HIDD_SERIAL_H */
