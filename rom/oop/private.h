#ifndef PRIVATE_H
#define PRIVATE_H 
/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Private methods and attrs
    Lang: english
*/

#ifndef OOP_META_H
#   include <oop/meta.h>
#endif

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

/**********************
**  Private methods  **
**********************/

enum
{
   MO_meta_allocdisptabs = NUM_M_Meta,
   MO_meta_freedisptabs,
   MO_meta_iterateifs,
   MO_meta_findmethod,
   MO_meta_findinterface,
   MO_meta_getifinfo,
   
   NUMTOTAL_M_Meta
};


struct P_meta_allocdisptabs
{
    MethodID mid;
    /* The superclass of the created class */
    Class *superclass;		    
    
    /* interface descruption table */
    struct InterfaceDescr *ifdescr;
    
};

struct P_meta_freedisptabs
{
    MethodID mid;
};


struct P_meta_iterateifs
{
    MethodID mid;
    IPTR *iterval_ptr;
    STRPTR *interface_id_ptr;
    ULONG *num_methods_ptr;
};

struct P_meta_findmethod
{
    MethodID mid;
    MethodID method_to_find;
};


struct P_meta_getifinfo
{
    MethodID  mid;
    STRPTR interface_id;
    ULONG  *num_methods_ptr;
    
};


BOOL meta_allocdisptabs(Object *o, Class *super, struct InterfaceDescr *ifdescr);
VOID meta_freedisptabs(Object *o);
struct IFMethod *meta_iterateifs(
		 Object *o
		,IPTR *iterval_ptr
		,STRPTR *interface_id_ptr
		,ULONG *num_methods_ptr);

struct IFMethod *meta_findmethod(Object *o, MethodID method_to_find, struct Library *OOPBase);		

struct IFMethod * meta_getifinfo(Object *o, STRPTR interface_id, ULONG *num_methods_ptr);


/********************
**  Private attrs  **
********************/


#endif /* PRIVATE_H */
