#ifndef LIBRARIES_AMIGAGUIDE_H
#define LIBRARIES_AMIGAGUIDE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif 

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif 

#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif 

#ifndef EXEC_SEMAPHORES_H
#   include <exec/semaphores.h>
#endif

#ifndef INTUITION_INTUITION_H
#   include <intuition/intuition.h>
#endif

#ifndef INTUITION_SCREENS_H
#   include <intuition/screens.h>
#endif

#ifndef INTUITION_CLASSUSR_H
#   include <intuition/classusr.h>
#endif

#ifndef DOS_DOS_H
#   include <dos/dos.h>
#endif

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#ifndef APSH_TOOL_ID
#define	APSH_TOOL_ID 11000L
#define	StartupMsgID		(APSH_TOOL_ID+1L)
#define	LoginToolID		(APSH_TOOL_ID+2L)
#define	LogoutToolID		(APSH_TOOL_ID+3L)
#define	ShutdownMsgID		(APSH_TOOL_ID+4L)
#define	ActivateToolID		(APSH_TOOL_ID+5L)
#define	DeactivateToolID	(APSH_TOOL_ID+6L)
#define	ActiveToolID		(APSH_TOOL_ID+7L)
#define	InactiveToolID		(APSH_TOOL_ID+8L)
#define	ToolStatusID		(APSH_TOOL_ID+9L)
#define	ToolCmdID		(APSH_TOOL_ID+10L)
#define	ToolCmdReplyID		(APSH_TOOL_ID+11L)
#define	ShutdownToolID		(APSH_TOOL_ID+12L)
#endif

/* Attributes accepted by GetAmigaGuideAttr() */
#define	AGA_Dummy		(TAG_USER)
#define	AGA_Path		(AGA_Dummy+1)
#define	AGA_XRefList		(AGA_Dummy+2)
#define	AGA_Activate		(AGA_Dummy+3)
#define	AGA_Context		(AGA_Dummy+4)
#define	AGA_HelpGroup		(AGA_Dummy+5)	/* (ULONG) Unique identifier */
#define	AGA_Reserved1		(AGA_Dummy+6)
#define	AGA_Reserved2		(AGA_Dummy+7)
#define	AGA_Reserved3		(AGA_Dummy+8)
#define	AGA_ARexxPort		(AGA_Dummy+9)	/* (struct MsgPort *) Pointer to the ARexx message port */
#define	AGA_ARexxPortName	(AGA_Dummy+10)	/* (STRPTR) Used to specify the ARexx port name (not copied) */
#define AGA_Secure  	    	(AGA_Dummy+11)

typedef void * AMIGAGUIDECONTEXT;

struct AmigaGuideMsg
{
    struct Message	 agm_Msg;	/* Embedded Exec message structure */
    ULONG		 agm_Type;	/* Type of message */
    APTR		 agm_Data;	/* Pointer to message data */
    ULONG		 agm_DSize;	/* Size of message data */
    ULONG		 agm_DType;	/* Type of message data */
    ULONG		 agm_Pri_Ret;	/* Primary return value */
    ULONG		 agm_Sec_Ret;	/* Secondary return value */
    APTR		 agm_System1;
    APTR		 agm_System2;
};

/* Allocation description structure */
struct NewAmigaGuide
{
    BPTR		 nag_Lock;	/* Lock on the document directory */
    STRPTR		 nag_Name;	/* Name of document file */
    struct Screen	*nag_Screen;	/* Screen to place windows within */
    STRPTR		 nag_PubScreen;	/* Public screen name to open on */
    STRPTR		 nag_HostPort;	/* Application's ARexx port name */
    STRPTR		 nag_ClientPort;/* Name to assign to the clients ARexx port */
    STRPTR		 nag_BaseName;	/* Base name of the application */
    ULONG		 nag_Flags;	/* Flags */
    STRPTR		*nag_Context;	/* NULL terminated context table */
    STRPTR		 nag_Node;	/* Node to align on first (defaults to Main) */
    LONG		 nag_Line;	/* Line to align on */
    struct TagItem	*nag_Extens;	/* Tag array extension */
    VOID		*nag_Client;	/* Private! MUST be NULL */
};

/* public Client flags */
#define	HTF_LOAD_INDEX		(1L<<0)	/* Force load the index at init time */
#define	HTF_LOAD_ALL		(1L<<1)	/* Force load the entire database at init */
#define	HTF_CACHE_NODE		(1L<<2)	/* Cache each node as visited */
#define	HTF_CACHE_DB		(1L<<3)	/* Keep the buffers around until expunge */
#define	HTF_UNIQUE		(1L<<15)/* Unique ARexx port name */
#define	HTF_NOACTIVATE		(1L<<16)/* Don't activate window */

