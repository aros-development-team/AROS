/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * The registry of the stack's modules (net/netmodule.h) and of what they
 * register with the core: protocol numbers, interface events and address
 * requests, variables and statistics. The modules linked into the stack
 * binary are found through a symbol set, not by name.
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <sys/synch.h>

#include <kern/amiga_includes.h>
#include <aros/debug.h>
#include <proto/dos.h>
#include <dos/var.h>
#include <net/netmodule.h>
#include <bsdsocket/netcontrol.h>        /* NCM_MODULE_NAMELEN */

#if defined(__AROS__) && !defined(NETMODULE)
/*
 * The same line in the log as sys/systm.h's __log() writes, made by one
 * call of the core's net_debuglog() (kern/subr_prf.c) instead of three
 * bug() calls at each of the many messages of this file.
 */
#undef __log
#define __log(A, B, C...) net_debuglog(B , ## C)
#endif

struct netmodule *netmodules = NULL;

static void netiface_withdraw_all(struct netmodule *owner);
static int netmodule_selected(const char *modname);
static int netmodule_out_find(const char *name);
static int netmodule_out_note(const char *name, int namelen, int why);
static int netmodule_out_tell(int slot, struct netmodule *user);

/* why a module is not to be started for another one (netmodule_outtab) */
#define NMO_SWITCH  1       /* its test switch is set */
#define NMO_CONFIG  2       /* PROTOCOLS decides whether it runs, and does not name it */
#define NMO_MISSING 3       /* no library file of its name was found */

/*
 * The modules that are not to be started for another module that needs
 * them (netmodule_take_requires()), by name and with the reason: those a
 * test switch left out when the stack started, and those found out since.
 * Small and fixed - when it is full nothing is remembered, and the
 * question is answered anew each time. Emptied when the stack starts and
 * when it ends.
 */
#define NETMODULE_OUT_MAX   6
#define NETMODULE_OUT_NAME  16
static struct {
    char              no_name[NETMODULE_OUT_NAME];
    char              no_why;       /* NMO_xxx; 0: free */
    struct netmodule *no_told;      /* the last module that was told so in the log;
                                       compared, never followed */
} netmodule_outtab[NETMODULE_OUT_MAX];


struct netmodule *
netmodule_find(const char *name)
{
    struct netmodule *nm;

    for(nm = netmodules; nm; nm = nm->nm_next)
        if(strcmp(nm->nm_name, name) == 0)
            return (nm);
    return (NULL);
}

/*
 * `prefix', the first `namelen' characters of `name' in small letters and
 * `suffix', as one string in `buf'. 0, and `buf' untouched, when it does
 * not fit. The one place the names made from a module's name (its library
 * file, its test switch, its name in MODULES) are put together.
 */
static int
netmodule_mkname(char *buf, int buflen, const char *prefix, const char *name, int namelen,
                 const char *suffix)
{
    int plen = strlen(prefix), slen = strlen(suffix), i;

    if(plen + namelen + slen + 1 > buflen)
        return (0);
    bcopy(prefix, buf, plen);
    buf += plen;
    for(i = 0; i < namelen; i++)
        *buf++ = (name[i] >= 'A' && name[i] <= 'Z') ? name[i] - 'A' + 'a' : name[i];
    bcopy(suffix, buf, slen + 1);
    return (1);
}

/*
 * The library file a module of this name is looked for in when it is not
 * part of the stack binary: <name><kind>, the name in small letters, the
 * kind NETMODULE_KIND_PROT or NETMODULE_KIND_ATCP (net/netmodule.h) - the
 * caller knows which from where the name came. 0 when `buf' is too small.
 */
int
netmodule_libname(const char *name, int namelen, const char *kind, char *buf, int buflen)
{
    return (netmodule_mkname(buf, buflen, "", name, namelen, kind));
}

/*
 * Start what a module needs; each one gains a user. On failure the ones
 * already taken are given back.
 *
 * A needed module that is not known - not part of the stack binary, not
 * loaded - is looked for as a library file, as a module the configuration
 * selects is: a protocol module, <name>.prot. (This reads the disk with the stack fenced when the module
 * that needs it is itself being loaded from a library while the stack
 * runs; whoever minds loads the needed one first. At start-up nobody is
 * kept waiting.)
 *
 * WHAT THE CONFIGURATION LEFT OUT STAYS OUT. Being needed starts a module
 * only when the configuration has no say about it, or said yes:
 *
 *   - a module whose test switch is set is never started for another;
 *   - a module that says of itself that the configuration selects it
 *     (NMF_SELECTABLE: the protocol families) is started for another only
 *     when PROTOCOLS names it;
 *   - any other module (the transports, a third party's module that
 *     another of its modules needs) is started, and loaded, as before.
 *
 * The core has no list of the selectable modules: the flag is in the
 * module's descriptor. For a module in a library file that is not loaded
 * the descriptor cannot be seen, so the file is opened and attached -
 * which starts nothing - the flag read, and a module that turns out to be
 * selectable and not selected is detached and leaves memory again. That
 * is done once: the name is remembered (netmodule_outtab) for as long as
 * the stack runs, and the next request that needs it is refused without
 * the disk being read.
 *
 * The start of the module that needs it then fails with ENXIO ("not
 * configured") and one line in the log that names both. ENXIO and not
 * ENOENT, which stays the answer for a needed module that is not
 * installed; and not EAFNOSUPPORT or ENOPROTOOPT, which an application
 * would read as a verdict on the socket or the request it used - the
 * registry does not know that the module left out is an address family.
 */
