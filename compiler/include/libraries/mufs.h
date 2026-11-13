#ifndef LIBRARIES_MUFS_H
#define LIBRARIES_MUFS_H

/*
    Copyright (C) 2019-2026, The AROS Development Team. All rights reserved.

    Desc: DEPRECATED compatibility header. Everything now lives in
          <libraries/security.h>; this file only provides the historical
          multiuser.library names for sources that still include it.
*/

#include <libraries/security.h>

#define MULTIUSERNAME           "multiuser.library"
#define MULTIUSERVERSION        (40)

#define MULTIUSERCATALOGNAME    "multiuser.catalog"
#define MULTIUSERCATALOGVERSION (1)

/*
 * MultiUser names for sources written against multiuser.library (pfs3 ...).
 * Only the pieces of the API such sources actually use; add as needed.
 * The id constants are the original MultiUser values (root 0xffff, nobody 0),
 * see <libraries/security.h>.
 */
#define muExtOwner              secExtOwner
#define muUserInfo              secUserInfo
#define muGroupInfo             secGroupInfo

#define muOWNER_SYSTEM          secOWNER_SYSTEM
#define muOWNER_NOBODY          secOWNER_NOBODY
#define muMASK_UID              secMASK_UID
#define muMASK_GID              secMASK_GID
#define muROOT_UID              secROOT_UID
#define muROOT_GID              secROOT_GID
#define muNOBODY_UID            secNOBODY_UID
#define muNOBODY_GID            secNOBODY_GID
#define muUSERIDSIZE            secUSERIDSIZE
#define muGROUPIDSIZE           secGROUPIDSIZE
#define muDEFPROTECTION         secDEFPROTECTION
#define muSecGroups(x)          secSecGroups(x)

#define muRelB_ROOT_UID         secRelB_ROOT_UID
#define muRelB_ROOT_GID         secRelB_ROOT_GID
#define muRelB_NOBODY           secRelB_NOBODY
#define muRelB_UID_MATCH        secRelB_UID_MATCH
#define muRelB_GID_MATCH        secRelB_GID_MATCH
#define muRelB_PRIM_GID         secRelB_PRIM_GID
#define muRelB_NO_OWNER         secRelB_NO_OWNER
#define muRelF_ROOT_UID         secRelF_ROOT_UID
#define muRelF_ROOT_GID         secRelF_ROOT_GID
#define muRelF_NOBODY           secRelF_NOBODY
#define muRelF_UID_MATCH        secRelF_UID_MATCH
#define muRelF_GID_MATCH        secRelF_GID_MATCH
#define muRelF_PRIM_GID         secRelF_PRIM_GID
#define muRelF_NO_OWNER         secRelF_NO_OWNER
#define muRel_PROPERTY_ACCESS   secRelF_PROPERTY_ACCESS

#define muFS_API_VERSION        secFS_API_VERSION
#define muKeyFileName           secKey_FileName

/* functions (with <proto/security.h>) */
#define muGetTaskOwner          secGetTaskOwner
#define muGetTaskExtOwner       secGetTaskExtOwner
#define muFreeExtOwner          secFreeExtOwner
#define muGetRelationshipA      secGetRelationshipA
#define muGetDefProtection      secGetDefProtection
#define muGetPktOwner           secGetPktOwner
#define muGetPktDefProtection   secGetPktDefProtection
#define muFSRendezVous          secFSRendezVous
#define muGetUserInfo           secGetUserInfo
#define muAllocUserInfo         secAllocUserInfo
#define muFreeUserInfo          secFreeUserInfo
#define muGetGroupInfo          secGetGroupInfo
#define muAllocGroupInfo        secAllocGroupInfo
#define muFreeGroupInfo         secFreeGroupInfo

#endif /* LIBRARIES_MUFS_H */
