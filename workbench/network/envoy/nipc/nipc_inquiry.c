/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: nipc.library - network inquiry on UDP port 376
          (re/spec/nipc-resolver-inquiry-config.md §1-§3): requests,
          Realm Server transit handling, replies, and the host-name
          lookup used by FindEntity().
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <string.h>

#include <proto/nipc.h>

#include "nipc_intern.h"

#define ENTITY(n)               ((struct Entity *)((IPTR)(n) - offsetof(struct Entity, Node)))
#define TAGVAL(t)               ((ULONG)((t) & 0x7FFFFFFF))

static ULONG RandomSeed = 0x1D872B41;

static ULONG Random(struct NIPCBase *NIPCBase)
{
    struct timeval tv;
    GetNow(NIPCBase, &tv);
    RandomSeed = RandomSeed * 1103515245UL + 12345UL + tv.tv_micro;
    return RandomSeed >> 8;
}

BOOL RealmServerInEffect(struct NIPCBase *NIPCBase)
{
    return NIPCBase->Config.UseRealmServer && NIPCBase->Config.RealmServer != 0;
}

static BOOL IsStringTag(ULONG tag)
{
    switch (tag)
    {
    case MATCH_REALM: case MATCH_HOSTNAME: case MATCH_SERVICE: case MATCH_ENTITY: case MATCH_OWNER:
    case QUERY_LIBVERSION:
        return TRUE;
    }
    return FALSE;
}

/* reply items whose ti_Data is a number, not a string offset */
static BOOL IsNumericTag(ULONG tag)
{
    switch (tag)
    {
    case QUERY_IPADDR: case MATCH_IPADDR: case QUERY_ATTNFLAGS: case MATCH_ATTNFLAGS:
    case QUERY_CHIPREVBITS: case MATCH_CHIPREVBITS: case QUERY_MAXFASTMEM: case MATCH_MAXFASTMEM:
    case QUERY_AVAILFASTMEM: case MATCH_AVAILFASTMEM: case QUERY_MAXCHIPMEM: case MATCH_MAXCHIPMEM:
    case QUERY_AVAILCHIPMEM: case MATCH_AVAILCHIPMEM: case QUERY_KICKVERSION: case MATCH_KICKVERSION:
    case QUERY_WBVERSION: case MATCH_WBVERSION: case QUERY_NIPCVERSION: case MATCH_NIPCVERSION:
        return TRUE;
    }
    return FALSE;
}

/* ---- packet builder ---------------------------------------------------- */

struct PktBuilder
{
    UBYTE   *Data;
    ULONG   Size;       /* allocated */
    ULONG   Items;      /* items written (excluding terminator) */
    ULONG   StrEnd;     /* end of string area, relative to the string base */
    UBYTE   *Strings;   /* temporary string area */
    ULONG   StrSize;
};

static BOOL PbInit(struct PktBuilder *pb, ULONG maxitems, ULONG maxstr)
{
    memset(pb, 0, sizeof(*pb));
    pb->Size = INQ_HDRLEN + (maxitems + 1) * 8;
    if (!(pb->Data = AllocVec(pb->Size + maxstr, MEMF_CLEAR | MEMF_PUBLIC)))
        return FALSE;
    pb->Strings = pb->Data + pb->Size;
    pb->StrSize = maxstr;
    return TRUE;
}

/* items are written at their final position; strings are collected and
 * appended when the packet is finished, so string offsets are known then */
static BOOL PbItem(struct PktBuilder *pb, ULONG tag, ULONG data)
{
    UBYTE *p = pb->Data + INQ_HDRLEN + pb->Items * 8;
    if (INQ_HDRLEN + (pb->Items + 1) * 8 >= pb->Size)
        return FALSE;
    nipc_put32(p, tag);
    nipc_put32(p + 4, data);
    pb->Items++;
    return TRUE;
}

