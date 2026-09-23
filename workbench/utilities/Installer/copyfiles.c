/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

/* copyfiles.c -- (copyfiles), (copylib) and the file helpers behind
   (getversion), (getsum), (foreach), (patmatch), (protect), (textfile)
   and (tooltype). The tree walk is planned first and executed second so
   a destination inside the source is never re-copied and the gauge knows
   the total up front. */

#include "Installer.h"
#include "execute.h"
#include "misc.h"
#include "gui.h"
#include "variables.h"
#include "cleanup.h"
#include "copyfiles.h"

#include <dos/dosasl.h>
#include <exec/libraries.h>

/* External variables */
extern InstallerPrefs preferences;
extern int error;

#define COPYBUFSIZE     65536
#define VERWINDOW       256     /* bytes read after "$VER:" */

/* One planned copy */
struct job
{
    char *src, *dst;
    int isdir;
    ULONG prot;
    struct DateStamp ds;
    char *comment;
};

struct copyctx
{
    struct ParameterList *pl;
    int copyfail, copyflags;    /* local (optional)/(delopts) view */
    int infos, fonts, nogauge;
    int all, files, choices;
    char *pattern;              /* parsed (ParsePatternNoCase) or NULL */
    struct job *job;
    int njobs, capjobs, nfiles;
};

/* One directory entry, as listed by list_dir() */
struct entry
{
    char *name;
    LONG type;
    ULONG prot;
    struct DateStamp ds;
    char *comment;
};


/* ---------------------------------------------------------------------- */
/* Small helpers                                                          */

static char *join_path(const char *dir, const char *name)
{
char *out;

    out = malloc(strlen(dir) + strlen(name) + 2);
    outofmem(out);
    strcpy(out, dir);
    if (!AddPart(out, (STRPTR)name, strlen(dir) + strlen(name) + 2))
    {
        free(out);
        return NULL;
    }
    return out;
}

static int ends_with_info(const char *name)
{
int l = strlen(name);

    return (l > 5 && strcasecmp(name + l - 5, ".info") == 0);
}

/* fib_DirEntryType of a path, 0 if it does not exist; fills fib if given */
static LONG entry_type(const char *path, struct FileInfoBlock *fib)
{
BPTR lock;
LONG type = 0;
struct FileInfoBlock *ownfib = NULL;

    lock = Lock((STRPTR)path, SHARED_LOCK);
    if (lock == BNULL)
    {
        return 0;
    }
    if (fib == NULL)
    {
        ownfib = AllocDosObject(DOS_FIB, NULL);
        fib = ownfib;
    }
    if (fib != NULL && Examine(lock, fib))
    {
        type = fib->fib_DirEntryType;
    }
    UnLock(lock);
    if (ownfib)
    {
        FreeDosObject(DOS_FIB, ownfib);
    }
    return type;
}

static void transcript(const char *a, const char *b, const char *c, const char *d)
{
    if (preferences.transcriptstream == BNULL)
    {
        return;
    }
    if (a) Write(preferences.transcriptstream, (STRPTR)a, strlen(a));
    if (b) Write(preferences.transcriptstream, (STRPTR)b, strlen(b));
    if (c) Write(preferences.transcriptstream, (STRPTR)c, strlen(c));
    if (d) Write(preferences.transcriptstream, (STRPTR)d, strlen(d));
}

/*
 * List a directory. Returns the number of entries, -1 on error.
 * The array and its strings are malloc()ed; free with free_entries().
 */
static int list_dir(const char *dir, struct entry **out)
{
BPTR lock;
struct FileInfoBlock *fib;
struct entry *e = NULL;
int n = 0, cap = 0;

    *out = NULL;
    lock = Lock((STRPTR)dir, SHARED_LOCK);
    if (lock == BNULL)
    {
        return -1;
    }
    fib = AllocDosObject(DOS_FIB, NULL);
    if (fib == NULL || !Examine(lock, fib) || fib->fib_DirEntryType <= 0)
    {
        if (fib) FreeDosObject(DOS_FIB, fib);
        UnLock(lock);
        return -1;
    }
    while (ExNext(lock, fib))
    {
        if (n == cap)
        {
            cap = cap ? cap * 2 : 32;
            e = realloc(e, cap * sizeof(struct entry));
            outofmem(e);
        }
        e[n].name = strdup(fib->fib_FileName);
        outofmem(e[n].name);
        e[n].type = fib->fib_DirEntryType;
        e[n].prot = fib->fib_Protection;
        e[n].ds = fib->fib_Date;
        e[n].comment = fib->fib_Comment[0] ? strdup(fib->fib_Comment) : NULL;
        n++;
    }
    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);
    *out = e;
    return n;
}

static void free_entries(struct entry *e, int n)
{
int i;

    for (i = 0 ; i < n ; i++)
    {
        free(e[i].name);
        free(e[i].comment);
    }
    free(e);
}

static struct entry *find_entry(struct entry *e, int n, const char *name)
{
int i;

    for (i = 0 ; i < n ; i++)
    {
        if (strcasecmp(e[i].name, name) == 0)
        {
            return &e[i];
        }
    }
    return NULL;
}


