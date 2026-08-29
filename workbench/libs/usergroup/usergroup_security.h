/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: usergroup.library on top of security.library.

          When security.library is resident, the credentials, the user and
          group databases, the login name and the umask all come from it, so
          that AmiTCP-style software sees the same users as the rest of the
          multi-user system. Without the library nothing changes: the
          original (netinfo.device / credential) code paths are used.
*/
#ifndef USERGROUP_SECURITY_H
#define USERGROUP_SECURITY_H

#include <exec/types.h>
#include <exec/tasks.h>
#include <aros/types/uid_t.h>
#include <aros/types/gid_t.h>
#include <aros/types/mode_t.h>

struct Library;
struct passwd;
struct group;
struct UserGroupCredentials;

BOOL  ugSecActive(struct Library *ugBase);
void  ugSecCleanup(struct Library *ugBase);

uid_t ugSecGetUid(struct Library *ugBase, BOOL effective);
gid_t ugSecGetGid(struct Library *ugBase, BOOL effective);
int   ugSecGetGroups(struct Library *ugBase, int ngroups, gid_t *groups);
int   ugSecSetReUid(struct Library *ugBase, uid_t ruid, uid_t euid);
int   ugSecSetUid(struct Library *ugBase, uid_t uid);
int   ugSecSetReGid(struct Library *ugBase, gid_t rgid, gid_t egid);
int   ugSecSetGid(struct Library *ugBase, gid_t gid);
int   ugSecSetGroups(struct Library *ugBase, int ngrp, const gid_t *groups);

mode_t ugSecUmask(struct Library *ugBase, mode_t newmask);
mode_t ugSecGetUmask(struct Library *ugBase);

struct passwd *ugSecGetPwNam(struct Library *ugBase, const char *name);
struct passwd *ugSecGetPwUid(struct Library *ugBase, uid_t uid);
void  ugSecSetPwEnt(struct Library *ugBase);
struct passwd *ugSecGetPwEnt(struct Library *ugBase);
void  ugSecEndPwEnt(struct Library *ugBase);

struct group *ugSecGetGrNam(struct Library *ugBase, const char *name);
struct group *ugSecGetGrGid(struct Library *ugBase, gid_t gid);
void  ugSecSetGrEnt(struct Library *ugBase);
struct group *ugSecGetGrEnt(struct Library *ugBase);
void  ugSecEndGrEnt(struct Library *ugBase);

char *ugSecGetLogin(struct Library *ugBase, char *buffer, ULONG size);
struct UserGroupCredentials *ugSecGetCredentials(struct Library *ugBase, struct Task *task,
                                                 struct UserGroupCredentials *creds);

#endif /* USERGROUP_SECURITY_H */