/* a string item: ti_Data = offset into the string area for now */
static BOOL PbStringItem(struct PktBuilder *pb, ULONG tag, CONST_STRPTR s, const UBYTE *prefix, ULONG prefixlen)
{
    ULONG len = strlen(s) + 1;
    if (pb->StrEnd + prefixlen + len > pb->StrSize)
        return FALSE;
    if (!PbItem(pb, tag, 0x80000000 | pb->StrEnd))
        return FALSE;
    if (prefixlen)
        CopyMem((APTR)prefix, pb->Strings + pb->StrEnd, prefixlen);
    CopyMem((APTR)s, pb->Strings + pb->StrEnd + prefixlen, len);
    pb->StrEnd += prefixlen + len;
    return TRUE;
}

/* finish: header, terminator, strings; returns the packet length */
static ULONG PbFinish(struct PktBuilder *pb, UWORD queryid, UWORD type, ULONG originator)
{
    ULONG strbase = INQ_HDRLEN + (pb->Items + 1) * 8, i, len;
    UBYTE *p;

    for (i = 0; i < pb->Items; i++)
    {
        p = pb->Data + INQ_HDRLEN + i * 8;
        if (nipc_get32(p + 4) & 0x80000000 && !IsNumericTag(nipc_get32(p)))
            nipc_put32(p + 4, strbase + (nipc_get32(p + 4) & 0x7FFFFFFF));
    }
    p = pb->Data + INQ_HDRLEN + pb->Items * 8;
    nipc_put32(p, 0);
    nipc_put32(p + 4, 0);
    memmove(pb->Data + strbase, pb->Strings, pb->StrEnd);
    len = strbase + pb->StrEnd;
    nipc_put32(pb->Data, len);
    nipc_put32(pb->Data + 4, INQ_HDRLEN);
    nipc_put32(pb->Data + 8, originator);
    nipc_put16(pb->Data + 12, queryid);
    nipc_put16(pb->Data + 14, type);
    return len;
}

/* ---- parsing a received packet ----------------------------------------- */

struct PktItem
{
    ULONG   Tag;
    ULONG   Data;       /* number, or string offset */
};

/* string at offset off, bounded by the packet */
static CONST_STRPTR PktString(const UBYTE *pkt, ULONG len, ULONG off)
{
    static const char empty[] = "";
    ULONG i;
    if (off >= len)
        return empty;
    for (i = off; i < len; i++)
        if (!pkt[i])
            return (CONST_STRPTR)pkt + off;
    return empty;
}

static LONG PktItems(const UBYTE *pkt, ULONG len, struct PktItem *items, LONG max)
{
    ULONG off = nipc_get32(pkt + 4);
    LONG n = 0;

    while (off + 8 <= len && n < max)
    {
        items[n].Tag = nipc_get32(pkt + off);
        items[n].Data = nipc_get32(pkt + off + 4);
        if (items[n].Tag == 0)
            break;
        if (items[n].Tag == TAG_IGNORE || items[n].Tag == TAG_SKIP || items[n].Tag == TAG_MORE)
        {
            off += 8;
            continue;
        }
        n++;
        off += 8;
    }
    return n;
}

/* ---- our own queries --------------------------------------------------- */

static ULONG RequestDest(struct NIPCBase *NIPCBase, UWORD type)
{
    return type == INQ_TYPE_REQUEST ? 0xFFFFFFFF : NIPCBase->Config.RealmServer;
}

struct Inquiry *InquiryStart(struct NIPCBase *NIPCBase, struct TagItem *tags, ULONG maxtime, ULONG maxresp,
                             struct Hook *hook, struct Task *caller, struct SuperReq *req)
{
    struct Inquiry *q;
    struct PktBuilder pb;
    struct TagItem *tag, *tstate = tags;
    BOOL hasrealm = FALSE, hasrealmlist = FALSE;
    UWORD type;
    ULONG n = 0, strbytes = 0;