/* ---------------------------------------------------------------------- */
/* Planning                                                               */

static void add_job(struct copyctx *ctx, const char *src, const char *dst, int isdir, struct entry *e)
{
struct job *j;
int i;

    /* A destination that lies inside the source must not be planned twice */
    for (i = 0 ; i < ctx->njobs ; i++)
    {
        if (strcasecmp(ctx->job[i].dst, dst) == 0)
        {
            return;
        }
    }
    if (ctx->njobs == ctx->capjobs)
    {
        ctx->capjobs = ctx->capjobs ? ctx->capjobs * 2 : 64;
        ctx->job = realloc(ctx->job, ctx->capjobs * sizeof(struct job));
        outofmem(ctx->job);
    }
    j = &ctx->job[ctx->njobs++];
    j->src = strdup(src);
    outofmem(j->src);
    j->dst = strdup(dst);
    outofmem(j->dst);
    j->isdir = isdir;
    j->prot = e ? e->prot : 0;
    if (e)
    {
        j->ds = e->ds;
    }
    else
    {
        memset(&j->ds, 0, sizeof(j->ds));
    }
    j->comment = (e && e->comment) ? strdup(e->comment) : NULL;
    if (!isdir)
    {
        ctx->nfiles++;
    }
}

static void plan_dir(struct copyctx *ctx, const char *src, const char *dst);

/* Plan one entry of srcdir; siblings are the listing it came from so
   (infos) and (fonts) can find their companions without extra Lock()s */
static void plan_entry(struct copyctx *ctx, const char *srcdir, const char *dstdir,
                       struct entry *e, struct entry *siblings, int nsiblings)
{
char *s, *d, *companion;
struct entry *ce;

    s = join_path(srcdir, e->name);
    d = join_path(dstdir, e->name);
    if (s == NULL || d == NULL)
    {
        free(s);
        free(d);
        return;
    }
    if (e->type > 0)
    {
        plan_dir(ctx, s, d);
    }
    else
    {
        add_job(ctx, s, d, FALSE, e);
        if (ctx->fonts)
        {
            /* "Foo.font" brings the "Foo" drawer with it */
            int l = strlen(e->name);
            if (l > 5 && strcasecmp(e->name + l - 5, ".font") == 0)
            {
                companion = strdup(e->name);
                outofmem(companion);
                companion[l - 5] = 0;
                ce = find_entry(siblings, nsiblings, companion);
                if (ce && ce->type > 0)
                {
                    char *cs = join_path(srcdir, companion);
                    char *cd = join_path(dstdir, companion);
                    if (cs && cd)
                    {
                        plan_dir(ctx, cs, cd);
                    }
                    free(cs);
                    free(cd);
                }
                free(companion);
            }
        }
    }
    if (ctx->infos && !ends_with_info(e->name))
    {
        companion = malloc(strlen(e->name) + 6);
        outofmem(companion);
        sprintf(companion, "%s.info", e->name);
        ce = find_entry(siblings, nsiblings, companion);
        if (ce && ce->type < 0)
        {
            char *cs = join_path(srcdir, companion);
            char *cd = join_path(dstdir, companion);
            if (cs && cd)
            {
                add_job(ctx, cs, cd, FALSE, ce);
            }
            free(cs);
            free(cd);
        }
        free(companion);
    }
    free(s);
    free(d);
}

/* A directory that was selected is copied whole (minus icons unless (infos)) */
static void plan_dir(struct copyctx *ctx, const char *src, const char *dst)
{
struct entry *e;
struct FileInfoBlock *fib;
struct entry self;
int n, i;

    fib = AllocDosObject(DOS_FIB, NULL);
    outofmem(fib);
    if (entry_type(src, fib) <= 0)
    {
        FreeDosObject(DOS_FIB, fib);
        return;
    }
    self.name = fib->fib_FileName;
    self.type = fib->fib_DirEntryType;
    self.prot = fib->fib_Protection;
    self.ds = fib->fib_Date;
    self.comment = fib->fib_Comment[0] ? fib->fib_Comment : NULL;
    add_job(ctx, src, dst, TRUE, &self);
    FreeDosObject(DOS_FIB, fib);

    n = list_dir(src, &e);
    for (i = 0 ; i < n ; i++)
    {
        if (ends_with_info(e[i].name) && !ctx->infos)
        {
            continue;
        }
        if (e[i].type > 0)
        {
            char *s = join_path(src, e[i].name);
            char *d = join_path(dst, e[i].name);
            if (s && d)
            {
                plan_dir(ctx, s, d);
            }
            free(s);
            free(d);
        }
        else
        {
            char *s = join_path(src, e[i].name);
            char *d = join_path(dst, e[i].name);
            if (s && d)
            {
                add_job(ctx, s, d, FALSE, &e[i]);
            }
            free(s);
            free(d);
        }
    }
    if (n > 0)
    {
        free_entries(e, n);
    }
}

