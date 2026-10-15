/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library built-in module "access": who may use which service,
          from where and when (account stack).

          Rules are read from SYS:Security/Access, one per line; the first
          rule that matches decides, no matching rule permits:

              <+|-> : <users> : <services> : <hosts> [: <days> <hhmm-hhmm>]

          users:    user IDs, @group (security.library groups) or ALL
          services: service names (patterns allowed) or ALL
          hosts:    remote host names (patterns allowed), LOCAL for a
                    request without a remote host, or ALL
          days:     Mo Tu We Th Fr Sa Su, Wk (Mo-Fr), Wd (Sa Su), Al;
                    several may be joined (MoWeFr); with a time range the
                    rule only applies on those days in that time (a range
                    past midnight, such as 2200-0600, wraps)

          Items are separated by blanks or commas, ';' starts a comment
          ('#' is a pattern character). Example:

              - : ALL : #? : ALL : Wd 0000-2400   ; nobody at weekends
              + : @staff : login : LOCAL
              - : ALL : login : ALL

          Without the file the module is ignored. On a configured
          multi-user system the file must belong to root and not be
          writable by others (PamTrusted()); otherwise everything is
          denied.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/security.h>
#include <proto/pam.h>
#include <dos/datetime.h>
#include <resources/log.h>
#include <string.h>

#include "pam_intern.h"

#define ACCESS_FILE     "SYS:Security/Access"
#define MAXLINE         512

/* Does one item list match a value? Items: words, '@group', ALL, LOCAL */
static BOOL ListMatches(struct PamBase *PamBase, STRPTR list, CONST_STRPTR value, BOOL hosts, struct Library *secBase, CONST_STRPTR user)
{
    char pat[128];
    STRPTR item;

    while (*list)
    {
        while (*list == ' ' || *list == '\t' || *list == ',')
            list++;
        if (!*list)
            break;
        item = list;
        while (*list && *list != ' ' && *list != '\t' && *list != ',')
            list++;
        if (*list)
            *list++ = '\0';

        if (!Stricmp(item, "ALL"))
            return TRUE;
        if (hosts && !Stricmp(item, "LOCAL"))
        {
            if (!value || !value[0])
                return TRUE;
            continue;
        }
        if (!value)
            continue;
        if (item[0] == '@')
        {
            /* group membership through security.library */
            struct secUserInfo *ui;
            struct secGroupInfo *gi;
            BOOL member = FALSE;

            if (!secBase || !user)
                continue;
            if ((ui = secAllocUserInfo()) && (gi = secAllocGroupInfo()))
            {
                strncpy(ui->UserID, user, secUSERIDSIZE - 1);
                strncpy(gi->GroupID, item + 1, secGROUPIDSIZE - 1);
                if (secGetUserInfo(ui, secKeyType_UserID) && secGetGroupInfo(gi, secKeyType_GroupID))
                {
                    UWORD i;

                    member = (ui->gid == gi->gid);
                    for (i = 0; !member && i < ui->NumSecGroups; i++)
                        member = (ui->SecGroups[i] == gi->gid);
                }
                secFreeGroupInfo(gi);
            }
            if (ui)
                secFreeUserInfo(ui);
            if (member)
                return TRUE;
            continue;
        }
        if (ParsePatternNoCase(item, pat, sizeof(pat)) >= 0 && MatchPatternNoCase(pat, (STRPTR)value))
            return TRUE;
    }
    return FALSE;
}

/* "MoTuWe 0800-1800": is now inside? */
static BOOL TimeMatches(struct PamBase *PamBase, STRPTR spec)
{
    static const char * const days[7] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };
    struct DateStamp ds;
    ULONG weekday, minute, from = 0, to = 24 * 60;
    BOOL dayok = FALSE;
    STRPTR p = spec, range;
    int i;

    DateStamp(&ds);
    weekday = (ds.ds_Days + 0) % 7;     /* 1978-01-01 was a Sunday */
    minute = ds.ds_Minute;

    while (*p == ' ' || *p == '\t')
        p++;
    for (range = p; *range && *range != ' ' && *range != '\t'; range++)
        ;
    /* the day part */
    while (p + 1 < range)
    {
        char d[3] = { p[0], p[1], 0 };

        if (!Stricmp(d, "Al"))
            dayok = TRUE;
        else if (!Stricmp(d, "Wk"))
            dayok |= (weekday >= 1 && weekday <= 5);
        else if (!Stricmp(d, "Wd"))
            dayok |= (weekday == 0 || weekday == 6);
        else
        {
            for (i = 0; i < 7; i++)
                if (!Stricmp(d, days[i]))
                    dayok |= (weekday == (ULONG)i);
        }
        p += 2;
    }
    if (!dayok)
        return FALSE;

    /* the time part: hhmm-hhmm */
    while (*range == ' ' || *range == '\t')
        range++;
    if (*range)
    {
        LONG a, b;
        if (StrToLong(range, &a) < 0)
            return FALSE;
        p = strchr(range, '-');
        if (!p || StrToLong(p + 1, &b) < 0)
            return FALSE;
        from = (a / 100) * 60 + a % 100;
        to = (b / 100) * 60 + b % 100;
    }
    if (from <= to)
        return minute >= from && minute < to;
    return minute >= from || minute < to;      /* past midnight */
}

