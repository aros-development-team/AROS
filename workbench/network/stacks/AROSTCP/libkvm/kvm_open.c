#include <exec/types.h>
#include <kvm.h>
#include <stdio.h>
#include "kvm_errors.h"

kvm_t *kvm_open(const char *execfile, const char *corefile, const char *swapfile, int flags, const char *errstr)
{
    char errbuf[32];
    kvm_t *res;

    res = kvm_openfiles(execfile, corefile, swapfile, flags, errbuf);
    if (!res && errstr) {
	fprintf (stderr, "%s: %s\n", errstr, errbuf);
	fflush (stderr);
    }
    return res;
}
