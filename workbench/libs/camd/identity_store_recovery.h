#ifndef CAMD_IDENTITY_STORE_RECOVERY_H
#define CAMD_IDENTITY_STORE_RECOVERY_H

#include <stddef.h>

enum CAMDIdentityStoreSlot {
    CAMD_IDENTITY_STORE_MAIN = 0,
    CAMD_IDENTITY_STORE_NEW,
    CAMD_IDENTITY_STORE_BACKUP,
    CAMD_IDENTITY_STORE_SLOT_COUNT
};

enum CAMDIdentityStoreLoadState {
    CAMD_IDENTITY_STORE_MISSING = 0,
    CAMD_IDENTITY_STORE_VALID,
    CAMD_IDENTITY_STORE_INVALID
};

typedef enum CAMDIdentityStoreLoadState (*CAMDIdentityStoreLoadFn)(
    void *context, enum CAMDIdentityStoreSlot slot);
typedef int (*CAMDIdentityStoreExistsFn)(
    void *context, enum CAMDIdentityStoreSlot slot);
typedef int (*CAMDIdentityStoreRemoveFn)(
    void *context, enum CAMDIdentityStoreSlot slot);
typedef int (*CAMDIdentityStoreMoveFn)(
    void *context, enum CAMDIdentityStoreSlot from,
    enum CAMDIdentityStoreSlot to);

struct CAMDIdentityStoreOpsV1 {
    size_t Size;
    unsigned int Version;
    void *Context;
    CAMDIdentityStoreLoadFn Load;
    CAMDIdentityStoreExistsFn Exists;
    CAMDIdentityStoreRemoveFn Remove;
    CAMDIdentityStoreMoveFn Move;
};

/* Select and promote the best complete startup snapshot. */
int camd_identity_store_recover(const struct CAMDIdentityStoreOpsV1 *ops);

/* Install an already durable NEW snapshot using MAIN/BACKUP rotation. */
int camd_identity_store_install(const struct CAMDIdentityStoreOpsV1 *ops);

#endif
