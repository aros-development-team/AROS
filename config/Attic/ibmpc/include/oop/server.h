#ifndef OOP_SERVER_H
#define OOP_SERVER_H

/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Include for server class.
    Lang: english
*/


extern ULONG __OOPI_Server;

#define IID_Server "Server"
#define CLID_Server "serverclass"

enum {
    moServer_AddObject = 0,
    moServer_RemoveObject,
    moServer_FindObject,
    moServer_Run,
    
    num_Server_Methods
};

#define ServerBase (__OOPI_Server)

#define M_Server_AddObject	(ServerBase + moServer_AddObject)
#define M_Server_RemoveObject	(ServerBase + moServer_RemoveObject)
#define M_Server_FindObject	(ServerBase + moServer_FindObject)
#define M_Server_Run		(ServerBase + moServer_Run)

/* Message parameter structs */

struct P_Server_AddObject
{
    ULONG  MethodID;
    Object *Object;
    STRPTR ObjectID;
};

struct P_Server_RemoveObject
{
    ULONG  MethodID;
    STRPTR ObjectID;
};

struct P_Server_FindObject
{
    ULONG  MethodID;
    STRPTR ObjectID;
};

#define Server_AddObject(o, object, id)	\
({		\
    struct P_Server_AddObject msg;	\
    msg.MethodID = GetMethodID(IID_Server, moServer_AddObject);	\
    msg.Object = object;		\
    msg.ObjectID = id;			\
    ((BOOL)DoMethod(o, (Msg)&msg));		\
})

#define Server_RemoveObject(o, id)	\
({		\
    struct P_Server_RemoveObject msg;		\
    msg.MethodID = GetMethodID(IID_Server, moServer_RemoveObject);	\
    msg.ObjectID = id;				\
    DoMethod(o, (Msg)&msg);			\
})

#define Server_FindObject(o, id)	\
({					\
    struct P_Server_FindObject msg;	\
    msg.MethodID = GetMethodID(IID_Server, moServer_FindObject);	\
    msg.ObjectID = id;			\
    ((Object *)DoMethod(o, (Msg)&msg));		\
})

#define Server_Run(o)			\
({					\
    ULONG mid = GetMethodID(IID_Server, moServer_Run);		\
    DoMethod(o, (Msg)&mid);		\
})

#endif /* OOP_SERVER_H */
