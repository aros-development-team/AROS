/* Copyright (C) 2026 John KNIPPER <code@jkn.me>.
 * Distributed under the AROS Public License 1.1. */
#define _GNU_SOURCE 1
#define _LARGEFILE64_SOURCE 1
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static int failures, checks;
#define CHECK(c) do { checks++; if (!(c)) { failures++; printf("[LSTAT] FAIL line=%d expression=%s errno=%d\n", __LINE__, #c, errno); } } while(0)
static void both(const char *path, mode_t type) {
 struct stat a={0}; struct stat64 b={0};
 CHECK(lstat(path,&a)==0); CHECK((a.st_mode&S_IFMT)==type);
 CHECK(lstat64(path,&b)==0); CHECK((b.st_mode&S_IFMT)==type);
}
int main(int argc,char **argv) {
 char file[512],live[512],dead[512],missing[512],raw[512],dir[512],child[512],parentlink[512],aliaschild[512];
 int fd=-1,created=0;
 if(argc!=2||!strchr(argv[1],':')||strlen(argv[1])>400){puts("[LSTAT] usage: LstatProbe RAM:unique-new-root");return 20;}
 #define PATH(v,s) snprintf(v,sizeof(v),"%s/%s",argv[1],s)
 PATH(file,"referent");PATH(live,"live-link");PATH(dead,"dangling-link");PATH(missing,"missing");PATH(raw,"raw-\xe9-link");PATH(dir,"directory");PATH(child,"directory/child");PATH(parentlink,"parent-link");PATH(aliaschild,"parent-link/child");
 if(mkdir(argv[1],0700)){printf("[LSTAT] root-create FAIL errno=%d\n",errno);return 20;}created=1;
 fd=open(file,O_CREAT|O_EXCL|O_WRONLY,0600);CHECK(fd>=0);if(fd>=0){CHECK(write(fd,"referent-safe",13)==13);CHECK(close(fd)==0);fd=-1;}
 CHECK(symlink(file,live)==0);CHECK(symlink(missing,dead)==0);CHECK(symlink(missing,raw)==0);
 both(file,S_IFREG);both(live,S_IFLNK);both(dead,S_IFLNK);both(raw,S_IFLNK);both(argv[1],S_IFDIR);both("RAM:",S_IFDIR);
 {struct stat a={0};struct stat64 b={0};CHECK(stat(live,&a)==0&&S_ISREG(a.st_mode));CHECK(stat64(live,&b)==0&&S_ISREG(b.st_mode));
 errno=0;CHECK(stat(dead,&a)==-1&&errno==ENOENT);errno=0;CHECK(stat64(dead,&b)==-1&&errno==ENOENT);
 errno=0;CHECK(lstat(missing,&a)==-1&&errno==ENOENT);errno=0;CHECK(lstat64(missing,&b)==-1&&errno==ENOENT);}
 CHECK(mkdir(dir,0700)==0);fd=open(child,O_CREAT|O_EXCL|O_WRONLY,0600);CHECK(fd>=0);if(fd>=0){CHECK(close(fd)==0);fd=-1;}
 if(symlink(dir,parentlink)==0){int before=failures;both(parentlink,S_IFLNK);both(aliaschild,S_IFREG);printf("[LSTAT] parent-link result=%s\n",failures==before?"PASS":"FAIL");}
 else printf("[LSTAT] parent-link result=UNSUPPORTED errno=%d\n",errno);
 {char buffer[32]={0};fd=open(file,O_RDONLY);CHECK(fd>=0);if(fd>=0){CHECK(read(fd,buffer,sizeof(buffer))==13&&!memcmp(buffer,"referent-safe",13));CHECK(close(fd)==0);fd=-1;}}
 if(created){CHECK(unlink(raw)==0);CHECK(unlink(dead)==0);CHECK(unlink(live)==0);if(unlink(parentlink)&&errno!=ENOENT)CHECK(0);CHECK(unlink(child)==0);CHECK(rmdir(dir)==0);CHECK(unlink(file)==0);CHECK(rmdir(argv[1])==0);}
 printf("[LSTAT] result=%s checks=%d failures=%d scope=native-lstat32-lstat64\n",failures?"FAIL":"PASS",checks,failures);return failures?20:0;
}