static int
netmodule_take_requires(struct netmodule *nm)
{
    struct netmodule *dep;
    int i, error = 0;

    if(nm->nm_requires == NULL)
        return (0);
    for(i = 0; nm->nm_requires[i] != NULL; i++) {
        const char *rq = nm->nm_requires[i];
        int rqlen = strlen(rq);
        int slot = -1, why = 0, fresh = 0;
        char lib[48];

        dep = netmodule_known(rq);
        if(dep == NULL || !(dep->nm_flags & NMF_STARTED)) {
            /* decided before, at start-up or by an earlier request */
            if((slot = netmodule_out_find(rq)) >= 0) {
                why = netmodule_outtab[slot].no_why;
                if(why == NMO_MISSING && dep != NULL) {
                    /* it was loaded by hand since */
                    netmodule_outtab[slot].no_why = 0;
                    slot = -1;
                    why = 0;
                }
            }
            if(why == 0 && netmodule_leftout(rq, rqlen))
                why = NMO_SWITCH;
            if(why == 0 && dep == NULL) {
                /* attached, not started: its descriptor can be read now */
                if(netmodule_libname(rq, rqlen, NETMODULE_KIND_PROT, lib, sizeof(lib)))
                    (void)netmodule_attach(lib, &dep);
                /* the file must hold the module it is named after */
                if(dep != NULL && strcmp(dep->nm_name, rq) != 0) {
                    __log(LOG_ERR, "%s holds module %s, not %s.", lib, dep->nm_name, rq);
                    (void)netmodule_unload(dep);
                    dep = NULL;
                }
                if(dep != NULL)
                    fresh = 1;
                else
                    why = NMO_MISSING;
            }
            if(why == 0 && (dep->nm_flags & NMF_SELECTABLE) && !netmodule_selected(rq))
                why = NMO_CONFIG;
            if(why != 0 && slot < 0)
                slot = netmodule_out_note(rq, rqlen, why);
        }
        if(why != 0) {
            /* one line for each module that asks, not one for each request */
            if(netmodule_out_tell(slot, nm)) {
                if(why == NMO_MISSING)
                    __log(LOG_ERR, "module %s needs module %s, which is not available.",
                          nm->nm_name, rq);
                else {
                    /* "(<variable> is set)" or "(PROTOCOLS=<list>)": one message */
                    char var[48];
                    const char *a = "PROTOCOLS=", *b = netmodule_protocols ? netmodule_protocols
                                                        : NETMODULE_PROTOCOLS_DEFAULT;

                    if(why == NMO_SWITCH) {
                        netmodule_leaveout_name(rq, rqlen, var, sizeof(var));
                        a = var;
                        b = " is set";
                    }
                    __log(LOG_ERR, "module %s needs module %s, which this configuration "
                          "leaves out (%s%s): %s is not started.",
                          nm->nm_name, rq, a, b, nm->nm_name);
                }
            }
            error = (why == NMO_MISSING) ? ENOENT : ENXIO;
        } else if(dep->nm_flags & NMF_STARTING) {
            __log(LOG_ERR, "module %s needs module %s, which needs it in turn.",
                  nm->nm_name, dep->nm_name);
            error = ELOOP;
        } else if(!(dep->nm_flags & NMF_STARTED) && (error = netmodule_register(dep)) != 0) {
            __log(LOG_ERR, "module %s needs module %s, which could not be started (error %ld).",
                  nm->nm_name, dep->nm_name, (long)error);
        } else if(fresh)
            __log(LOG_NOTICE, "module %s loaded from %s (module %s needs it).",
                  dep->nm_name, lib, nm->nm_name);
        if(error != 0) {
            /* what was brought into memory for this and is not used leaves it again */
            if(fresh && dep != NULL && !(dep->nm_flags & (NMF_STARTED | NMF_STARTING)))
                (void)netmodule_unload(dep);
            while(--i >= 0)
                if((dep = netmodule_known(nm->nm_requires[i])) != NULL)
                    dep->nm_refs--;
            return (error);
        }
        dep->nm_refs++;
    }
    return (0);
}

static void
netmodule_give_requires(struct netmodule *nm)
{
    struct netmodule *dep;
    int i;

    if(nm->nm_requires == NULL)
        return;
    for(i = 0; nm->nm_requires[i] != NULL; i++)
        if((dep = netmodule_known(nm->nm_requires[i])) != NULL)
            dep->nm_refs--;
}

/*
 * Register a module and start it, after the modules it needs. On an error
 * it is not left registered.
 */
int
netmodule_register(struct netmodule *nm)
{
    struct netmodule **nmp;
    int error;

    /* (a module built for version 2 lacks nm_shutdown, which is only
     * read where the size says it is there) */
    if(nm == NULL || nm->nm_name == NULL || nm->nm_start == NULL ||
            nm->nm_size < NETMODULE_SIZE_V2 ||
            nm->nm_version < 2 || nm->nm_version > NETMODULE_VERSION)
        return (EINVAL);
    if((nm->nm_flags & NMF_STARTED) || netmodule_find(nm->nm_name) != NULL)
        return (EEXIST);
    if(nm->nm_flags & NMF_STARTING)
        return (ELOOP);

    nm->nm_refs = 0;
    nm->nm_next = NULL;
    nm->nm_flags |= NMF_STARTING;
    if((error = netmodule_take_requires(nm)) == 0 &&
            (error = (*nm->nm_start)(nm)) != 0) {
        netiface_withdraw_all(nm);      /* what a failed start left published */
        netmodule_give_requires(nm);
    }
    nm->nm_flags &= ~NMF_STARTING;
    if(error != 0)
        return (error);
    nm->nm_flags |= NMF_STARTED;

    for(nmp = &netmodules; *nmp; nmp = &(*nmp)->nm_next)
        ;
    *nmp = nm;
    return (0);
}

/*
 * The stack is ending (netmodule_stop_all()): the rule below, which keeps
 * a running stack from being left with nothing a socket could be made of,
 * does not hold any more.
 */
static int netmodule_ending = 0;

/*
 * Stop a module and take it off the registry. Refused, with the reason,
 * while it is in use or when it cannot be stopped at all.
 *
 * THE LAST ONE STAYS. Of the modules the configuration selects
 * (NMF_SELECTABLE: the protocol families) at least one runs for as long as
 * the stack does: start-up refuses to go on without one
 * (netmodule_protocols_started(), kern/amiga_main.c), and a stop that
 * would leave none is refused here with EBUSY, for the same reason - there
 * would be nothing a socket could be made of, the loopback interface would
 * have no address, and the modules that only serve the families (the
 * transports) would be stopped under a stack that still runs. Which
 * modules those are the registry does not know: it counts the flag. The
 * stack's own shutdown is not held to this.
 */