static void free_jobs(struct copyctx *ctx)
{
int i;

    for (i = 0 ; i < ctx->njobs ; i++)
    {
        free(ctx->job[i].src);
        free(ctx->job[i].dst);
        free(ctx->job[i].comment);
    }
    free(ctx->job);
    ctx->job = NULL;
    ctx->njobs = ctx->capjobs = ctx->nfiles = 0;
}

/*
 * (optional ...) and (delopts ...) given to this command modify the
 * global copy options for this command only.
 */
static void local_copyopts(struct ParameterList *pl, int *fail, int *flags)
{
int i;

    *fail = preferences.copyfail;
    *flags = preferences.copyflags;
    for (i = 0 ; i < GetPL(pl, _OPTIONAL).intval ; i++)
    {
        char *o = GetPL(pl, _OPTIONAL).arg[i];
        if (strcasecmp(o, "fail") == 0)             *fail = COPY_FAIL;
        else if (strcasecmp(o, "nofail") == 0)      *fail = COPY_NOFAIL;
        else if (strcasecmp(o, "oknodelete") == 0)  *fail = COPY_OKNODELETE;
        else if (strcasecmp(o, "force") == 0)       *flags |= COPY_FORCE;
        else if (strcasecmp(o, "askuser") == 0)     *flags |= COPY_ASKUSER;
    }
    for (i = 0 ; i < GetPL(pl, _DELOPTS).intval ; i++)
    {
        char *o = GetPL(pl, _DELOPTS).arg[i];
        if (strcasecmp(o, "nofail") == 0 || strcasecmp(o, "oknodelete") == 0)
        {
            *fail = COPY_FAIL;
        }
        else if (strcasecmp(o, "force") == 0)       *flags &= ~COPY_FORCE;
        else if (strcasecmp(o, "askuser") == 0)     *flags &= ~COPY_ASKUSER;
    }
    /* A novice is never asked */
    if (get_var_int("@user-level") == _NOVICE)
    {
        *flags &= ~COPY_ASKUSER;
    }
}


/* ---------------------------------------------------------------------- */
/* Execution                                                              */

#define COPY_OK         0
#define COPY_SKIPPED    1
#define COPY_FAILED     2

/* Make sure a directory exists; returns COPY_OK/COPY_FAILED */
static int ensure_dir(const char *dir, int *created)
{
BPTR lock;
LONG type;

    *created = FALSE;
    type = entry_type(dir, NULL);
    if (type > 0)
    {
        return COPY_OK;
    }
    if (type < 0)
    {
        SetIoErr(ERROR_OBJECT_EXISTS);
        return COPY_FAILED;
    }
    lock = CreateDir((STRPTR)dir);
    if (lock == BNULL)
    {
        return COPY_FAILED;
    }
    UnLock(lock);
    *created = TRUE;
    manifest_log('D', dir);
    transcript("Created directory \"", dir, "\".\n", NULL);
    return COPY_OK;
}

/* Copy one file, cloning protection bits, comment and date */
static int copy_one(struct copyctx *ctx, struct job *j)
{
BPTR in, out;
struct FileInfoBlock *fib;
LONG type, n, err = 0;
char *buf;

    fib = AllocDosObject(DOS_FIB, NULL);
    outofmem(fib);
    type = entry_type(j->dst, fib);
    if (type > 0)
    {
        FreeDosObject(DOS_FIB, fib);
        SetIoErr(ERROR_OBJECT_EXISTS);
        return COPY_FAILED;
    }
    if (type < 0)
    {
        /* Destination exists */
        if (ctx->copyflags & COPY_ASKUSER)
        {
            char *msg = malloc(strlen(j->dst) + 64);
            int yes;
            outofmem(msg);
            sprintf(msg, "\"%s\" already exists.\nReplace it?", j->dst);
            yes = request_yesno(msg, ctx->pl, TRUE);
            free(msg);
            if (!yes)
            {
                FreeDosObject(DOS_FIB, fib);
                transcript("Kept existing \"", j->dst, "\".\n", NULL);
                return COPY_SKIPPED;
            }
        }
        if (fib->fib_Protection & FIBF_DELETE)
        {
            if (ctx->copyflags & COPY_FORCE)
            {
                SetProtection((STRPTR)j->dst, 0);
            }
            else
            {
                FreeDosObject(DOS_FIB, fib);
                SetIoErr(ERROR_DELETE_PROTECTED);
                return COPY_FAILED;
            }
        }
    }
    FreeDosObject(DOS_FIB, fib);

    in = Open((STRPTR)j->src, MODE_OLDFILE);
    if (in == BNULL)
    {
        return COPY_FAILED;
    }
    out = Open((STRPTR)j->dst, MODE_NEWFILE);
    if (out == BNULL)
    {
        err = IoErr();
        Close(in);
        SetIoErr(err);
        return COPY_FAILED;
    }
    buf = malloc(COPYBUFSIZE);
    outofmem(buf);
    while ((n = Read(in, buf, COPYBUFSIZE)) > 0)
    {
        if (Write(out, buf, n) != n)
        {
            err = IoErr();
            break;
        }
    }
    if (n < 0)
    {
        err = IoErr();
    }
    free(buf);
    Close(out);
    Close(in);
    if (err)
    {
        DeleteFile((STRPTR)j->dst);
        SetIoErr(err);
        return COPY_FAILED;
    }
    SetProtection((STRPTR)j->dst, j->prot & ~FIBF_ARCHIVE);
    if (j->comment)
    {
        SetComment((STRPTR)j->dst, (STRPTR)j->comment);
    }
    if (j->ds.ds_Days || j->ds.ds_Minute || j->ds.ds_Tick)
    {
        SetFileDate((STRPTR)j->dst, &j->ds);
    }
    manifest_log('F', j->dst);
    transcript("Copied \"", j->src, "\" to \"", j->dst);
    transcript("\".\n", NULL, NULL, NULL);
    return COPY_OK;
}

