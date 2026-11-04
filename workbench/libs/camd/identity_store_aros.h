#ifndef CAMD_IDENTITY_STORE_AROS_H
#define CAMD_IDENTITY_STORE_AROS_H

struct CAMDIdentityMap;
struct CAMDIdentityStoreAROS;

struct CAMDIdentityStoreAROS *camd_identity_store_aros_create(void);
void camd_identity_store_aros_destroy(struct CAMDIdentityStoreAROS *store);
struct CAMDIdentityMap *camd_identity_store_aros_map(
    struct CAMDIdentityStoreAROS *store);

#endif
