/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Stubs for private methods.
    Lang: english
*/
#include <proto/oop.h>
#include <oop/oop.h>
#include <oop/meta.h>
#include "private.h"
#include "intern.h"

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#define OOPBase ((struct IntOOPBase *)(OCLASS(OCLASS(o)))->UserData )

BOOL meta_allocdisptabs(Object *o, Class *super, struct InterfaceDescr *ifdescr)
{
    static MethodID mid = 0UL;
    struct P_meta_allocdisptabs p;
    
    if (!mid)
    	mid = GetMethodID(IID_Meta, MIDX_meta_allocdisptabs);
    
    p.mid = mid;
    p.superclass = super;
    p.ifdescr = ifdescr;
    
    return ( DoMethod(o, (Msg)&p) );
}

VOID meta_freedisptabs(Object *o)
{
    static MethodID mid = 0UL;
    struct P_meta_freedisptabs p;
    
    if (!mid)
    	mid = GetMethodID(IID_Meta, MIDX_meta_freedisptabs);
	
    p.mid = mid;
    
    DoMethod(o, (Msg)&p);
    
    return;
    
}


struct IFMethod *meta_iterateifs(Object *o, IPTR *iterval_ptr, STRPTR *interface_id_ptr, ULONG *num_methods_ptr)
{
    static MethodID mid = 0UL;
    struct P_meta_iterateifs p;
    
    if (!mid)
    	mid = GetMethodID(IID_Meta, MIDX_meta_iterateifs);
	
    p.mid		= mid;
    p.iterval_ptr	= iterval_ptr;
    p.interface_id_ptr	= interface_id_ptr;
    p.num_methods_ptr	= num_methods_ptr;
    
    return (struct IFMethod *)DoMethod(o, (Msg)&p);
	
}

struct IFMethod *meta_getifinfo(Object *o, STRPTR interface_id, ULONG *num_methods_ptr)
{
    static MethodID mid = 0UL;
    struct P_meta_getifinfo p;
    
    if (!mid)
    	mid = GetMethodID(IID_Meta, MIDX_meta_getifinfo);
	
    p.mid		= mid;
    p.interface_id	= interface_id;
    p.num_methods_ptr	= num_methods_ptr;
    
    return (struct IFMethod *)DoMethod(o, (Msg)&p);
}


#undef OOPBase

struct IFMethod *meta_findmethod(Object *o, MethodID method_to_find, struct Library *OOPBase)
{
    static MethodID mid = 0UL;
    struct P_meta_findmethod p;
    
    if (!mid)
    	mid = GetMethodID(IID_Meta, MIDX_meta_findmethod);
	
    p.mid		= mid;
    p.method_to_find	= method_to_find;
    
    return (struct IFMethod *)DoMethod(o, (Msg)&p);
}