/* Run the plan. Returns 1 if everything asked for is in place. */
static int run_jobs(struct copyctx *ctx)
{
int i, done = 0, failed = 0, created, r;

    for (i = 0 ; i < ctx->njobs ; i++)
    {
        struct job *j = &ctx->job[i];

        if (preferences.pretend)
        {
            transcript(j->isdir ? "Would create directory \"" : "Would copy to \"", j->dst, "\".\n", NULL);
            continue;
        }
        if (!ctx->nogauge && !j->isdir)
        {
            update_copying(j->dst, done);
        }
        if (j->isdir)
        {
            r = ensure_dir(j->dst, &created);
        }
        else
        {
            r = copy_one(ctx, j);
            done++;
        }
        if (r == COPY_FAILED)
        {
            failed++;
            set_variable("@ioerr", NULL, IoErr());
            transcript("Failed to copy \"", j->src, "\" to \"", j->dst);
            transcript("\".\n", NULL, NULL, NULL);
            if (ctx->copyfail == COPY_FAIL)
            {
                error = DOSERROR;
                traperr("Could not copy \"%s\"!\n", j->src);
            }
        }
    }
    if (!ctx->nogauge && ctx->nfiles && !preferences.pretend)
    {
        update_copying("", done);
    }
    return failed == 0;
}


/* ---------------------------------------------------------------------- */
/* (copyfiles)                                                            */

