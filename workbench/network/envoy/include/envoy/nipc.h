#ifndef ENVOY_NIPC_H
#define ENVOY_NIPC_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library - tags, types and structures (the public interface
          of the Envoy developer kit, carried over for compatibility).
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#define NIPCNAME                "nipc.library"

/*------------------------------------------------------------------------*/
/* Tags for AllocTransactionA() */
#define TRN_Dummy               (TAG_USER + 0xB1100)
#define TRN_AllocReqBuffer      (TRN_Dummy + 1)     /* (ULONG) allocate a request buffer of this size  */
#define TRN_AllocRespBuffer     (TRN_Dummy + 2)     /* (ULONG) allocate a response buffer of this size */
#define TRN_ReqDataNIPCBuff     (TRN_Dummy + 3)     /* (BOOL) trans_RequestData is a struct NIPCBuff   */
#define TRN_RespDataNIPCBuff    (TRN_Dummy + 4)     /* (BOOL) trans_ResponseData is a struct NIPCBuff  */

/* Tags for CreateEntityA(), GetEntityAttrsA(), SetEntityAttrsA() */
#define ENT_Dummy               (TAG_USER + 0xB1000)
#define ENT_Name                (ENT_Dummy + 1)     /* (STRPTR) name, up to 63 characters              */
#define ENT_Public              (ENT_Dummy + 2)     /* (BOOL) findable by FindEntity()                 */
#define ENT_Signal              (ENT_Dummy + 3)     /* (ULONG) signal bit number to set on arrival     */
#define ENT_AllocSignal         (ENT_Dummy + 4)     /* (ULONG *) allocate a signal bit, store it here  */
#define ENT_TimeoutLinks        (ENT_Dummy + 5)     /* (ULONG) drop idle server links after n seconds  */
#define ENT_NameLength          (ENT_Dummy + 6)     /* (ULONG) size of the ENT_Name buffer (Get)        */
#define ENT_Release             (ENT_Dummy + 7)     /* (BOOL) give up ownership                        */
#define ENT_Inherit             (ENT_Dummy + 8)     /* (BOOL) take ownership                           */

/* Entities are opaque */
struct Entity;

struct Transaction
{
    struct Message  trans_Msg;              /* carrier for local transactions; private           */
    struct Entity  *trans_SourceEntity;     /* filled in by nipc.library                         */
    struct Entity  *trans_DestinationEntity;/* filled in by nipc.library                         */
    UBYTE           trans_Command;          /* server-defined command                            */
    UBYTE           trans_Type;             /* TYPE_#?                                           */
    ULONG           trans_Error;            /* ENVOYERR_#? or server-defined                     */
    ULONG           trans_Flags;            /* TRANSF_#?                                         */
    ULONG           trans_Sequence;         /* private                                           */
    APTR            trans_RequestData;      /* request buffer                                    */
    ULONG           trans_ReqDataLength;    /* size of the request buffer                        */
    ULONG           trans_ReqDataActual;    /* bytes of request data to send                     */
    APTR            trans_ResponseData;     /* response buffer                                   */
    ULONG           trans_RespDataLength;   /* size of the response buffer                       */
    ULONG           trans_RespDataActual;   /* bytes of response data (set by the server)        */
    UWORD           trans_Timeout;          /* seconds of server processing time; 0 = none       */
    UWORD           trans_Reserved;
    IPTR            trans_ClientPrivate;    /* for the client; never transmitted                 */
    IPTR            trans_ServerPrivate;    /* for the server; never transmitted                 */
};

/* trans_Type */
#define TYPE_REQUEST            0       /* waiting to be serviced                            */
#define TYPE_RESPONSE           1       /* has been serviced (or failed)                     */
#define TYPE_SERVICING          2       /* fetched with GetTransaction()                     */
#define TYPE_NOT_ISSUED         255     /* never sent                                        */

/* trans_Flags */
#define TRANSB_REQBUFFERALLOC   0       /* FreeTransaction() frees the request buffer        */
#define TRANSB_RESPBUFFERALLOC  1       /* FreeTransaction() frees the response buffer       */
#define TRANSB_NOWAIT           2       /* BeginTransaction() must not block                 */
#define TRANSB_REQUESTTABLE     3       /* (reserved)                                        */
#define TRANSB_RESPONSETABLE    4       /* (reserved)                                        */
#define TRANSB_REQNIPCBUFF      5       /* trans_RequestData is a struct NIPCBuff *          */
#define TRANSB_RESPNIPCBUFF     6       /* trans_ResponseData is a struct NIPCBuff *         */