int
netmodule_unregister(struct netmodule *nm)
{
    struct netmodule **nmp, *other;
    int error;

    for(nmp = &netmodules; *nmp; nmp = &(*nmp)->nm_next)
        if(*nmp == nm)
            break;
    if(*nmp == NULL)
        return (ENOENT);
    if((nm->nm_flags & NMF_PERMANENT) || nm->nm_stop == NULL)
        return (EPERM);
    if(netmodule_users(nm) != 0)
        return (EBUSY);
    if((nm->nm_flags & NMF_SELECTABLE) && !netmodule_ending) {
        for(other = netmodules; other != NULL; other = other->nm_next)
            if(other != nm && (other->nm_flags & NMF_SELECTABLE))
                break;
        if(other == NULL) {
            __log(LOG_NOTICE, "module %s is not stopped: it is the last protocol family "
                  "that runs, and a stack without one cannot make a socket. "
                  "Start another first, or stop the stack.", nm->nm_name);
            return (EBUSY);
        }
    }
    if(nm->nm_can_stop && (error = (*nm->nm_can_stop)(nm)) != 0)
        return (error);
    if((error = (*nm->nm_stop)(nm)) != 0)
        return (error);

    *nmp = nm->nm_next;
    nm->nm_next = NULL;
    nm->nm_flags &= ~NMF_STARTED;
    netiface_withdraw_all(nm);  /* nothing that needs it runs: nm_refs was 0 */
    netmodule_give_requires(nm);
    return (0);
}

/*
 * A user of the module that attached the socket domain of `family', taken
 * (delta 1) or given back (delta -1) by ANOTHER module that uses that
 * family without a socket and without naming its module: a tunnel carried
 * by the family (the 6in4 tunnel sends through IPv4's output and holds one
 * of its protocol numbers for as long as a tunnel interface exists). The
 * count is the module's nm_refs, so the family is not stopped under its
 * user and the control port reports the user. ENOENT when no started
 * module has a domain of that family. With the stack fenced.
 */
int
netmodule_hold_family(int family, int delta)
{
    struct netmodule *nm;

    for(nm = netmodules; nm != NULL; nm = nm->nm_next)
        if(nm->nm_domain != NULL && nm->nm_domain->dom_family == family &&
                family != AF_UNSPEC) {
            if(delta < 0 && nm->nm_refs + delta < 0)
                return (EINVAL);
            nm->nm_refs += delta;
            return (0);
        }
    return (ENOENT);
}

/*
 * Named interfaces (net/netmodule.h): a small table, walked when a module
 * starts or stops and never for a call.
 */
static struct netiface *netifaces[NETIFACE_MAX];

int
netiface_publish(struct netmodule *owner, struct netiface *nif)
{
    int i, slot = -1;

    if(owner == NULL || nif == NULL || nif->nif_name == NULL ||
            nif->nif_size < sizeof(struct netiface))
        return (EINVAL);
    for(i = 0; i < NETIFACE_MAX; i++) {
        if(netifaces[i] == NULL) {
            if(slot < 0)
                slot = i;
        } else if(netifaces[i] == nif || strcmp(netifaces[i]->nif_name, nif->nif_name) == 0)
            return (EEXIST);
    }
    if(slot < 0)
        return (ENOSPC);
    nif->nif_owner = owner;
    netifaces[slot] = nif;
    return (0);
}

int
netiface_withdraw(struct netiface *nif)
{
    int i;

    for(i = 0; i < NETIFACE_MAX; i++)
        if(nif != NULL && netifaces[i] == nif) {
            netifaces[i] = NULL;
            nif->nif_owner = NULL;
            return (0);
        }
    return (ENOENT);
}

static void
netiface_withdraw_all(struct netmodule *owner)
{
    int i;

    for(i = 0; i < NETIFACE_MAX; i++)
        if(netifaces[i] != NULL && netifaces[i]->nif_owner == owner) {
            netifaces[i]->nif_owner = NULL;
            netifaces[i] = NULL;
        }
}

/*
 * No count is kept here. The fetcher must name the publisher in its
 * nm_requires: the registry started the publisher before the fetcher's
 * nm_start and holds it (nm_refs, netmodule_take_requires()) until the
 * fetcher has stopped, and netmodule_unregister() refuses to stop a module
 * that is held. So a published table outlives every pointer fetched here.
 */
const struct netiface *
netiface_fetch(struct netmodule *user, const char *name, unsigned int version,
               unsigned long size)
{
    struct netiface *nif = NULL;
    int i;

    if(user == NULL || name == NULL)
        return (NULL);
    for(i = 0; i < NETIFACE_MAX; i++)
        if(netifaces[i] != NULL && strcmp(netifaces[i]->nif_name, name) == 0) {
            nif = netifaces[i];
            break;
        }
    if(nif == NULL) {
        __log(LOG_ERR, "module %s: nothing is published as %s.", user->nm_name, name);
        return (NULL);
    }
    if(nif->nif_version < version || nif->nif_size < size) {
        __log(LOG_ERR, "module %s needs version %ld (%ld bytes) of %s; module %s "
              "publishes version %ld (%ld bytes).", user->nm_name, (long)version,
              (long)size, name, nif->nif_owner->nm_name, (long)nif->nif_version,
              (long)nif->nif_size);
        return (NULL);
    }
    if(nif->nif_owner != user) {
        for(i = 0; user->nm_requires != NULL && user->nm_requires[i] != NULL; i++)
            if(strcmp(user->nm_requires[i], nif->nif_owner->nm_name) == 0)
                break;
        if(user->nm_requires == NULL || user->nm_requires[i] == NULL) {
            __log(LOG_ERR, "module %s uses %s without naming module %s as needed: refused "
                  "(nothing would keep %s from being stopped under it).", user->nm_name,
                  name, nif->nif_owner->nm_name, nif->nif_owner->nm_name);
            return (NULL);
        }
    }
    return (nif);
}

/*
 * Protocol dispatch tables (net/netmodule.h).
 */
int
protodispatch_init(struct protodispatch *pd, struct protosw *deflt)
{
    bzero((caddr_t)pd, sizeof(*pd));
    pd->pd_tab = (struct protosw **)bsd_malloc(8 * sizeof(struct protosw *), M_TEMP, M_NOWAIT);
    if(pd->pd_tab == NULL)
        return (ENOBUFS);
    pd->pd_max = 8;
    pd->pd_tab[0] = deflt;
    pd->pd_n = 1;
    return (0);
}