    /* count and validate */
    tstate = tags;
    while ((tag = NextTagItem(&tstate)))
    {
        n++;
        if (tag->ti_Tag == MATCH_REALM) hasrealm = TRUE;
        if (tag->ti_Tag == QUERY_REALMS) hasrealmlist = TRUE;
        if (IsStringTag(tag->ti_Tag) && tag->ti_Data)
            strbytes += strlen((STRPTR)tag->ti_Data) + 1;
        if (tag->ti_Tag == MATCH_LIBVERSION && tag->ti_Data)
            strbytes += 4 + strlen((STRPTR)tag->ti_Data + 4) + 1;
    }
    if ((hasrealm || hasrealmlist) && !RealmServerInEffect(NIPCBase))
        return NULL;
    type = RealmServerInEffect(NIPCBase) ? INQ_TYPE_TRANSIT : INQ_TYPE_REQUEST;
    if (hasrealmlist)
        type = INQ_TYPE_REALMLIST;

    if (!(q = AllocVec(sizeof(struct Inquiry), MEMF_CLEAR | MEMF_PUBLIC)))
        return NULL;
    if (!PbInit(&pb, n + 1, strbytes + 80))
    {
        FreeVec(q);
        return NULL;
    }
    tstate = tags;
    while ((tag = NextTagItem(&tstate)))
    {
        if (IsStringTag(tag->ti_Tag))
            PbStringItem(&pb, tag->ti_Tag, tag->ti_Data ? (CONST_STRPTR)tag->ti_Data : (CONST_STRPTR)"", NULL, 0);
        else if (tag->ti_Tag == MATCH_LIBVERSION && tag->ti_Data)
            PbStringItem(&pb, tag->ti_Tag, (CONST_STRPTR)tag->ti_Data + 4, (UBYTE *)tag->ti_Data, 4);
        else
            PbItem(&pb, tag->ti_Tag, tag->ti_Data);
    }
    if (type == INQ_TYPE_TRANSIT && !hasrealm && !hasrealmlist)
        PbStringItem(&pb, MATCH_REALM, NIPCBase->Config.RealmName, NULL, 0);

    q->QueryID = NIPCBase->NextQueryID++;
    if (!NIPCBase->NextQueryID)
        NIPCBase->NextQueryID = 1;
    q->PacketLen = PbFinish(&pb, q->QueryID, type, 0);
    q->Packet = pb.Data;
    q->Type = type;
    q->Hook = hook;
    q->Caller = caller;
    q->Req = req;
    q->Remaining = maxtime + 1;
    q->Responses = maxresp > 65535 ? 65535 : maxresp;
    AddTail((struct List *)&NIPCBase->Inquiries, (struct Node *)q);
    NetSendUdp(NIPCBase, RequestDest(NIPCBase, type), q->Packet, q->PacketLen);
    return q;
}

static void FinishInquiry(struct NIPCBase *NIPCBase, struct Inquiry *q, BOOL callhook)
{
    Remove((struct Node *)q);
    if (q->Done)
        q->Done(NIPCBase, q, q->ResultIP);
    else if (q->Hook && callhook)
        CallHookPkt(q->Hook, q->Caller, NULL);
    if (q->Req)
        ReplySuperReq(NIPCBase, q->Req);
    FreeVec(q->Packet);
    FreeVec(q);
}

void InquiryCancel(struct NIPCBase *NIPCBase, struct Inquiry *q)
{
    struct Inquiry *n;
    ForeachNode(&NIPCBase->Inquiries, n)
    {
        if (n == q)
        {
            q->Done = NULL;
            q->Hook = NULL;
            FinishInquiry(NIPCBase, q, FALSE);
            return;
        }
    }
}

