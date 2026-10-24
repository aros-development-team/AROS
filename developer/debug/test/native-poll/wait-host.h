/* Copyright (C) 2026 John KNIPPER <code@jkn.me>.
 * Licensed under the AROS Public License (APL), Version 1.1.
 * Modeled Exec timer/selection transport for the actual shared wait engine.
 */
#include <time.h>
#include <strings.h>
#define FREAD 1
#define FWRITE 2
typedef ULONG fd_mask;
typedef unsigned int u_int;
#define tv_secs tv_sec
#define tv_micro tv_usec
#define NFDBITS 32
#define howmany(x,y) (((x)+(y)-1)/(y))
#ifndef ERESTART
#define ERESTART 999
#endif
#define aligned_bzero(p,n) memset(p,0,n)
#define aligned_bcopy(a,b,n) memcpy(b,a,n)
#define CHECK_TASK() assert(libPtr->thisTask==FindTask(NULL))
#define API_STD_RETURN(e,r) do{if(e){libPtr->defErrno=(e);return -1;}return (r);}while(0)
static struct SignalSemaphore select_semaphore;
static _Thread_local long long deadline;
static _Thread_local int armed,timer_starts,timer_aborts,wait_calls,spurious,interrupt_wait,fail_buffer;
static atomic_int paused_wait,wait_entered,release_wait;
static long long now_ns(void){struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return (long long)t.tv_sec*1000000000+t.tv_nsec;}
static void sleep_ns(long long n){if(n>0){struct timespec t={n/1000000000,n%1000000000};while(nanosleep(&t,&t)&&errno==EINTR){}}}
static BOOL allocDataBuffer(struct DataBuffer *b,size_t n){
 if(fail_buffer>0&&--fail_buffer==0)return FALSE;
 if(n<=b->db_Size)return TRUE;
 void *p=calloc(1,n);if(!p)return FALSE;
 if(b->db_Size)memcpy(p,b->db_Addr,b->db_Size);
 free(b->db_Addr);b->db_Addr=p;b->db_Size=n;return TRUE;
}
static void ObtainSyscallSemaphore(struct SocketBase *c){(void)c;ObtainSemaphore(&syscall_semaphore);}
static void ReleaseSyscallSemaphore(struct SocketBase *c){(void)c;ReleaseSemaphore(&syscall_semaphore);}
static void sbselqueue(struct sockbuf *buffer,struct SocketBase *c){
 (void)buffer;if(c->p_sb)c->p_sb->s_count++;
}
static void unselect(struct newselbuf *b){b->s_count=0;}
static int itimerfix(struct timeval *t){return t->tv_sec<0||t->tv_usec<0||t->tv_usec>=1000000?EINVAL:0;}
static void tsleep_send_timeout(struct SocketBase *c,struct timeval *t){
 (void)c;assert(!armed);armed=1;timer_starts++;deadline=now_ns()+t->tv_sec*1000000000LL+t->tv_usec*1000LL;
}
static void tsleep_abort_timeout(struct SocketBase *c,struct timeval *t){(void)c;(void)t;assert(armed);armed=0;timer_aborts++;}
static void tsleep_enter(struct SocketBase *c,caddr_t b,const char *why){(void)c;(void)b;(void)why;}
static int tsleep_main(struct SocketBase *c,ULONG mask){
 (void)c;(void)mask;wait_calls++;
 if(interrupt_wait){sleep_ns(1000000);return ERESTART;}
 if(atomic_exchange(&paused_wait,0)){
  atomic_store(&wait_entered,1);
  while(!atomic_load(&release_wait))sleep_ns(1000000);
  return 0;
 }
 if(spurious>0){spurious--;sleep_ns(1000000);return 0;}
 assert(armed);sleep_ns(deadline-now_ns());return EWOULDBLOCK;
}
static ULONG SetSignal(ULONG set,ULONG mask){(void)set;(void)mask;return 0;}
static int selscan(struct SocketBase *,fd_set*,fd_set*,fd_set*,fd_mask*,int,int*,int*);
static int soo_select(struct socket *,int,struct SocketBase *);