static LONG AccessAcctMgmt(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    struct Library *secBase = NULL;
    CONST_STRPTR user;
    BPTR file, lock;
    STRPTR line, field[5], p;
    LONG res = PAM_IGNORE, n, lineno = 0;

    if ((res = PamGetUser(h, &user, NULL)) != PAM_SUCCESS)
        return res;
    res = PAM_IGNORE;

    if (!(lock = Lock(ACCESS_FILE, SHARED_LOCK)))
        return PAM_IGNORE;
    if (!PamTrusted(PamBase, lock, TRUE))
    {
        UnLock(lock);
        PamLogA(PamBase, h, LOGF_Flag_Type_Error, ACCESS_FILE " refused: not owned by root or writable by others", NULL);
        return PAM_PERM_DENIED;
    }
    if (!(file = OpenFromLock(lock)))
    {
        UnLock(lock);
        return PAM_PERM_DENIED;
    }
    if (PamSecurityState(PamBase) == PAM_STATE_CONFIGURED)
        secBase = OpenLibrary(SECURITYNAME, 45);

    if ((line = AllocVec(MAXLINE, MEMF_ANY)))
    {
        while (res == PAM_IGNORE && FGets(file, line, MAXLINE - 1))
        {
            lineno++;
            if ((p = strchr(line, ';')))
                *p = '\0';
            /* split at ':' */
            for (n = 0, p = line; n < 5; n++)
            {
                field[n] = p;
                if (!(p = strchr(p, ':')))
                {
                    n++;
                    break;
                }
                *p++ = '\0';
            }
            for (p = field[0]; *p == ' ' || *p == '\t'; p++)
                ;
            if (!*p || *p == '\n' || *p == '\r')
                continue;
            if (n < 4 || (*p != '+' && *p != '-'))
            {
                PamLogA(PamBase, h, LOGF_Flag_Type_Error, ACCESS_FILE ": bad line %ld", (IPTR []){ lineno });
                res = PAM_PERM_DENIED;          /* a broken policy denies */
                break;
            }
            /* strip the line end from the last field */
            {
                STRPTR e = field[n - 1] + strlen(field[n - 1]);
                while (e > field[n - 1] && (e[-1] == '\n' || e[-1] == '\r'))
                    *--e = '\0';
            }
            if (!ListMatches(PamBase, field[1], user, FALSE, secBase, user))
                continue;
            if (!ListMatches(PamBase, field[2], h->Service, FALSE, secBase, user))
                continue;
            if (!ListMatches(PamBase, field[3], h->RHost, TRUE, secBase, user))
                continue;
            if (n == 5 && !TimeMatches(PamBase, field[4]))
                continue;
            res = (*p == '+') ? PAM_SUCCESS : PAM_PERM_DENIED;
        }
        FreeVec(line);
    }
    else
        res = PAM_BUF_ERR;
    Close(file);
    if (secBase)
        CloseLibrary(secBase);

    if (res == PAM_PERM_DENIED)
    {
        struct pam_message m = { PAM_ERROR_MSG, "Access denied." };
        const struct pam_message *mp = &m;
        struct pam_response *r = NULL;

        PamLogA(PamBase, h, LOGF_Flag_Type_Warn, "access denied for %s", (IPTR []){ (IPTR)user });
        if (!(flags & PAM_SILENT) && PamDoConverse(PamBase, h, 1, &mp, &r) == PAM_SUCCESS)
            PamFreeResponses(PamBase, 1, r);
    }
    /* no rule matched: permitted */
    return (res == PAM_IGNORE) ? PAM_SUCCESS : res;
}

const struct PamBuiltin PamBuiltinAccess =
{
    "access", { NULL, NULL, AccessAcctMgmt, NULL, NULL, NULL }
};
