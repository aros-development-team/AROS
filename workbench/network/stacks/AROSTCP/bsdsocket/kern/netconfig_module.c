/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005-2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

/*
 * The configuration readers as a module (kern/netconfig_ops.h): module
 * "netconfig", in netconfig.atcp, or - in a monolithic program,
 * or while conf/group-netconfig.cfg builds it into the program - part of
 * the stack program, found through the symbol set NETMODULES. Either way
 * it is not started with the stack's other modules: the stack's task
 * starts it for a read (start-up, a reload) and stops it after, and an
 * ARexx command that reads configuration text has it started for the
 * command (the core's demand slot "netconfig").
 *
 * What is here was moved, unchanged but for the names it reaches the core
 * by, from the files that read the configuration:
 *
 *   general.config, the command line      kern/amiga_config.c
 *   db/netdb and what it includes          kern/amiga_netdb.c
 *   the interface line's arguments         net/sana2config.c
 *   db/services.d                          kern/amiga_svcmgr.c
 *
 * What it fills in is the core's and outlives it: the network database
 * (NDB, from the core's pool), the variables of general.config (set with
 * the core's setvalue()), the interfaces, addresses, routes, DNS servers,
 * the deferred interface lines (netdb_defer_add()), the launched service
 * processes. It keeps nothing between two reads but the three flags below,
 * which its stop sets back. It needs no other module: it runs before the
 * protocol families, which it selects, and asks the core which of them run
 * (netfamily_running()).
 */

#include <conf.h>

#include <dos/dos.h>
#include <dos/rdargs.h>
#include <dos/dostags.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/errno.h>

#include <api/amiga_api.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_dhcp.h>
#include <kern/amiga_includes.h>
#include <kern/amiga_config.h>
#include <kern/amiga_netdb.h>
#include <kern/accesscontrol.h>
#include <libraries/netservice.h>

#include "net/netdbpaths.h"
#include <net/sana2config.h>
#include <net/if_sana.h>
#include <net/netmodule.h>
#include <net/netdemand.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <protos/net/if_protos.h>
#include <protos/net/route_protos.h>
#include <kern/netconfig_ops.h>

#include <net/netcore.h>            /* the core's names, as the module's table has them */
#include <net/netcore_map.h>        /* always the last include */

#ifdef NETMODULE
/*
 * A library file of its own: dos.library, which the core does not hand
 * over, opened by the module itself and closed when it stops. In the
 * stack program the program's own base serves.
 */
struct DosLibrary *DOSBase = NULL;

static int
ncfg_libs_open(void)
{
    if(DOSBase == NULL)
        DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 36);
    return (DOSBase != NULL);
}

static void
ncfg_libs_close(void)
{
    if(DOSBase != NULL)
        CloseLibrary((struct Library *)DOSBase);
    DOSBase = NULL;
}
#else
#define ncfg_libs_open()    1
#define ncfg_libs_close()
#endif

/*
 * Equal but for the case of ASCII letters. The C library's stricmp()
 * would bring its locale tables into a library that has no C library of
 * its own (the module links the static one).
 */
static BOOL
ncfg_keyeq(const char *k, const char *lit)
{
    while(*k && *lit) {
        char a = *k, b = *lit;
        if(a >= 'a' && a <= 'z') a -= 32;
        if(b >= 'a' && b <= 'z') b -= 32;
        if(a != b)
            return FALSE;
        k++; lit++;
    }
    return *k == '\0' && *lit == '\0';
}

/* ======================================================================
 * general.config and the command line (from kern/amiga_config.c)
 * ====================================================================== */

/* Command-line template */
static const TEXT cmd_template[] = "WITH/K,NOO=NOCONFIG/S,DEBUG/S";

#define CL_WITH   0
#define CL_NOCFG  1
#define CL_DEBUG  2		/* Currently default, does nothing */
#define CL_SIZE   3

/*
 * Read command file
 */
static LONG
ncfg_parsefile(UBYTE const *name, UBYTE **errstrp, struct CSource *res)
{
    LONG retval = RETURN_OK, ioerr = 0;
    struct CSource arg;
    int line = 0;
    BPTR fh;
    UBYTE *buf;
#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) parsefile('%s')\n", name));
#endif

    D(Printf("Loading config file: %s\n", name);)
    buf = AllocMem(CONFIGLINELEN, MEMF_PUBLIC);

    if(buf) {
        arg.CS_Buffer = buf;
        if(fh = Open(name, MODE_OLDFILE)) {
            while(FGets(fh, buf, CONFIGLINELEN)) {
                line++;
                if(*buf == '#')
                    continue;
                arg.CS_Length = strnlen(buf, CONFIGLINELEN);
                arg.CS_CurChr = 0;
                retval = setvalue(&arg, errstrp, res);

                if(retval == RETURN_OK)
                    continue;
                if(retval != RETURN_WARN) {  /* severe error */
                    error_request("Fatal configuration error in file %s at line %ld, col %ld\n%s\nAROSTCP will quit", (IPTR)name,
                                  (IPTR)line, (IPTR)arg.CS_CurChr, (IPTR)*errstrp);
                    break;
                }

                /* Print the error to the "stdout" */
#if defined(__AROS__)
                D(bug("[AROSTCP](netconfig) parsefile: %s: line %ld, col %ld: %s",
                      name, line, arg.CS_CurChr, *errstrp));
#endif
                error_request("Configuration error in file %s at line %ld, col %ld\n%s", (IPTR)name, (IPTR)line, (IPTR)arg.CS_CurChr,
                              (IPTR)*errstrp);
                retval = RETURN_OK;
            }
            /* Check file error */
            ioerr = IoErr();

            Close(fh);
        } else {
            ioerr = IoErr();
        }

        if(ioerr) {
            Fault(ioerr, name, res->CS_Buffer, res->CS_Length);
            *errstrp = res->CS_Buffer;
            retval = RETURN_ERROR;
        }
        if(!fh)
            error_request("Unable to open configuration file\n%s", (IPTR)*errstrp);

        FreeMem(buf, CONFIGLINELEN);
    } else {
        *errstrp = ERR_MEMORY;
        retval = RETURN_FAIL;
    }

    return retval;
}

/*
 * Read command line arguments and configuration file
 */
static BOOL
ncfg_readconfig(void)
{
    UBYTE result[REPLYBUFLEN + 1]; /* for error returns */
    struct CSource res;
    struct RDArgs *rdargs = NULL;
    IPTR args[CL_SIZE] = { 0 };
    UBYTE *errstr;
    LONG error = 0;
#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) readconfig()\n"));
#endif

    res.CS_Buffer = result;
    res.CS_Length = sizeof(result);
    res.CS_CurChr = 0;

    /* Parse command line arguments, if any */
    rdargs = ReadArgs(cmd_template, args, NULL);

    if(!rdargs) {
#if defined(__AROS__)
        D(bug("[AROSTCP](netconfig) readconfig: Bad Args\n"));
#endif
        {
            /* VPrintf(): Printf()'s argument walker would be linked into the library */
            IPTR a[1] = { (IPTR)cmd_template };

            VPrintf("Argument error. Template: %s\n", (RAWARG)a);
        }
        return FALSE;
    }

    if(!args[CL_NOCFG])
        /* Read default configuration file */
        error = ncfg_parsefile(config_path, &errstr, &res);

    if(!error && args[CL_WITH])
        /* Read given file */
        error = ncfg_parsefile((STRPTR)args[CL_WITH], &errstr, &res);

    FreeArgs(rdargs);

    if(error) {
#if defined(__AROS__)
        D(bug("[AROSTCP](netconfig) readconfig: TCP/IP Configuration: %s\n", errstr));
#endif
        IPTR a[1] = { (IPTR)errstr };

        VPrintf("TCP/IP Configuration: %s\n", (RAWARG)a);
    }

    return ((BOOL)!error);
}

/* ======================================================================
 * The interface line's arguments (from net/sana2config.c)
 * ====================================================================== */

static const char template[] = SSC_TEMPLATE;

/*
 * Parse the configuration
 */
static struct ssconfig *
ssconfig_parse(struct RDArgs *rdargs)
{
    struct ssconfig *config = AllocVec(sizeof(*config), MEMF_CLEAR | MEMF_PUBLIC);

    D(bug("[AROSTCP] ssconfig_parse()\n"));

    if(config != NULL) {

        if(ReadArgs(template, (IPTR *)config->args, rdargs)) {
            config->rdargs = rdargs;
            config->flags |= SSCF_RDARGS;
            return config;
        } else {
            FreeVec(config);
        }
    }
    return NULL;
}

/*
 * Free the configuration
 */
static void
ssconfig_free(struct ssconfig config[])
{
    D(bug("[AROSTCP] ssconfig_free()\n"));
    if(config->flags & SSCF_RDARGS)
        FreeArgs(config->rdargs);
    FreeVec(config);
}

/* ======================================================================
 * db/netdb and the files it includes (from kern/amiga_netdb.c)
 * ====================================================================== */

static LONG read_netdb(struct NetDataBase *ndb, UBYTE *fname,
                       UBYTE **errstrp, struct CSource *res,
                       int prefixindex, ULONG flags);

/*
 * Templates for Arexx commands and DB files
 */
static const char NETDBENTRY[] =
    "WITH,I=INTERFACE,H=HOST,N=NET,S=SERVICE,P=PROTOCOL,R=ROUTE,NS=NAMESERVER,DO=DOMAIN,RC,ACC=ACCESS";

enum ndbtype { KNDB_WITH, KNDB_IFACE, KNDB_HOST, KNDB_NET, KNDB_SERV, KNDB_PROTO,
               KNDB_RT, KNDB_DNS, KNDB_DOM, KNDB_RC, KNDB_ACC
             };

static const char NETDBTEMPLATE[] =
    "$NAME$/A,$ENTRY$/A,$ALIAS$/M";

static const char PROTOCOL_TEMPLATE[] =
    "$NAME$/A,$NUMBER$/A/N,$ALIAS$/M";

enum ndbarg { KNDB_NAME, KNDB_DATA, KNDB_ALIAS };

#define NDBARGS 3

static const char ROUTE_TEMPLATE[] =
    "HOST/S,NET/S,DEST/A,GW=GATEWAY/A";

enum rtarg { KRT_HOST, KRT_NET, KRT_DEST, KRT_GATE, RTARGS };

static const char ACCESS_TEMPLATE[] =
    "$PORT$/A,$HOSTMASK$/A,$ACCESS$/A,LOG/S";

enum accarg { KACC_PORT, KACC_HOSTMASK, KACC_ACCESS, KACC_LOG };

#define ACCARGS 4

