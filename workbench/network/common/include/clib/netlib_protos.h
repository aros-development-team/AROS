#ifndef CLIB_NETLIB_PROTOS_H
#define CLIB_NETLIB_PROTOS_H
/*
**      $Filename: clib/netlib_protos.h $
**	$Release$
**      $Revision$
**      $Date$
**
**	Prototypes for netlib utility functions
**
**	Copyright © 1993 AmiTCP/IP Group, <AmiTCP-Group@hut.fi>
**                  Helsinki University of Technology, Finland.
**                  All rights reserved.
**	Copyright © 2005 Pavel Fedin
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef SYS_TYPES_H
#include <sys/types.h>
#endif
#ifndef SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#ifndef EXEC_LIBRARIES_H
struct Library;
#endif
#ifndef SYS_TIME_H
#include <sys/time.h>
#endif
#ifndef SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifndef NETDB_H
struct hostent;
struct netent;
struct servent;
struct protoent;
#endif
#ifndef NETINET_IN_H
#include <netinet/in.h>
#endif
#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif
#ifndef SYS_STAT_H
#include <sys/stat.h>
#endif
//#include <lineread.h>
#include <stdio.h>

/* chmod.c */
int chmod(const char *path, mode_t mode);

/* chown.c */
int chown(const char *name, uid_t uid, gid_t gid);

/* dummy.c */
struct netent  *getnetent(void);
struct servent  *getservent(void);

/* getopt.c */
extern int opterr;
extern int optind;
extern int optopt;
extern char *optarg;
int   getopt(int argc, char * const *argv, const char *opts);

/* gettimeofday.c */
int   gettimeofday(struct timeval *tp, struct timezone *tzp);

/* herror.c */
void  herror(const char *banner);

/* init_inet_daemon.c */
int init_inet_daemon(void);

/* ioctl.c */
int ioctl(int fd, unsigned int request, char *argp);

/* iomode.c */
int iomode(int fd, int mode);

/* isatty.c */
int isatty(int fd);

/* lineread.c */
int lineRead(struct LineRead * rl);

/* perror.c */
void  perror(const char *banner);

/* popen.c */
FILE *popen(const char *cmd, const char *mode);
FILE *popenl(const char *arg0, ...);
int pclose(FILE *fptr);
char *mktemp(char * template);

/* printfault.c */
void  PrintNetFault(LONG code, const UBYTE *banner);

/* printuserfault.c */
void  PrintUserFault(LONG code, const UBYTE *banner);

/* rcmd.c */
int   rcmd(char **, int, const char *, const char *, const char *, int *);
int   rresvport(int *alport);

/* serveraccept.c */
long serveraccept(char *pname, struct sockaddr_in *ha);

/* set_socket_stdio.c */
int set_socket_stdio(int sock);

/* setegid.c */
int setegid(gid_t g);

/* seteuid.c */
int seteuid(uid_t u);

/* sleep.c */
unsigned sleep(unsigned secs);

/* stat.c */
int stat(const char *name, struct stat *st);

/* strerror.c */
char *strerror(int code);

/* stubs.c */
#ifndef inet_ntoa
char * inet_ntoa(struct in_addr addr);
#endif
#ifndef inet_makeaddr
struct in_addr inet_makeaddr(int net, int host);
#endif
#ifndef inet_lnaof
unsigned long inet_lnaof(struct in_addr addr);
#endif
#ifndef inet_netof
unsigned long inet_netof(struct in_addr addr);
#endif
#ifndef select
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exeptfds,
	   struct timeval *timeout);
#endif

/* getpid.c */
#ifndef getpid
pid_t getpid(void);
#endif

/* syslog.c */
void  openlog(const char *ident, int logstat, int logfac);
void  closelog(void);
int   setlogmask(int pmask);

/* timerinit.c */
extern long __local_to_GMT;

/* usleep.c */
void usleep(unsigned int usecs);

/* utime.c */
#ifndef UTIME_H
struct utimbuf;
#endif
int utime(const char *name, const struct utimbuf *times);

/* asprintf.c */
int asprintf(char **ret, const char *format, ...);

/* vasprintf.c */
int vasprintf(char **ret, const char *format, va_list ap);

/* err.c */
void verrc(int eval, int code, const char *fmt, va_list ap) __attribute__((noreturn));
void vwarnc(int code, const char *fmt, va_list ap);

#if 0 /* TODO */

/* _close.c */
int __close(int fd);

/* _dup.c */
/* _dup2.c */

/* access.c */
int __access(const char *name, int mode);

/* dostat.c */
void __dostat(struct FileInfoBlock *fib, struct stat *st);

/* fhopen.c */
int fhopen(long file, int mode);

/* fib.c */
extern struct FileInfoBlock __dostat_fib[1];

/* _fstat.c */
int fstat(int fd, struct stat *st);

/* _lseek.c */
long __lseek(int fd, long rpos, int mode);

/* _open.c */
int __open(const char *name, int mode, ...);

/* _read.c */
int __read(int fd, void *buffer, unsigned int length);

/* _write.c */
int __write(int fd, const void *buffer, unsigned int length);

#endif

#endif /* !CLIB_NETLIB_PROTOS_H */