int
protodispatch_set(struct protodispatch *pd, int proto, struct protosw *pr)
{
    struct protosw **tab;
    int i;

    if(proto <= 0 || proto > 255 || pr == NULL || pd->pd_tab == NULL)
        return (EINVAL);
    if(pd->pd_x[proto] != 0)
        return (pd->pd_tab[pd->pd_x[proto]] == pr ? 0 : EEXIST);

    /* an entry for this protosw already, or a free one */
    for(i = 1; i < pd->pd_n; i++)
        if(pd->pd_tab[i] == pr)
            break;
    if(i == pd->pd_n)
        for(i = 1; i < pd->pd_n; i++)
            if(pd->pd_tab[i] == NULL)
                break;
    if(i == pd->pd_n) {
        if(pd->pd_n == 255)
            return (ENOSPC);
        if(pd->pd_n == pd->pd_max) {
            int max = pd->pd_max > 127 ? 255 : pd->pd_max * 2;
            struct protosw **old = pd->pd_tab;
            spl_t s;

            tab = (struct protosw **)bsd_malloc(max * sizeof(struct protosw *), M_TEMP, M_NOWAIT);
            if(tab == NULL)
                return (ENOBUFS);
            bcopy((caddr_t)old, (caddr_t)tab, pd->pd_n * sizeof(struct protosw *));
            s = splimp();
            pd->pd_tab = tab;
            pd->pd_max = max;
            splx(s);
            bsd_free((caddr_t)old, M_TEMP);
        }
        i = pd->pd_n;
        pd->pd_n++;
    }
    pd->pd_tab[i] = pr;
    pd->pd_x[proto] = i;
    return (0);
}

int
protodispatch_clear(struct protodispatch *pd, int proto, struct protosw *pr)
{
    int i, p;

    if(proto <= 0 || proto > 255 || pd->pd_tab == NULL)
        return (EINVAL);
    if((i = pd->pd_x[proto]) == 0 || pd->pd_tab[i] != pr)
        return (ENOENT);
    pd->pd_x[proto] = 0;
    for(p = 1; p < 256; p++)
        if(pd->pd_x[p] == i)
            return (0);         /* another number still uses the entry */
    pd->pd_tab[i] = NULL;
    return (0);
}

/*
 * The family that owns the table stops: the pointer array goes back to
 * the core's pool and the table is as it was before protodispatch_init().
 * The owner has taken the table back from publication first; a protocol
 * of another module that still had a number is the owner's to have
 * refused the stop for (its user count).
 */
void
protodispatch_done(struct protodispatch *pd)
{
    struct protosw **tab;
    spl_t s = splimp();

    tab = pd->pd_tab;
    bzero((caddr_t)pd, sizeof(*pd));
    splx(s);
    if(tab != NULL)
        bsd_free((caddr_t)tab, M_TEMP);
}

/*
 * The families' dispatch tables, by family (net/netmodule.h): how a
 * protocol of one module takes a protocol number of a family in another.
 */
static struct {
    int                   pf_family;
    struct protodispatch *pf_pd;
} protodispatch_families[PROTODISPATCH_FAMILIES];

int
protodispatch_publish(int family, struct protodispatch *pd)
{
    int i, free = -1;

    if(family == 0)
        return (EINVAL);
    for(i = 0; i < PROTODISPATCH_FAMILIES; i++) {
        if(protodispatch_families[i].pf_pd != NULL &&
                protodispatch_families[i].pf_family == family) {
            if(pd != NULL)
                return (EEXIST);
            protodispatch_families[i].pf_pd = NULL;
            return (0);
        }
        if(protodispatch_families[i].pf_pd == NULL && free < 0)
            free = i;
    }
    if(pd == NULL)
        return (ENOENT);
    if(free < 0)
        return (ENOSPC);
    protodispatch_families[free].pf_family = family;
    protodispatch_families[free].pf_pd = pd;
    return (0);
}

struct protodispatch *
protodispatch_of(int family)
{
    int i;

    for(i = 0; i < PROTODISPATCH_FAMILIES; i++)
        if(protodispatch_families[i].pf_pd != NULL &&
                protodispatch_families[i].pf_family == family)
            return (protodispatch_families[i].pf_pd);
    return (NULL);
}

/*
 * The kinds of tunnel interface the modules make (net/netmodule.h), in the
 * order they were registered in.
 */
static const struct nettunnel *nettunnels[NETTUNNEL_MAX];

int
nettunnel_register(const struct nettunnel *nt)
{
    int i;

    if(nt == NULL || nt->nt_name == NULL || nt->nt_make == NULL)
        return (EINVAL);
    for(i = 0; i < NETTUNNEL_MAX && nettunnels[i] != NULL; i++)
        if(nettunnels[i] == nt || strcmp(nettunnels[i]->nt_name, nt->nt_name) == 0)
            return (EEXIST);
    if(i == NETTUNNEL_MAX)
        return (ENOSPC);
    nettunnels[i] = nt;
    return (0);
}

int
nettunnel_unregister(const struct nettunnel *nt)
{
    int i;

    for(i = 0; i < NETTUNNEL_MAX && nettunnels[i] != nt; i++)
        ;
    if(nt == NULL || i == NETTUNNEL_MAX)
        return (ENOENT);
    for(; i < NETTUNNEL_MAX - 1; i++)
        nettunnels[i] = nettunnels[i + 1];
    nettunnels[NETTUNNEL_MAX - 1] = NULL;
    return (0);
}

void
nettunnel_seen(struct ifnet *ifp, ULONG gen)
{
    int i;

    for(i = 0; i < NETTUNNEL_MAX && nettunnels[i] != NULL; i++)
        if(nettunnels[i]->nt_seen != NULL)
            (*nettunnels[i]->nt_seen)(ifp, gen);
}

void
nettunnel_prune(ULONG gen)
{
    int i;

    /* from the last: a kind's prune cannot unregister another kind, but
     * walking backwards stays right if it unregisters itself */
    for(i = NETTUNNEL_MAX - 1; i >= 0; i--)
        if(nettunnels[i] != NULL && nettunnels[i]->nt_prune != NULL)
            (*nettunnels[i]->nt_prune)(gen);
}

const struct nettunnel *
nettunnel_find(const char *name)
{
    int i;

    for(i = 0; i < NETTUNNEL_MAX && nettunnels[i] != NULL; i++)
        if(name == NULL || strcmp(nettunnels[i]->nt_name, name) == 0)
            return (nettunnels[i]);
    return (NULL);
}

/*
 * Interface events (net/netmodule.h).
 */
static const struct netfamily_ifops *netfamily_ifops[NETFAMILY_IFOPS_MAX];
static int netfamily_ifops_count = 0;