static const char WITH_TEMPLATE[] =
    "$FILE$/A,PREFIX/K";
#define WITHARGS 2
enum witharg { WITH_FILE, WITH_PREFIX };

/*
 * Marks an interface that is being configured. Only a SANA-II interface
 * has these flags and anything that looks at them (sana_ioctl(),
 * run_dhcp()); a tunnel interface is its module's own and has neither.
 */
static void
ifconf_mark(struct ifnet *ifp, UWORD flags)
{
    struct sana_softc *ssc = sana_softc_of(ifp);

    if(ssc != NULL)
        ssc->ss_cflags |= flags;
}

/* prototypes for the netdb parsing functions */

static LONG addwith(struct NetDataBase *ndb,
                    struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
static LONG addifent(struct NetDataBase *ndb,
                     struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
static LONG addhostent(struct NetDataBase *ndb,
                       struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
static LONG addnetent(struct NetDataBase *ndb,
                      struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
static LONG addservent(struct NetDataBase *ndb,
                       struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
static LONG addprotoent(struct NetDataBase *ndb,
                        struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
static LONG addrtent(struct NetDataBase *ndb,
                     struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
static LONG addnameservent(struct NetDataBase *ndb,
                           struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
static LONG adddomainent(struct NetDataBase *ndb,
                         struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
static LONG addrcent(struct NetDataBase *ndb,
                     struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
static LONG addaccessent(struct NetDataBase *ndb,
                         struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);
static LONG addndbent(struct NetDataBase *ndb,
                      struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG ifflags);

typedef LONG(*ndb_parse_f)(struct NetDataBase *ndb,
                           struct RDArgs *rdargs, UBYTE **errstrp, struct CSource *res, ULONG flags);

/* Array of parsing functions. Note that the order is same as in the
 * NETDBENTRY.
 */
static const ndb_parse_f ndb_parse_funs[] = {
    addwith,
    addifent,
    addhostent,
    addnetent,
    addservent,
    addprotoent,
    addrtent,
    addnameservent,
    adddomainent,
    addrcent,
    addaccessent
};

/*
 * Alloc a NetDataBase
 */
static struct NetDataBase *
alloc_netdb(struct NetDataBase *ndb)
{
#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) alloc_netdb()\n"));
#endif

    if(ndb ||
            (ndb = bsd_malloc(sizeof(*NDB), M_NETDB, M_WAITOK))) {
        struct MinList *gl;

        for(gl = (struct MinList *)&ndb->ndb_Hosts;
                gl <= (struct MinList *)&ndb->ndb_Domains;
                gl++)
            NewList((struct List *)gl);

    }

    /*
     * bsd_malloc() can still return NULL even with M_WAITOK; bail out
     * before dereferencing ndb.
     */
    if(ndb == NULL) {
#if defined(__AROS__)
        D(bug("[AROSTCP](netconfig) alloc_netdb: failed to allocate ndb\n"));
#endif
        return NULL;
    }

    ndb->ndb_AccessCount = 0;
    if((ndb->ndb_AccessTable =
                bsd_malloc(TMPACTSIZE, M_NETDB, M_WAITOK)) == NULL) {
        bsd_free(ndb, M_NETDB);
        ndb = NULL;
#if defined(__AROS__)
        D(bug("[AROSTCP](netconfig) alloc_netdb: failed to allocate ndb_AccessTable\n"));
#endif
        return NULL;
    }
#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) alloc_netdb: Allocated ndb = 0x%p, ndb_AccessTable = 0x%p\n", ndb,
          ndb->ndb_AccessTable));
#endif
    DNETDB(else __log(LOG_DEBUG, "Allocated ndb = 0x%p, ndb_AccessTable = 0x%p", ndb, ndb->ndb_AccessTable);)
        return ndb;
}

/*
 * Free a NetDataBase
 * Caller must have a write lock on NDB
 */
static void
free_netdb(struct NetDataBase *ndb)
{
    struct GenentNode *gn;
    struct MinList *gl;
#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) free_netdb( 0x%p )\n", ndb));
#endif
    for(gl = (struct MinList *)&ndb->ndb_Hosts;
            gl <= (struct MinList *)&ndb->ndb_Domains;
            gl++)
        while(gn = (struct GenentNode *)RemHead((struct List *)gl))
            bsd_free(gn, M_NETDB);

    if(ndb->ndb_AccessTable != NULL) {
        bsd_free(ndb->ndb_AccessTable, M_NETDB);
        ndb->ndb_AccessTable = NULL;
    }
    bsd_free(ndb, M_NETDB);
}

#ifdef DEBUG
static UBYTE *zap;
static size_t zap_size;
#endif

/*
 * Copy alias list to ato, alias strings and name to cto
 */
static void
aliascpy(UBYTE *cto, UBYTE *name, UBYTE **ato, UBYTE **afrom)
{
#ifdef DEBUG
    UBYTE *logname = name;
#endif
#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) aliascopy()\n"));
#endif

    do {
        while(*cto++ = *name++);
    } while(afrom && (name = *afrom++) && (*ato++ = cto));

    *ato = NULL;

#ifdef DEBUG
    if(cto != zap + zap_size) {
        __log(LOG_ERR, "%s: mismatch in size %ld != expected %ld\n",
              logname, cto - (UBYTE *)zap, zap_size);
    }
#endif
}

/*
 * Allocate a netdb node
 *
 * nodesize is the size of the base structure, additional space
 * is allocated for the name and the aliases.
 * alias is NULL terminated array of alias name pointers.
 * Number of aliases is returned via aliasp.
 *
 * size field of the allocated node is set to the total size - size for
 * the MinNode and the size field itself.
 */
static void *
node_alloc(size_t nodesize, UBYTE *name, UBYTE **alias, int *aliasp)
{
    struct GenentNode *gn;
#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) node_alloc()\n"));
#endif

    nodesize += strnlen(name, MAXHOSTNAMELEN) + 1;	/* Add space needed for the name */

    *aliasp = 1;
    nodesize += sizeof(char *);	/* Alias list NULL terminator */

    /* Calculate the size of the aliases */
    if(alias) {
        while(*alias) {
            (*aliasp)++;
            nodesize += strnlen(*alias++, MAXHOSTNAMELEN) + 1 + sizeof(char *);
        }
    }
    gn = bsd_malloc(nodesize, M_NETDB, M_WAITOK);
    /*
     * set the ent size
     */
    if(gn)
        gn->gn_EntSize = nodesize - sizeof(struct GenentNode);

#ifdef DEBUG
    zap_size = nodesize;
    zap = (char *)gn;
#endif
    return gn;
}

/*
 * Parse an include entry.
 */
static LONG
addwith(struct NetDataBase *ndb,
        struct RDArgs *rdargs,
        UBYTE **errstrp, struct CSource *res, ULONG flags)
{
    LONG retval = RETURN_OK;
    IPTR Args[WITHARGS] = { 0 };
    int which;

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) addwith()\n"));
#endif

    if(rdargs = ReadArgs(WITH_TEMPLATE, Args, rdargs)) {
        if(Args[WITH_PREFIX] == 0)	/* no prefix given */
            which = -1;
        else {
            /* match given prefix */
            which = FindArg(NETDBENTRY, (UBYTE *)Args[WITH_PREFIX]);
            if(which < 0) {
                *errstrp = ERR_VALUE;
                retval = RETURN_WARN;
            }
        }
        if(retval == RETURN_OK) {
            retval = read_netdb(ndb, (UBYTE *)Args[WITH_FILE], errstrp, res,
                                which, flags);
        }
        FreeArgs(rdargs);
    } else {
        *errstrp = ERR_SYNTAX;
        retval = RETURN_WARN;
    }
    return retval;
}

/* Copy an address */
static int inline setaddr(struct sockaddr_in *sa, char *addr, u_short af)
{
    sa->sin_len = sizeof(struct sockaddr_in);
    sa->sin_family = af;
    return __inet_aton(addr, &sa->sin_addr);
}

#if INET6
/* Parse an IPv6 address string into a sockaddr_in6 (the parser is the core's). */
static int inline setaddr6(struct sockaddr_in6 *sa6, char *addr)
{
    memset(sa6, 0, sizeof(*sa6));
    sa6->sin6_len    = sizeof(struct sockaddr_in6);
    sa6->sin6_family = AF_INET6;
    return inet6_aton(addr, &sa6->sin6_addr);
}

/* Build an IPv6 prefix mask from a prefix length (0-128). */
static void setprefixmask6(struct sockaddr_in6 *mask, int plen)
{
    u_char *cp;
    memset(mask, 0, sizeof(*mask));
    mask->sin6_len    = sizeof(struct sockaddr_in6);
    mask->sin6_family = AF_INET6;
    if(plen < 0)   plen = 0;
    if(plen > 128) plen = 128;
    for(cp = (u_char *)&mask->sin6_addr; plen > 7; plen -= 8)
        * cp++ = 0xff;
    if(plen > 0)
        *cp = (u_char)(0xff << (8 - plen));
}
#endif /* INET6 */

static UBYTE ERR_IFSHUTDOWN[] = "Shut down because an interface failed to start";
static UBYTE ERR_IFEXISTS[]   = "Interface already exists";
static UBYTE ERR_IFNODEVICE[] = "Interface has no DEVICE";
static UBYTE ERR_IFNOTDST[]   = "Tunnel interface needs a valid TDST";
static UBYTE ERR_IFNOOPEN[]   = "Interface device did not open";
static UBYTE ERR_IFDEFERRED[] = "Interface device not available yet, will keep trying";
/* the user asked for the shutdown: the fatal-error requesters would only repeat it */
static BOOL ndb_userquit = FALSE;
static BOOL ndb_deferretry = FALSE;   /* addifent() is being run for a deferred line */
static BOOL ndb_deferfailed;          /* ... and its device still did not open */

/*
 * An end point of a tunnel (TSRC, TDST) as a socket address of the family
 * that carries the tunnel, which the tunnel's kind names (nt_carrier): the
 * text is read as whatever kind of address it is, and must turn out to be
 * one of that family.
 */
static int
tunnel_setaddr(struct sockaddr_storage *ss, char *addr, int carrier)
{
    memset(ss, 0, sizeof(*ss));
    if(!setaddr((struct sockaddr_in *)ss, addr, AF_INET)
#if INET6
            && !setaddr6((struct sockaddr_in6 *)ss, addr)
#endif
      )
        return 0;
    return ss->ss_family == carrier;
}

/*
 * Parse an interface entry.
 */
static LONG
addifent(struct NetDataBase *ndb,
         struct RDArgs *rdargs,
         UBYTE **errstrp, struct CSource *res, ULONG flags)
{
    struct ssconfig *ssc;
    struct ifnet *ifp;
    struct ifaliasreq ifr;
    char *cp, *ep;
    LONG retval = RETURN_OK;
    /* which families are part of the running stack: the settings of one
     * that is not are ignored, and never decide whether the interface
     * comes up */
    int have4 = netfamily_running(AF_INET);
#if INET6
    int have6 = netfamily_running(AF_INET6);
#else
    int have6 = 0;
#endif
    /* the unparsed line, in case the interface has to be deferred */
    const char *srcline = (const char *)rdargs->RDA_Source.CS_Buffer + rdargs->RDA_Source.CS_CurChr;
    LONG srclen = rdargs->RDA_Source.CS_Length - rdargs->RDA_Source.CS_CurChr;

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) addifent()\n"));
#endif

    if(flags & (NETDB_IFF_MODIFYOLD | NETDB_IFF_ADDNEW)) {
        ssc = ssconfig_parse(rdargs);
        if(ssc) {
            ifp = ifunit(ssc->args->a_name);
            if(ifp && (flags & NETDB_IFF_REPORT) && !(flags & NETDB_IFF_MODIFYOLD)) {
                /* ADD leaves an existing interface as it is, but says so */
                *errstrp = ERR_IFEXISTS;
                retval = RETURN_WARN;
            }
            if((!ifp) && (flags & NETDB_IFF_ADDNEW)) {
                DIFCONF(Printf("Adding new interface %s\n", ssc->args->a_name);) {
                    size_t nlen = strnlen(ssc->args->a_name, IFNAMSIZ - 1);
                    memcpy(ssc->name, ssc->args->a_name, nlen);
                    ssc->name[nlen] = '\0';
                    cp = ssc->name;
                }

                for(; *cp; cp++)
                    if(*cp >= '0' && *cp <= '9')
                        break;

                ep = cp;
                for(ssc->unit = 0; *cp >= '0' && *cp <= '9';)
                    ssc->unit = ssc->unit * 10 + *cp++ - '0';

                *ep = 0;
                if(ssc->args->a_tunnel) {
                    /*
                     * A tunnel pseudo-interface: no SANA-II device. Which
                     * kind of tunnel, and which module makes it, is not
                     * this reader's to know: it asks for the kind the
                     * running stack has (net/netmodule.h; today the 6in4
                     * tunnel, which IPv6 registers).
                     */
                    const struct nettunnel *nt = nettunnel_find(NULL);
                    struct sockaddr_storage tsrc, tdst;
                    UBYTE ttl = ssc->args->a_ttl ? (UBYTE)*ssc->args->a_ttl : 64;

                    if(nt == NULL) {
                        /* without the module that makes tunnels the line
                         * is refused, the stack goes on */
                        __log(LOG_ERR, "addifent: tunnel %s: no module of the running "
                              "stack makes tunnel interfaces\n", ssc->args->a_name);
                        *errstrp = ERR_IFNOOPEN;
                        ssconfig_free(ssc);
                        return RETURN_WARN;
                    }
                    memset(&tsrc, 0, sizeof(tsrc));
                    if(ssc->args->a_tdst &&
                       tunnel_setaddr(&tdst, ssc->args->a_tdst, nt->nt_carrier)) {
                        /* TSRC is optional: when omitted (the "any" address)
                         * the outer source is auto-selected from the egress
                         * interface by the carrier's output - robust to DHCP
                         * and correct behind NAT. */
                        if(!ssc->args->a_tsrc ||
                           !tunnel_setaddr(&tsrc, ssc->args->a_tsrc, nt->nt_carrier)) {
                            memset(&tsrc, 0, sizeof(tsrc));
                            tsrc.ss_family = nt->nt_carrier;
                        }
                        ifp = (*nt->nt_make)(ssc, (struct sockaddr *)&tsrc,
                                             (struct sockaddr *)&tdst, ttl);
                        if(ifp == NULL) {
                            /* the tunnel needs both its family and its carrier in the
                             * running stack: without them the line is refused, the
                             * stack goes on */
                            *errstrp = ERR_IFNOOPEN;
                            ssconfig_free(ssc);
                            return RETURN_WARN;
                        }
                    } else {
                        __log(LOG_ERR, "addifent: tunnel %s needs a valid TDST\n",
                              ssc->args->a_name);
                        *errstrp = ERR_IFNOTDST;
                        ssconfig_free(ssc);
                        return RETURN_WARN;
                    }
                } else
                if(!ssc->args->a_dev) {
                    __log(LOG_ERR, "addifent: interface %s has no DEVICE\n",
                          ssc->args->a_name);
                    *errstrp = ERR_IFNODEVICE;
                    ssconfig_free(ssc);
                    return RETURN_WARN;
                } else {
                    /*
                     * A device or unit that does not open - hardware that is
                     * absent or not ready yet - is that interface's problem,
                     * not a reason to quit. An interface marked DEFER is
                     * tried again in the background until it opens; for any
                     * other the user is asked - while the stack starts. Once it
                     * runs (NETDB_IFF_NOASK) there is nobody to hold it up
                     * for: the interface is skipped, and an ADD says so
                     * (NETDB_IFF_REPORT).
                     */
                    while(!(ifp = iface_make(ssc))) {
                        LONG choice;

                        if(ndb_deferretry) {
                            ndb_deferfailed = TRUE;     /* netdb_defer_timer() comes back */
                            break;
                        }
                        if(ssc->args->a_defer && srclen > 0 &&
                                netdb_defer_add(ssc->args->a_name, srcline, srclen)) {
                            __log(LOG_NOTICE, "%s: device %s not available yet, will keep trying.\n",
                                  ssc->args->a_name, ssc->args->a_dev);
                            if(flags & NETDB_IFF_REPORT) {
                                *errstrp = ERR_IFDEFERRED;
                                retval = RETURN_WARN;
                            }
                            break;
                        }
                        if(flags & NETDB_IFF_NOASK)
                            choice = IFFAIL_SKIP;
                        else
                            choice = iface_fail_request(ssc->args->a_name, ssc->args->a_dev,
                                                        ssc->args->a_unit ? *ssc->args->a_unit : 0);
                        if(choice == IFFAIL_RETRY)
                            continue;
                        if(choice == IFFAIL_SHUTDOWN)
                            ndb_userquit = TRUE;
                        else {
                            __log(LOG_ERR, "addifent: interface %s did not start, skipped\n",
                                  ssc->args->a_name);
                            if(flags & NETDB_IFF_REPORT) {
                                *errstrp = ERR_IFNOOPEN;
                                retval = RETURN_ERROR;
                            }
                        }
                        break;
                    }
                    if(!ifp && !ndb_userquit) {
                        ssconfig_free(ssc);
                        return retval;
                    }
                }
                if(!ifp && ndb_userquit) {
                    *errstrp = ERR_IFSHUTDOWN;
                    retval = RETURN_FAIL;
                } else if(!ifp) {
#if defined(__AROS__)
                    D(bug("[AROSTCP](netconfig) addifent: failed to create interface '%s'\n", ssc->args->a_name));
#endif
                    *errstrp = ERR_MEMORY;
                    retval = RETURN_FAIL;
                } else {
#if defined(__AROS__)
                    D(bug("[AROSTCP](netconfig) addifent: created new interface '%s'\n", ssc->args->a_name));
#endif
                    flags |= NETDB_IFF_MODIFYOLD;
                }
            }
            if((ifp) && (flags & NETDB_IFF_MODIFYOLD)) {
#if defined(__AROS__)
                D(bug("[AROSTCP](netconfig) addifent: configuring interface '%s'\n", ssc->args->a_name));
#endif
                DIFCONF(Printf("Setting up interface %s\n", ssc->args->a_name);)
                memset(&ifr, 0, sizeof(ifr));
                /* one line in the log for the family the line has settings
                 * of and the running stack has not */
                if(!have4 && (ssc->args->a_ip || ssc->args->a_netmask ||
                              ssc->args->a_gw || ssc->args->a_dns)) {
                    __log(LOG_NOTICE, "%s: IPv4 is not part of the running stack, "
                          "its IPv4 settings (IP, NETMASK, GW, DNS) are ignored\n",
                          ssc->args->a_name);
                    ifp->if_data.ifi_aros_usedhcp = 0;
                }
                if(!have6 && (ssc->args->a_ip6 || ssc->args->a_prefixlen ||
                              ssc->args->a_gw6 || ssc->args->a_dns6)) {
                    __log(LOG_NOTICE, "%s: IPv6 is not part of the running stack, "
                          "its IPv6 settings (IP6, PREFIXLEN, GW6, DNS6) are ignored\n",
                          ssc->args->a_name);
                    ifp->if_data.ifi_aros_usedhcp6 = 0;
                }
                if(ssc->args->a_ip && have4) {
                    if(!ssc->args->a_up) {
                        DIFCONF(Printf("> No auto-online\n");)
                        ifconf_mark(ifp, SSF_NOUP);
                    }
                    DIFCONF(Printf("> IP address: %s\n", ssc->args->a_ip);)
                    if(ncfg_keyeq(ssc->args->a_ip, "DHCP")) {
                        DIFCONF(Printf("> Using DHCP\n");)
                        ifp->if_data.ifi_aros_usedhcp = 1;
                        if(ssc->args->a_up) {
                            if(api_state == API_SHOWN)
                                run_dhclient(ifp);
                            else
                                ifconf_mark(ifp, SSF_DELAYUP);
                        }
                    } else if(ncfg_keyeq(ssc->args->a_ip, "AUTO")) {
                        /* Automatic: use IPv4LL (RFC 3927) to assign 169.254.x.x */
                        DIFCONF(Printf("> Using Auto (IPv4LL / RFC 3927)\n");)
                        ifp->if_data.ifi_aros_usedhcp = 0;
                        if(ssc->args->a_up) {
                            if(api_state == API_SHOWN)
                                (void)netfamily_if_event_to(AF_INET, NFE_AUTOADDR, ifp);
                            else
                                ifconf_mark(ifp, SSF_DELAYUP);
                        }
                    } else {
                        ifp->if_data.ifi_aros_usedhcp = 0;
                        if(setaddr((struct sockaddr_in *)&ifr.ifra_addr, ssc->args->a_ip, AF_INET)) {
                            if(ssc->args->a_netmask) {
                                DIFCONF(Printf("> Netmask: %s\n", ssc->args->a_netmask);)
                                if(!setaddr((struct sockaddr_in *)&ifr.ifra_mask, ssc->args->a_netmask, AF_UNSPEC)) {
                                    *errstrp = ERR_SYNTAX;
                                    retval = RETURN_WARN;
                                }
                            }
                            (void)netfamily_control(AF_INET, NULL, SIOCAIFADDR,
                                                    (caddr_t)&ifr, ifp);
                        } else {
                            *errstrp = ERR_SYNTAX;
                            retval = RETURN_WARN;
                        }
                    }
                }
            }
            /*
             * Whether the interface comes up is the line's to say (UP), not
             * a family's. Until here only setting an address started the
             * device and put it online (sana_ioctl(SIOCSIFADDR)), and the
             * address was IPv4's: an interface on a stack without IPv4, or
             * one whose line names no IPv4 address at all, never came up -
             * and so had no link-local address, no SLAAC, nothing. Such an
             * interface is started here. With UP it goes online; without,
             * a line that names an address still has its device
             * configured, as setting the address would have done.
             *
             * Where IPv4 runs and the line has an IP (an address, DHCP or
             * AUTO) nothing is done here: IPv4 starts the device as it
             * always did, and at the moment it always did - the DHCP
             * client and IPv4LL depend on that order.
             */
            if(ifp && (flags & NETDB_IFF_MODIFYOLD) &&
                    !(have4 && ssc->args->a_ip) &&
                    (ssc->args->a_up || ssc->args->a_ip))
                (void)sana_bringup(ifp, ssc->args->a_up != 0);
#if INET6
            /*
             * The link-local address is the stack's own, made by in6_if_up()
             * when the interface comes up. An in-place reload deletes it with
             * every other address (sana_reconfig_teardown()) but leaves the
             * interface up and running, so nothing would make it again, and
             * without it the interface has no IPv6 at all: make it here, with
             * its DAD and the Router Solicitation after it, as when the
             * interface comes up (RFC 4861 section 6.3.7). An interface that
             * is not up and running yet gets it when it is (SIOCSIFFLAGS,
             * sana_restore()); one that still has it is left alone.
             */
            if(ifp && (flags & NETDB_IFF_MODIFYOLD) &&
                    (ifp->if_flags & (IFF_UP | IFF_DRV_RUNNING)) ==
                    (IFF_UP | IFF_DRV_RUNNING)) {
                (void)netfamily_if_event(NFE_UP, ifp);
            }
            /* Apply IPv6 address based on configured mode */
            if((ifp) && (flags & NETDB_IFF_MODIFYOLD) && ssc->args->a_ip6 && have6) {
                if(strncmp(ssc->args->a_ip6, "AUTO", 4) == 0) {
                    /* Link-local only: stack already assigns fe80:: via RA/NDP,
                     * nothing more to do */
                    DIFCONF(Printf("> IPv6: link-local only (AUTO)\n");)
                    ifp->if_data.ifi_aros_usedhcp6 = 0;
                } else if(strncmp(ssc->args->a_ip6, "DHCP", 4) == 0) {
                    /* DHCPv6 */
                    DIFCONF(Printf("> IPv6 address: using DHCPv6\n");)
#if DHCP6
                    ifp->if_data.ifi_aros_usedhcp6 = 1;
                    if(ssc->args->a_up) {
                        if(api_state == API_SHOWN)
                            run_dhclient6(ifp);
                        else
                            ifconf_mark(ifp, SSF_DELAYUP);
                    }
#endif
                } else {
                    /* Static IPv6 address */
                    struct in6_aliasreq in6r;
                    int plen = ssc->args->a_prefixlen ? (int) * ssc->args->a_prefixlen : 64;

                    DIFCONF(Printf("> IPv6 address: %s prefixlen %d\n",
                                   ssc->args->a_ip6, plen);)

                    memset(&in6r, 0, sizeof(in6r));
                    {
                        size_t nlen = strnlen(ssc->args->a_name,
                                              sizeof(in6r.ifra_name) - 1);
                        memcpy(in6r.ifra_name, ssc->args->a_name, nlen);
                    }

                    if(setaddr6(&in6r.ifra_addr, ssc->args->a_ip6)) {
                        setprefixmask6(&in6r.ifra_prefixmask, plen);
                        /* For a 6in4 tunnel the IPv6 peer is GW6 - set it as
                         * the point-to-point destination so the peer host
                         * route (and hence the ::/0 route via it) is installed. */
                        if(ssc->args->a_tunnel && ssc->args->a_gw6 &&
                                ssc->args->a_gw6[0] != '\0') {
                            setaddr6(&in6r.ifra_dstaddr, ssc->args->a_gw6);
                        }
                        {
                            int e6 = netfamily_control(AF_INET6, NULL, SIOCAIFADDR_IN6,
                                                       (caddr_t)&in6r, ifp);
                            if(e6 != 0)
                                __log(LOG_WARNING,
                                      "addifent: SIOCAIFADDR_IN6 failed "
                                      "for %s (err %d)\n", ssc->args->a_name, e6);
                        }
                    } else {
                        __log(LOG_WARNING,
                              "addifent: bad IPv6 address '%s'\n",
                              ssc->args->a_ip6);
                        *errstrp = ERR_SYNTAX;
                        retval = RETURN_WARN;
                    }
                }
            }
#endif /* INET6 */
            /* Install per-interface default routes if GW/GW6 specified */
            if(ifp && (flags & NETDB_IFF_MODIFYOLD) && have4 && ssc->args->a_gw &&
                    ssc->args->a_gw[0] != '\0') {
                struct ortentry route;
                struct sockaddr_in *dst4 = (struct sockaddr_in *)&route.rt_dst;
                struct sockaddr_in *gw4  = (struct sockaddr_in *)&route.rt_gateway;
                memset(&route, 0, sizeof(route));
                dst4->sin_family      = AF_INET;
                dst4->sin_len         = sizeof(*dst4);
                dst4->sin_addr.s_addr = INADDR_ANY;  /* default route: 0.0.0.0 */
                if(setaddr(gw4, ssc->args->a_gw, AF_INET)) {
                    route.rt_flags = RTF_UP | RTF_GATEWAY;
                    rtioctl(SIOCADDRT, (caddr_t)&route);
                }
            }
#if INET6
            if(ifp && (flags & NETDB_IFF_MODIFYOLD) && have6 && ssc->args->a_gw6 &&
                    ssc->args->a_gw6[0] != '\0') {
                struct sockaddr_in6 dst6, gw6, mask6;
                memset(&dst6, 0, sizeof(dst6));
                memset(&gw6,  0, sizeof(gw6));
                memset(&mask6, 0, sizeof(mask6));
                dst6.sin6_family = AF_INET6;
                dst6.sin6_len    = sizeof(dst6);
                /* dst6.sin6_addr is all-zeros = default route ::/0 */
                /* An all-zero netmask (::) makes this a ::/0 network route,
                 * not a host route to the unspecified address. */
                mask6.sin6_family = AF_INET6;
                mask6.sin6_len    = sizeof(mask6);
                if(setaddr6(&gw6, ssc->args->a_gw6)) {
                    rtrequest(RTM_ADD,
                              (struct sockaddr *)&dst6,
                              (struct sockaddr *)&gw6,
                              (struct sockaddr *)&mask6,
                              RTF_UP | RTF_GATEWAY,
                              NULL);
                }
            }
#endif /* INET6 */
            /* Register this interface's per-protocol DNS servers. */
            if(ifp && (flags & NETDB_IFF_MODIFYOLD))
                ifnet_set_dns(ifp, have4 ? ssc->args->a_dns : NULL,
                              have6 ? ssc->args->a_dns6 : NULL);
            /* Mark this interface as present in the (re)read config, so a
             * reload can disable interfaces that dropped out of it. */
            if(ifp)
                sana_mark_seen(ifp);
            ssconfig_free(ssc);
        } else {
            *errstrp = ERR_SYNTAX;
            retval = RETURN_WARN;
        }
    }
    return retval;
}


/*
 * Parse a service entry.
 */
static LONG
addservent(struct NetDataBase *ndb,
           struct RDArgs *rdargs,
           UBYTE **errstrp, struct CSource *res, ULONG flags)
{
    LONG retval;
    IPTR Args[NDBARGS] = { 0 };
    struct ServentNode *sn;
    int aliases, plen;

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) addservent()\n"));
#endif

    if(rdargs = ReadArgs(NETDBTEMPLATE, Args, rdargs)) {
        /* convert port number */
        UBYTE *s_proto = (UBYTE *)Args[KNDB_DATA];
        LONG tmp;
        if((plen = StrToLong(s_proto, &tmp)) > 0 &&
                s_proto[plen++] == '/') {
            Args[KNDB_DATA] = tmp;
            int protonamelen = strnlen(s_proto = s_proto + plen, MAXHOSTNAMELEN) + 1;
            sn = node_alloc(sizeof(*sn) + protonamelen,
                            (UBYTE *)Args[KNDB_NAME],
                            (UBYTE **)Args[KNDB_ALIAS], &aliases);
            if(sn) {
                UBYTE **alias = (UBYTE **)(sn + 1);
                UBYTE *name = (UBYTE *)(alias + aliases);

                /* struct servent: the port in network byte order (it was
                 * stored as the number, which is the same only on a
                 * big-endian machine) */
                sn->sn_Ent.s_port = htons((u_short)Args[KNDB_DATA]);
                sn->sn_Ent.s_proto = name;
                memcpy(name, s_proto, protonamelen);
                sn->sn_Ent.s_name = name + protonamelen;
                sn->sn_Ent.s_aliases = (char **)alias;

                /* Copy aliases */
                aliascpy(sn->sn_Ent.s_name, (UBYTE *)Args[KNDB_NAME],
                         alias, (UBYTE **)Args[KNDB_ALIAS]);
                AddTail((struct List *)&ndb->ndb_Services, (struct Node *)sn);
                retval = RETURN_OK;
            } else {
                *errstrp = ERR_MEMORY;
                retval = RETURN_FAIL;
            }
        } else {
            *errstrp = ERR_VALUE;
            retval = RETURN_WARN;
        }
        FreeArgs(rdargs);
    } else {
        *errstrp = ERR_SYNTAX;
        retval = RETURN_WARN;
    }
    return retval;
}

/*
 * Parse a host entry.
 *
 * NOTE: The host entry has the address in the 'name' and the official name
 * in the 'data'.
 */
static LONG
addhostent(struct NetDataBase *ndb,
           struct RDArgs *rdargs,
           UBYTE **errstrp, struct CSource *res, ULONG flags)
{
    LONG retval;
    IPTR Args[NDBARGS] = { 0 };
    struct HostentNode *hn;
    struct in_addr addr;
    int aliases;
#if INET6
    struct in6_addr addr6;
#endif

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) addhostent()\n"));
#endif

    if(rdargs = ReadArgs(NETDBTEMPLATE, Args, rdargs)) {
#if INET6
        /* an IPv6 address: the same kind of entry, of the other family */
        if(strchr((char *)Args[KNDB_NAME], ':') != NULL &&
                inet6_aton((char *)Args[KNDB_NAME], &addr6)) {
            hn = node_alloc(sizeof(*hn) + 2 * sizeof(&addr6) + sizeof(addr6),
                            (UBYTE *)Args[KNDB_DATA],
                            (UBYTE **)Args[KNDB_ALIAS], &aliases);
            if(hn) {
                struct in6_addr **addrtbl = (struct in6_addr **)(hn + 1);
                UBYTE **alias = (UBYTE **)((UBYTE *)(addrtbl + 2) + sizeof(addr6));
                UBYTE *name = (UBYTE *)(alias + aliases);

                hn->hn_Ent.h_addrtype = AF_INET6;
                hn->hn_Ent.h_length = sizeof(addr6);
                hn->hn_Ent.h_addr_list = (char **)addrtbl;
                hn->hn_Ent.h_name = name;
                hn->hn_Ent.h_aliases = (char **)alias;

                addrtbl[0] = (struct in6_addr *)(addrtbl + 2);
                addrtbl[1] = NULL;
                bcopy(&addr6, addrtbl[0], sizeof(addr6));

                aliascpy(hn->hn_Ent.h_name, (UBYTE *)Args[KNDB_DATA],
                         alias, (UBYTE **)Args[KNDB_ALIAS]);
                AddTail((struct List *)&ndb->ndb_Hosts, (struct Node *)hn);
                retval = RETURN_OK;
            } else {
                *errstrp = ERR_MEMORY;
                retval = RETURN_FAIL;
            }
        } else
#endif
        /* convert ip address */
        if(__inet_aton((char *)Args[KNDB_NAME], &addr)) {
            hn = node_alloc(sizeof(*hn) + 2 * sizeof(&addr) + sizeof(addr),
                            (UBYTE *)Args[KNDB_DATA],
                            (UBYTE **)Args[KNDB_ALIAS], &aliases);
            if(hn) {
                struct in_addr **addrtbl = (struct in_addr **)(hn + 1);
                UBYTE **alias = (UBYTE **)((UBYTE *)(addrtbl + 2) + sizeof(addr));
                UBYTE *name = (UBYTE *)(alias + aliases);

                hn->hn_Ent.h_addrtype = AF_INET;
                hn->hn_Ent.h_length = sizeof(addr);
                hn->hn_Ent.h_addr_list = (char **)addrtbl;
                hn->hn_Ent.h_name = name;
                hn->hn_Ent.h_aliases = (char **)alias;

                /* Make address list */
                addrtbl[0] = (struct in_addr *)(addrtbl + 2);
                addrtbl[1] = NULL;
                bcopy(&addr, addrtbl[0], sizeof(addr));

                /* Copy aliases */
                aliascpy(hn->hn_Ent.h_name, (UBYTE *)Args[KNDB_DATA],
                         alias, (UBYTE **)Args[KNDB_ALIAS]);
                AddTail((struct List *)&ndb->ndb_Hosts, (struct Node *)hn);
                retval = RETURN_OK;
            } else {
                *errstrp = ERR_MEMORY;
                retval = RETURN_FAIL;
            }
        } else {
            *errstrp = ERR_VALUE;
            retval = RETURN_WARN;
        }
        FreeArgs(rdargs);
    } else {
        *errstrp = ERR_SYNTAX;
        retval = RETURN_WARN;
    }
    return retval;
}

/*
 * Parse a net entry.
 */
static LONG
addnetent(struct NetDataBase *ndb,
          struct RDArgs *rdargs,
          UBYTE **errstrp, struct CSource *res, ULONG flags)
{
    LONG retval;
    IPTR Args[NDBARGS] = { 0 };
    struct NetentNode *nn;
    ULONG net;
    int aliases;

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) addnetent()\n"));
#endif

    if(rdargs = ReadArgs(NETDBTEMPLATE, Args, rdargs)) {
        /* the network number, in host order (getnetent(3)), read as
         * inet_network() reads it, as the BSDs do */
        if((net = __inet_network((char *)Args[KNDB_DATA])) != INADDR_NONE) {
            nn = node_alloc(sizeof(*nn),
                            (UBYTE *)Args[KNDB_NAME],
                            (UBYTE **)Args[KNDB_ALIAS], &aliases);
            if(nn) {
                UBYTE **alias = (UBYTE **)(nn + 1);
                UBYTE *name = (UBYTE *)(alias + aliases);

                nn->nn_Ent.n_addrtype = AF_INET;
                nn->nn_Ent.n_name = name;
                nn->nn_Ent.n_aliases = (char **)alias;
                nn->nn_Ent.n_net = net;

                /* Copy aliases */
                aliascpy(nn->nn_Ent.n_name, (UBYTE *)Args[KNDB_NAME],
                         alias, (UBYTE **)Args[KNDB_ALIAS]);
                AddTail((struct List *)&ndb->ndb_Networks, (struct Node *)nn);
                retval = RETURN_OK;
            } else {
                *errstrp = ERR_MEMORY;
                retval = RETURN_FAIL;
            }
        } else {
            *errstrp = ERR_VALUE;
            retval = RETURN_WARN;
        }
        FreeArgs(rdargs);
    } else {
        *errstrp = ERR_SYNTAX;
        retval = RETURN_WARN;
    }
    return retval;
}

/*
 * Parse a protocol entry.
 */
static LONG
addprotoent(struct NetDataBase *ndb,
            struct RDArgs *rdargs,
            UBYTE **errstrp, struct CSource *res, ULONG flags)
{
    LONG retval;
    IPTR Args[NDBARGS] = { 0 };
    struct ProtoentNode *pn;
    int aliases;

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) addprotoent()\n"));
#endif

    if(rdargs = ReadArgs(PROTOCOL_TEMPLATE, Args, rdargs)) {

        if(Args[KNDB_DATA]) {
            pn = node_alloc(sizeof(*pn), (UBYTE *)Args[KNDB_NAME],
                            (UBYTE **)Args[KNDB_ALIAS], &aliases);
            if(pn) {
                UBYTE **alias = (UBYTE **)(pn + 1);
                UBYTE *name = (UBYTE *)(alias + aliases);

                pn->pn_Ent.p_name = name;
                pn->pn_Ent.p_aliases = (char **)alias;
                pn->pn_Ent.p_proto = *(LONG *)Args[KNDB_DATA];

                /* Copy name and aliases */
                aliascpy(name, (UBYTE *)Args[KNDB_NAME],
                         alias, (UBYTE **)Args[KNDB_ALIAS]);
                AddTail((struct List *)&ndb->ndb_Protocols, (struct Node *)pn);
                retval = RETURN_OK;
            } else {
                *errstrp = ERR_MEMORY;
                retval = RETURN_FAIL;
            }
        } else {
            *errstrp = ERR_VALUE;
            retval = RETURN_WARN;
        }
        FreeArgs(rdargs);
    } else {
        *errstrp = ERR_SYNTAX;
        retval = RETURN_WARN;
    }
    return retval;
}

/*
 * Parse a route entry
 */
static LONG
addrtent(struct NetDataBase *ndb,
         struct RDArgs *rdargs,
         UBYTE **errstrp, struct CSource *res, ULONG flags)
{
    LONG retval;
    IPTR Args[RTARGS] = { 0 };
    struct ortentry route;

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) addrtent()\n"));
#endif

    if(rdargs = ReadArgs(ROUTE_TEMPLATE, Args, rdargs)) {
        /* (it was strupr() and strcmp(): the same test, the text left as it is) */
        if(ncfg_keyeq((char *)Args[KRT_DEST], "DEFAULT")) {
            struct sockaddr_in *rodst_saddr = (struct sockaddr_in *)&route.rt_dst;
            rodst_saddr->sin_addr.s_addr = 0;
            route.rt_dst.sa_family = AF_INET;
            route.rt_dst.sa_len = sizeof(struct sockaddr_in);
        } else {
            if(!(setaddr((struct sockaddr_in *)&route.rt_dst, (APTR)Args[KRT_DEST], AF_INET)))
                goto bad;
        }

        if(setaddr((struct sockaddr_in *)&route.rt_gateway, (APTR)Args[KRT_GATE], AF_INET)) {
            route.rt_flags = RTF_UP | RTF_GATEWAY;
            if(Args[KRT_HOST])
                route.rt_flags |= RTF_HOST;
            rtioctl(SIOCADDRT, (caddr_t)&route);
            retval = RETURN_OK;
        } else {
bad:
            *errstrp = ERR_SYNTAX;
            retval = RETURN_WARN;
        }
        FreeArgs(rdargs);
    } else {
        *errstrp = ERR_SYNTAX;
        retval = RETURN_WARN;
    }
    return retval;
}

/*
 * Parse a Name Server entry
 */
static LONG
addnameservent(struct NetDataBase *ndb,
               struct RDArgs *rdargs,
               UBYTE **errstrp, struct CSource *res, ULONG flags)
{
    UBYTE Buffer[KEYWORDLEN];
    LONG  BufLen = sizeof(Buffer);
    struct in_addr ns_addr;
    struct in6_addr ns_addr6;
    int family = AF_INET;
    struct NameserventNode *nsn;

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) addnameservent()\n"));
#endif

    if(ReadItem(Buffer, BufLen, &rdargs->RDA_Source) <= 0) {
#if defined(__AROS__)
        D(bug("[AROSTCP](netconfig) addnameservent: Error in syntax\n"));
#endif
        *errstrp = ERR_SYNTAX;
        return RETURN_WARN;
    }
    /*
     * An IPv4 or an IPv6 address. A link-local IPv6 server needs a zone,
     * which a NAMESERVER line has no room for: it goes with its interface
     * (DNS6=, or learned from a Router Advertisement) instead.
     */
    if(!__inet_aton(Buffer, &ns_addr)) {
        if(!inet6_aton(Buffer, &ns_addr6) || IN6_IS_ADDR_LINKLOCAL(&ns_addr6) ||
                IN6_IS_ADDR_UNSPECIFIED(&ns_addr6) || IN6_IS_ADDR_MULTICAST(&ns_addr6)) {
#if defined(__AROS__)
            D(bug("[AROSTCP](netconfig) addnameservent: error with address\n"));
#endif
            *errstrp = ERR_VALUE;
            return RETURN_WARN;
        }
        family = AF_INET6;
        ns_addr.s_addr = 0;
    }
    if((nsn = bsd_malloc(sizeof(*nsn), M_NETDB, M_WAITOK)) == NULL) {
#if defined(__AROS__)
        D(bug("[AROSTCP](netconfig) addnameservent: couldnt allocate entry\n"));
#endif
        *errstrp = ERR_MEMORY;
        return RETURN_FAIL;
    }
    nsn->nsn_EntSize = sizeof(nsn->nsn_Ent);
    nsn->nsn_Family = family;
    nsn->nsn_Ent.ns_addr.s_addr = ns_addr.s_addr;
    if(family == AF_INET6)
        nsn->nsn_Addr6 = ns_addr6;
    nsn->nsn_OwnerIndex = 0;
    nsn->nsn_Expire = 0;

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) addnameservent: Added nameserver %s (0x%08lx) to netdb = 0x%p\n", Buffer,
          nsn->nsn_Ent.ns_addr.s_addr, ndb));
