#ifndef OOP_INTERFACE_H
#define OOP_INTERFACE_H

/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Include file for interface class
    Lang: english
*/

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

extern ULONG __OOPI_Interface;

#define GUID_Interface "Interface"

#define INTERFACECLASS "interfaceclass"

#define InterfaceBase (__OOPI_Interface)

#define IsInterfaceAttr(attr) \
    (((attr) & ~(METHOD_MASK)) == (__OOPI_Interface))



enum {
    AIDX_Interface_TargetObject= 0,
    AIDX_Interface_InterfaceID,
    
    NUM_A_Interface
};

#define A_Interface_TargetObject 	(InterfaceBase + AIDX_Interface_TargetObject)
#define A_Interface_InterfaceID		(InterfaceBase + AIDX_Interface_InterfaceID)


typedef struct InterfaceStruct 
{
    IPTR (*Call)(struct InterfaceStruct *, Msg);
    Object	*TargetObject;
    
} Interface;

#endif /* OOP_INTERFACE_H */
