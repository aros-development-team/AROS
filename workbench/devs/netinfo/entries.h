#ifndef _ENTRIES_H_
#define _ENTRIES_H_
/* 
 * entries.h --- db units
 *
 * Author: ppessi <Pekka.Pessi@hut.fi>
 *
 * Copyright © 1993 AmiTCP/IP Group, <AmiTCP-group@hut.fi>
 *                  Helsinki University of Technology, Finland.
 */

#include "base.h"

struct NetInfoEnt {
  UBYTE *nie_name;
  ULONG  nie_fill;
  ULONG  nie_id;
};

/*
 * Common values are represented with this structure
 */
struct Ent {
  struct Node e_node;
  UWORD       e_tlen;		/* size of text data */
  UBYTE      *e_name;
  LONG        e_pwd;
  LONG        e_id;
} __packed;

struct PasswdEnt {
  struct Node          pe_node;
  UWORD                pe_tlen;
  struct NetInfoPasswd pe_passwd[1];
} __packed;

/* we may have an extended version of passwd */
#define PASSWDFIELDS 7

struct GroupEnt {
  struct Node         ge_node;
  UWORD               ge_tlen;
  struct NetInfoGroup ge_group[1];
  ULONG               ge_nmembers;	/* actually, # of members + 1 */
} __packed;

/*
 * Entry Node Type
 */
#define ENT_PASSWD 30
#define ENT_GROUP  31

/*
 * The changed entries are marked
 */
#define ENT_CHANGED 1

/*
 * Maximum entry length
 */
#define MAXLINELENGTH 1024

/* 
 * As this far there are no quick commands
 * we bother to lock only niu_ReqLock
 */
#define DbMapLock(u) 
#define DbMapLockShared(u) 
#define DbMapUnlock(u) 

/* common methods in entries.c */ 
void EntCleanup(struct NetInfoDevice *, struct NetInfoMap *nim);
void EntHandleNotify(struct NetInfoDevice *, struct NetInfoMap *nim);

typedef struct NetInfoMap *(*init_map_func_t)(struct NetInfoDevice *);
struct NetInfoMap *InitPasswdMap(struct NetInfoDevice *);
struct NetInfoMap *InitGroupMap(struct NetInfoDevice *);

/* Other prototypes */
struct Ent *InternalSetEnts(struct NetInfoDevice *, struct NetInfoMap *nim);
void InternalEndEnts(struct NetInfoDevice *, struct NetInfoMap *nim);
struct Ent *GetNextEnt(struct Ent *e);

/* Utility */
static inline char *stpcopy(char *to, const char *from)
{
  while (*to++ = *from++)
    ;
  return to;
}

#endif /* _ENTRIES_H_ */
