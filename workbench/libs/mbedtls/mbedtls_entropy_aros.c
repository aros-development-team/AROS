/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    mbedTLS hardware entropy source for AROS.

    Draws from entropy.resource, the system CSPRNG that every architecture
    seeds from its own software collector and, where the board or CPU has
    one, a hardware source (x86 RDRAND/RDSEED, the Raspberry Pi RNG, ...).
    The glue deliberately knows nothing about SoC registers: probing them
    from here is not portable (KrnGetSystemAttr(KATTR_PeripheralBase) is -1
    on platforms without one, which once sent sshd into a bus fault on
    riscv64), and the resource already does the detection properly.
*/

#include "mbedtls/entropy.h"
#include <proto/exec.h>
#include <proto/entropy.h>
#include <resources/entropy.h>

/* entropy.resource, opened on first use - resources are never closed */
static APTR entropyres;

/*
 * mbedtls_hardware_poll — called by mbedTLS entropy collector.
 * Must fill 'output' with 'len' random bytes and set *olen.
 */
int mbedtls_hardware_poll(void *data, unsigned char *output,
                          size_t len, size_t *olen)
{
    /* local base for the GetEntropy() inline */
    struct Library *EntropyBase;

    (void)data;

    *olen = 0;

    if (entropyres == NULL)
        entropyres = OpenResource(ENTROPYNAME);
    EntropyBase = entropyres;

    /* No system entropy source: fail loudly rather than degrade to
     * something weak - the caller's DRBG seeding reports the error. */
    if (EntropyBase == NULL)
        return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;

    while (len > 0)
    {
        /* keep each request within what the resource serves per call */
        ULONG chunk = len > 256 ? 256 : (ULONG)len;

        if (GetEntropy(output, chunk) != (LONG)chunk)
            return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;

        output += chunk;
        len -= chunk;
        *olen += chunk;
    }

    return 0;
}