#endif
    DNETDB(__log(LOG_DEBUG, "Added nameserver %s (0x%08lx) to netdb = 0x%p\n", Buffer, nsn->nsn_Ent.ns_addr.s_addr, ndb);
          )

    AddTail((struct List *)&ndb->ndb_NameServers, (struct Node *)nsn);
    return RETURN_OK;
}

/*
 * Parse a Domain Name entry
 */
static LONG
adddomainent(struct NetDataBase *ndb,
             struct RDArgs *rdargs,
             UBYTE **errstrp, struct CSource *res, ULONG flags)
{
    UBYTE Buffer[REPLYBUFLEN];
    LONG  BufLen = sizeof(Buffer);
    struct DomainentNode *dn;
    short  nodesize;

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) adddomainent()\n"));
#endif

    if(ReadItem(Buffer, BufLen, &rdargs->RDA_Source) <= 0) {
        *errstrp = ERR_SYNTAX;
        return RETURN_WARN;
    }
    nodesize = sizeof(*dn) + strnlen(Buffer, BufLen) + 1;
    if((dn = bsd_malloc(nodesize, M_NETDB, M_WAITOK)) == NULL) {
        *errstrp = ERR_MEMORY;
        return RETURN_FAIL;
    }
    dn->dn_EntSize = nodesize - sizeof(struct GenentNode);
    dn->dn_Ent.d_name = (char *)(dn + 1);
    dn->dn_OwnerIndex = 0;
    dn->dn_Expire = 0;

    memcpy((char *)(dn + 1), Buffer, nodesize - sizeof(*dn));

    AddTail((struct List *)&ndb->ndb_Domains, (struct Node *)dn);
    return RETURN_OK;
}