void InquiryHandleReq(struct NIPCBase *NIPCBase, struct SuperReq *req)
{
    if (req->Type == SREQ_INQUIRYABORT)
    {
        struct Inquiry *q;
        req->Error = 1;
        ForeachNode(&NIPCBase->Inquiries, q)
        {
            if (q->Hook == req->Hook)
            {
                FinishInquiry(NIPCBase, q, TRUE);
                req->Error = 0;
                break;
            }
        }
    }
    else
    {
        struct Inquiry *q = InquiryStart(NIPCBase, req->Tags, req->MaxTime, req->MaxResponses, req->Hook, req->Caller, NULL);
        req->Error = q ? 0 : 1;
    }
    ReplySuperReq(NIPCBase, req);
}

/* A reply for one of our queries */
static void HandleReply(struct NIPCBase *NIPCBase, UBYTE *pkt, ULONG len)
{
    UWORD id = nipc_get16(pkt + 12);
    struct Inquiry *q;
    struct TagItem *tl;
    struct PktItem items[64];
    LONG n, i;
    IPTR r;

    ForeachNode(&NIPCBase->Inquiries, q)
        if (q->QueryID == id)
            break;
    if (!q || (struct MinNode *)q == (struct MinNode *)&NIPCBase->Inquiries.mlh_Tail)
        return;
    /* ForeachNode leaves q at the tail sentinel when nothing matched */
    if (q->QueryID != id)
        return;

    n = PktItems(pkt, len, items, 63);
    if (q->Done)
    {
        for (i = 0; i < n; i++)
        {
            if (items[i].Tag == QUERY_IPADDR)
            {
                q->ResultIP = items[i].Data;
                break;
            }
        }
        FinishInquiry(NIPCBase, q, FALSE);
        return;
    }
    if (!q->Hook)
        return;
    if (!(tl = AllocVec((n + 1) * sizeof(struct TagItem), MEMF_CLEAR | MEMF_PUBLIC)))
        return;
    for (i = 0; i < n; i++)
    {
        tl[i].ti_Tag = items[i].Tag;
        if (IsNumericTag(items[i].Tag))
            tl[i].ti_Data = items[i].Data;
        else
            tl[i].ti_Data = (IPTR)PktString(pkt, len, items[i].Data);
    }
    tl[n].ti_Tag = TAG_DONE;
    r = CallHookPkt(q->Hook, q->Caller, tl);
    FreeVec(tl);
    if (q->Responses)
        q->Responses--;
    if (!r)
        FinishInquiry(NIPCBase, q, FALSE);
    else if (q->Responses == 0)
        FinishInquiry(NIPCBase, q, TRUE);
}

/* ---- answering --------------------------------------------------------- */

static ULONG LibVersion(struct NIPCBase *NIPCBase, CONST_STRPTR name)
{
    struct Library *lib = OpenLibrary(name, 0);
    ULONG v = 0;
    if (lib)
    {
        v = ((ULONG)lib->lib_Version << 16) | lib->lib_Revision;
        CloseLibrary(lib);
    }
    return v;
}

