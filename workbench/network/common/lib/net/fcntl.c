/*
 * This routine currently offers minimal functionality and was written at
 * early stage of dhclient porting. Since then dhclient port architecture
 * was reconsidered and this function was cancelled. If needed it can be
 * reincarnated in future netlib versions to be fully functional and
 * usable on any Level-1 file (or socket) descriptor.
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/filio.h>
#include <proto/socket.h>

int fcntl(int fd, int cmd, ...)
{
    va_list a;
    int res1, res2;
    long arg;
    long nbio = 0;
    long async = 0;

    switch (cmd) {
    case F_GETFL:
	/* Report the current status flags.  Callers such as iperf3's
	   setnonblocking() read the flags, OR in O_NONBLOCK and write them
	   back, and abort on a negative result - so a valid value is required.
	   Only O_NONBLOCK is meaningful for a socket here. */
	return 0;
    case F_SETFL:
	va_start(a, cmd);
	arg = va_arg(a, long);
	va_end(a);
	if (arg & O_NONBLOCK)
	    nbio = 1;
	if (arg & O_ASYNC)
	    async = 1;
	res1 = IoctlSocket(fd, FIONBIO, (char *)&nbio);
	res2 = IoctlSocket(fd, FIOASYNC, (char *)&async);
	return (res1 | res2);
    case F_GETFD:
    case F_SETFD:
	/* No exec across sockets on AROS; treat FD_CLOEXEC as a no-op. */
	return 0;
    default:
	errno = EINVAL;
	return -1;
    }
}