/*
 * Parse an RC script entry
 */
static LONG
addrcent(struct NetDataBase *ndb,
         struct RDArgs *rdargs,
         UBYTE **errstrp, struct CSource *res, ULONG flags)
{
    UBYTE Buffer[REPLYBUFLEN];
    LONG  BufLen = sizeof(Buffer);
    struct RcentNode *dn;
    short  nodesize;

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) addrcent()\n"));
#endif

    if(ReadItem(Buffer, BufLen, &rdargs->RDA_Source) <= 0) {
        *errstrp = ERR_SYNTAX;
        return RETURN_WARN;
    }
    nodesize = sizeof(*dn) + strnlen(Buffer, BufLen) + 1;
    if((dn = bsd_malloc(nodesize, M_NETDB, M_WAITOK)) == NULL) {
        *errstrp = ERR_MEMORY;
        return RETURN_FAIL;
    }
    dn->rn_EntSize = nodesize - sizeof(struct GenentNode);
    dn->rn_Ent = (char *)(dn + 1);

    memcpy((char *)(dn + 1), Buffer, nodesize - sizeof(*dn));

    AddTail((struct List *)&ndb->ndb_Rc, (struct Node *)dn);
    return RETURN_OK;
}


/*
 * Parse a access control entry.. after reading the whole netdatabase
 * access list must be reorganized;
 */