int
netfamily_ifops_register(const struct netfamily_ifops *nfo)
{
    int i;

    if(nfo == NULL || nfo->nfo_event == NULL)
        return (EINVAL);
    for(i = 0; i < netfamily_ifops_count; i++)
        if(netfamily_ifops[i] == nfo)
            return (EEXIST);
    if(netfamily_ifops_count == NETFAMILY_IFOPS_MAX)
        return (ENOSPC);
    for(i = netfamily_ifops_count; i > 0 && netfamily_ifops[i - 1]->nfo_order > nfo->nfo_order; i--)
        netfamily_ifops[i] = netfamily_ifops[i - 1];
    netfamily_ifops[i] = nfo;
    netfamily_ifops_count++;
    return (0);
}

int
netfamily_ifops_unregister(const struct netfamily_ifops *nfo)
{
    int i;

    for(i = 0; i < netfamily_ifops_count; i++)
        if(netfamily_ifops[i] == nfo)
            break;
    if(i == netfamily_ifops_count)
        return (ENOENT);
    for(; i < netfamily_ifops_count - 1; i++)
        netfamily_ifops[i] = netfamily_ifops[i + 1];
    netfamily_ifops_count--;
    return (0);
}

int
netfamily_if_event(int event, struct ifnet *ifp)
{
    int i, n = 0;

    for(i = 0; i < netfamily_ifops_count; i++)
        n += (*netfamily_ifops[i]->nfo_event)(event, ifp);
    return (n);
}

/* A family's address request; EAFNOSUPPORT when the family is not part of the running stack. */
int
netfamily_control(int family, struct socket *so, int cmd, caddr_t data, struct ifnet *ifp)
{
    int i;

    for(i = 0; i < netfamily_ifops_count; i++)
        if(netfamily_ifops[i]->nfo_family == family && netfamily_ifops[i]->nfo_control != NULL)
            return ((*netfamily_ifops[i]->nfo_control)(so, cmd, data, ifp));
    return (EAFNOSUPPORT);
}

/* An event for one family alone (NFE_AUTOADDR). */
int
netfamily_if_event_to(int family, int event, struct ifnet *ifp)
{
    int i;

    for(i = 0; i < netfamily_ifops_count; i++)
        if(netfamily_ifops[i]->nfo_family == family)
            return ((*netfamily_ifops[i]->nfo_event)(event, ifp));
    return (0);
}

/* A question about an address of a family; -1 when nobody answers it. */
int
netfamily_addrquery(int family, int what, const struct sockaddr *sa, struct sockaddr *res)
{
    int i;

    for(i = 0; i < netfamily_ifops_count; i++)
        if(netfamily_ifops[i]->nfo_family == family && netfamily_ifops[i]->nfo_addrquery != NULL)
            return ((*netfamily_ifops[i]->nfo_addrquery)(what, sa, res));
    return (-1);
}

/*
 * The variables and statistics blocks of the modules (net/netmodule.h).
 * Looked up by name when somebody asks for one, never per packet.
 */
static struct {
    const struct netvar *nt_tab;
    int                  nt_count;
} netvar_tables[NETVAR_TABLES_MAX];

extern void config_var_rebind(void);    /* kern/config_var.c */

int
netvar_register(const struct netvar *tab, int count)
{
    int i, free = -1;

    if(tab == NULL || count <= 0)
        return (EINVAL);
    for(i = 0; i < NETVAR_TABLES_MAX; i++) {
        if(netvar_tables[i].nt_tab == tab)
            return (EEXIST);
        if(netvar_tables[i].nt_tab == NULL && free < 0)
            free = i;
    }
    if(free < 0)
        return (ENOSPC);
    netvar_tables[free].nt_count = count;
    netvar_tables[free].nt_tab = tab;
    config_var_rebind();        /* the configuration variables that stand for them */
    return (0);
}

int
netvar_unregister(const struct netvar *tab)
{
    int i;

    for(i = 0; i < NETVAR_TABLES_MAX; i++)
        if(tab != NULL && netvar_tables[i].nt_tab == tab) {
            netvar_tables[i].nt_tab = NULL;
            netvar_tables[i].nt_count = 0;
            config_var_rebind();
            return (0);
        }
    return (ENOENT);
}

const struct netvar *
netvar_find(const char *name)
{
    int i, j;

    for(i = 0; i < NETVAR_TABLES_MAX; i++)
        for(j = 0; j < netvar_tables[i].nt_count; j++)
            if(strcmp(netvar_tables[i].nt_tab[j].nv_name, name) == 0)
                return (&netvar_tables[i].nt_tab[j]);
    return (NULL);
}

const struct netvar *
netvar_find_mib(int family, int proto, int id, int *known)
{
    const struct netvar *nv;
    int i, j;

    *known = 0;
    if(family == 0)
        return (NULL);
    for(i = 0; i < NETVAR_TABLES_MAX; i++)
        for(j = 0; j < netvar_tables[i].nt_count; j++) {
            nv = &netvar_tables[i].nt_tab[j];
            if(nv->nv_family != family || nv->nv_proto != proto)
                continue;
            *known = 1;
            if(nv->nv_id == id)
                return (nv);
        }
    return (NULL);
}

/*
 * Whether an address family is part of the running stack: its socket
 * domain is attached. For the parts of the core that are handed a family
 * by a configuration line or an application and must not offer what the
 * stack cannot do (the interface configuration, getaddrinfo()). The list
 * changes only when a module starts or stops.
 */
int
netfamily_running(int family)
{
    struct domain *dp;

    for(dp = domains; dp; dp = dp->dom_next)
        if(dp->dom_family == family)
            return (1);
    return (0);
}

/*
 * Built-in modules that are not started with the stack. A fixed, small
 * table: a module loaded from disk registers itself and is not listed here.
 */
#define NETMODULE_KNOWN_MAX 8
static struct netmodule *netmodule_knowntab[NETMODULE_KNOWN_MAX];

void
netmodule_know(struct netmodule *nm)
{
    int i;

    for(i = 0; i < NETMODULE_KNOWN_MAX; i++) {
        if(netmodule_knowntab[i] == nm)
            return;
        if(netmodule_knowntab[i] == NULL) {
            nm->nm_flags |= NMF_BUILTIN;
            netmodule_knowntab[i] = nm;
            return;
        }
    }
    __log(LOG_ERR, "module %s: the stack knows %ld modules of its own program at most "
          "(NETMODULE_KNOWN_MAX); it cannot be started.", nm->nm_name, (long)NETMODULE_KNOWN_MAX);
}

