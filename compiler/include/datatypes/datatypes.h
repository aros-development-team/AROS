#ifndef DATATYPES_DATATYPES_H
#define DATATYPES_DATATYPES_H

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef    EXEC_TYPES_H
#include  <exec/types.h>
#endif
#ifndef    EXEC_LISTS_H
#include  <exec/lists.h>
#endif
#ifndef    EXEC_NODES_H
#include  <exec/nodes.h>
#endif
#ifndef    EXEC_LIBRARIES_H
#include  <exec/libraries.h>
#endif
#ifndef    LIBRARIES_IFFPARSE_H
#include  <libraries/iffparse.h>
#endif
#ifndef	   DOS_DOS_H
#include  <dos/dos.h>
#endif


#define ID_DTYP MAKE_ID('D','T','Y','P')
#define ID_DTHD MAKE_ID('D','T','H','D')


struct DataTypeHeader
{
    STRPTR	 dth_Name;           /* Name of the data type */
    STRPTR	 dth_BaseName;       /* Base name of the data type */
    STRPTR	 dth_Pattern;        /* File name match pattern */
    WORD	*dth_Mask;           /* Comparison mask (binary) */
    ULONG	 dth_GroupID;        /* DataType Group */
    ULONG	 dth_ID;             /* DataType ID (same as IFF FORM type) */
    WORD	 dth_MaskLen;        /* Length of the comparison mask */
    WORD	 dth_Pad;            /* Unused at present (must be 0) */
    UWORD	 dth_Flags;          /* Flags -- see below */
    UWORD	 dth_Priority;
};

#define	 DTHSIZE       sizeof(struct DataTypeHeader)

/* Types */
#define DTF_TYPE_MASK  0x000F
#define DTF_BINARY     0x0000
#define DTF_ASCII      0x0001
#define DTF_IFF        0x0002
#define DTF_MISC       0x0003

#define DTF_CASE       0x0010      /* Case is important */

#define DTF_SYSTEM1    0x1000      /* For system use only */


/*****   Group ID and ID   ************************************************/

/* System file -- executable, directory, library, font and so on. */
#define	GID_SYSTEM      MAKE_ID('s','y','s','t')
#define ID_BINARY       MAKE_ID('b','i','n','a')
#define ID_EXECUTABLE   MAKE_ID('e','x','e','c')
#define ID_DIRECTORY    MAKE_ID('d','i','r','e')
#define ID_IFF          MAKE_ID('i','f','f',0)

/* Text, formatted or not */
#define	GID_TEXT        MAKE_ID('t','e','x','t')
#define ID_ASCII        MAKE_ID('a','s','c','i')

/* Formatted text combined with graphics or other DataTypes */
#define GID_DOCUMENT    MAKE_ID('d','o','c','u')

/* Sound */
#define GID_SOUND       MAKE_ID('s','o','u','n')

/* Musical instrument */
#define GID_INSTRUMENT  MAKE_ID('i','n','s','t')

/* Musical score */
#define GID_MUSIC       MAKE_ID('m','u','s','i')

/* Picture */
#define GID_PICTURE     MAKE_ID('p','i','c','t')

/* Animated pictures */
#define GID_ANIMATION   MAKE_ID('a','n','i','m')

/* Animation with audio */
#define GID_MOVIE       MAKE_ID('m','o','v','i')


/**************************************************************************/


#define ID_CODE         MAKE_ID('D','T','C','D')


struct DTHookContext
{
    struct Library       *dthc_SysBase;
    struct Library       *dthc_DOSBase;
    struct Library       *dthc_IFFParseBase;
    struct Library       *dthc_UtilityBase;

    /* File context */
    BPTR                  dthc_Lock;
    struct FileInfoBlock *dthc_FIB;
    BPTR                  dthc_FileHandle;   /* Pointer to file handle 
						(may be NULL) */
    struct IFFHandle     *dthc_IFF;          /* Pointer to IFFHandle 
						(may be NULL) */
    STRPTR                dthc_Buffer;       /* Buffer... */
    ULONG                 dthc_BufferLength; /* ... and corresponding length */
};


#define ID_TOOL MAKE_ID('D','T','T','L')

struct Tool
{
    UWORD    tn_Which;
    UWORD    tn_Flags;           /* Flags -- see below */
    STRPTR   tn_Program;         /* Application to use */
};


enum
{ 
    TW_MISC = 0,
    TW_INFO,
    TW_BROWSE,
    TW_EDIT,
    TW_PRINT,
    TW_MAIL
};


#define TF_LAUNCH_MASK       0x000F
#define TF_SHELL             0x0001
#define TF_WORKBENCH         0x0002
#define TF_RX                0x0003


/* Tags for use with FindToolNodeA(), GetToolAttrsA() and so on */

#define TOOLA_Dummy          TAG_USER
#define TOOLA_Program        (TOOLA_Dummy + 1)
#define TOOLA_Which          (TOOLA_Dummy + 2)
#define TOOLA_LaunchType     (TOOLA_Dummy + 3)


#define ID_TAGS      MAKE_ID('D','T','T','G')


/*************************************************************************/


#ifndef	DATATYPE
#define	DATATYPE
struct DataType
{
    struct Node            dtn_Node1;         /* These two nodes are for... */
    struct Node            dtn_Node2;         /* ...system use only! */
    struct DataTypeHeader *dtn_Header;
    struct List            dtn_ToolList;      /* Tool nodes */
    STRPTR                 dtn_FunctionName;  /* Name of comparison routine */
    struct TagItem        *dtn_AttrList;      /* Object creation tags */
    ULONG                  dtn_Length;        /* Length of the memory block */
};
#endif

#define	DTNSIZE	sizeof(struct DataType)


struct ToolNode
{
    struct Node	 tn_Node;
    struct Tool  tn_Tool;
    ULONG	 tn_Length;  /* Length of the memory block */
};

#define TNSIZE  sizeof(struct ToolNode)


#ifndef ID_NAME
#define ID_NAME   MAKE_ID('N','A','M','E')
#endif


/* Text ID:s */
#define DTERROR_UNKNOWN_DATATYPE        2000
#define DTERROR_COULDNT_SAVE            2001
#define DTERROR_COULDNT_OPEN            2002
#define DTERROR_COULDNT_SEND_MESSAGE    2003

/* new for V40 */
#define DTERROR_COULDNT_OPEN_CLIPBOARD  2004
#define DTERROR_Reserved                2005
#define DTERROR_UNKNOWN_COMPRESSION     2006
#define DTERROR_NOT_ENOUGH_DATA         2007
#define DTERROR_INVALID_DATA            2008

#define DTMSG_TYPE_OFFSET               2100


#endif /* DATATYPES_DATATYPES_H */


