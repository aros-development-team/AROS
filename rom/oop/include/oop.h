#ifndef OOP_OOP_H
#define OOP_OOP_H
/*
    Copyright 1995-1997 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#define AROSOOP_NAME "oop.library"
typedef ULONG Object;

typedef ULONG MethodID;
typedef ULONG AttrID;

typedef ULONG AttrBase;

typedef struct
{
    MethodID MID;
} *Msg;


struct ABDescr
{
    STRPTR interfaceID;
    AttrBase *attrBase;
};

typedef struct IClass Class;

struct IClass
{

    /* Array of pointers to methodtables for this class */
    struct Node 	ClassNode;
    
    
    ULONG InstOffset;
    APTR UserData;
    IPTR (*DoMethod)(Object *, Msg);
    IPTR (*CoerceMethod)(Class *, Object *, Msg);
    IPTR (*DoSuperMethod)(Class *, Object *, Msg);

};



struct _Object
{
    Class *o_Class;
};



/* Macros */


#define BASEOBJECT(obj) ((Object *)(_OBJ(obj) + 1))
#define _OBJECT(obj) (_OBJ(obj) - 1)
#define _OBJ(obj) ((struct _Object *)(obj))

#define INST_DATA(cl, obj) \
	(((VOID *)(obj)) + (cl)->InstOffset)

#define OCLASS(obj) \
	(_OBJECT(obj)->o_Class)


#define DoMethod(o, msg) ( (OCLASS(o))->DoMethod((o), (msg)) )
#define DoSuperMethod(cl, o, msg) ((cl)->DoSuperMethod(cl, o, msg))
#define CoerceMethod(cl, o, msg) ((cl)->CoerceMethod(cl, o, msg))

#define METHODDEF(x) (IPTR (*)())x


#define IS_IF_ATTR(attr, idx, attrbase, numifattrs) ( ((idx) = (attr) - (attrbase)) < (numifattrs) )

struct InterfaceDescr
{
    struct MethodDescr *MethodTable;
    STRPTR InterfaceID;
    ULONG NumMethods; /* Number of methods in the methodtable */
};

struct MethodDescr
{
    IPTR (*MethodFunc)();
    ULONG MethodIdx;
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
    MethodID mID;
    struct TagItem *attrList;
};

struct pRoot_Set
{
    MethodID mID;
    struct TagItem *attrList;
};

struct pRoot_Get
{
    MethodID mID;
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

#define CallMethod(m) ( (m)->methodFunc((m)->methodClass, (m)->targetObject, (m)->message) )

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
    Object	*targetObject;
    Msg		message;
    Class	*methodClass;
    IPTR	(*methodFunc)(Class *, Object *, Msg);
} Method;


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


typedef struct InterfaceStruct
{
    IPTR (*callMethod)(struct InterfaceStruct *, Msg);
    Object	*targetObject;
    
} Interface;


/***********************
**  Some metaclasses  **
***********************/

#define CLID_MIMeta   "mimetaclass"	/* Supports multiple interfaces	  */
#define CLID_SIMeta   "simetaclass"	/* Supports only single intefaces */



#endif /* OOP_OOP_H */