static LONG
addaccessent(struct NetDataBase *ndb,
             struct RDArgs *rdargs,
             UBYTE **errstrp, struct CSource *res, ULONG ifflags)
{
    LONG retval = RETURN_WARN;
    IPTR Args[ACCARGS] = { 0 };

    ULONG host, mask;
    UWORD port, flags = ACF_CONTINUE;

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) addaccessent()\n"));
#endif

    if(ndb->ndb_AccessCount >= TMPACTSIZE / sizeof(struct AccessItem)) {
        *errstrp = "Too many access control items\n";
        return retval; /* copy propagation expected */
    }

    if((rdargs = ReadArgs(ACCESS_TEMPLATE, Args, rdargs)) != NULL) {

        if(strcmp((char *)Args[KACC_PORT], "*") == 0)
            port = 0;
        else if(strcmp((char *)Args[KACC_PORT], "@") == 0) {
            port = 0;
            flags |= ACF_PRIVONLY;
        } else if(StrToLong((char *)Args[KACC_PORT], (LONG *)&host) > 0
                  && host != 0) {
            if(host > 0xffff) {
                *errstrp = "Illegal port value\n";
                goto exit;
            }
            port = host;
        } else {
            struct ServentNode *entNode;

            if((entNode =
                        findServentNode(ndb, (char *)Args[KACC_PORT], "tcp")) != NULL)
                port = ntohs(entNode->sn_Ent.s_port);  /* the table is in host order */
            else {
                *errstrp = "Illegal port value\n";
                goto exit;
            }
        }
        {
            int zmask = 0xFFFFFFFF;
            int i = 0, ls = 0, dots = 0;

#define hm ((char *)Args[KACC_HOSTMASK])

            while((hm[i] >= '0' && hm[i] <= '9') || hm[i] == '.' || hm[i] == '*') {
                if(hm[i] == '.') {
                    ls = 0;
                    dots++;
                } else if(hm[i] == '*') {
                    hm[i] = '0';
                    ls = 1;
                    /* An IPv4 address has at most 3 dots; a malformed mask
                     * with more would shift by >= 32 bits (undefined), so
                     * clamp the octet index before the shift. */
                    if(dots <= 3)
                        zmask ^= (0xFF000000 >> 8 * dots);
                }
                i++;
            }
            if(ls == 1)
                while(dots++ < 3)
                    zmask ^= (0xFF000000 >> 8 * dots);

            if(hm[i] == '/') {
                hm[i++] = '\0';
                if(__inet_aton(&hm[i], (struct in_addr *)&mask) == 0) {
                    *errstrp = "Illegal mask value\n";
                    goto exit;
                }
            } else
                mask = 0xffffffff;

            mask &= zmask;

            if(__inet_aton(hm, (struct in_addr *)&host) == 0) {
                *errstrp = "Illegal host value\n";
                goto exit;
            }
#undef hm
        }
        if(strcmp((char *)Args[KACC_ACCESS], "allow") == 0)
            flags |= ACF_ALLOW;
        else if(strcmp((char *)Args[KACC_ACCESS], "deny") != 0) {
            *errstrp = "Illegal access value\n";
            goto exit;
        }

        if(Args[KACC_LOG])
            flags |= ACF_LOG;

        ndb->ndb_AccessTable[ndb->ndb_AccessCount].ai_port = port;
        ndb->ndb_AccessTable[ndb->ndb_AccessCount].ai_host = host;
        ndb->ndb_AccessTable[ndb->ndb_AccessCount].ai_mask = mask;
        ndb->ndb_AccessTable[ndb->ndb_AccessCount].ai_flags = flags;
        ndb->ndb_AccessCount++;

        retval = 0;
exit:
        FreeArgs(rdargs);
    } else
        *errstrp = ERR_SYNTAX;

    return retval;
}

