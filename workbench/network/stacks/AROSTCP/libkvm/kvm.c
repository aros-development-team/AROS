#include <amitcp/socketbasetags.h>
#include <utility/hooks.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <kvm.h>
#include <netdb.h>

#include <proto/socket.h>
#define SYSTEM_PRIVATE
#include <proto/miami.h>

#include <version.h>

ULONG __kvm_errno = NULL;

kvm_t *kvm_openfiles(const char *execfile, const char *corefile, const char *swapfile, int flags, char *errstr)
{
    STRPTR StackVer = "";
    /* We have only one core and kernel (currently running) */
    if (execfile || corefile) {
	strcpy (errstr, "Operation not supported");
	return NULL;
    }
    SocketBaseTags(SBTM_GETREF(SBTC_RELEASESTRPTR), (ULONG)&StackVer, TAG_DONE);
    if (strcmp(StackVer,STACK_RELEASE)) {
	strcpy (errstr, "Wrong bsdsocket.library version");
	return NULL;
    }
    return (kvm_t *)(flags|0xdead0000); /* A clever trick */
}

int kvm_close(kvm_t *kd)
{
    return 0;
}

int kvm_nlist (kvm_t *kd, struct nlist *nl)
{
    int inval = 0;

    for (; nl->n_name && nl->n_name[0]; nl++) {
	nl->n_value = (unsigned long)FindKernelVar (nl->n_name);
	if (nl->n_value)
	    nl->n_type = N_TEXT;
	else {
	    nl->n_type = 0;
	    inval++;
	}
	nl->n_other = 0;
	nl->n_desc = 0;
    }
    return inval;
}

