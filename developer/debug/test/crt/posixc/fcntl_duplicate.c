/* Finite public-API duplicate/CLOEXEC regression. Sole parent arg: self path. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <proto/dos.h>
#include "test.h"
void cleanup(void) {}
static int inspect(int fd,int *error) {errno=0;int rc=fcntl(fd,F_GETFD);*error=errno;return rc;}
int main(int argc,char **argv) {
 int e,flags;
 if(argc==4 && !strcmp(argv[1],"--child")) {
  int original=atoi(argv[2]),copy=atoi(argv[3]);
  flags=inspect(original,&e);printf("CHILD original=%d flags=%d errno=%d\n",original,flags,e);
  if(flags<0)return 21;
  flags=inspect(copy,&e);printf("CHILD duplicate=%d flags=%d errno=%d expected_EBADF=%d\n",copy,flags,e,EBADF);
  return flags==-1 && e==EBADF ? 0:22;
 }
 char program[1024];
 TEST(NameFromLock(GetProgramDir(), (STRPTR)program, sizeof(program)) != 0);
 TEST(AddPart((STRPTR)program, FilePart((CONST_STRPTR)argv[0]), sizeof(program)) != 0);
 int p[2],plain=-1,copy=-1,fail=0,status=0;char original_arg[32],copy_arg[32];
 if(pipe(p))return 3;
 TEST(fcntl(p[0],F_SETFD,FD_CLOEXEC)==0);
 int ordinary=dup(p[0]);
 TEST(ordinary>=0);
 TEST(fcntl(ordinary,F_GETFD)==0);
 TEST(close(ordinary)==0);
 TEST(fcntl(p[0],F_SETFD,0)==0);
 errno=0;TEST(fcntl(p[0],F_DUPFD_CLOEXEC,-1)==-1 && errno==EINVAL);
 errno=0;TEST(fcntl(-1,F_DUPFD_CLOEXEC,32)==-1 && errno==EBADF);
 printf("CONSTANTS F_DUPFD=%d F_DUPFD_CLOEXEC=%d F_GETFD=%d FD_CLOEXEC=%d\n",F_DUPFD,F_DUPFD_CLOEXEC,F_GETFD,FD_CLOEXEC);
 errno=0;plain=fcntl(p[0],F_DUPFD,32);e=errno;
 printf("DUPFD minimum=32 result=%d errno=%d\n",plain,e);
 if(plain<32)fail=10;
 else {flags=inspect(plain,&e);printf("DUPFD flags=%d errno=%d\n",flags,e);if(flags<0 || (flags&FD_CLOEXEC))fail=11;close(plain);}
 errno=0;copy=fcntl(p[0],F_DUPFD_CLOEXEC,32);e=errno;
 printf("DUPFD_CLOEXEC minimum=32 result=%d errno=%d\n",copy,e);
 if(copy<32)fail=12;
 else {flags=inspect(copy,&e);printf("DUPFD_CLOEXEC flags=%d errno=%d\n",flags,e);if(flags<0 || !(flags&FD_CLOEXEC))fail=13;}
 flags=inspect(p[0],&e);printf("ORIGINAL fd=%d flags=%d errno=%d\n",p[0],flags,e);if(flags<0)fail=14;
 fflush(stdout);
 if(copy>=32 && !fail) {
  snprintf(original_arg,sizeof(original_arg),"%d",p[0]);snprintf(copy_arg,sizeof(copy_arg),"%d",copy);
  pid_t child=vfork();
  if(child==0) {execl(program,program,"--child",original_arg,copy_arg,(char*)0);_exit(111);}
  if(child<0)fail=15;
  else {errno=0;pid_t got=waitpid(child,&status,0);e=errno;printf("COLLECT expected=%ld got=%ld status=%d errno=%d\n",(long)child,(long)got,status,e);if(got!=child || status)fail=16;}
 }
 if(copy>=0) {close(copy);}
 close(p[0]);close(p[1]);
 printf("FCNTL_DUPLICATE_RESULT=%d\n",fail);fflush(stdout);TEST(fail == 0);return OK;
}
