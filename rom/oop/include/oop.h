#ifndef OOP_OOP_H
#define OOP_OOP_H

/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif

#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#define AROSOOP_NAME "oop.library"

typedef ULONG OOP_Object;

typedef ULONG OOP_MethodID;
typedef ULONG OOP_AttrID;

typedef ULONG OOP_AttrBase;

typedef ULONG OOP_AttrCheck;

#define GOT_ATTR(code, pre_tag, pre_ac)	\
    ((pre_ac ## _attrcheck & (1L << pre_tag ## _ ## code)) == (1L << pre_tag ## _ ## code))

#define FOUND_ATTR(code, pre_tag, pre_ac)	\
    pre_ac ## _attrcheck |= (1L << pre_tag ## _ ## code)

#define DECLARE_ATTRCHECK(pre_tag)	\
    OOP_AttrCheck pre_tag ## _attrcheck = 0UL
    
#define ATTRCHECK(pre_tag)	\
    pre_tag ## _attrcheck
    
enum {
    ooperr_ParseAttrs_TooManyAttrs = 1
};

typedef struct
{
    OOP_MethodID MID;
} *OOP_Msg;


struct OOP_ABDescr
{
    STRPTR  	    interfaceID;
    OOP_AttrBase    *attrBase;
};

typedef struct OOP_IClass OOP_Class;

struct OOP_IClass
{
    /* Array of pointers to methodtables for this class */
    struct Node 	ClassNode;    
    struct Library  	*OOPBasePtr;
    ULONG   	    	InstOffset;
    APTR    	    	UserData;
    IPTR    	    	(*cl_DoMethod)(OOP_Object *, OOP_Msg);
    IPTR    	    	(*cl_CoerceMethod)(OOP_Class *, OOP_Object *, OOP_Msg);
    IPTR    	    	(*cl_DoSuperMethod)(OOP_Class *, OOP_Object *, OOP_Msg);
};



struct _OOP_Object
{
    OOP_Class *o_Class;
};



/* Macros */


#define OOP_BASEOBJECT(obj) ((OOP_Object *)(_OOP_OBJ(obj) + 1))
#define _OOP_OBJECT(obj) (_OOP_OBJ(obj) - 1)
#define _OOP_OBJ(obj) ((struct _OOP_Object *)(obj))

#define OOP_INST_DATA(cl, obj) \
	((APTR)(((UBYTE *)obj) + (cl)->InstOffset))

#define OOP_OCLASS(obj) \
	(_OOP_OBJECT(obj)->o_Class)

#define OOP_OOPBASE(obj) \
    	(OOP_OCLASS(obj)->OOPBasePtr)

#define OOP_DoMethod(o, msg) ( (OOP_OCLASS(o))->cl_DoMethod((o), (msg)) )
#define OOP_DoSuperMethod(cl, o, msg) ((cl)->cl_DoSuperMethod(cl, o, msg))
#define OOP_CoerceMethod(cl, o, msg) ((cl)->cl_CoerceMethod(cl, o, msg))

#define OOP_METHODDEF(x) (IPTR (*)())x


#define IS_IF_ATTR(attr, idx, attrbase, numifattrs) ( ((idx) = (attr) - (attrbase)) < (numifattrs) )

struct OOP_InterfaceDescr
{
    struct OOP_MethodDescr  *MethodTable;
    STRPTR  	    	    InterfaceID;
    ULONG   	    	    NumMethods; /* Number of methods in the methodtable */
};

typedef IPTR (*OOP_MethodFunc)(OOP_Class *cl, OOP_Object *o, OOP_Msg msg);

struct OOP_MethodDescr
{
    OOP_MethodFunc MethodFunc;
    ULONG   MethodIdx;
};


/* Some basic interfaces and classes */

/*********************
**  rootclass defs  **
*********************/

#define IID_Root "Root"
#define CLID_Root "rootclass"


enum
{
    moRoot_New = 0,
    moRoot_Dispose,
    moRoot_Set,
    moRoot_Get,
    
    num_Root_Methods
};
    

struct pRoot_New
{
    OOP_MethodID mID;
    struct TagItem *attrList;
};

struct pRoot_Set
{
    OOP_MethodID mID;
    struct TagItem *attrList;
};

struct pRoot_Get
{
    OOP_MethodID mID;
    ULONG attrID;
    IPTR *storage;
};

/**************************
**  meta interface defs  **
**************************/

#define IID_Meta "Meta"

#define MetaAttrBase (__IMeta)


enum
{
    num_Meta_Methods
};

enum {
    aoMeta_SuperID = 0,
    aoMeta_InterfaceDescr,
    aoMeta_ID,
    aoMeta_SuperPtr,
    aoMeta_InstSize,
    aoMeta_DoMethod,
    aoMeta_CoerceMethod,
    aoMeta_DoSuperMethod,
    
    num_Meta_Attrs
};

#define aMeta_SuperID 		(MetaAttrBase + aoMeta_SuperID)
#define aMeta_InterfaceDescr	(MetaAttrBase + aoMeta_InterfaceDescr)
#define aMeta_ID 		(MetaAttrBase + aoMeta_ID)
#define aMeta_SuperPtr		(MetaAttrBase + aoMeta_SuperPtr)
#define aMeta_InstSize		(MetaAttrBase + aoMeta_InstSize)
#define aMeta_DoMethod		(MetaAttrBase + aoMeta_DoMethod)
#define aMeta_CoerceMethod	(MetaAttrBase + aoMeta_CoerceMethod)
#define aMeta_DoSuperMethod	(MetaAttrBase + aoMeta_DoSuperMethod)

/***********************
**  methodclass defs  **
***********************/

extern ULONG __IMethod;

#define IID_Method "Method"

#define CLID_Method "methodclass"

#define MethodAttrBase (__IMethod)

#define OOP_CallMethod(m) ( (m)->methodFunc((m)->methodClass, (m)->targetObject, (m)->message) )

enum {
    aoMethod_TargetObject= 0,
    aoMethod_Message,
    aoMethod_MethodID,
    
    num_Method_Attrs
};

#define aMethod_TargetObject 	(MethodAttrBase + aoMethod_TargetObject)
#define aMethod_Message		(MethodAttrBase + aoMethod_Message)
#define aMethod_MethodID 	(MethodAttrBase + aoMethod_MethodID)

typedef struct
{
    OOP_Object		*targetObject;
    OOP_Msg		message;
    OOP_Class		*methodClass;
    OOP_MethodFunc 	methodFunc;
} OOP_Method;


/**************************
**  interfaceclass defs  **
**************************/

extern ULONG __IInterface;

#define IID_Interface "Interface"

#define CLID_Interface "interfaceclass"

#define InterfaceAttrBase (__IInterface)


enum {
    aoInterface_TargetObject= 0,
    aoInterface_InterfaceID,
    
    NUM_A_Interface
};

#define aInterface_TargetObject 	(InterfaceAttrBase + aoInterface_TargetObject)
#define aInterface_InterfaceID		(InterfaceAttrBase + aoInterface_InterfaceID)


typedef struct OOP_InterfaceStruct
{
    IPTR    	(*callMethod)(struct OOP_InterfaceStruct *, OOP_Msg);
    OOP_Object	*targetObject;
    
} OOP_Interface;


/***********************
**  Some metaclasses  **
***********************/

#define CLID_MIMeta   "mimetaclass"	/* Supports multiple interfaces	  */
#define CLID_SIMeta   "simetaclass"	/* Supports only single intefaces */



#endif /* OOP_OOP_H */