int do_copyfiles(struct ParameterList *pl)
{
struct copyctx ctx;
char *source, *dest, *patbuf = NULL;
LONG stype;
int result = 0, i, single;

    memset(&ctx, 0, sizeof(ctx));
    ctx.pl = pl;

    if (GetPL(pl, _SOURCE).used != 1 || GetPL(pl, _SOURCE).intval < 1)
    {
        error = SCRIPTERROR;
        traperr("<copyfiles> requires (source)!\n", NULL);
    }
    if (GetPL(pl, _DEST).used != 1 || GetPL(pl, _DEST).intval < 1)
    {
        error = SCRIPTERROR;
        traperr("<copyfiles> requires (dest)!\n", NULL);
    }
    source = GetPL(pl, _SOURCE).arg[0];
    dest = GetPL(pl, _DEST).arg[0];

    local_copyopts(pl, &ctx.copyfail, &ctx.copyflags);
    ctx.infos = GetPL(pl, _INFOS).used;
    ctx.fonts = GetPL(pl, _FONTS).used;
    ctx.nogauge = GetPL(pl, _NOGAUGE).used;
    ctx.all = GetPL(pl, _ALL).used;
    ctx.files = GetPL(pl, _FILES).used;
    ctx.choices = GetPL(pl, _CHOICES).used;
    if (GetPL(pl, _PATTERN).used && GetPL(pl, _PATTERN).intval > 0)
    {
        char *pat = GetPL(pl, _PATTERN).arg[0];
        patbuf = malloc(strlen(pat) * 2 + 2);
        outofmem(patbuf);
        if (ParsePatternNoCase((STRPTR)pat, patbuf, strlen(pat) * 2 + 2) < 0)
        {
            error = BADPARAMETER;
            traperr("Bad pattern \"%s\"!\n", pat);
        }
        ctx.pattern = patbuf;
    }

    stype = entry_type(source, NULL);
    if (stype == 0)
    {
        set_variable("@ioerr", NULL, IoErr());
        transcript("Source \"", source, "\" not found.\n", NULL);
        free(patbuf);
        if (ctx.copyfail == COPY_FAIL)
        {
            error = DOSERROR;
            traperr("Source \"%s\" not found!\n", source);
        }
        return 0;
    }
    single = (stype < 0);

    if (single)
    {
        /* One file into the destination directory */
        struct entry *e;
        char *srcdir, *name, *d;
        int n;

        name = FilePart((STRPTR)source);
        srcdir = strdup(source);
        outofmem(srcdir);
        *(PathPart(srcdir)) = 0;
        n = list_dir(srcdir[0] ? srcdir : "", &e);
        d = join_path(dest, GetPL(pl, _NEWNAME).used && GetPL(pl, _NEWNAME).intval > 0
                            ? GetPL(pl, _NEWNAME).arg[0] : name);
        if (d)
        {
            struct entry *self = find_entry(e, n, name);
            int created;
            /* the destination directory is created on demand */
            if (!preferences.pretend && ensure_dir(dest, &created) == COPY_FAILED)
            {
                set_variable("@ioerr", NULL, IoErr());
                if (ctx.copyfail == COPY_FAIL)
                {
                    error = DOSERROR;
                    traperr("Could not create \"%s\"!\n", dest);
                }
            }
            else
            {
                add_job(&ctx, source, d, FALSE, self);
                if (ctx.infos && self)
                {
                    char *si = malloc(strlen(source) + 6), *di = malloc(strlen(d) + 6);
                    outofmem(si);
                    outofmem(di);
                    sprintf(si, "%s.info", source);
                    sprintf(di, "%s.info", d);
                    if (entry_type(si, NULL) < 0)
                    {
                        add_job(&ctx, si, di, FALSE, find_entry(e, n, FilePart(si)));
                    }
                    free(si);
                    free(di);
                }
            }
            free(d);
        }
        if (n > 0)
        {
            free_entries(e, n);
        }
        free(srcdir);
        /* (confirm) on a single file is a plain proceed/skip */
        if (ctx.njobs && GetPL(pl, _CONFIRM).used == 1 && !request_confirm(pl))
        {
            free_jobs(&ctx);
        }
    }
    else
    {
        /* Select the top-level entries, then confirm the list if asked */
        struct entry *e;
        char **names = NULL, *sel = NULL;
        int n, nsel = 0;

        n = list_dir(source, &e);
        if (n < 0)
        {
            n = 0;
        }
        names = malloc((n + GetPL(pl, _CHOICES).intval + 1) * sizeof(char *));
        outofmem(names);
        if (ctx.choices)
        {
            for (i = 0 ; i < GetPL(pl, _CHOICES).intval ; i++)
            {
                names[nsel++] = GetPL(pl, _CHOICES).arg[i];
            }
        }
        else
        {
            for (i = 0 ; i < n ; i++)
            {
                if (ends_with_info(e[i].name) && !ctx.infos)
                {
                    continue;
                }
                if (ctx.infos && ends_with_info(e[i].name) && !ctx.pattern)
                {
                    /* icons ride with their file, never on their own */
                    continue;
                }
                if (e[i].type > 0 && (ctx.files || (!ctx.all && !ctx.pattern)))
                {
                    continue;
                }
                if (ctx.pattern && !MatchPatternNoCase(ctx.pattern, e[i].name))
                {
                    continue;
                }
                names[nsel++] = e[i].name;
            }
        }
        names[nsel] = NULL;

        if (nsel && GetPL(pl, _CONFIRM).used == 1)
        {
            sel = request_files(pl, names, nsel);
            if (sel == NULL)
            {
                nsel = 0;
            }
        }

        for (i = 0 ; i < nsel ; i++)
        {
            if (sel && !sel[i])
            {
                continue;
            }
            if (ctx.choices)
            {
                /* a choice may name "sub/dir/file" */
                char *s = join_path(source, names[i]);
                char *d = join_path(dest, names[i]);
                struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
                outofmem(fib);
                if (s && d && entry_type(s, fib))
                {
                    struct entry self, *sib = NULL;
                    char *sdir, *ddir;
                    int nsib;
                    self.name = fib->fib_FileName;
                    self.type = fib->fib_DirEntryType;
                    self.prot = fib->fib_Protection;
                    self.ds = fib->fib_Date;
                    self.comment = fib->fib_Comment[0] ? fib->fib_Comment : NULL;
                    sdir = strdup(s);
                    outofmem(sdir);
                    *(PathPart(sdir)) = 0;
                    ddir = strdup(d);
                    outofmem(ddir);
                    *(PathPart(ddir)) = 0;
                    nsib = list_dir(sdir, &sib);
                    plan_entry(&ctx, sdir, ddir, &self, sib, nsib < 0 ? 0 : nsib);
                    if (nsib > 0)
                    {
                        free_entries(sib, nsib);
                    }
                    free(sdir);
                    free(ddir);
                }
                else
                {
                    transcript("Choice \"", names[i], "\" not found.\n", NULL);
                }
                FreeDosObject(DOS_FIB, fib);
                free(s);
                free(d);
            }
            else
            {
                plan_entry(&ctx, source, dest, find_entry(e, n, names[i]), e, n);
            }
        }
        free(sel);
        free(names);

        if (ctx.njobs)
        {
            /* the destination directory itself, and its icon if (infos) */
            int created;
            if (!preferences.pretend && ensure_dir(dest, &created) == COPY_FAILED)
            {
                set_variable("@ioerr", NULL, IoErr());
                free_jobs(&ctx);
                if (ctx.copyfail == COPY_FAIL)
                {
                    error = DOSERROR;
                    traperr("Could not create \"%s\"!\n", dest);
                }
            }
            else if (ctx.infos && !ctx.choices && !ctx.files)
            {
                int l = strlen(source);
                if (l && source[l - 1] != ':' && source[l - 1] != '/' && dest[strlen(dest) - 1] != ':')
                {
                    char *si = malloc(l + 6), *di = malloc(strlen(dest) + 6);
                    outofmem(si);
                    outofmem(di);
                    sprintf(si, "%s.info", source);
                    sprintf(di, "%s.info", dest);
                    if (entry_type(si, NULL) < 0)
                    {
                        add_job(&ctx, si, di, FALSE, NULL);
                    }
                    free(si);
                    free(di);
                }
            }
        }
        if (n > 0)
        {
            free_entries(e, n);
        }
    }

    if (ctx.njobs)
    {
        if (!ctx.nogauge && !preferences.pretend)
        {
            char *msg = malloc(strlen(dest) + 32);
            outofmem(msg);
            sprintf(msg, "Copying files to \"%s\"...", dest);
            show_copying(msg, ctx.nfiles);
            free(msg);
        }
        result = run_jobs(&ctx);
    }
    else
    {
        result = 1;
    }
    free_jobs(&ctx);
    free(patbuf);
    return result;
}


