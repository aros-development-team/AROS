#include "identity_store_recovery.h"

static int common_valid(const struct CAMDIdentityStoreOpsV1 *ops)
{
    return ops && ops->Size == sizeof(*ops) && ops->Version == 1;
}

static int promote(const struct CAMDIdentityStoreOpsV1 *ops,
                   enum CAMDIdentityStoreSlot candidate,
                   enum CAMDIdentityStoreLoadState main_state)
{
    if (main_state == CAMD_IDENTITY_STORE_INVALID &&
        !ops->Remove(ops->Context, CAMD_IDENTITY_STORE_MAIN))
        return 0;
    if (!ops->Move(ops->Context, candidate, CAMD_IDENTITY_STORE_MAIN))
        return 0;
    (void)ops->Remove(ops->Context, CAMD_IDENTITY_STORE_BACKUP);
    return 1;
}

int camd_identity_store_recover(const struct CAMDIdentityStoreOpsV1 *ops)
{
    enum CAMDIdentityStoreLoadState main_state, new_state, backup_state;

    if (!common_valid(ops) || !ops->Load || !ops->Remove || !ops->Move)
        return 0;
    main_state = ops->Load(ops->Context, CAMD_IDENTITY_STORE_MAIN);
    if (main_state == CAMD_IDENTITY_STORE_VALID)
        return 1;
    new_state = ops->Load(ops->Context, CAMD_IDENTITY_STORE_NEW);
    if (new_state == CAMD_IDENTITY_STORE_VALID)
        return promote(ops, CAMD_IDENTITY_STORE_NEW, main_state);
    backup_state = ops->Load(ops->Context, CAMD_IDENTITY_STORE_BACKUP);
    if (backup_state == CAMD_IDENTITY_STORE_VALID)
        return promote(ops, CAMD_IDENTITY_STORE_BACKUP, main_state);
    return main_state == CAMD_IDENTITY_STORE_MISSING &&
           new_state == CAMD_IDENTITY_STORE_MISSING &&
           backup_state == CAMD_IDENTITY_STORE_MISSING;
}

int camd_identity_store_install(const struct CAMDIdentityStoreOpsV1 *ops)
{
    int had_main;

    if (!common_valid(ops) || !ops->Exists || !ops->Remove || !ops->Move)
        return 0;
    had_main = ops->Exists(ops->Context, CAMD_IDENTITY_STORE_MAIN);
    if (had_main) {
        if (ops->Exists(ops->Context, CAMD_IDENTITY_STORE_BACKUP) &&
            !ops->Remove(ops->Context, CAMD_IDENTITY_STORE_BACKUP))
            return 0;
        if (!ops->Move(ops->Context, CAMD_IDENTITY_STORE_MAIN,
                       CAMD_IDENTITY_STORE_BACKUP))
            return 0;
    }
    if (!ops->Move(ops->Context, CAMD_IDENTITY_STORE_NEW,
                   CAMD_IDENTITY_STORE_MAIN)) {
        if (had_main)
            (void)ops->Move(ops->Context, CAMD_IDENTITY_STORE_BACKUP,
                            CAMD_IDENTITY_STORE_MAIN);
        return 0;
    }
    if (had_main)
        (void)ops->Remove(ops->Context, CAMD_IDENTITY_STORE_BACKUP);
    return 1;
}