/*
 * Add an entry into NetDB.
 * Caller must have a write lock on ndb
 */
static LONG
addndbent(struct NetDataBase *ndb,
          struct RDArgs *rdargs,
          UBYTE **errstrp, struct CSource *res, ULONG flags)
{
    if(NDB) {
        LONG item;
        LONG which;                     /* FindArg() returns -1; an enum may be unsigned */
        UBYTE Buffer[KEYWORDLEN];

#if defined(__AROS__)
        D(bug("[AROSTCP](netconfig) addndbent()\n"));
#endif

        /* Get entry type */
        item = ReadItem(Buffer, sizeof(Buffer), &rdargs->RDA_Source);

        if(item == 0)
            return RETURN_OK;		/* empty line */
        if(item < 0) {
            DNETDB(Printf("addndbent: syntax error\n");)
            *errstrp = ERR_SYNTAX;
            return RETURN_WARN;
        }
        if((which = FindArg(NETDBENTRY, Buffer)) < 0) {
            DNETDB(Printf("Unknown keyword\n");)
            *errstrp = ERR_UNKNOWN;
            return RETURN_WARN;
        }

        DNETDB(Printf("Keyword number: %ld\n", which);)
        which =  ndb_parse_funs[which](ndb, rdargs, errstrp, res, flags);
        return which;

    } else {
        *errstrp = ERR_NONETDB;
        return RETURN_FAIL;
    }
}

/*
 * Read in a NetDataBase file
 */