/* ---------------------------------------------------------------------- */
/* Versions                                                               */

/*
 * Find "$VER:" in a file and parse "name ver.rev" after it.
 * Returns 1 and fills ver/rev, 0 if there is no usable version string.
 */
int scan_version(const char *file, ULONG *ver, ULONG *rev)
{
BPTR fh;
char *buf, *p, *end, tail[VERWINDOW + 1];
LONG n, base = 0, keep = 0, i, found = 0;

    fh = Open((STRPTR)file, MODE_OLDFILE);
    if (fh == BNULL)
    {
        return 0;
    }
    buf = malloc(COPYBUFSIZE + 8);
    outofmem(buf);
    while (!found && (n = Read(fh, buf + keep, COPYBUFSIZE)) > 0)
    {
        end = buf + keep + n;
        for (p = buf ; p + 5 <= end ; p++)
        {
            if (p[0] == '$' && memcmp(p, "$VER:", 5) == 0)
            {
                LONG pos = base + (p - buf) + 5;
                Seek(fh, pos, OFFSET_BEGINNING);
                i = Read(fh, tail, VERWINDOW);
                if (i < 0) i = 0;
                tail[i] = 0;
                /* skip blanks, the name, blanks */
                for (p = tail ; *p == ' ' || *p == '\t' ; p++);
                while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
                for ( ; *p == ' ' || *p == '\t' ; p++);
                if (*p >= '0' && *p <= '9')
                {
                    *ver = 0;
                    *rev = 0;
                    while (*p >= '0' && *p <= '9') *ver = *ver * 10 + (*p++ - '0');
                    if (*p == '.')
                    {
                        p++;
                        while (*p >= '0' && *p <= '9') *rev = *rev * 10 + (*p++ - '0');
                    }
                    found = 1;
                }
                else
                {
                    /* not a version string -- carry on after it */
                    Seek(fh, pos, OFFSET_BEGINNING);
                    base = pos;
                    keep = 0;
                    p = NULL;
                }
                break;
            }
        }
        if (found || p == NULL)
        {
            continue;
        }
        /* keep the last 4 bytes in case "$VER:" straddles the chunk */
        base += (end - buf) - 4;
        memmove(buf, end - 4, 4);
        keep = 4;
    }
    free(buf);
    Close(fh);
    return found;
}

/* Version of a library or device that is already loaded, 0 if none */
ULONG resident_version(const char *name)
{
struct Library *lib;
ULONG v = 0;

    Forbid();
    lib = (struct Library *)FindName(&SysBase->LibList, (STRPTR)name);
    if (lib == NULL)
    {
        lib = (struct Library *)FindName(&SysBase->DeviceList, (STRPTR)name);
    }
    if (lib != NULL)
    {
        v = ((ULONG)lib->lib_Version << 16) | lib->lib_Revision;
    }
    Permit();
    return v;
}


/* ---------------------------------------------------------------------- */
/* (copylib)                                                              */

