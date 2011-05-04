#ifndef PRIVATE_H
#define PRIVATE_H 
/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private methods and attrs
    Lang: english
*/

#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

/**********************
**  Private methods  **
**********************/

enum
{
   MO_meta_allocdisptabs,
   MO_meta_freedisptabs,
   MO_meta_iterateifs,
   MO_meta_findmethod,
   MO_meta_findinterface,
   MO_meta_getifinfo,
   
   NUMTOTAL_M_Meta
};


struct P_meta_allocdisptabs
{
    OOP_MethodID mid;
    /* The superclass of the created class */
    OOP_Class *superclass;		    
    
    /* interface descruption table */
    const struct OOP_InterfaceDescr *ifdescr;
    
};

struct P_meta_freedisptabs
{
    OOP_MethodID mid;
};


struct P_meta_iterateifs
{
    OOP_MethodID mid;
    IPTR *iterval_ptr;
    STRPTR *interface_id_ptr;
    ULONG *num_methods_ptr;
};

struct P_meta_findmethod
{
    OOP_MethodID mid;
    OOP_MethodID method_to_find;
};


struct P_meta_getifinfo
{
    OOP_MethodID  mid;
    STRPTR interface_id;
    ULONG  *num_methods_ptr;
    
};


BOOL meta_allocdisptabs(OOP_Object *o, OOP_Class *super, struct OOP_InterfaceDescr *ifdescr);
VOID meta_freedisptabs(OOP_Object *o);
struct IFMethod *meta_iterateifs(
		 OOP_Object *o
		,IPTR *iterval_ptr
		,STRPTR *interface_id_ptr
		,ULONG *num_methods_ptr);

struct IFMethod *meta_findmethod(OOP_Object *o, OOP_MethodID method_to_find, struct Library *OOPBase);		

struct IFMethod * meta_getifinfo(OOP_Object *o, STRPTR interface_id, ULONG *num_methods_ptr);


/********************
**  Private attrs  **
********************/


#endif /* PRIVATE_H */