static LONG
read_netdb(struct NetDataBase *ndb, UBYTE *fname,
           UBYTE **errstrp, struct CSource *res, int prefixindex, ULONG flags)
{
    LONG warnval = RETURN_OK;
    LONG retval = RETURN_OK, ioerr = 0;
    UBYTE *p, *buf = AllocMem(CONFIGLINELEN, MEMF_PUBLIC);
    struct RDArgs *rdargs;
    BPTR fh;
    short line = 0;
    ndb_parse_f parser;
    BPTR lock, oldcd;

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) read_netdb('%s')\n", fname));
#endif

    DNETDB(Printf("Reading netdb file: %s\n", fname);)
    /* Get an exclusive lock on the database.
     * Multiple locks are OK (when this function is called recursively)
     */
    LOCK_W_NDB(ndb);
    if(ndb_Lock.ss_NestCount > 10) {
        UNLOCK_NDB(ndb);
        *errstrp = "Too many files included";
#if defined(__AROS__)
        D(bug("[AROSTCP](netconfig) read_netdb: Too many files included\n"));
#endif
        return RETURN_ERROR;
    }
    if(buf) {

        /* CD to netdb directory */
        lock = Lock(db_path, ACCESS_READ);
        if(lock)
            oldcd = CurrentDir(lock);

        if(fh = Open(fname, MODE_OLDFILE)) {
            if(rdargs = AllocDosObject(DOS_RDARGS, NULL)) {
                /* initialize CSource of the rdargs */
                rdargs->RDA_Source.CS_Buffer = buf;
                /* initialize rest fields (see dos/rdargs.h) */
                rdargs->RDA_DAList = 0;
                rdargs->RDA_ExtHelp = NULL;
                rdargs->RDA_Flags = 0;

                if(prefixindex < 0)
                    parser = addndbent;	/* no prefix */
                else
                    parser = ndb_parse_funs[prefixindex];

                while(FGets(fh, buf, CONFIGLINELEN - 1)) {
                    line++;		/* maintain line number */
                    /* pass by white space */
                    for(p = buf; *p == ' ' || *p == '\t' || *p == '\r'; p++)
                        ;
                    rdargs->RDA_Source.CS_CurChr = p - buf;
                    if(*p == '#' || *p == ';' || *p == '\n')  /* only a comment line */
                        continue;
                    /* remove comments & calc length */
                    for(; *p; p++) {
                        if(*p == '#' || *p == ';') {
                            *p++ = '\n';
                            *p   = '\0';	/* terminate line */
                            break;
                        }
                    }
                    /* ensure that line ends with '\n' (ReadArgs() depends on it) */
                    if(*(p - 1) != '\n') {
                        *p++ = '\n';
                        *p   = '\0';
                    }
                    rdargs->RDA_Source.CS_Length = p - buf;
                    rdargs->RDA_Buffer = NULL;
                    rdargs->RDA_BufSiz = 0;
#if defined(__AROS__)
                    D(bug("[AROSTCP](netconfig) read_netdb: parsing line: %s, prefixindex=%ld\n", buf, prefixindex));
#endif
                    DNETDB(Printf("Processing line: %s, prefixindex=%ld\n", buf, prefixindex);)
                    retval = parser(ndb, rdargs, errstrp, res, flags);
                    if(retval == RETURN_OK)
                        continue;
                    if(retval != RETURN_WARN) {  /* severe error */
                        if(!ndb_userquit)
                            error_request("Fatal error in NetDB file %s at line %ld, col %ld\n%s\nAROSTCP will quit",
                                          (IPTR)fname, (IPTR)line, (IPTR)rdargs->RDA_Source.CS_CurChr, (IPTR)*errstrp);
                        break;
                    }
                    /* Log the error */
                    error_request("Error in NetDB file %s at line %ld, col %ld\n%s",
                                  (IPTR)fname, (IPTR)line, (IPTR)rdargs->RDA_Source.CS_CurChr, (IPTR)*errstrp);
                    __log(LOG_WARNING, "NetDB(%s) line %ld: %s before col %ld\n",
                          fname, (long)line, *errstrp, rdargs->RDA_Source.CS_CurChr);

                    warnval = retval;
                }
                /* Check file error */
                ioerr = IoErr();

                FreeDosObject(DOS_RDARGS, rdargs);
            }
            Close(fh);
        } else {
            ioerr = IoErr();
        }

        if(ioerr) {
#if defined(__AROS__)
            D(bug("[AROSTCP](netconfig) read_netdb: ioerror\n"));
#endif
            Fault(ioerr, "readnetdb", res->CS_Buffer, res->CS_Length);
            *errstrp = res->CS_Buffer;
            retval = RETURN_ERROR;
        }
        if((!fh) && (ndb_Lock.ss_NestCount == 1))
            error_request("Unable to open NetDB file %s\n%s", (IPTR)fname, (IPTR)*errstrp);

        /* return old current directory */
        if(lock) {
            CurrentDir(oldcd);
            UnLock(lock);
        }

        FreeMem(buf, CONFIGLINELEN);
    } else {
#if defined(__AROS__)
        D(bug("[AROSTCP](netconfig) read_netdb: Failed to allocate buffer\n"));
#endif
        *errstrp = ERR_MEMORY;
        retval = RETURN_FAIL;
    }

    UNLOCK_NDB(ndb);

    return retval > warnval ? retval : warnval;
}

/*
 * Parse the 'ADD' command
 */
static LONG
ncfg_netdb_add(struct CSource *csarg, UBYTE **errstrp, struct CSource *res)
{
    struct RDArgs *rdargs;
    LONG retval;

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) do_netdb()\n"));
#endif

    if(rdargs = AllocDosObject(DOS_RDARGS, NULL)) {
        /* initialize CSource of the rdargs */
        rdargs->RDA_Source = *csarg;
        /* initialize rest fields (see <dos/rdargs.h>) */
        rdargs->RDA_DAList = 0;
        rdargs->RDA_Buffer = NULL;
        rdargs->RDA_BufSiz = 0;
        rdargs->RDA_ExtHelp = NULL;
        rdargs->RDA_Flags = 0;

        LOCK_W_NDB(NDB);

        /* An interface line names one to bring up now, as at start;
         * an existing interface is left as it is. The stack is running:
         * a device that does not open is reported back, not asked about. */
        retval = addndbent(NDB, rdargs, errstrp, res,
                           NETDB_IFF_ADDNEW | NETDB_IFF_NOASK | NETDB_IFF_REPORT);

        UNLOCK_NDB(NDB);

        FreeDosObject(DOS_RDARGS, rdargs);
    } else
        retval = RETURN_FAIL;

    return retval;
}

/*
 * Read the Network Data Base at start-up (the locks and lists it is kept
 * under are the core's: init_netdb(), kern/amiga_netdb.c).
 */
static LONG
ncfg_netdb_init(void)
{
    UBYTE result[REPLYBUFLEN + 1]; /* for error returns */
    struct CSource res;
    UBYTE *errstr;
    LONG   retval;

    res.CS_Buffer = result;
    res.CS_Length = sizeof(result);
    res.CS_CurChr = 0;

    /* Allocate the NetDataBase */
    if(!(NDB = alloc_netdb(NULL))) {
        return RETURN_FAIL;
    }

    /* Read in the default data base file */
    retval = read_netdb(NDB, netdb_path, &errstr, &res, -1, NETDB_IFF_ADDNEW);
    if(retval == RETURN_WARN)
        retval = RETURN_OK;

    if(!retval)
        netdb_access_setup(NDB);

    return retval;
}

/*
 * A deferred interface line tried again (netdb_defer_timer(), the core,
 * with NDB locked): through addifent() as it would have been at startup,
 * so its addresses, DHCP and routes are set up the same way.
 */
static int
ncfg_defer_retry(const char *line, ULONG length, UBYTE **errstrp, struct CSource *res)
{
    struct RDArgs *rdargs;

    if((rdargs = AllocDosObjectTags(DOS_RDARGS, TAG_END)) == NULL)
        return 1;                   /* again at the next tick */
    rdargs->RDA_Source.CS_Buffer = (UBYTE *)line;
    rdargs->RDA_Source.CS_Length = length;
    rdargs->RDA_Source.CS_CurChr = 0;
    rdargs->RDA_DAList = 0;
    rdargs->RDA_Buffer = NULL;
    rdargs->RDA_BufSiz = 0;
    rdargs->RDA_ExtHelp = NULL;
    rdargs->RDA_Flags = 0;

    ndb_deferretry = TRUE;
    ndb_deferfailed = FALSE;
    addifent(NDB, rdargs, errstrp, res, NETDB_IFF_ADDNEW);
    ndb_deferretry = FALSE;

    FreeDosObject(DOS_RDARGS, rdargs);
    return ndb_deferfailed ? 1 : 0;
}

/*
 * Reset the NetDataBase
 */
static LONG
ncfg_netdb_reset(struct CSource *cs, UBYTE **errstrp, struct CSource *res)
{
    LONG retval;
    struct NetDataBase *newnetdb;

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) reset_netdb()\n"));
#endif

    /* Allocate a temporary NetDataBase */
    if(!(newnetdb = alloc_netdb(NULL))) {
        *errstrp = ERR_MEMORY;
        return RETURN_FAIL;
    }

    retval = read_netdb(newnetdb, netdb_path, errstrp, res, -1, 0);
    /* TODO: set flags here */

    if(retval == RETURN_OK) {
        netdb_access_setup(newnetdb);

        /* Now clear the old lists of the NDB */
        LOCK_W_NDB(NDB);

        free_netdb(NDB);
        NDB = newnetdb;

        UNLOCK_NDB(NDB);
        ndb_Serial++;
    } else {
        free_netdb(newnetdb);
    }

    return retval;
}

/*
 * Re-read the configuration database in place for a reload.  Like
 * reset_netdb(), but reads with NETDB_IFF_ADDNEW so interface lines are
 * re-applied through addifent() (an existing interface is reconfigured via its
 * MODIFYOLD path; a new one is created).  The one-time semaphore/DynDB/NDB
 * setup done by init_netdb() is NOT repeated - only the NDB content is swapped.
 */
static LONG
ncfg_netdb_reload(void)
{
    UBYTE result[REPLYBUFLEN + 1];
    struct CSource res;
    UBYTE *errstr = NULL;
    LONG retval;
    struct NetDataBase *newndb;

#if defined(__AROS__)
    D(bug("[AROSTCP](netconfig) netdb_reload()\n"));
#endif

    res.CS_Buffer = result;
    res.CS_Length = sizeof(result);
    res.CS_CurChr = 0;

    /* Drop stale deferred-interface retries from the old config before the
     * new one is read (new DEFER lines re-add themselves via addifent). */
    netdb_defer_forget(NULL);

    if(!(newndb = alloc_netdb(NULL)))
        return RETURN_FAIL;

    /* Start a new reload generation: addifent marks each interface it sees in
     * the config below, so sana_remove_unseen() can disable those dropped. */
    sana_reload_begin();

    /* ADDNEW creates interfaces that appeared in the new config; MODIFYOLD is
     * also needed so addifent re-applies config to interfaces that still exist
     * (at boot ADDNEW alone suffices because every interface is new).
     * NOASK: the stack is running, so a device that does not open is
     * skipped rather than stopping it for a requester. */
    retval = read_netdb(newndb, netdb_path, &errstr, &res, -1,
                        NETDB_IFF_ADDNEW | NETDB_IFF_MODIFYOLD | NETDB_IFF_NOASK);
    if(retval == RETURN_WARN)
        retval = RETURN_OK;

    if(retval == RETURN_OK) {
        netdb_access_setup(newndb);
        LOCK_W_NDB(NDB);
        free_netdb(NDB);
        NDB = newndb;
        UNLOCK_NDB(NDB);
        ndb_Serial++;
        /* Disable any interface no longer named in the new config. */
        sana_remove_unseen();
    } else {
        free_netdb(newndb);
    }

    return retval;
}

/* ======================================================================
 * db/services.d (from kern/amiga_svcmgr.c)
 * ====================================================================== */

