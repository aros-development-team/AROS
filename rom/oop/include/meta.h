#ifndef OOP_META_H
#define OOP_META_H

/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Include file for meta class
    Lang: english
*/

extern ULONG __OOPI_Meta;

#define IID_Meta "Meta"
#define CLID_Meta "metaclass"

#define MetaBase (__OOPI_Meta)

#define IsMetaAttr(attr) \
    (((attr) & ~(METHOD_MASK)) == (__OOPI_Meta))

enum
{
    NUM_M_Meta
};

enum {
    AO_Meta_SuperID = 0,
    AO_Meta_InterfaceDescr,
    AO_Meta_ID,
    AO_Meta_SuperPtr,
    AO_Meta_InstSize,
    AO_Meta_DoMethod,
    AO_Meta_CoerceMethod,
    AO_Meta_DoSuperMethod,
    
    NUM_A_Meta
};

#define A_Meta_SuperID 		(MetaBase + AO_Meta_SuperID)
#define A_Meta_InterfaceDescr	(MetaBase + AO_Meta_InterfaceDescr)
#define A_Meta_ID 		(MetaBase + AO_Meta_ID)
#define A_Meta_SuperPtr		(MetaBase + AO_Meta_SuperPtr)
#define A_Meta_InstSize		(MetaBase + AO_Meta_InstSize)
#define A_Meta_DoMethod		(MetaBase + AO_Meta_DoMethod)
#define A_Meta_CoerceMethod	(MetaBase + AO_Meta_CoerceMethod)
#define A_Meta_DoSuperMethod	(MetaBase + AO_Meta_DoSuperMethod)

#endif /* OOP_META_H */