static BOOL MatchItem(struct NIPCBase *NIPCBase, struct PktItem *it, const UBYTE *pkt, ULONG len, ULONG originator)
{
    CONST_STRPTR s;

    switch (it->Tag)
    {
    case MATCH_HOSTNAME:
        return !Stricmp(PktString(pkt, len, it->Data), NIPCBase->Config.HostName);
    case MATCH_REALM:
        s = RealmServerInEffect(NIPCBase) ? (CONST_STRPTR)NIPCBase->Config.RealmName : (CONST_STRPTR)"";
        return !Stricmp(PktString(pkt, len, it->Data), s);
    case MATCH_OWNER:
        return !Stricmp(PktString(pkt, len, it->Data), NIPCBase->Config.Owner);
    case MATCH_IPADDR:
        return NetIsLocalAddress(NIPCBase, it->Data);
    case MATCH_SERVICE:
        return FALSE;                               /* services.library: later */
    case MATCH_ENTITY:
    {
        BOOL found;
        ObtainSemaphore(&NIPCBase->Sem);
        found = FindPublicEntity(NIPCBase, PktString(pkt, len, it->Data)) != NULL;
        ReleaseSemaphore(&NIPCBase->Sem);
        return found;
    }
    case MATCH_ATTNFLAGS:
        return (it->Data & SysBase->AttnFlags) != 0;
    case MATCH_CHIPREVBITS:
        return FALSE;
    case MATCH_MAXFASTMEM:
        return AvailMem(MEMF_FAST | MEMF_TOTAL) >= it->Data;
    case MATCH_AVAILFASTMEM:
        return AvailMem(MEMF_FAST) >= it->Data;
    case MATCH_MAXCHIPMEM:
        return AvailMem(MEMF_CHIP | MEMF_TOTAL) >= it->Data;
    case MATCH_AVAILCHIPMEM:
        return AvailMem(MEMF_CHIP) >= it->Data;
    case MATCH_KICKVERSION:
        return (((ULONG)SysBase->LibNode.lib_Version << 16) | SysBase->SoftVer) >= it->Data;
    case MATCH_WBVERSION:
        return LibVersion(NIPCBase, "version.library") >= it->Data;
    case MATCH_NIPCVERSION:
        return (((ULONG)NIPC_VERSION << 16) | NIPC_REVISION) >= it->Data;
    case MATCH_LIBVERSION:
        if (it->Data + 4 < len)
            return LibVersion(NIPCBase, PktString(pkt, len, it->Data + 4)) >= nipc_get32(pkt + it->Data);
        return FALSE;
    }
    return TRUE;                                    /* unknown and MATCH_MACHDESC: pass */
}

