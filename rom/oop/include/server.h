#ifndef OOP_SERVER_H
#define OOP_SERVER_H

/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Include for server class.
    Lang: english
*/


extern ULONG __OOPI_Server;

#define GUID_Server "Server"
#define SERVERCLASS "serverclass"

enum {
    MIDX_Server_AddObject = 0,
    MIDX_Server_RemoveObject,
    MIDX_Server_FindObject,
    MIDX_Server_Run,
    
    NUM_M_Server
};

#define ServerBase (__OOPI_Server)

#define M_Server_AddObject	(ServerBase + MIDX_Server_AddObject)
#define M_Server_RemoveObject	(ServerBase + MIDX_Server_RemoveObject)
#define M_Server_FindObject	(ServerBase + MIDX_Server_FindObject)
#define M_Server_Run		(ServerBase + MIDX_Server_Run)

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
    msg.MethodID = M_Server_AddObject;	\
    msg.Object = object;		\
    msg.ObjectID = id;			\
    ((BOOL)DoMethod(o, (Msg)&msg));		\
})

#define Server_RemoveObject(o, id)	\
({		\
    struct P_Server_RemoveObject msg;		\
    msg.MethodID = M_Server_RemoveObject;	\
    msg.ObjectID = id;				\
    DoMethod(o, (Msg)&msg);			\
})

#define Server_FindObject(o, id)	\
({					\
    struct P_Server_FindObject msg;	\
    msg.MethodID = M_Server_FindObject;	\
    msg.ObjectID = id;			\
    ((Object *)DoMethod(o, (Msg)&msg));		\
})

#define Server_Run(o)			\
({					\
    ULONG mid = M_Server_Run;		\
    DoMethod(o, (Msg)&mid);		\
})

#endif /* OOP_SERVER_H */