/* By name: a started module, or a built-in or loaded one that is not running. */
struct netmodule *
netmodule_known(const char *name)
{
    struct netmodule *nm;
    int i;

    if((nm = netmodule_find(name)) != NULL)
        return (nm);
    for(i = 0; i < NETMODULE_KNOWN_MAX && netmodule_knowntab[i]; i++)
        if(strcmp(netmodule_knowntab[i]->nm_name, name) == 0)
            return (netmodule_knowntab[i]);
    /* one from a library file that is loaded but not running (kern/net_modload.c) */
    return (netmodule_loaded(name));
}

int
netmodule_use(struct netmodule *nm)
{
    if(nm->nm_flags & NMF_STARTED)
        return (0);
    return (netmodule_register(nm));
}

long
netmodule_users(struct netmodule *nm)
{
    return (nm->nm_refs + (nm->nm_domain ? nm->nm_domain->dom_refs : 0));
}

/*
 * The modules linked into the stack binary. Each has its descriptor in its
 * own file and puts it into this set (NETMODULE_BUILTIN()); the core names
 * none of them.
 */
THIS_PROGRAM_HANDLES_SYMBOLSET(NETMODULES)
/*
 * Declared here, defined (empty, weak) in kern/uipc_domain.c: were the
 * two-entry default defined in this file, the compiler would know the
 * array's length and reading past it - the list the linker makes is longer -
 * would be undefined behaviour.
 */
DECLARESET(NETMODULES)
/* ... and for the same reason the demand slots' set, walked in kern/net_demand.c, is defined here */
DEFINESET(NETDEMANDS)

/*
 * (The routing domain is no longer the core's: the administration module
 * attaches it when it is first needed - kern/admin_module.c.)
 */

/*
 * The configuration items PROTOCOLS and MODULES (kern/config_var.c), as
 * they were written; NULL when the configuration has none.
 */
char *netmodule_protocols = NULL;
char *netmodule_modules = NULL;
static int netmodule_nprotocols = 0;    /* of the selected modules, how many were started with the stack */

int
netmodule_protocols_started(void)
{
    return (netmodule_nprotocols);
}

/*
 * The next name in a list "INET,INET6" (commas or blanks between them):
 * where it begins and its length, 0 at the end; *listp is moved past it.
 */
static int
netmodule_nextname(const char **listp, const char **namep)
{
    const char *p = *listp;
    int len = 0;

    while(*p == ',' || *p == ' ' || *p == '\t')
        p++;
    *namep = p;
    while(p[len] != '\0' && p[len] != ',' && p[len] != ' ' && p[len] != '\t')
        len++;
    *listp = p + len;
    return (len);
}

/* the same name, whatever the case of its letters */
static int
netmodule_samename(const char *a, int alen, const char *b)
{
    int i;

    for(i = 0; i < alen; i++) {
        char ca = a[i], cb = b[i];

        if(ca >= 'A' && ca <= 'Z')
            ca += 'a' - 'A';
        if(cb >= 'A' && cb <= 'Z')
            cb += 'a' - 'A';
        if(cb == '\0' || ca != cb)
            return (0);
    }
    return (b[alen] == '\0');
}

static int
netmodule_listed(const char *list, const char *modname)
{
    const char *name;
    int len;

    while((len = netmodule_nextname(&list, &name)) != 0)
        if(netmodule_samename(name, len, modname))
            return (1);
    return (0);
}

/* Whether the configuration - PROTOCOLS, or the default for it - names the module. */
static int
netmodule_selected(const char *modname)
{
    return (netmodule_listed(netmodule_protocols ? netmodule_protocols
                                                 : NETMODULE_PROTOCOLS_DEFAULT, modname));
}

/*
 * The table of the modules that are not to be started for another one
 * (netmodule_outtab, at the head of this file).
 */
static int
netmodule_out_find(const char *name)
{
    int i;

    for(i = 0; i < NETMODULE_OUT_MAX; i++)
        if(netmodule_outtab[i].no_why != 0 &&
                netmodule_samename(name, strlen(name), netmodule_outtab[i].no_name))
            return (i);
    return (-1);
}

/* the slot, or -1 when the name is too long or the table is full */
static int
netmodule_out_note(const char *name, int namelen, int why)
{
    int i;

    if(namelen >= NETMODULE_OUT_NAME)
        return (-1);
    for(i = 0; i < NETMODULE_OUT_MAX; i++)
        if(netmodule_outtab[i].no_why != 0 &&
                netmodule_samename(name, namelen, netmodule_outtab[i].no_name))
            break;
    if(i == NETMODULE_OUT_MAX)
        for(i = 0; i < NETMODULE_OUT_MAX && netmodule_outtab[i].no_why != 0; i++)
            ;
    if(i == NETMODULE_OUT_MAX)
        return (-1);
    bcopy(name, netmodule_outtab[i].no_name, namelen);
    netmodule_outtab[i].no_name[namelen] = '\0';
    netmodule_outtab[i].no_why = why;
    netmodule_outtab[i].no_told = NULL;
    return (i);
}

/* not 0 when `user' is to be told in the log (it was not the last one told) */
static int
netmodule_out_tell(int slot, struct netmodule *user)
{
    if(slot < 0)
        return (1);
    if(netmodule_outtab[slot].no_told == user)
        return (0);
    netmodule_outtab[slot].no_told = user;
    return (1);
}

static void
netmodule_out_forget(void)
{
    bzero((caddr_t)netmodule_outtab, sizeof(netmodule_outtab));
}

/*
 * TEST SWITCH (net/netmodule.h): the variable that keeps a module out,
 * from the module's name. The two families' variables keep the names the
 * test harness has always used; this table is the only place in the core
 * where the names of protocol modules appear, and it selects nothing in
 * a stack nobody set such a variable for.
 */
static const struct {
    const char *lo_module, *lo_var;
} netmodule_leaveouts[] = {
    { "inet",   "AROSTCP/WithoutIPv4" },
    { "inet6",  "AROSTCP/WithoutIPv6" },
};

void
netmodule_leaveout_name(const char *name, int namelen, char *buf, int buflen)
{
    int i;

    for(i = 0; i < (int)(sizeof(netmodule_leaveouts) / sizeof(netmodule_leaveouts[0])); i++)
        if(netmodule_samename(name, namelen, netmodule_leaveouts[i].lo_module)) {
            strncpy(buf, netmodule_leaveouts[i].lo_var, buflen - 1);
            buf[buflen - 1] = '\0';
            return;
        }
    if(!netmodule_mkname(buf, buflen, "AROSTCP/Without_", name, namelen, ""))
        buf[0] = '\0';
}

