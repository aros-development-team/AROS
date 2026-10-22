/* Copyright (C) 2026 John KNIPPER <code@jkn.me>.
 * Licensed under the AROS Public License (APL), Version 1.1. */
static LONG call(ULONG op,LONG fd,LONG a,LONG b,LONG c){
 struct aros_socket_request r={.version=AROS_SOCKET_ABI,.operation=op,.fd=fd,.arg0=a,.arg1=b,.arg2=c};
 assert(native_call(&r,NULL)==0);return r.result;
}
static void *creator(void *unused){(void)unused;return (void *)(intptr_t)call(AROS_SOCKET_CREATE,0,2,SOCK_STREAM,0);}
static void *reader(void *arg){
 char bytes[8];LONG error;
 SIPTR n=__fdhook((int)(intptr_t)arg,__FDH_READ,(IPTR)bytes,sizeof(bytes),0,&error);
 assert(n==8&&error==0&&bytes[0]==0x42);return NULL;
}
static void *cloner(void *arg){
 LONG fd=(intptr_t)arg;
 for(int i=0;i<128;i++){
  LONG copy=call(AROS_SOCKET_CLONE,fd,3,0,0);
  call(AROS_SOCKET_CLOSE,copy,0,0,0);
 }
 return NULL;
}
static void init_lock(struct SignalSemaphore *s){
 pthread_mutexattr_t attr;assert(!pthread_mutexattr_init(&attr));
 assert(!pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE));
 assert(!pthread_mutex_init(&s->mutex,&attr));pthread_mutexattr_destroy(&attr);
}
int main(void){
 init_lock(&registry.fd_Lock);init_lock(&syscall_semaphore);init_lock(&baselist_semaphore);
 registry.fd_LibNode.lib_Version=51;
 assert(native_socket_setup());
 /* A reservation without a provider has no admitted callable hook table. */
 int orphan=1;LONG orphan_fd,error;
 assert(FD_Alloc(3,4,&orphan,&orphan_fd)==0);
 assert(__fdhook(orphan_fd,__FDH_READ,0,0,0,&error)==-1&&error==EBADF);
 assert(FD_Free(orphan_fd,4,&registry)==0);
 struct SocketBase base={.thisTask=FindTask(NULL)};
 struct aros_socket_request invalid={.version=99};
 assert(NativeSocket(NULL,sizeof(invalid),&base)==EINVAL);
 assert(NativeSocket(&invalid,sizeof(invalid),&base)==EINVAL);
 pthread_t thread;void *value;
 assert(!pthread_create(&thread,NULL,creator,NULL));assert(!pthread_join(thread,&value));
 LONG fd=(intptr_t)value;char buffer[16];
 assert(fd>=3&&native_objects==1&&master.lib_OpenCnt==11&&!contexts);
 assert(__fdhook(fd,__FDH_READ,(IPTR)buffer,16,0,&error)==16&&error==0);
 /* Execute the actual ioctl wrapper too: no borrowed provider precheck. */
 int available=-1, nonblocking=1;
 assert(fixture_ioctl(fd,(int)FIONREAD,&available)==0&&available==0);
 assert(fixture_ioctl(fd,(int)FIONBIO,&nonblocking)==0);
 assert(fixture_ioctl(-1,(int)FIONREAD,&available)==-1&&errno==EBADF);

 assert(__fdhook(fd,__FDH_WRITE,(IPTR)buffer,16,0,&error)==16&&error==0);
 assert(__fdhook(fd,__FDH_FCNTL,F_SETFD,FD_CLOEXEC,0,&error)==0);
 assert(__fdhook(fd,__FDH_FCNTL,F_GETFD,0,0,&error)==FD_CLOEXEC);
 LONG clone=__fdhook(fd,__FDH_FCNTL,F_DUPFD,20,0,&error);
 assert(clone>=20&&error==0&&native_objects==1);
 assert(__fdhook(clone,__FDH_FCNTL,F_GETFD,0,0,&error)==0);
 call(AROS_SOCKET_CLOSE,fd,0,0,0);
 assert(__fdhook(clone,__FDH_READ,(IPTR)buffer,16,0,&error)==16);
 assert(__fdhook(clone,__FDH_FCNTL,F_SETFL,O_NONBLOCK,0,&error)==0);
 assert(__fdhook(clone,__FDH_FCNTL,F_GETFL,0,0,&error)&O_NONBLOCK);
 struct aros_socket_request r={.version=AROS_SOCKET_ABI,.fd=clone,.operation=AROS_SOCKET_POLL,.arg0=2,.arg1=1250};
 assert(native_call(&r,NULL)==0&&r.result==2&&!contexts);
 r.operation=AROS_SOCKET_RECV;r.buffer=buffer;r.length=(IPTR)INT_MAX+1;
 assert(native_call(&r,NULL)==EINVAL&&!contexts);
 atomic_store(&fail_context,1);r.length=16;
 assert(native_call(&r,NULL)==ENOMEM&&!contexts);
 struct linger linger={1,3};
 r.operation=AROS_SOCKET_SETOPT;r.arg0=SOL_SOCKET;r.arg1=SO_LINGER;r.buffer=&linger;r.length=sizeof(linger);
 assert(native_call(&r,NULL)==ENOPROTOOPT);
 assert(__fdhook(clone,__FDH_CLOSE,0,0,0,&error)==0&&error==0);
 assert(!native_objects&&master.lib_OpenCnt==10&&!contexts);
 /* A blocked call retains the old reservation/object while its exact integer
    is retired and reused. Neither old admission nor finalization can touch the
    replacement. The creator is already gone. */
 fd=call(AROS_SOCKET_CREATE,0,2,SOCK_STREAM,0);
 atomic_store(&pause_recv,1);
 int before=atomic_load(&destroyed_count);
 assert(!pthread_create(&thread,NULL,reader,(void *)(intptr_t)fd));
 while(!atomic_load(&recv_entered)){}
 call(AROS_SOCKET_CLOSE,fd,0,0,0);
 assert(atomic_load(&destroyed_count)==before&&!native_socket_withdraw());
 LONG replacement=call(AROS_SOCKET_CREATE,0,2,SOCK_DGRAM,0);
 assert(replacement==fd);
 atomic_store(&release_recv,1);assert(!pthread_join(thread,NULL));
 assert(atomic_load(&destroyed_count)==before+1&&FD_GetOwner(replacement)==FD_OWNER_NATIVE_SOCKET);
 assert(__fdhook(replacement,__FDH_WRITE,(IPTR)buffer,16,0,&error)==16);
 /* Atomic allocation-backed duplication is stress-tested against actual
    registry reservation code rather than a guessed free descriptor number. */
 for(int i=0;i<1000;i++){
  LONG copy=call(AROS_SOCKET_CLONE,replacement,3,0,0);
  assert(copy!=replacement);
  call(AROS_SOCKET_CLOSE,copy,0,0,0);
 }
 pthread_t cloners[4];
 for(int i=0;i<4;i++)assert(!pthread_create(&cloners[i],NULL,cloner,(void *)(intptr_t)replacement));
 for(int i=0;i<4;i++)assert(!pthread_join(cloners[i],NULL));
 assert(__fdhook(replacement,__FDH_CLOSE,0,0,0,&error)==0);
 assert(!native_objects&&!contexts&&!fd_library_refs&&master.lib_OpenCnt==10);
 /* Failure paths consume an unregistered socket and roll back allocations. */
 for(int failure=1;failure<=2;failure++){
  atomic_store(&fail_at,failure);
  r=(struct aros_socket_request){.version=1,.operation=AROS_SOCKET_CREATE,.arg0=2,.arg1=SOCK_STREAM};
  assert(native_call(&r,NULL)==ENOMEM&&!native_objects&&!contexts);
 }
 master.lib_OpenCnt=UINT16_MAX;
 r=(struct aros_socket_request){.version=1,.operation=AROS_SOCKET_CREATE,.arg0=2,.arg1=SOCK_STREAM};
 assert(native_call(&r,NULL)==EMFILE&&!native_objects&&!contexts);
 master.lib_OpenCnt=10;
 assert(native_socket_withdraw());
 assert(atomic_load(&created_count)==atomic_load(&destroyed_count));
 FreeVec(registry.fd_Table);
 assert(!atomic_load(&allocations));
 printf("PASS native owned socket admission/lifetime: creator exit, blocked read + close/reuse, 1512 clones (4 concurrent workers), failures; transport modeled\n");
 return 0;
}
