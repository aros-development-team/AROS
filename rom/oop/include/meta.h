#ifndef OOP_META_H
#define OOP_META_H

/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Include file for meta class
    Lang: english
*/

extern ULONG __OOPI_Meta;

#define GUID_Meta "Meta"
#define METACLASS "metaclass"

#define MetaBase (__OOPI_Meta)

#define IsMetaAttr(attr) \
    (((attr) & ~(METHOD_MASK)) == (__OOPI_Meta))


enum {
    AIDX_Class_SuperID = 0,
    AIDX_Class_InterfaceDescr,
    AIDX_Class_ID,
    AIDX_Class_SuperPtr,
    AIDX_Class_InstSize,
    AIDX_Class_DoMethod,
    
    NUM_A_Class
};

#define A_Class_SuperID 	(MetaBase + AIDX_Class_SuperID)
#define A_Class_InterfaceDescr	(MetaBase + AIDX_Class_InterfaceDescr)
#define A_Class_ID 		(MetaBase + AIDX_Class_ID)
#define A_Class_SuperPtr	(MetaBase + AIDX_Class_SuperPtr)
#define A_Class_InstSize	(MetaBase + AIDX_Class_InstSize)
#define A_Class_DoMethod	(MetaBase + AIDX_Class_DoMethod)

#endif /* OOP_META_H */