int
netmodule_leftout(const char *name, int namelen)
{
    char var[48], dummy[4];

    /* asked inside a socket call of a task that is not a process: there is
     * no DOS for it, and what start-up found is remembered by name */
    if(FindTask(NULL)->tc_Node.ln_Type != NT_PROCESS)
        return (0);
    netmodule_leaveout_name(name, namelen, var, sizeof(var));
    return (var[0] != '\0' && GetVar(var, dummy, sizeof(dummy), GVF_GLOBAL_ONLY) != -1);
}

/*
 * A selected module that its test switch `var' keeps out at start-up: not
 * started for another module either, and two lines in the log - "module
 * <name> is not started" for one of the binary, "<file> is not loaded" for
 * one in a library file. The same text for both, put together here.
 */
static void
netmodule_leaveout_log(const char *name, int namelen, const char *subject, int library,
                       const char *var)
{
    (void)netmodule_out_note(name, namelen, NMO_SWITCH);
    __log(LOG_NOTICE, "%s%s is not %s (%s is set).", library ? "" : "module ", subject,
          library ? "loaded" : "started", var);
    if(netmodule_protocols != NULL)
        __log(LOG_NOTICE, "(PROTOCOLS=%s selects %s: the test switch %s wins.)",
              netmodule_protocols, library ? "it" : subject, var);
}

/*
 * Start the protocol modules of the stack. (The routing domain is the
 * administration module's, started on first use: kern/admin_module.c.)
 *
 * WHICH modules is configuration: the item PROTOCOLS of db/general.config
 * (or of a file given with WITH), a list of module names -
 *
 *      PROTOCOLS=INET,INET6        both families (also when the item is absent)
 *      PROTOCOLS=INET              IPv4 only
 *      PROTOCOLS=INET6             IPv6 only
 *
 * The core knows none of these names. Each is looked for first among the
 * modules linked into the stack binary (the symbol set NETMODULES, whose
 * members are all made known by name here, started or not), and otherwise
 * loaded as the library <name>.prot (netmodule_load()). Nothing
 * is started for its own sake that the list does not name: the transports
 * run because a family needs them (nm_requires), and so are found the
 * same way, in the binary or as transport.prot.
 *
 * A library that does not attach, a module whose start fails: an error
 * in the log that names the module, and the stack goes on with what it
 * has. A library file that is not there at all is
 *   - an ERROR naming the file when the configuration names the module
 *     (PROTOCOLS=...): what was asked for cannot be had;
 *   - a NOTICE when the item is absent and the default list stands: a
 *     basic installation need not have every library, and the stack runs
 *     the families that are installed.
 * When none of the selected modules runs, the stack has no protocol
 * family at all and its start-up stops (netmodule_protocols_started(),
 * kern/amiga_main.c).
 *
 * Order: the modules of the binary in the order they give (10 IPv6, 20
 * IPv4) - the order the domains have always been initialised in, and with
 * domain_attach() linking at the end also the order they have always been
 * listed in - whatever the order of the list; then the libraries in the
 * order of the list (the core cannot know more of a library than the name
 * before it is loaded).
 *
 * TEST SWITCH, kept for the test harness (which selects a configuration
 * without editing a file) and NOT the way to configure a stack: a module
 * is not started when the global variable that stands for it exists
 * (netmodule_leaveout_name(): AROSTCP/WithoutIPv6, AROSTCP/WithoutIPv4 -
 * ENV:AROSTCP/..., any contents; a module of the binary may name its own
 * in nm_leaveout). The variable is found from the module's NAME, so a
 * module in a library file is left out before its file is opened; one of
 * the binary stays known by name and can be asked about. When the
 * configuration selects a module and its switch says to leave it out, the
 * switch wins, and the log says so.
 */
void
netmodule_start_builtin(void)
{
    const char *list = netmodule_protocols ? netmodule_protocols
                                           : NETMODULE_PROTOCOLS_DEFAULT;
    const char *rest, *name;
    struct netmodule *nm;
    long pos;
    int len, error;

    netmodule_nprotocols = 0;
    netmodule_out_forget();

    /* every module of the binary is known by name before any is started:
     * one that is needed is then found, whatever its place in the set */
    ForeachElementInSet(SETNAME(NETMODULES), 1, pos, nm)
        netmodule_know(nm);

    ForeachElementInSet(SETNAME(NETMODULES), 1, pos, nm) {
        char dummy[4];

        if(!netmodule_listed(list, nm->nm_name))
            continue;
        if((nm->nm_leaveout != NULL &&
                GetVar(nm->nm_leaveout, dummy, sizeof(dummy), GVF_GLOBAL_ONLY) != -1) ||
                netmodule_leftout(nm->nm_name, strlen(nm->nm_name))) {
            char var[48];

            len = strlen(nm->nm_name);
            netmodule_leaveout_name(nm->nm_name, len, var, sizeof(var));
            nm->nm_flags &= ~NMF_PERMANENT;
            /* nor is it started later because another module needs it */
            netmodule_leaveout_log(nm->nm_name, len, nm->nm_name, 0,
                                   nm->nm_leaveout ? nm->nm_leaveout : var);
        } else if(nm->nm_flags & NMF_STARTED)
            netmodule_nprotocols++;     /* another one needed it */
        else if((error = netmodule_register(nm)) == 0)
            netmodule_nprotocols++;
        else
            __log(LOG_ERR, "module %s could not be started (error %ld): "
                  "the stack runs without it.", nm->nm_name, (long)error);
    }

    /* the selected modules that are not part of the binary */
    for(rest = list; (len = netmodule_nextname(&rest, &name)) != 0;) {
        char lib[48];
        int known = 0;

        ForeachElementInSet(SETNAME(NETMODULES), 1, pos, nm)
            if(netmodule_samename(name, len, nm->nm_name))
                known = 1;
        if(known)
            continue;
        if(!netmodule_libname(name, len, NETMODULE_KIND_PROT, lib, sizeof(lib))) {
            __log(LOG_ERR, "PROTOCOLS: the module name %.20s... is too long.", name);
            continue;
        }
        /* the test switch, from the name: before the file is opened */
        if(netmodule_leftout(name, len)) {
            char var[48];

            netmodule_leaveout_name(name, len, var, sizeof(var));
            netmodule_leaveout_log(name, len, lib, 1, var);
            continue;
        }
        /* another module needed it, and it is loaded and running already */
        if((nm = netmodule_loaded_file(lib)) != NULL) {
            if(nm->nm_flags & NMF_STARTED)
                netmodule_nprotocols++;
            continue;
        }
        /*
         * The loader says in the log what was wrong, with the name of the
         * library. Only a file that is not there is treated differently:
         * named in the configuration it is an error (the loader's, and
         * the line below); selected by the default it is the loader's
         * notice and nothing more.
         */
        if(netmodule_protocols == NULL) {
            if(netmodule_load_optional(lib, &nm) == 0)
                netmodule_nprotocols++;
        } else if(netmodule_load(lib, &nm) == 0)
            netmodule_nprotocols++;
        else
            __log(LOG_ERR, "PROTOCOLS=%s asks for %s, which could not be loaded: "
                  "the stack runs without it.", netmodule_protocols, lib);
    }

    if(netmodule_nprotocols == 0)
        __log(LOG_EMERG, "No protocol family runs (PROTOCOLS=%s): "
              "there is nothing a socket could be made of.", list);
}

