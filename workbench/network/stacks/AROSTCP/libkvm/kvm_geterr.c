#include <exec/types.h>
#include <kvm.h>
#include "kvm_errors.h"

char *kvm_errors[] = {
    "Permission denied",
    "Invalid address"
};

char *kvm_geterr(kvm_t *kd)
{
    return (kvm_errors[__kvm_errno]);
}
