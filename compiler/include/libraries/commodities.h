#ifndef LIBRARIES_COMMODITIES_H
#define LIBRARIES_COMMODITIES_H

/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Includes for commodities.library.
*/

#ifndef EXEC_PORTS_H
#   include <exec/ports.h>
#endif
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef COMMODITIES_BASE_H
typedef LONG CxObj;
typedef LONG CxMsg;
#endif /* COMMODITIES_BASE_H */

typedef LONG (*PFL)();

struct NewBroker
{
    BYTE             nb_Version;         /* see below */
    CONST_STRPTR     nb_Name;
    CONST_STRPTR     nb_Title;
    CONST_STRPTR     nb_Descr;
    WORD             nb_Unique;          /* see below */
    WORD             nb_Flags;           /* see below */
    BYTE             nb_Pri;
    struct MsgPort * nb_Port;
    WORD             nb_ReservedChannel;
};

/* nb_Version */
#define NB_VERSION 5

/* nb_Unique */
#define NBU_DUPLICATE 0
#define NBU_UNIQUE    (1<<0)
#define NBU_NOTIFY    (1<<1)

/* nb_Flags */
#define COF_SHOW_HIDE (1<<2)

#define CBD_NAMELEN  24 /* length of nb_Name */
#define CBD_TITLELEN 40 /* length of nb_Title */
#define CBD_DESCRLEN 40 /* length of nb_Descr */

/* return values of CxBroker() */
#define CBERR_OK      0
#define CBERR_SYSERR  1
#define CBERR_DUP     2
#define CBERR_VERSION 3

/* return values of CxObjError() */
#define COERR_ISNULL     (1<<0)
#define COERR_NULLATTACH (1<<1)
#define COERR_BADFILTER  (1<<2)
#define COERR_BADTYPE    (1<<3)

#define CXM_IEVENT  (1<<5)
#define CXM_COMMAND (1<<6)

#define CXCMD_DISABLE   (15)
#define CXCMD_ENABLE    (17)
#define CXCMD_APPEAR    (19)
#define CXCMD_DISAPPEAR (21)
#define CXCMD_KILL      (23)
#define CXCMD_UNIQUE    (25)
#define CXCMD_LIST_CHG  (27)

#define CX_INVALID    0
#define CX_FILTER     1
#define CX_TYPEFILTER 2
#define CX_SEND       3
#define CX_SIGNAL     4
#define CX_TRANSLATE  5
#define CX_BROKER     6
#define CX_DEBUG      7
#define CX_CUSTOM     8
#define CX_ZERO       9

/* Macros */
#define CxFilter(d)         CreateCxObj((LONG)CX_FILTER,    (IPTR)(d),      0L)
#define CxSender(port,id)   CreateCxObj((LONG)CX_SEND,      (IPTR)(port),   (LONG)(id))
#define CxSignal(task,sig)  CreateCxObj((LONG)CX_SIGNAL,    (IPTR)(task),   (LONG)(sig))
#define CxTranslate(ie)     CreateCxObj((LONG)CX_TRANSLATE, (IPTR)(ie),     0L)
#define CxDebug(id)         CreateCxObj((LONG)CX_DEBUG,     (IPTR)(id),     0L)
#define CxCustom(action,id) CreateCxObj((LONG)CX_CUSTOM,    (IPTR)(action), (LONG)(id))

struct InputXpression
{
    UBYTE ix_Version;   /* see below */
    UBYTE ix_Class;
    UWORD ix_Code;
    UWORD ix_CodeMask;
    UWORD ix_Qualifier;
    UWORD ix_QualMask;  /* see below */
    UWORD ix_QualSame;  /* see below */
};
typedef struct InputXpression IX;

/* ix_Version */
#define IX_VERSION 2

/* ix_QualMask */
#define IX_NORMALQUALS 0x7FFF

/* ix_QualSame */
#define IXSYM_SHIFT (1<<0)
#define IXSYM_CAPS  (1<<1)
#define IXSYM_ALT   (1<<2)
#define IXSYM_SHIFTMASK (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)
#define IXSYM_CAPSMASK  (IXSYM_SHIFTMASK    | IEQUALIFIER_CAPSLOCK)
#define IXSYM_ALTMASK   (IEQUALIFIER_LALT   | IEQUALIFIER_RALT)

#define NULL_IX(ix) ((ix)->ix_Class == IECLASS_NULL)

#endif /* LIBRARIES_COMMODITIES_H */