int do_copylib(struct ParameterList *pl)
{
struct copyctx ctx;
char *source, *dest, *d, *msg;
ULONG sver = 0, srev = 0, dver = 0, drev = 0;
int have_src, have_dst, docopy, ask, created, result = 0;
LONG dtype;
struct entry *e;
struct entry *self;
char *srcdir;
int n;

    memset(&ctx, 0, sizeof(ctx));
    ctx.pl = pl;
    if (GetPL(pl, _SOURCE).used != 1 || GetPL(pl, _SOURCE).intval < 1)
    {
        error = SCRIPTERROR;
        traperr("<copylib> requires (source)!\n", NULL);
    }
    if (GetPL(pl, _DEST).used != 1 || GetPL(pl, _DEST).intval < 1)
    {
        error = SCRIPTERROR;
        traperr("<copylib> requires (dest)!\n", NULL);
    }
    source = GetPL(pl, _SOURCE).arg[0];
    dest = GetPL(pl, _DEST).arg[0];
    local_copyopts(pl, &ctx.copyfail, &ctx.copyflags);
    ctx.infos = GetPL(pl, _INFOS).used;
    ctx.nogauge = GetPL(pl, _NOGAUGE).used;

    if (entry_type(source, NULL) >= 0)
    {
        set_variable("@ioerr", NULL, IoErr());
        transcript("Source \"", source, "\" not found.\n", NULL);
        if (ctx.copyfail == COPY_FAIL)
        {
            error = DOSERROR;
            traperr("Source \"%s\" not found!\n", source);
        }
        return 0;
    }
    d = join_path(dest, GetPL(pl, _NEWNAME).used && GetPL(pl, _NEWNAME).intval > 0
                        ? GetPL(pl, _NEWNAME).arg[0] : (char *)FilePart((STRPTR)source));
    if (d == NULL)
    {
        return 0;
    }

    have_src = scan_version(source, &sver, &srev);
    dtype = entry_type(d, NULL);
    have_dst = (dtype < 0) ? scan_version(d, &dver, &drev) : 0;
    /* asked only when the script says (confirm) and the user is up to it */
    ask = (GetPL(pl, _CONFIRM).used == 1 && get_var_int("@user-level") >= GetPL(pl, _CONFIRM).intval);

    if (dtype == 0)
    {
        docopy = ask ? request_confirm(pl) : 1;
    }
    else if (dtype > 0)
    {
        SetIoErr(ERROR_OBJECT_EXISTS);
        docopy = 0;
        set_variable("@ioerr", NULL, ERROR_OBJECT_EXISTS);
    }
    else if (!have_src || !have_dst || sver > dver || (sver == dver && srev > drev))
    {
        /* newer, or nobody can tell: install it */
        docopy = ask ? request_confirm(pl) : 1;
    }
    else
    {
        /* same or older than what is there: only on explicit request */
        docopy = 0;
        if (ask)
        {
            msg = malloc(strlen(d) + 160);
            outofmem(msg);
            sprintf(msg, "\"%s\"\nis version %lu.%lu, the one to install is %lu.%lu.\nReplace it anyway?",
                    d, (unsigned long)dver, (unsigned long)drev, (unsigned long)sver, (unsigned long)srev);
            docopy = request_yesno(msg, pl, FALSE);
            free(msg);
        }
        else
        {
            transcript("Kept \"", d, "\" (not older than the one to install).\n", NULL);
        }
    }

    if (!docopy)
    {
        free(d);
        return 1;
    }

    /* the user already said yes -- do not ask again per file */
    ctx.copyflags &= ~COPY_ASKUSER;
    if (!preferences.pretend && ensure_dir(dest, &created) == COPY_FAILED)
    {
        set_variable("@ioerr", NULL, IoErr());
        free(d);
        if (ctx.copyfail == COPY_FAIL)
        {
            error = DOSERROR;
            traperr("Could not create \"%s\"!\n", dest);
        }
        return 0;
    }
    srcdir = strdup(source);
    outofmem(srcdir);
    *(PathPart(srcdir)) = 0;
    n = list_dir(srcdir[0] ? srcdir : "", &e);
    self = find_entry(e, n, FilePart((STRPTR)source));
    add_job(&ctx, source, d, FALSE, self);
    if (ctx.infos)
    {
        char *si = malloc(strlen(source) + 6), *di = malloc(strlen(d) + 6);
        outofmem(si);
        outofmem(di);
        sprintf(si, "%s.info", source);
        sprintf(di, "%s.info", d);
        if (entry_type(si, NULL) < 0)
        {
            add_job(&ctx, si, di, FALSE, find_entry(e, n, FilePart(si)));
        }
        free(si);
        free(di);
    }
    if (n > 0)
    {
        free_entries(e, n);
    }
    free(srcdir);
    if (!ctx.nogauge && !preferences.pretend)
    {
        msg = malloc(strlen(d) + 32);
        outofmem(msg);
        sprintf(msg, "Installing \"%s\"...", d);
        show_copying(msg, ctx.nfiles);
        free(msg);
    }
    result = run_jobs(&ctx);
    free_jobs(&ctx);
    free(d);
    return result;
}


/* ---------------------------------------------------------------------- */
/* (getsum)                                                               */

/* Rotate-and-add over the bytes; 0 if the file cannot be read */
LONG file_checksum(const char *file)
{
BPTR fh;
ULONG sum = 0;
LONG n, i;
UBYTE *buf;

    fh = Open((STRPTR)file, MODE_OLDFILE);
    if (fh == BNULL)
    {
        return 0;
    }
    buf = malloc(COPYBUFSIZE);
    outofmem(buf);
    while ((n = Read(fh, buf, COPYBUFSIZE)) > 0)
    {
        for (i = 0 ; i < n ; i++)
        {
            sum = ((sum << 1) | (sum >> 31)) + buf[i];
        }
    }
    free(buf);
    Close(fh);
    return (LONG)sum;
}


/* ---------------------------------------------------------------------- */
/* (protect)                                                              */

/*
 * Apply a protection string to bits: "+s-e" relative, "rwed" absolute.
 * AmigaDOS keeps r/w/e/d as *denied* bits, s/p/a/h as *set* bits.
 */
