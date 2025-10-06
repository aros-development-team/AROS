/*
 * groupunit.c --- unit for groups
 *
 * Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-Group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "entries.h"

#include <string.h>
#if defined(DEBUG)
#include <assert.h>
#include "assert.h"
#endif

static void *CopyEntToGroup(struct NetInfoReq *req, struct Ent *e);
static struct Ent *CopyGroupToEnt(struct NetInfoDevice *nid, struct NetInfoReq *req);
static struct Ent *ParseGroup(struct NetInfoDevice *nid, register UBYTE *p);
static int PrintGroup(struct NetInfoDevice *nid, BPTR file, struct Ent *e);
static void GetGroups(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim);

const static struct MapMethods group_methods[1] = {
    {
        /* Other methods */
        ParseGroup,
        PrintGroup,
        CopyEntToGroup,
        CopyGroupToEnt,
        EntCleanup,
        GetGroups,
        EntHandleNotify,
    },
};

/*
 * Allocate and initialize group map
 */
struct NetInfoMap *InitGroupMap(struct NetInfoDevice *nid)
{
    struct NetInfoMap *nim = NULL;

    D(bug("[NetInfo] %s()\n", __func__));

    if (nid->nid_dbgroup) {
        nim = AllocVec(sizeof(*nim), MEMF_CLEAR|MEMF_PUBLIC);
        if (nim) {
            nim->nim_Methods = group_methods;
            nim->nim_Name = "group";
            nim->nim_Filename = nid->nid_dbgroup;
        }
    }
    return nim;
}

/*
 * Copy an GroupEnt structure to group structure
 */
static void *CopyEntToGroup(struct NetInfoReq *req, struct Ent *e)
{
    struct GroupEnt *ge = (struct GroupEnt *)e;
    struct NetInfoGroup *gr = req->io_Data;
    STRPTR *mto = (STRPTR *)(gr + 1);
    UBYTE  *to = (UBYTE *)(mto + ge->ge_nmembers);

    ULONG size = req->io_Length;
    ULONG actual = sizeof(*gr) + ge->ge_tlen + ge->ge_nmembers * sizeof(UBYTE *);

    UBYTE **mfrom;

    D(bug("[NetInfo] %s()\n", __func__));

    req->io_Actual = actual;

    if (size < actual) {
        return NULL;
    }

    /* copy group strings */
    to = stpcopy(gr->gr_name = to, ge->ge_group->gr_name);
    to = stpcopy(gr->gr_passwd = to, ge->ge_group->gr_passwd);
    /* .. and id */
    gr->gr_gid = ge->ge_group->gr_gid;

    /* .. and members */
    for (gr->gr_mem = mto, mfrom = ge->ge_group->gr_mem;
            *mfrom && (*mto++ = to); ) {
        to = stpcopy(to, *mfrom++);
    }
    *mto = NULL;
#if defined(DEBUG)
    assert(to <= (UBYTE *)req->io_Data + req->io_Length);
#endif

    return gr;
}

/*
 * Copy an group structure into a internal entry
 */
static struct Ent *CopyGroupToEnt(struct NetInfoDevice *nid, struct NetInfoReq *req)
{
    struct NetInfoGroup *gr = (struct NetInfoGroup*)req->io_Data;
    struct GroupEnt *ge;
    UBYTE *to, **mem, **mto;
    short nmem = 1;
    ULONG txtlen = 0;

    D(bug("[NetInfo] %s()\n", __func__));

    /*
    * These cause EFAULT
    */
    if (gr == NULL || (1 & (IPTR)gr) != 0 ||
            gr->gr_name == NULL ||
            gr->gr_passwd == NULL ||
            gr->gr_mem == NULL || (1 & (IPTR)gr->gr_mem)) {
        req->io_Error = IOERR_BADADDRESS;
        return NULL;
    }

    for (mem = gr->gr_mem; *mem; mem++, nmem++) {
        if (*mem == NULL) {
            req->io_Error = IOERR_BADADDRESS;
            return NULL;
        }
        txtlen += strlen(*mem) + 1;
    }

    txtlen += strlen(gr->gr_name) + strlen(gr->gr_passwd) + 2;

    ge = AllocVec(sizeof(*ge) + nmem * sizeof(*mem) + txtlen, MEMF_CLEAR);
    if (ge == NULL) {
        req->io_Error = NIERR_NOMEM;
        return NULL;
    }

    ge->ge_node.ln_Type = ENT_GROUP;
    ge->ge_node.ln_Pri  = ENT_CHANGED;
    ge->ge_tlen = txtlen;
    ge->ge_group->gr_gid = gr->gr_gid;

    ge->ge_nmembers = nmem;
    ge->ge_group->gr_mem = mto = (UBYTE **)(ge + 1);
    to = (UBYTE *)(mto + nmem);
    /* Copy normal strings.. */
    to = stpcopy(ge->ge_group->gr_name   = to, gr->gr_name);
    to = stpcopy(ge->ge_group->gr_passwd = to, gr->gr_passwd);
    /* ..and members */
    for (mem = gr->gr_mem; *mem; mem++) {
        to = stpcopy(*mto++ = to, *mem);
        nmem--;
    }
    *mem = NULL;
#if defined(DEBUG)
    assert(to == ge->ge_group->gr_name + txtlen);
    assert(nmem == 1);
#endif
    return (struct Ent *)ge;
}

