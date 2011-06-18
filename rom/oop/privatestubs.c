/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stubs for private methods.
    Lang: english
*/
#include <proto/oop.h>
#include <oop/oop.h>
#include "private.h"
#include "intern.h"
#include <oop/static_mid.h>

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define OOPBase (OOP_OOPBASE(o))

BOOL meta_allocdisptabs(OOP_Object *o, OOP_Class *super, struct OOP_InterfaceDescr *ifdescr)
{
    STATIC_MID;
    struct P_meta_allocdisptabs p;
    
    if (!static_mid) static_mid = OOP_GetMethodID(IID_Meta, MO_meta_allocdisptabs);
    
    p.mid = static_mid;
    p.superclass = super;
    p.ifdescr = ifdescr;
    
    return ( OOP_DoMethod(o, (OOP_Msg)&p) );
}

VOID meta_freedisptabs(OOP_Object *o)
{
    STATIC_MID;
    struct P_meta_freedisptabs p;
    
    if (!static_mid) static_mid = OOP_GetMethodID(IID_Meta, MO_meta_freedisptabs);
	
    p.mid = static_mid;
    
    OOP_DoMethod(o, (OOP_Msg)&p);
    
    return;
    
}


struct IFMethod *meta_iterateifs(OOP_Object *o, IPTR *iterval_ptr, CONST_STRPTR *interface_id_ptr, ULONG *num_methods_ptr)
{
    STATIC_MID;
    struct P_meta_iterateifs p;
    
    if (!static_mid) static_mid = OOP_GetMethodID(IID_Meta, MO_meta_iterateifs);
	
    p.mid		= static_mid;
    p.iterval_ptr	= iterval_ptr;
    p.interface_id_ptr	= interface_id_ptr;
    p.num_methods_ptr	= num_methods_ptr;
    
    return (struct IFMethod *)OOP_DoMethod(o, (OOP_Msg)&p);
	
}

struct IFMethod *meta_getifinfo(OOP_Object *o, CONST_STRPTR interface_id, ULONG *num_methods_ptr)
{
    STATIC_MID;
    struct P_meta_getifinfo p;
    
    if (!static_mid) static_mid = OOP_GetMethodID(IID_Meta, MO_meta_getifinfo);
	
    p.mid		= static_mid;
    p.interface_id	= interface_id;
    p.num_methods_ptr	= num_methods_ptr;
    
    return (struct IFMethod *)OOP_DoMethod(o, (OOP_Msg)&p);
}


#undef OOPBase

struct IFMethod *meta_findmethod(OOP_Object *o, OOP_MethodID method_to_find, struct Library *OOPBase)
{
    STATIC_MID;
    struct P_meta_findmethod p;
    
    if (!static_mid) static_mid = OOP_GetMethodID(IID_Meta, MO_meta_findmethod);
	
    p.mid		= static_mid;
    p.method_to_find	= method_to_find;
    
    return (struct IFMethod *)OOP_DoMethod(o, (OOP_Msg)&p);
}