ULONG apply_protect_string(ULONG bits, const char *str)
{
int mode = 0;   /* 0 absolute, 1 add, -1 remove */
ULONG setbit;
int rwed;

    if (str[0] != '+' && str[0] != '-')
    {
        bits = FIBF_READ | FIBF_WRITE | FIBF_EXECUTE | FIBF_DELETE;
        mode = 1;
    }
    for ( ; *str ; str++)
    {
        switch (*str)
        {
            case '+': mode = 1;  continue;
            case '-': mode = -1; continue;
            case 'r': case 'R': setbit = FIBF_READ;    rwed = 1; break;
            case 'w': case 'W': setbit = FIBF_WRITE;   rwed = 1; break;
            case 'e': case 'E': setbit = FIBF_EXECUTE; rwed = 1; break;
            case 'd': case 'D': setbit = FIBF_DELETE;  rwed = 1; break;
            case 's': case 'S': setbit = FIBF_SCRIPT;  rwed = 0; break;
            case 'p': case 'P': setbit = FIBF_PURE;    rwed = 0; break;
            case 'a': case 'A': setbit = FIBF_ARCHIVE; rwed = 0; break;
            case 'h': case 'H': setbit = FIBF_HOLD;    rwed = 0; break;
            default: continue;
        }
        if ((mode > 0) ^ rwed)
        {
            bits |= setbit;
        }
        else
        {
            bits &= ~setbit;
        }
    }
    return bits;
}


/* ---------------------------------------------------------------------- */
/* (foreach)                                                              */

/*
 * Entries of dir matching pattern (AmigaDOS wildcards, case-insensitive).
 * Fills malloc()ed arrays of names and fib_DirEntryTypes; returns the
 * count, -1 if dir cannot be read.
 */
int dir_matches(const char *dir, const char *pattern, char ***names, LONG **types)
{
struct entry *e;
char *patbuf;
int n, i, m = 0;

    *names = NULL;
    *types = NULL;
    patbuf = malloc(strlen(pattern) * 2 + 2);
    outofmem(patbuf);
    if (ParsePatternNoCase((STRPTR)pattern, patbuf, strlen(pattern) * 2 + 2) < 0)
    {
        free(patbuf);
        return -1;
    }
    n = list_dir(dir, &e);
    if (n < 0)
    {
        free(patbuf);
        return -1;
    }
    *names = malloc((n + 1) * sizeof(char *));
    outofmem(*names);
    *types = malloc((n + 1) * sizeof(LONG));
    outofmem(*types);
    for (i = 0 ; i < n ; i++)
    {
        if (MatchPatternNoCase(patbuf, e[i].name))
        {
            (*names)[m] = strdup(e[i].name);
            outofmem((*names)[m]);
            (*types)[m] = e[i].type;
            m++;
        }
    }
    (*names)[m] = NULL;
    if (n > 0)
    {
        free_entries(e, n);
    }
    free(patbuf);
    return m;
}


/* ---------------------------------------------------------------------- */
/* (tooltype)                                                             */

/* Private, editable copy of an icon's tooltype array */
char **tooltypes_clone(char **tt)
{
char **out;
int n = 0, i;

    if (tt)
    {
        while (tt[n]) n++;
    }
    out = malloc((n + 1) * sizeof(char *));
    outofmem(out);
    for (i = 0 ; i < n ; i++)
    {
        out[i] = strdup(tt[i]);
        outofmem(out[i]);
    }
    out[n] = NULL;
    return out;
}

void tooltypes_free(char **tt)
{
int i;

    if (tt)
    {
        for (i = 0 ; tt[i] ; i++)
        {
            free(tt[i]);
        }
        free(tt);
    }
}

/* Does entry hold tooltype name? Disabled "(NAME=..)" entries count too. */
static int tooltype_is(const char *entry, const char *name)
{
int l = strlen(name);

    if (*entry == '(')
    {
        entry++;
    }
    return (strncasecmp(entry, name, l) == 0 && (entry[l] == '=' || entry[l] == ')' || entry[l] == 0));
}

/*
 * Set (value != NULL) or delete (value == NULL) a tooltype in a cloned
 * array. Returns the array, possibly moved by realloc().
 */
char **tooltypes_set(char **tt, const char *name, const char *value)
{
int i, n = 0, hit = -1;
char *entry = NULL;

    while (tt[n])
    {
        if (hit < 0 && tooltype_is(tt[n], name))
        {
            hit = n;
        }
        n++;
    }
    if (value != NULL)
    {
        entry = malloc(strlen(name) + strlen(value) + 2);
        outofmem(entry);
        if (value[0])
        {
            sprintf(entry, "%s=%s", name, value);
        }
        else
        {
            strcpy(entry, name);
        }
    }
    if (hit >= 0)
    {
        free(tt[hit]);
        if (entry)
        {
            tt[hit] = entry;
        }
        else
        {
            for (i = hit ; i < n ; i++)
            {
                tt[i] = tt[i + 1];
            }
        }
    }
    else if (entry)
    {
        tt = realloc(tt, (n + 2) * sizeof(char *));
        outofmem(tt);
        tt[n] = entry;
        tt[n + 1] = NULL;
    }
    return tt;
}