/* Split "KEY = VALUE" in place, trimming surrounding whitespace.  Returns FALSE
 * for blank / comment (# or ;) / non-assignment lines. */
static BOOL svc_cfg_kv(char *line, char **key, char **val)
{
    char *p = line, *eq, *e;

    while(*p == ' ' || *p == '\t')
        p++;
    if(*p == '\0' || *p == '\n' || *p == '\r' || *p == '#' || *p == ';')
        return FALSE;

    eq = p;
    while(*eq && *eq != '=' && *eq != '\n')
        eq++;
    if(*eq != '=')
        return FALSE;

    *key = p;
    e = eq;                             /* terminate + rtrim the key */
    *eq = '\0';
    while(e > p && (e[-1] == ' ' || e[-1] == '\t'))
        *--e = '\0';

    p = eq + 1;                         /* value: ltrim then rtrim */
    while(*p == ' ' || *p == '\t')
        p++;
    *val = p;
    e = p;
    while(*e && *e != '\n' && *e != '\r')
        e++;
    while(e > p && (e[-1] == ' ' || e[-1] == '\t'))
        e--;
    *e = '\0';
    return TRUE;
}

/* Interpret a config value as a boolean (dflt when empty).  False = no / false
 * / 0 / off; anything else is true. */
static BOOL svc_cfg_bool(const char *v, BOOL dflt)
{
    char c;

    if(v == NULL || v[0] == '\0')
        return dflt;
    c = v[0];
    if(c == 'n' || c == 'N' || c == 'f' || c == 'F' || c == '0')
        return FALSE;
    if((c == 'o' || c == 'O') && (v[1] == 'f' || v[1] == 'F'))   /* "off" */
        return FALSE;
    return TRUE;
}

/* Read one service config file (the current dir is db/services.d) and bring the
 * service into line with it.  The service name is the file name; the file holds
 * its management config:
 *   Path=<executable>              (required)
 *   Order=<n>                      (bring-up priority; higher launches first)
 *   StopSig=<bit>                  (signal bit to stop it; default CTRL-C = 12)
 *   Policy=signal|restart|ignore   (reload policy; default signal)
 *   Enabled=yes|no                 (default yes)
 * An enabled service not yet running is launched; a disabled service that IS
 * running is stopped.  So a service is turned off by setting Enabled=no (it
 * takes effect at the next boot or reload) WITHOUT deleting its script.
 * The service records and processes are the core's (svcmgr_find(),
 * svcmgr_launch(), svcmgr_stop_one(), kern/amiga_svcmgr.c). */
static void svc_apply_cfg(CONST_STRPTR name)
{
    BPTR  fh;
    char  line[256];
    char  path[256];
    ULONG stopsig = 0, policy = NSRP_SIGNAL;
    BOOL  havepath = FALSE, enabled = TRUE;
    struct SvcProc *existing;

    if(name[0] == '\0' || name[0] == '.')   /* skip "", hidden, "." / ".." */
        return;

    fh = Open((STRPTR)name, MODE_OLDFILE);   /* relative to services.d */
    if(fh == BNULL)
        return;

    while(FGets(fh, (STRPTR)line, sizeof(line))) {
        char *key = NULL, *val = NULL;

        if(!svc_cfg_kv(line, &key, &val))
            continue;

        if(ncfg_keyeq(key, "Path")) {
            int i;
            for(i = 0; i < (int)sizeof(path) - 1 && val[i]; i++)
                path[i] = val[i];
            path[i]  = '\0';
            havepath = (i > 0);
        } else if(ncfg_keyeq(key, "StopSig")) {
            LONG n = 0;
            const char *v = val;
            while(*v >= '0' && *v <= '9')
                n = n * 10 + (*v++ - '0');
            if(n >= 0 && n < 32)
                stopsig = 1UL << n;
        } else if(ncfg_keyeq(key, "Policy")) {
            if(val[0] == 'r' || val[0] == 'R')
                policy = NSRP_RESTART;
            else if(val[0] == 'i' || val[0] == 'I')
                policy = NSRP_IGNORE;
            else
                policy = NSRP_SIGNAL;
        } else if(ncfg_keyeq(key, "Enabled")) {
            enabled = svc_cfg_bool(val, TRUE);
        }
        /* Order= sequences the scan (ncfg_services_scan), not this function;
         * Name= and ConfigTool= are UI-side keys (Network prefs). */
    }
    Close(fh);

    existing = svcmgr_find(name);
    if(enabled) {
        if(existing)                        /* already running */
            return;
        if(havepath)
            svcmgr_launch(name, path, stopsig, policy);
        else
            __log(LOG_ERR, "servicemgr: service '%s' has no Path", name);
    } else if(existing) {
        /* Disabled but running: keep the script, stop the service. */
        __log(LOG_NOTICE, "servicemgr: service '%s' disabled, stopping", name);
        svcmgr_stop_one(existing, FALSE);
    }
}

/* Read just a service file's Order= (launch priority, higher first; 0 when
 * absent).  The current dir is db/services.d. */
static LONG svc_read_order(CONST_STRPTR name)
{
    BPTR fh;
    char line[256];
    LONG order = 0;

    fh = Open((STRPTR)name, MODE_OLDFILE);
    if(fh == BNULL)
        return 0;
    while(FGets(fh, (STRPTR)line, sizeof(line))) {
        char *key = NULL, *val = NULL;

        if(!svc_cfg_kv(line, &key, &val))
            continue;
        if(ncfg_keyeq(key, "Order")) {
            const char *v = val;
            LONG n = 0;

            while(*v >= '0' && *v <= '9')
                n = n * 10 + (*v++ - '0');
            order = n;
        }
    }
    Close(fh);
    return order;
}

#define SVC_MAXSCAN 32

struct SvcScanEnt {
    char name[108];                 /* fib_FileName size */
    LONG order;
};

/* svcmgr_launch_all(), the core's, when the service manager runs */
static void ncfg_services_scan(void)
{
    BPTR dblock, svclock, old;
    struct FileInfoBlock *fib;
    struct SvcScanEnt *found;
    int nfound = 0, i, j;

    dblock = Lock(db_path, ACCESS_READ);
    if(dblock == BNULL)
        return;
    old = CurrentDir(dblock);
    /* One file per service under db/services.d/.  (db/services itself is the
     * IANA port-name database read by getservbyname, hence the ".d" dir name
     * to avoid colliding with it.) */
    svclock = Lock("services.d", ACCESS_READ);
    if(svclock == BNULL) {
        CurrentDir(old);
        UnLock(dblock);
        return;             /* no services.d -> no external service daemons */
    }

    found = AllocVec(sizeof(struct SvcScanEnt) * SVC_MAXSCAN,
                     MEMF_PUBLIC | MEMF_CLEAR);
    fib = AllocDosObject(DOS_FIB, NULL);
    if(fib != NULL && Examine(svclock, fib)) {
        CurrentDir(svclock);    /* so Open(name) resolves inside services.d */
        D(bug("[AROSTCP](netconfig) services: scanning db/services.d\n"));
        while(ExNext(svclock, fib)) {
            if(fib->fib_DirEntryType > 0)       /* skip subdirectories */
                continue;
            if(found != NULL && nfound < SVC_MAXSCAN) {
                int k;

                /* collect first: launches are sequenced by Order= below */
                for(k = 0; k < (int)sizeof(found[0].name) - 1 && fib->fib_FileName[k]; k++)
                    found[nfound].name[k] = fib->fib_FileName[k];
                found[nfound].name[k] = '\0';
                found[nfound].order = svc_read_order(found[nfound].name);
                nfound++;
            } else
                svc_apply_cfg(fib->fib_FileName);   /* overflow: scan order */
        }
        /* Launch highest Order first; insertion sort keeps equal orders in
         * scan order. */
        for(i = 1; i < nfound; i++) {
            struct SvcScanEnt tmp = found[i];

            for(j = i; j > 0 && found[j - 1].order < tmp.order; j--)
                found[j] = found[j - 1];
            found[j] = tmp;
        }
        for(i = 0; i < nfound; i++)
            svc_apply_cfg(found[i].name);
    }
    if(fib != NULL)
        FreeDosObject(DOS_FIB, fib);
    FreeVec(found);
    CurrentDir(old);
    UnLock(svclock);
    UnLock(dblock);
}

/* ======================================================================
 * The module
 * ====================================================================== */

static const struct netconfig_ops netconfig_ops = {
    sizeof(struct netconfig_ops),
    ncfg_readconfig,
    ncfg_parsefile,
    ncfg_netdb_init,
    ncfg_netdb_reload,
    ncfg_netdb_add,
    ncfg_netdb_reset,
    ncfg_defer_retry,
    ncfg_services_scan
};

/*
 * Not while an interface line marked DEFER waits: the stack's timer
 * applies it through this module when its device turns up, and does not
 * load a library itself (kern/netconfig_stubs.c).
 */
static int
netconfig_can_stop(struct netmodule *nm)
{
    return (netdb_defer_pending() ? EBUSY : 0);
}

static int
netconfig_start(struct netmodule *nm)
{
    if(!ncfg_libs_open()) {
        ncfg_libs_close();
        return (ENOMEM);
    }
    /* once the table is in the slot, reads come */
    return (netdemand_set(NETCONFIG_MODULE, &netconfig_ops));
}

static int
netconfig_stop(struct netmodule *nm)
{
    int error;

    /* refused, with nothing changed, while a line waits or a task is inside */
    if((error = netconfig_can_stop(nm)) != 0 ||
            (error = netdemand_set(NETCONFIG_MODULE, NULL)) != 0)
        return (error);
    /* the static data as the file has it */
    ndb_userquit = FALSE;
    ndb_deferretry = FALSE;
    ndb_deferfailed = FALSE;
    ncfg_libs_close();
    return (0);
}

/* the stack ends and the module did not stop: what it opened goes */
static void
netconfig_shutdown(struct netmodule *nm)
{
    ncfg_libs_close();
}

/*
 * The generic test switch AROSTCP/Without_netconfig keeps it out
 * (netmodule_leaveout_name()); it needs no variable of its own.
 */
NETMODULE_DESCRIPTOR(netconfig_module) = {
    sizeof(struct netmodule), NETMODULE_VERSION, NETCONFIG_MODULE, 0,
    netconfig_start, netconfig_stop, netconfig_can_stop,
    0, NULL,
    NULL,           /* no domain */
    NULL,           /* needs no other module */
    NULL,           /* nm_leaveout: the generic switch */
    NULL,           /* nm_library: the loader's */
    netconfig_shutdown
};
NETMODULE_BUILTIN(netconfig_module, 60)