static void BuildReply(struct NIPCBase *NIPCBase, const UBYTE *pkt, ULONG len, struct PktItem *items, LONG n, ULONG originator)
{
    struct PktBuilder pb;
    struct InqReply *rep;
    LONG i;
    char self[NIPC_HOSTSIZE];
    ULONG entities = 0, realms = 0;
    struct MinNode *en;
    struct Realm *r;

    ObtainSemaphore(&NIPCBase->Sem);
    ForeachNode(&NIPCBase->Entities, en)
        if ((ENTITY(en)->Flags & (ENTF_PUBLIC | ENTF_LINK | ENTF_DELETED)) == ENTF_PUBLIC) entities++;
    ReleaseSemaphore(&NIPCBase->Sem);
    ForeachNode(&NIPCBase->Realms, r) realms++;

    if (!PbInit(&pb, n + entities + realms + 4, len + entities * NIPC_NAMESIZE + realms * 64 + 512))
        return;
    LocalHostName(NIPCBase, self, sizeof(self));

    for (i = 0; i < n; i++)
    {
        struct PktItem *it = &items[i];
        switch (it->Tag)
        {
        case MATCH_REALM: case MATCH_HOSTNAME: case MATCH_SERVICE: case MATCH_ENTITY: case MATCH_OWNER:
            PbStringItem(&pb, it->Tag, PktString(pkt, len, it->Data), NULL, 0);
            break;
        case MATCH_LIBVERSION:
            if (it->Data + 4 < len)
                PbStringItem(&pb, it->Tag, PktString(pkt, len, it->Data + 4), pkt + it->Data, 4);
            break;
        case QUERY_IPADDR:
        {
            struct Iface *ifa = NetIfaceFor(NIPCBase, originator);
            PbItem(&pb, it->Tag, ifa ? ifa->Address : 0);
            break;
        }
        case QUERY_HOSTNAME:
            PbStringItem(&pb, it->Tag, self, NULL, 0);
            break;
        case QUERY_OWNER:
            PbStringItem(&pb, it->Tag, NIPCBase->Config.Owner, NULL, 0);
            break;
        case QUERY_REALMS:
            ForeachNode(&NIPCBase->Realms, r)
                PbStringItem(&pb, it->Tag, r->Name, NULL, 0);
            break;
        case QUERY_SERVICE:
            break;                                  /* services.library: later */
        case QUERY_ENTITY:
            ObtainSemaphore(&NIPCBase->Sem);
            ForeachNode(&NIPCBase->Entities, en)
            {
                struct Entity *e = ENTITY(en);
                if ((e->Flags & (ENTF_PUBLIC | ENTF_LINK | ENTF_DELETED)) == ENTF_PUBLIC)
                    PbStringItem(&pb, it->Tag, e->Name, NULL, 0);
            }
            ReleaseSemaphore(&NIPCBase->Sem);
            break;
        case QUERY_ATTNFLAGS:
            PbItem(&pb, it->Tag, SysBase->AttnFlags);
            break;
        case QUERY_CHIPREVBITS:
            PbItem(&pb, it->Tag, 0);
            break;
        case QUERY_MAXFASTMEM:
            PbItem(&pb, it->Tag, AvailMem(MEMF_FAST | MEMF_TOTAL));
            break;
        case QUERY_AVAILFASTMEM:
            PbItem(&pb, it->Tag, AvailMem(MEMF_FAST));
            break;
        case QUERY_MAXCHIPMEM:
            PbItem(&pb, it->Tag, AvailMem(MEMF_CHIP | MEMF_TOTAL));
            break;
        case QUERY_AVAILCHIPMEM:
            PbItem(&pb, it->Tag, AvailMem(MEMF_CHIP));
            break;
        case QUERY_KICKVERSION:
            PbItem(&pb, it->Tag, ((ULONG)SysBase->LibNode.lib_Version << 16) | SysBase->SoftVer);
            break;
        case QUERY_WBVERSION:
            PbItem(&pb, it->Tag, LibVersion(NIPCBase, "version.library"));
            break;
        case QUERY_NIPCVERSION:
            PbItem(&pb, it->Tag, ((ULONG)NIPC_VERSION << 16) | NIPC_REVISION);
            break;
        case QUERY_LIBVERSION:
        {
            UBYTE ver[4];
            CONST_STRPTR name = PktString(pkt, len, it->Data);
            nipc_put32(ver, LibVersion(NIPCBase, name));
            PbStringItem(&pb, it->Tag, name, ver, 4);
            break;
        }
        default:
            /* numeric MATCH_ tags are echoed; anything else gets a meaningless offset */
            PbItem(&pb, it->Tag, IsNumericTag(it->Tag) ? it->Data : 0);
            break;
        }
    }
    {
        ULONG rlen = PbFinish(&pb, nipc_get16(pkt + 12), INQ_TYPE_REPLY, 0);
        if ((rep = AllocVec(sizeof(struct InqReply) + rlen, MEMF_PUBLIC)))
        {
            rep->DestIP = originator;
            rep->DelayTicks = Random(NIPCBase) % 5;        /* 0 .. 0.4 s */
            rep->Len = rlen;
            CopyMem(pb.Data, rep->Data, rlen);
            AddTail((struct List *)&NIPCBase->InqReplies, (struct Node *)rep);
        }
    }
    FreeVec(pb.Data);
}

static void HandleRequest(struct NIPCBase *NIPCBase, UBYTE *pkt, ULONG len, ULONG srcip)
{
    struct PktItem items[64];
    LONG n, i;
    ULONG originator = nipc_get32(pkt + 8);

    if (!originator)
        originator = srcip;
    n = PktItems(pkt, len, items, 64);
    for (i = 0; i < n; i++)
    {
        if ((items[i].Tag & 1) && items[i].Tag >= MATCH_IPADDR && items[i].Tag <= MATCH_NIPCVERSION)
        {
            if (!MatchItem(NIPCBase, &items[i], pkt, len, originator))
                return;
        }
        if (items[i].Tag == QUERY_IPADDR && !NetIfaceFor(NIPCBase, originator))
            return;
    }
    BuildReply(NIPCBase, pkt, len, items, n, originator);
}