/****** netinfo.device/group ***********************************************

   NAME
        group - format of the group permissions file

   DESCRIPTION
        The file <AmiTCP:db/group> consists of newline separated ASCII
        records, one per group, containing four bar `|' separated fields.
        These fields are as follows:

              group     Name of the group.
              passwd    Group's encrypted password.
              gid       The group's decimal ID.
              member    Group members.

        The group field is the group name used for granting file access to
        users who are members of the group.  The gid field is the number
        associated with the group name.  They should both be unique across
        the system (and often across a group of systems) since they control
        file access.  The passwd field is an optional encrypted password.
        This field is rarely used and an asterisk is normally placed in it
        rather than leaving it blank.  The member field contains the names
        of users granted the priviledges of group. The member names are
        separated by commas with out spaces or newlines.  A user is
        automatically in a group if that group was specified in their
        AmiTCP:db/passwd entry and does not need to be added to that group
        in the AmiTCP:db/group file.

   FILES
        AmiTCP:db/group

   SEE ALSO
        passwd

   HISTORY
        A group file format appeared in Version 6 AT&T UNIX.

****************************************************************************
*/

/*
 * Parse the group entry
 */
static struct Ent *ParseGroup(struct NetInfoDevice *nid, register UBYTE *p)
{
    int l;
    UBYTE *np, *name, *pass, *id_s, *memlist, **members;
    ULONG nmembers;
    LONG gid, txtlen = 0;

    struct GroupEnt *ge;

    D(bug("[NetInfo] %s()\n", __func__));

    np = p;

    name = strsep((char **)&np, "|\n");
    pass = strsep((char **)&np, "|\n");
    id_s = strsep((char **)&np, "|\n");
    memlist = np;
    if ((l = StrToLong(id_s, &gid)) <= 0 || id_s[l] != '\0')
        return NULL;
    for (nmembers = 1; (p = strsep((char **)&np, ", \n")) && np;) {
        if (*p) nmembers++;
        else txtlen--;
        if (*np == '\0')
            break;
    }

#if defined(DEBUG)
    assert(np != NULL);
#endif

    if (!name || !pass || !id_s || !np)
        return NULL;

    txtlen += np - name - (memlist - id_s);
    ge = AllocVec(sizeof(*ge) + nmembers * sizeof(UBYTE*) + txtlen, MEMF_CLEAR);
    if (!ge)
        return NULL;

    ge->ge_node.ln_Type = ENT_GROUP;

    ge->ge_nmembers = nmembers;
    ge->ge_tlen = txtlen;

    members = (UBYTE **)(ge + 1);
    p = (UBYTE *)(members + nmembers);

    /* Copy name */
    p = stpcopy(ge->ge_group->gr_name = p, name);
    p = stpcopy(ge->ge_group->gr_passwd = p, pass);

    ge->ge_group->gr_gid = gid;
    ge->ge_group->gr_mem = members;

    /* Copy the memberlist */
    while (nmembers > 1) {
        if (*memlist) {
            nmembers--;
            *members++ = p;
            while (*p++ = *memlist++)
                ;
        } else {
            memlist++;
        }
    }
    *members = NULL;

#if defined(DEBUG)
    assert(p == ge->ge_group->gr_name + txtlen);
#endif

    return (struct Ent *)ge;
}

/*
 * Print out an group entry
 */
static int PrintGroup(struct NetInfoDevice *nid, BPTR file, struct Ent *e)
{
    struct GroupEnt *ge = (struct GroupEnt *)e;
    UBYTE **member = ge->ge_group->gr_mem;
    UBYTE *fmt = "%s";

    D(bug("[NetInfo] %s()\n", __func__));

    VFPrintf(file, "%s|%s|%ld|", (RAWARG)ge->ge_group);
    while (*member) {
        FPrintf(file, fmt, *member++);
        fmt = ",%s";
    }
    FPutC(file, '\n');

    return 0;
}

/*
 * Execute NI_MEMBERS command
 */
static void GetGroups(struct NetInfoDevice *nid, struct NetInfoReq *req, struct NetInfoMap *nim)
{
    int maxgroups = req->io_Length / sizeof(LONG);
    LONG *groups = (LONG *)req->io_Data;
    STRPTR uname;
    register int ngroups = 0;
    register struct GroupEnt *ge;
    register int i;
    BYTE retval = 0;

    D(bug("[NetInfo] %s()\n", __func__));

#if __WORDSIZE == 32
    uname = (STRPTR)req->io_Offset;
#else
    uname = (STRPTR)(((IPTR)req->io_Offset << 32) | req->io_Actual);
#endif

    ge = (struct GroupEnt *)InternalSetEnts(nid, nim);

    while (ge = (struct GroupEnt *)GetNextEnt((struct Ent *)ge)) {
        for (i = 0; ge->ge_group->gr_mem[i]; i++)
            if (strcmp(ge->ge_group->gr_mem[i], uname) == 0) {
                if (ngroups == maxgroups) {
                    retval = NIERR_TOOSMALL;
                    goto toomany;
                }
                groups[ngroups++] = ge->ge_group->gr_gid;
            }
    }

toomany:
    InternalEndEnts(nid, nim);

    req->io_Actual = ngroups * sizeof(LONG);
    req->io_Error = retval;
    TermIO(req);
}
