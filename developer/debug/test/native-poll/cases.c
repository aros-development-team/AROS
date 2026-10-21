/* Copyright (C) 2026 John KNIPPER <code@jkn.me>.
 * Licensed under the AROS Public License (APL), Version 1.1. */
static int checks;
#define CASE(x) do{assert(x);checks++;}while(0)
static LONG operation(ULONG op,LONG fd,LONG a,LONG b){
 struct aros_socket_request r={.version=AROS_SOCKET_ABI,.operation=op,.fd=fd,.arg0=a,.arg1=b};
 CASE(native_call(&r,NULL)==0);return r.result;
}
static LONG batch(struct aros_socket_pollfd *fds,ULONG n,LONG timeout,LONG expected_error){
 struct aros_socket_request r={.version=AROS_SOCKET_ABI,.operation=AROS_SOCKET_POLL_BATCH,.buffer=fds,.length=n,.arg1=timeout};
 LONG e=native_call(&r,NULL);CASE(e==expected_error);CASE(!contexts);CASE(!armed);return r.result;
}
static struct socket *socket_for(LONG fd){
 struct fd_lease lease={0};CASE(!FD_Acquire(fd,&lease));struct socket *so=((struct native_binding*)lease.data)->object->socket;FD_Release(&lease);return so;
}
static void init_lock(struct SignalSemaphore *s){pthread_mutexattr_t a;assert(!pthread_mutexattr_init(&a));assert(!pthread_mutexattr_settype(&a,PTHREAD_MUTEX_RECURSIVE));assert(!pthread_mutex_init(&s->mutex,&a));pthread_mutexattr_destroy(&a);}
struct race {LONG fd;struct socket *old;};
static void *poll_worker(void *arg){
 struct race *r=arg;struct aros_socket_pollfd p={r->fd,AROS_POLL_IN,0};
 CASE(batch(&p,1,500,0)==1);CASE(p.revents==AROS_POLL_IN);return NULL;
}
int main(void){
 init_lock(&registry.fd_Lock);init_lock(&syscall_semaphore);init_lock(&baselist_semaphore);init_lock(&select_semaphore);
 registry.fd_LibNode.lib_Version=51;CASE(native_socket_setup());
 LONG a=operation(AROS_SOCKET_CREATE,0,2,SOCK_STREAM),b=operation(AROS_SOCKET_CREATE,0,2,SOCK_STREAM);
 struct socket *sa=socket_for(a),*sb=socket_for(b);
 struct aros_socket_pollfd fds[3]={{a,AROS_POLL_IN,99},{b,AROS_POLL_OUT,99},{-3,-1,99}};
 sa->so_rcv.sb_cc=4;sb->so_snd.sb_cc=10;
 CASE(batch(fds,3,100,0)==2);CASE(fds[0].revents==AROS_POLL_IN);CASE(fds[1].revents==AROS_POLL_OUT);CASE(fds[2].revents==0);CASE(!timer_starts);
 fds[1].fd=a;fds[1].events=AROS_POLL_IN;CASE(batch(fds,2,0,0)==2);
 fds[1].fd=999;fds[1].events=0;CASE(batch(fds,2,-1,0)==2);CASE(fds[1].revents==AROS_POLL_NVAL);CASE(!timer_starts);
 /* Data which was not requested must not create a false result or hot loop. */
 fds[0].events=0;spurious=3;long long start=now_ns();CASE(batch(fds,1,25,0)==0);
 CASE(now_ns()-start>=20000000);CASE(timer_starts==1&&timer_aborts==1&&wait_calls==4);CASE(fds[0].revents==0);
 sa->so_error=ECONNRESET;CASE(batch(fds,1,-1,0)==1);CASE(fds[0].revents==AROS_POLL_ERR);CASE(sa->so_error==ECONNRESET);sa->so_error=0;
 sa->so_state=SS_CANTRCVMORE|SS_CANTSENDMORE;fds[0].events=AROS_POLL_IN|AROS_POLL_OUT;
 CASE(batch(fds,1,-1,0)==1);CASE(fds[0].revents==(AROS_POLL_HUP|AROS_POLL_IN));
 fds[0].events=0;CASE(batch(fds,1,-1,0)==1);CASE(fds[0].revents==AROS_POLL_HUP);
 sa->so_state=SS_CANTRCVMORE;fds[0].events=AROS_POLL_IN;CASE(batch(fds,1,0,0)==1);CASE(fds[0].revents==AROS_POLL_IN);
 sa->so_type=SOCK_DGRAM;sa->so_state=0;sa->so_rcv.sb_cc=0;CASE(batch(fds,1,0,0)==0);CASE(fds[0].revents==0);
 sa->so_rcv.sb_mb=sa;CASE(batch(fds,1,0,0)==1);CASE(fds[0].revents==AROS_POLL_IN);
 sa->so_state=SS_CANTRCVMORE|SS_CANTSENDMORE;CASE(batch(fds,1,0,0)==1);CASE(!(fds[0].revents&AROS_POLL_HUP));
 sa->so_rcv.sb_mb=NULL;sa->so_state=0;sa->so_type=SOCK_STREAM;
 sa->so_oobmark=1;fds[0].events=AROS_POLL_PRI;CASE(batch(fds,1,0,0)==1);CASE(fds[0].revents==AROS_POLL_PRI);sa->so_oobmark=0;
 start=now_ns();CASE(batch(NULL,0,10,0)==0);CASE(now_ns()-start>=8000000);
 interrupt_wait=1;CASE(batch(NULL,0,-1,EINTR)==-1);interrupt_wait=0;
 /* A failure before wait-buffer allocation must not leave an armed timer. */
 int starts=timer_starts;fail_buffer=2;CASE(batch(NULL,0,50,ENOMEM)==-1);CASE(timer_starts==starts);fail_buffer=0;
 CASE(batch(NULL,1,0,EINVAL)==-1);CASE(batch(fds,1,-2,EINVAL)==-1);
 LONG posixfd;CASE(!FD_Alloc(3,FD_OWNER_POSIXC,NULL,&posixfd));fds[0].fd=posixfd;CASE(batch(fds,1,0,EOPNOTSUPP)==-1);CASE(!FD_Free(posixfd,FD_OWNER_POSIXC,&registry));
 /* The legacy select entry still scans/copies its original bitset format. */
 struct SocketBase *legacy=api_hookbase_open();legacy->dTable=&sa;legacy->dTableSize=1;
 fd_set reads,writes,excepts;FD_ZERO(&reads);FD_ZERO(&writes);FD_ZERO(&excepts);FD_SET(0,&reads);FD_SET(0,&writes);FD_SET(0,&excepts);
 struct timeval zero={0,0};sa->so_rcv.sb_cc=1;sa->so_snd.sb_cc=1;sa->so_oobmark=1;
 CASE(__WaitSelect(1,&reads,&writes,&excepts,&zero,NULL,legacy)==3);
 CASE(FD_ISSET(0,&reads)&&FD_ISSET(0,&writes)&&FD_ISSET(0,&excepts));
 freeDataBuffer(&legacy->selitems);legacy->dTable=NULL;legacy->dTableSize=0;api_hookbase_close(legacy);sa->so_rcv.sb_cc=0;sa->so_snd.sb_cc=0;sa->so_oobmark=0;
 /* Failed allocations must release prior leases/provider admission. */
 fds[0].fd=a;fds[0].events=AROS_POLL_IN;
 for(int n=1;n<=6;n++){
  int before=atomic_load(&allocations);atomic_store(&fail_at,n);
  struct aros_socket_request r={.operation=AROS_SOCKET_POLL_BATCH,.buffer=fds,.length=1,.arg1=0};
  LONG e=native_call(&r,NULL);CASE(e==0||e==ENOMEM);atomic_store(&fail_at,0);
  CASE(atomic_load(&allocations)==before);CASE(!contexts&&!registry.fd_OwnerUsers[FD_OWNER_NATIVE_SOCKET]);
 }
 atomic_store(&fail_context,1);CASE(batch(fds,1,0,ENOMEM)==-1);
 /* Close returns while the admitted wait retains the old socket identity. */
 struct race race={a,sa};pthread_t thread;atomic_store(&paused_wait,1);CASE(!pthread_create(&thread,NULL,poll_worker,&race));
 while(!atomic_load(&wait_entered))sleep_ns(1000000);
 operation(AROS_SOCKET_CLOSE,a,0,0);LONG replacement=operation(AROS_SOCKET_CREATE,0,2,SOCK_STREAM);CASE(replacement==a);CASE(socket_for(a)!=sa);
 ObtainSemaphore(&syscall_semaphore);sa->so_rcv.sb_cc=1;ReleaseSemaphore(&syscall_semaphore);atomic_store(&release_wait,1);CASE(!pthread_join(thread,NULL));
 operation(AROS_SOCKET_CLOSE,replacement,0,0);operation(AROS_SOCKET_CLOSE,b,0,0);
 CASE(!native_objects&&master.lib_OpenCnt==10&&!contexts);CASE(native_socket_withdraw());
 printf("PASS actual native batch poll + shared wait engine: %d checks, real host deadlines, modeled Exec transport, close/reuse retained identity\n",checks);
 return 0;
}