static void HandleTransit(struct NIPCBase *NIPCBase, UBYTE *pkt, ULONG len, ULONG srcip)
{
    struct PktItem items[64];
    LONG n, i;
    CONST_STRPTR realm = NULL;
    struct Realm *r;
    BOOL any = FALSE;

    if (nipc_get32(pkt + 8) == 0 && (srcip >> 24) != 127)
        nipc_put32(pkt + 8, srcip);
    if (!NIPCBase->Config.IsRealmServer)
    {
        nipc_put16(pkt + 14, INQ_TYPE_REQUEST);
        HandleRequest(NIPCBase, pkt, len, srcip);
        return;
    }
    n = PktItems(pkt, len, items, 64);
    for (i = 0; i < n; i++)
        if (items[i].Tag == MATCH_REALM) { realm = PktString(pkt, len, items[i].Data); break; }
    if (!realm)
        return;
    ForeachNode(&NIPCBase->Realms, r)
    {
        if (Stricmp(r->Name, realm))
            continue;
        if (r->Local)
        {
            nipc_put16(pkt + 14, INQ_TYPE_REQUEST);
            NetSendUdp(NIPCBase, r->Address, pkt, len);
            any = TRUE;
            /* the stack loops our own directed broadcast back to us, so we
             * answer it like any other member of the realm */
        }
        else if (r->Address != srcip)
        {
            nipc_put16(pkt + 14, INQ_TYPE_TRANSIT);
            NetSendUdp(NIPCBase, r->Address, pkt, len);
        }
    }
    (void)any;
}

void InquiryInput(struct NIPCBase *NIPCBase, ULONG srcip, UBYTE *data, ULONG len)
{
    ULONG plen;
    UWORD type;

    if (len < INQ_HDRLEN + 8)
        return;
    if ((srcip >> 24) == 127 && !NetIsLocalAddress(NIPCBase, srcip))
        return;
    plen = nipc_get32(data);
    if (plen > len)
        plen = len;
    type = nipc_get16(data + 14);
    switch (type)
    {
    case INQ_TYPE_REQUEST:
    case INQ_TYPE_REALMLIST:
        HandleRequest(NIPCBase, data, plen, srcip);
        break;
    case INQ_TYPE_TRANSIT:
        HandleTransit(NIPCBase, data, plen, srcip);
        break;
    case INQ_TYPE_REPLY:
        HandleReply(NIPCBase, data, plen);
        break;
    }
}

/* every 0.1 s: delayed replies */
void InquiryTick(struct NIPCBase *NIPCBase)
{
    struct InqReply *rep, *next;

    ForeachNodeSafe(&NIPCBase->InqReplies, rep, next)
    {
        if (rep->DelayTicks)
        {
            rep->DelayTicks--;
            continue;
        }
        Remove((struct Node *)rep);
        NetSendUdp(NIPCBase, rep->DestIP, rep->Data, rep->Len);
        FreeVec(rep);
    }
}

/* every second: retransmit or finish our queries */
void InquiryHeartbeat(struct NIPCBase *NIPCBase)
{
    struct Inquiry *q, *next;

    ForeachNodeSafe(&NIPCBase->Inquiries, q, next)
    {
        if (q->Remaining && --q->Remaining == 0)
            FinishInquiry(NIPCBase, q, TRUE);
        else
            NetSendUdp(NIPCBase, RequestDest(NIPCBase, q->Type), q->Packet, q->PacketLen);
    }
}

void InquiryShutdown(struct NIPCBase *NIPCBase)
{
    struct Inquiry *q;
    struct InqReply *rep;

    while ((q = (struct Inquiry *)GetHead(&NIPCBase->Inquiries)))
        FinishInquiry(NIPCBase, q, TRUE);
    while ((rep = (struct InqReply *)RemHead((struct List *)&NIPCBase->InqReplies)))
        FreeVec(rep);
}