#define	HTFC_SYSGADS		0x80000000

/* Callback function ID's */
#define	HTH_OPEN		0
#define	HTH_CLOSE		1

#define	HTERR_NOT_ENOUGH_MEMORY		100L
#define	HTERR_CANT_OPEN_DATABASE	101L
#define	HTERR_CANT_FIND_NODE		102L
#define	HTERR_CANT_OPEN_NODE		103L
#define	HTERR_CANT_OPEN_WINDOW		104L
#define	HTERR_INVALID_COMMAND		105L
#define	HTERR_CANT_COMPLETE		106L
#define	HTERR_PORT_CLOSED		107L
#define	HTERR_CANT_CREATE_PORT		108L
#define	HTERR_KEYWORD_NOT_FOUND		113L

typedef struct AmigaGuideHost * AMIGAGUIDEHOST;

/* Cross reference node */
struct XRef
{
    struct Node		 xr_Node;	/* Embedded node */
    UWORD		 xr_Pad;	/* Padding */
    struct DocFile	*xr_DF;		/* Document defined in */
    STRPTR		 xr_File;	/* Name of document file */
    STRPTR		 xr_Name;	/* Name of item */
    LONG		 xr_Line;	/* Line defined at */
};

#define	XRSIZE	(sizeof (struct XRef))

/* Types of cross reference nodes */
#define	XR_GENERIC	0
#define	XR_FUNCTION	1
#define	XR_COMMAND	2
#define	XR_INCLUDE	3
#define	XR_MACRO	4
#define	XR_STRUCT	5
#define	XR_FIELD	6
#define	XR_TYPEDEF	7
#define	XR_DEFINE	8

/* Callback handle */
struct AmigaGuideHost
{
    struct Hook		 agh_Dispatcher;/* Dispatcher */
    ULONG		 agh_Reserved;	/* Must be 0 */
    ULONG		 agh_Flags;
    ULONG		 agh_UseCnt;	/* Number of open nodes */
    APTR		 agh_SystemData;/* Reserved for system use */
    APTR		 agh_UserData;	/* Anything you want... */
};

/* Methods */
#define	HM_FINDNODE	1
#define	HM_OPENNODE	2
#define	HM_CLOSENODE	3
#define	HM_EXPUNGE	10		/* Expunge DataBase */

/* HM_FINDNODE */
struct opFindHost
{
    ULONG MethodID;
    struct TagItem * ofh_Attrs;		/*  R: Additional attributes */
    STRPTR ofh_Node;			/*  R: Name of node */
    STRPTR ofh_TOC;			/*  W: Table of Contents */
    STRPTR ofh_Title;			/*  W: Title to give to the node */
    STRPTR ofh_Next;			/*  W: Next node to browse to */
    STRPTR ofh_Prev;			/*  W: Previous node to browse to */
};

/* HM_OPENNODE, HM_CLOSENODE */
struct opNodeIO
{
    ULONG MethodID;
    struct TagItem * onm_Attrs;		/*  R: Additional attributes */
    STRPTR onm_Node;			/*  R: Node name and arguments */
    STRPTR onm_FileName;		/*  W: File name buffer */
    STRPTR onm_DocBuffer;		/*  W: Node buffer */
    ULONG onm_BuffLen;			/*  W: Size of buffer */
    ULONG onm_Flags;			/* RW: Control flags */
};

/* onm_Flags */
#define	HTNF_KEEP	(1L<<0)	/* Don't flush this node until database is closed */
#define	HTNF_RESERVED1	(1L<<1)	/* Reserved for system use */
#define	HTNF_RESERVED2	(1L<<2)	/* Reserved for system use */
#define	HTNF_ASCII	(1L<<3)	/* Node is straight ASCII */
#define	HTNF_RESERVED3	(1L<<4)	/* Reserved for system use */
#define	HTNF_CLEAN	(1L<<5)	/* Remove the node from the database */
#define	HTNF_DONE	(1L<<6)	/* Done with node */

/* onm_Attrs */
#define	HTNA_Dummy	(TAG_USER)
#define	HTNA_Screen	(HTNA_Dummy+1)	/* (struct Screen *) Screen that window resides in */
#define	HTNA_Pens	(HTNA_Dummy+2)	/* Pen array (from DrawInfo) */
#define	HTNA_Rectangle	(HTNA_Dummy+3)	/* Window box */

#define	HTNA_HelpGroup	(HTNA_Dummy+5)	/* (ULONG) unique identifier */

/* HM_EXPUNGE */
struct opExpungeNode
{
    ULONG MethodID;
    struct TagItem * oen_Attrs;		/*  R: Additional attributes */
};

#endif /* LIBRARIES_AMIGAGUIDE_H */
