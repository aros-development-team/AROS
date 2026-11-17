/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * mdns_pub.c - a name's publications, one per link
 *
 * An owner (the host, a service) keeps its name and data once; where the
 * name is in probing and announcing is kept per link it is published on,
 * and on each link per address family (mdnsd.h, struct MDNSPub). These are
 * the pieces every owner uses: finding, adding and removing a link's
 * publication; a family's round - three probes, two announcements, and two
 * more when its data change. What goes out at each step is the owner's
 * (mdns_host.c).
 */

#include <exec/types.h>
#include <string.h>
#include <proto/exec.h>
#include <sys/socket.h>

#include "mdnsd.h"

UWORD
mdnsFamOf(UWORD af)
{
    return af == AF_INET6 ? MDNS_FAM_IPV6 : MDNS_FAM_IPV4;
}

UWORD
mdnsDnssdFamily(UWORD fam)
{
    return fam == MDNS_FAM_IPV6 ? DNSSD_ADDRESS_IPV6 : DNSSD_ADDRESS_IPV4;
}

UWORD
mdnsFamOfDnssd(UWORD family)
{
    return family == DNSSD_ADDRESS_IPV6 ? MDNS_FAM_IPV6 : MDNS_FAM_IPV4;
}

/*
 * An owner's publications are an array, one per link, found by the link -
 * the first field of each. These work on an array of any such element, by
 * its size.
 */
static APTR
pubs_find(APTR pubs, UWORD count, ULONG size, ULONG link)
{
    UBYTE *p = pubs;
    UWORD i;

    for (i = 0; i < count; i++, p += size) {
        if (((struct MDNSPub *)p)->p_Link == link) {
            return p;
        }
    }
    return NULL;
}

/* The link's publication, a new one (both families off) if it had none:
 * at the end of the array, made bigger (AllocVec(), *pubs and *max) when
 * it is full; NULL with no memory. */
static APTR
pubs_add(APTR *pubs, UWORD *count, UWORD *max, ULONG size, ULONG link)
{
    UBYTE *p = pubs_find(*pubs, *count, size, link);

    if (p != NULL) {
        return p;
    }
    if (*count >= *max) {
        UWORD more = *max == 0 ? 1 : (UWORD)(*max * 2);
        UBYTE *bigger = AllocVec((ULONG)more * size, MEMF_ANY);

        if (bigger == NULL) {
            return NULL;
        }
        if (*pubs != NULL) {
            memcpy(bigger, *pubs, (ULONG)*count * size);
            FreeVec(*pubs);
        }
        *pubs = bigger;
        *max = more;
    }
    p = (UBYTE *)*pubs + (ULONG)(*count)++ * size;
    memset(p, 0, size);
    ((struct MDNSPub *)p)->p_Link = link;
    return p;
}

/* The link's publication goes; the last takes its place. */
static void
pubs_remove(APTR pubs, UWORD *count, ULONG size, ULONG link)
{
    UBYTE *p = pubs_find(pubs, *count, size, link);

    if (p != NULL) {
        const UBYTE *last = (const UBYTE *)pubs + (ULONG)--(*count) * size;

        if (p != last) {
            memcpy(p, last, size);
        }
    }
}

struct MDNSPub *
mdnsPubFind(struct MDNSPub *pubs, UWORD count, ULONG link)
{
    return pubs_find(pubs, count, sizeof(*pubs), link);
}

/* In an array that grows (AllocVec()) as links come; the owner frees it. */
struct MDNSPub *
mdnsPubAdd(struct MDNSPub **pubs, UWORD *count, UWORD *max, ULONG link)
{
    return pubs_add((APTR *)pubs, count, max, sizeof(**pubs), link);
}

void
mdnsPubRemove(struct MDNSPub *pubs, UWORD *count, ULONG link)
{
    pubs_remove(pubs, count, sizeof(*pubs), link);
}

/* A family's round from the first probe, due at next (mdnsTicks()). */
void
mdnsFamilyStart(struct MDNSFamilyPub *fp, ULONG next)
{
    fp->fp_State = MDNS_PUB_PROBING;
    fp->fp_Step = 0;
    fp->fp_Updates = 0;
    fp->fp_Next = next;
}

