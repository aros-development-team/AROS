#ifndef OOP_METHOD_H
#define OOP_METHOD_H

/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Include file for method class
    Lang: english
*/

extern ULONG __OOPI_Method;

#define IID_Method "Method"

#define CLID_Method "methodclass"

#define MethodBase (__OOPI_Method)

#define IsMethodAttr(attr) \
    (((attr) & ~(METHOD_MASK)) == (__OOPI_Method))


#define CallMethod(m) ( (m)->MethodFunc((m)->MClass, (m)->TargetObject, (m)->Message) )

enum {
    AIDX_Method_TargetObject= 0,
    AIDX_Method_Message,
    AIDX_Method_MethodID,
    
    NUM_A_Method
};

#define A_Method_TargetObject 	(MethodBase + AIDX_Method_TargetObject)
#define A_Method_Message	(MethodBase + AIDX_Method_Message)
#define A_Method_MethodID 	(MethodBase + AIDX_Method_MethodID)

typedef struct
{
    Object	*TargetObject;
    Msg		Message;
    Class	*MClass;
    IPTR	(*MethodFunc)(Class *, Object *, Msg);
} Method;

#endif /* OOP_METHOD_H */
