/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>

#include <errno.h>
#include <sys/stat.h>

#include "__errno.h"

ULONG prot_u2a(mode_t protect);

int chmod(const char *path, mode_t mode)
{
    if (!SetProtection(path, prot_u2a(mode)))
    {
    	errno = IoErr2errno(IoErr());
	return -1;
    }

    return 0;
}


/* taken from emul_handler */
ULONG prot_u2a(mode_t protect)
{
    ULONG aprot = FIBF_SCRIPT;

    /* The following three (AROS) flags are low-active! */
    if (!(protect & S_IRUSR))
	aprot |= FIBF_READ;
    if (!(protect & S_IWUSR))
	aprot |= FIBF_WRITE;
    if (!(protect & S_IXUSR))
	aprot |= FIBF_EXECUTE;

    /* The following flags are high-active again. */
    if ((protect & S_IRGRP))
	aprot |= FIBF_GRP_READ;
    if ((protect & S_IWGRP))
	aprot |= FIBF_GRP_WRITE;
    if ((protect & S_IXGRP))
	aprot |= FIBF_GRP_EXECUTE;
    if ((protect & S_IROTH))
	aprot |= FIBF_OTR_READ;
    if ((protect & S_IWOTH))
	aprot |= FIBF_OTR_WRITE;
    if ((protect & S_IXOTH))
	aprot |= FIBF_OTR_EXECUTE;

    return aprot;
}