/*
 * The optional modules the configuration wants running from the start
 * (net/netmodule.h): the item MODULES, a list of module names as PROTOCOLS
 * is - MODULES=RESOLVER,PACKET - in any letter case. Each is started as on
 * its first use (netdemand_run(): a module of the program is started, else
 * its library loaded, its test switch obeyed), in the order of the list,
 * by the stack's task before the API is shown: the memory is taken at
 * start, and nothing is read from disk later. A name that cannot be had -
 * not installed, not a module, its start failing - is an error in the log
 * that names it; the stack runs without it, and its first use is answered
 * as for a module that is not installed (until the next reload).
 */
void
netmodule_start_listed(void)
{
    const char *rest, *name;
    char mod[NCM_MODULE_NAMELEN];
    int len, error;

    if(netmodule_modules == NULL)
        return;
    for(rest = netmodule_modules; (len = netmodule_nextname(&rest, &name)) != 0;) {
        /* module names are written in small letters */
        if(!netmodule_mkname(mod, sizeof(mod), "", name, len, "")) {
            __log(LOG_ERR, "MODULES: the module name %.20s... is too long.", name);
            continue;
        }
        if((error = netdemand_run(mod)) != 0)
            __log(LOG_ERR, "MODULES=%s asks for module %s, which could not be started "
                  "(error %ld): the stack runs without it.", netmodule_modules, mod, (long)error);
    }
}

/*
 * The stack is ending and is still whole (kern/amiga_main.c deinit_all(),
 * step 1): every started module that lets itself be stopped is stopped by
 * its own nm_stop, which undoes what it registered and frees what it
 * allocated while everything it uses is still there. Last started first -
 * the list is in the order of the starts, so a module is asked before the
 * ones it needs - and again from the end after each one that stopped,
 * because a module may be held by another that is earlier in the list (the
 * tunnel's hold on its carrier).
 * No socket exists any more, and the rule that one protocol family stays
 * is lifted.
 *
 * A module that refuses, or whose stop fails, stays started and is named
 * in the log: it is cut off wholesale later, when nothing can call it any
 * more (netmodule_shutdown_all()). One that cannot be stopped at all (no
 * nm_stop, NMF_PERMANENT) is not asked and not named.
 *
 * Modules in library files are then unloaded by netmodule_unload_all().
 */
void
netmodule_stop_all(void)
{
    struct netmodule *nm, *last, *limit;

    netmodule_ending = 1;
    for(;;) {
        /* from the end of the list towards its head: the first that stops */
        for(limit = NULL;; limit = last) {
            for(last = NULL, nm = netmodules; nm != NULL && nm != limit; nm = nm->nm_next)
                if(nm->nm_stop != NULL && !(nm->nm_flags & NMF_PERMANENT))
                    last = nm;
            if(last == NULL || netmodule_unregister(last) == 0)
                break;
            /* refused for now: the ones before it, then all over again */
        }
        if(last == NULL)
            break;              /* a whole pass in which none stopped */
    }

    /* what is left would not stop; each said why when it was asked */
    for(nm = netmodules; nm != NULL; nm = nm->nm_next)
        if(nm->nm_stop != NULL && !(nm->nm_flags & NMF_PERMANENT))
            __log(LOG_NOTICE, "module %s did not stop at shutdown (%ld user(s)): "
                  "it is cut off instead.", nm->nm_name, (long)netmodule_users(nm));
}

/*
 * The stack is ending (net/netmodule.h; kern/amiga_main.c deinit_all()
 * says where in the order). The modules that are still started are told,
 * last started first - the list is in the order of the starts, and what a
 * module needs was started before it - and then the core forgets them and
 * what they registered: every table below may hold pointers into a module
 * library that is about to leave memory.
 */
void
netmodule_shutdown_all(void)
{
    struct netmodule *nm, *prev;

    for(;;) {
        /* the last of the list that has not been told */
        for(prev = NULL, nm = netmodules; nm != NULL; nm = nm->nm_next)
            if(nm->nm_flags & NMF_STARTED)
                prev = nm;
        if((nm = prev) == NULL)
            break;
        if(NETMODULE_HAS(nm, nm_shutdown) && nm->nm_shutdown != NULL)
            (*nm->nm_shutdown)(nm);
        nm->nm_flags &= ~(NMF_STARTED | NMF_STARTING);
        nm->nm_refs = 0;
    }
    while((nm = netmodules) != NULL) {
        netmodules = nm->nm_next;
        nm->nm_next = NULL;
    }
    bzero((caddr_t)netmodule_knowntab, sizeof(netmodule_knowntab));
    netmodule_nprotocols = 0;
    netmodule_ending = 0;       /* for the next start of the stack */
    netmodule_out_forget();

    /* every table as it was when the program was loaded */
    bzero((caddr_t)netifaces, sizeof(netifaces));
    bzero((caddr_t)protodispatch_families, sizeof(protodispatch_families));
    bzero((caddr_t)nettunnels, sizeof(nettunnels));
    netfamily_ifops_count = 0;
    bzero((caddr_t)netfamily_ifops, sizeof(netfamily_ifops));
    bzero((caddr_t)netvar_tables, sizeof(netvar_tables));
    config_var_rebind();        /* the configuration variables that stood for them */

    netinput_forget_all();      /* net/netisr.c: the input queues */
    domain_forget_all();        /* kern/uipc_domain.c: the domains and protocol tables */
    netdemand_forget_all();     /* kern/net_demand.c: demand slots and delegated ioctls */
}