/* Is the name the owner's by this family on this link, so that it answers
 * for it there? From its first announcement on. */
BOOL
mdnsFamilyAnswers(const struct MDNSFamilyPub *fp)
{
    return fp->fp_State == MDNS_PUB_ANNOUNCING ||
        fp->fp_State == MDNS_PUB_ANNOUNCED;
}

/*
 * Moves a family's round along when its next step is due (RFC 6762
 * sections 8.1, 8.3, 8.4): three probes 250 ms apart, then - nobody having
 * objected - two announcements a second apart; and, once announced, the
 * announcements due for changed data (fp_Updates). Returns what the owner
 * sends now (MDNS_STEP_*), and leaves in *soonest when the next step is due
 * (mdnsSooner()).
 */
UWORD
mdnsFamilyStep(struct MDNSFamilyPub *fp, ULONG now, ULONG *soonest)
{
    BOOL due = (LONG)(now - fp->fp_Next) >= 0;
    UWORD step = MDNS_STEP_NONE;

    switch (fp->fp_State) {
    case MDNS_PUB_PROBING:
        if (due && fp->fp_Step < MDNS_PROBE_COUNT) {
            step = fp->fp_Step == 0 ? MDNS_STEP_PROBE_FIRST : MDNS_STEP_PROBE;
            fp->fp_Step++;
            fp->fp_Next = now + MDNS_PROBE_TICKS;
        } else if (due) {
            /* nobody objected: the name is the owner's here */
            fp->fp_State = MDNS_PUB_ANNOUNCING;
            fp->fp_Step = 1;
            fp->fp_Updates = 0;
            fp->fp_Next = now + MDNS_SECOND_TICKS;
            step = MDNS_STEP_ANNOUNCE;
        }
        *soonest = mdnsSooner(*soonest, fp->fp_Next);
        break;
    case MDNS_PUB_ANNOUNCING:
        if (due) {
            fp->fp_Next = now + MDNS_SECOND_TICKS;
            if (++fp->fp_Step >= MDNS_ANNOUNCE_COUNT) {
                fp->fp_State = MDNS_PUB_ANNOUNCED;
            }
            step = MDNS_STEP_ANNOUNCE;
        }
        if (fp->fp_State == MDNS_PUB_ANNOUNCING) {
            *soonest = mdnsSooner(*soonest, fp->fp_Next);
        }
        break;
    case MDNS_PUB_ANNOUNCED:
        if (fp->fp_Updates != 0 && due) {
            fp->fp_Updates--;
            fp->fp_Next = now + MDNS_SECOND_TICKS;
            step = MDNS_STEP_ANNOUNCE;
        }
        if (fp->fp_Updates != 0) {
            *soonest = mdnsSooner(*soonest, fp->fp_Next);
        }
        break;
    }
    return step;
}

/*
 * The name answers on the link when any family does.
 */
BOOL
mdnsPubAnswers(const struct MDNSPub *pub)
{
    return mdnsFamilyAnswers(&pub->p_Family[MDNS_FAM_IPV4]) ||
        mdnsFamilyAnswers(&pub->p_Family[MDNS_FAM_IPV6]);
}

/* The owner's data changed, the name already its own: announced again,
 * twice, by every family that answers on the link (section 8.4). */
void
mdnsPubUpdate(struct MDNSPub *pub, ULONG now)
{
    UWORD fam;

    for (fam = 0; fam < MDNS_FAMS; fam++) {
        struct MDNSFamilyPub *fp = &pub->p_Family[fam];

        if (mdnsFamilyAnswers(fp)) {
            /* one already due keeps its time: a second between two
             * announcements of a record (section 6) */
            if (fp->fp_State == MDNS_PUB_ANNOUNCED && fp->fp_Updates == 0) {
                fp->fp_Next = now;
            }
            fp->fp_Updates = MDNS_ANNOUNCE_COUNT;
        }
    }
}