#define TRANSF_REQBUFFERALLOC   (1 << TRANSB_REQBUFFERALLOC)
#define TRANSF_RESPBUFFERALLOC  (1 << TRANSB_RESPBUFFERALLOC)
#define TRANSF_NOWAIT           (1 << TRANSB_NOWAIT)
#define TRANSF_REQUESTTABLE     (1 << TRANSB_REQUESTTABLE)
#define TRANSF_RESPONSETABLE    (1 << TRANSB_RESPONSETABLE)
#define TRANSF_REQNIPCBUFF      (1 << TRANSB_REQNIPCBUFF)
#define TRANSF_RESPNIPCBUFF     (1 << TRANSB_RESPNIPCBUFF)

/*------------------------------------------------------------------------*/
/* Scatter/gather buffers */
struct NIPCBuff
{
    struct MinNode      nbuff_Link;
    struct MinList      nbuff_Entries;      /* of struct NIPCBuffEntry                           */
    /* private part follows */
};

struct NIPCBuffEntry
{
    struct MinNode      nbe_Link;
    ULONG               nbe_Offset;         /* offset of this entry's data within the buffer     */
    ULONG               nbe_Length;         /* bytes of data in this entry                       */
    ULONG               nbe_PhysicalLength; /* bytes of memory at nbe_Data                       */
    UBYTE              *nbe_Data;
    /* private part follows */
};

/*------------------------------------------------------------------------*/
/* NIPCInquiryA() tags: QUERY_x asks for a value, MATCH_x restricts the
 * answering hosts. Values marked (STRPTR) are strings. */
#define QUERY_IPADDR            (TAG_USER + 0x2000)     /* (ULONG) IP address                        */
#define MATCH_IPADDR            (TAG_USER + 0x2001)
#define QUERY_REALMS            (TAG_USER + 0x2002)     /* (STRPTR) realm names known to a server    */
#define MATCH_REALM             (TAG_USER + 0x2003)
#define QUERY_HOSTNAME          (TAG_USER + 0x2004)     /* (STRPTR) host name, "realm:host" in a realm */
#define MATCH_HOSTNAME          (TAG_USER + 0x2005)
#define QUERY_SERVICE           (TAG_USER + 0x2006)     /* (STRPTR) one item per service             */
#define MATCH_SERVICE           (TAG_USER + 0x2007)
#define QUERY_ENTITY            (TAG_USER + 0x2008)     /* (STRPTR) one item per public entity       */
#define MATCH_ENTITY            (TAG_USER + 0x2009)
#define QUERY_OWNER             (TAG_USER + 0x200A)     /* (STRPTR) owner of the machine             */
#define MATCH_OWNER             (TAG_USER + 0x200B)
#define QUERY_MACHDESC          (TAG_USER + 0x200C)     /* unused                                    */
#define MATCH_MACHDESC          (TAG_USER + 0x200D)
#define QUERY_ATTNFLAGS         (TAG_USER + 0x200E)     /* (ULONG) ExecBase->AttnFlags               */
#define MATCH_ATTNFLAGS         (TAG_USER + 0x200F)
#define QUERY_LIBVERSION        (TAG_USER + 0x2010)     /* version of a named library (see autodoc)  */
#define MATCH_LIBVERSION        (TAG_USER + 0x2011)
#define QUERY_CHIPREVBITS       (TAG_USER + 0x2012)     /* (ULONG)                                   */
#define MATCH_CHIPREVBITS       (TAG_USER + 0x2013)
#define QUERY_MAXFASTMEM        (TAG_USER + 0x2014)     /* (ULONG)                                   */
#define MATCH_MAXFASTMEM        (TAG_USER + 0x2015)
#define QUERY_AVAILFASTMEM      (TAG_USER + 0x2016)     /* (ULONG)                                   */
#define MATCH_AVAILFASTMEM      (TAG_USER + 0x2017)
#define QUERY_MAXCHIPMEM        (TAG_USER + 0x2018)     /* (ULONG)                                   */
#define MATCH_MAXCHIPMEM        (TAG_USER + 0x2019)
#define QUERY_AVAILCHIPMEM      (TAG_USER + 0x2020)     /* (ULONG)                                   */
#define MATCH_AVAILCHIPMEM      (TAG_USER + 0x2021)
#define QUERY_KICKVERSION       (TAG_USER + 0x2022)     /* (ULONG) version<<16 | revision            */
#define MATCH_KICKVERSION       (TAG_USER + 0x2023)
#define QUERY_WBVERSION         (TAG_USER + 0x2024)     /* (ULONG) version<<16 | revision            */
#define MATCH_WBVERSION         (TAG_USER + 0x2025)
#define QUERY_NIPCVERSION       (TAG_USER + 0x2026)     /* (ULONG) version<<16 | revision            */
#define MATCH_NIPCVERSION       (TAG_USER + 0x2027)

/* Timer events; the structure is private */
struct NIPCEvent;

#endif /* ENVOY_NIPC_H */
