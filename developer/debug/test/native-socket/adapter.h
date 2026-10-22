/* Copyright (C) 2026 John KNIPPER <code@jkn.me>.
 * Licensed under the AROS Public License (APL), Version 1.1. */
static struct fd_base registry;
#define FD_Alloc(a,b,c,d) FD_Alloc(a,b,c,d,&registry)
#define FD_Reserve(a,b,c) FD_Reserve(a,b,c,&registry)
#define FD_Acquire(a,b) FD_Acquire(a,b,&registry)
#define FD_Release(a) FD_Release(a,&registry)
#define FD_Retire(a,b,c) FD_Retire(a,b,c,&registry)
#define FD_SetOwnerHooks(a,b) FD_SetOwnerHooks(a,b,&registry)
#define FD_GetData(a) FD_GetData(a,&registry)
#define FD_GetOwner(a) FD_GetOwner(a,&registry)
static struct Library *FDBase=&registry.fd_LibNode;
static struct Library master={10,1},*MasterSocketBase=&master;
static struct SignalSemaphore syscall_semaphore,baselist_semaphore;
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define SOCK_NONBLOCK 0x10000000
#define SS_NBIO 1
#define SS_CANTRCVMORE 2
#define SS_CANTSENDMORE 4
#define SS_RCVATMARK 8
#define SOL_SOCKET 65535
#define SO_LINGER 128
#define FIONBIO 0x8008667eU
#define FIONREAD 0x4008667fU
struct linger {int l_onoff;int l_linger;};
struct sockbuf {int sb_cc;void *sb_mb;};
struct socket {int so_refcnt,so_state,so_options,id,so_error,so_type,so_oobmark;
 struct sockbuf so_rcv,so_snd;void *so_pgid;};
typedef int spl_t;
static spl_t splnet(void){return 0;}
static void splx(spl_t value){(void)value;}
#define soreadable(so) ((so)->so_rcv.sb_cc || (so)->so_error || ((so)->so_state & SS_CANTRCVMORE))
#define sowriteable(so) ((so)->so_snd.sb_cc || (so)->so_error || ((so)->so_state & SS_CANTSENDMORE))
struct DataBuffer {void *db_Addr;size_t db_Size;};
struct SocketBase {void *thisTask;struct socket **dTable;int dTableSize,defErrno;
 struct DataBuffer selitems;struct newselbuf *p_sb;};
struct sockaddr {char bytes[16];};
#define timeval fixture_timeval
struct timeval {long tv_sec,tv_usec;};
typedef union {ULONG bits;ULONG fds_bits[8];} fd_set;
#define FD_ZERO(s) memset(s,0,sizeof(*(s)))
#define FD_SET(n,s) ((s)->fds_bits[(n)/32]|=1U<<((n)%32))
#define FD_ISSET(n,s) (((s)->fds_bits[(n)/32]>>((n)%32))&1)
static _Thread_local int task;
static atomic_int contexts,created_count,destroyed_count,fail_context;
static _Thread_local int last_seen;
static atomic_int pause_recv,recv_entered,release_recv;
static void *FindTask(void *unused){(void)unused;return &task;}
static struct SocketBase *api_hookbase_open(void){
 if(atomic_exchange(&fail_context,0))return NULL;
 struct SocketBase *p=calloc(1,sizeof(*p));assert(p);p->thisTask=FindTask(NULL);
 atomic_fetch_add(&contexts,1);return p;
}
static void api_hookbase_close(struct SocketBase *p){
 assert(p->thisTask==FindTask(NULL));assert(!p->dTable&&!p->selitems.db_Addr);
 atomic_fetch_sub(&contexts,1);free(p);
}
static void freeDataBuffer(struct DataBuffer *p){free(p->db_Addr);p->db_Addr=NULL;}
static struct socket *target(struct SocketBase *p){
 assert(p->thisTask==FindTask(NULL)&&p->dTableSize==1&&p->dTable[0]);
 last_seen=p->dTable[0]->id;return p->dTable[0];
}
static LONG socreate(int family,struct socket **so,int type,int protocol){
 (void)protocol;
 if(family!=2&&family!=28)return EAFNOSUPPORT;
 *so=calloc(1,sizeof(**so));assert(*so);(*so)->so_type=type;(*so)->id=atomic_fetch_add(&created_count,1)+1;return 0;
}
static LONG releaseSocketRef(struct socket *so){
 assert(so&&so->so_refcnt==1&&so->so_pgid==NULL&&!(so->so_options&SO_LINGER));
 so->so_refcnt=0;atomic_fetch_add(&destroyed_count,1);free(so);return 0;
}
static LONG native_bind(LONG fd,caddr_t addr,LONG len,struct SocketBase*p){(void)fd;(void)addr;(void)len;target(p);return 0;}
static LONG native_listen(LONG fd,LONG backlog,struct SocketBase*p){(void)fd;(void)backlog;target(p);return 0;}
static LONG native_accept(LONG fd,caddr_t addr,socklen_t *len,struct SocketBase*p,struct socket **so){
 (void)fd;(void)addr;(void)len;target(p);assert(so);LONG e=socreate(2,so,1,0);(*so)->so_refcnt=1;return e?-1:0;
}
static LONG __connect(LONG fd,const struct sockaddr*addr,LONG len,struct SocketBase*p){(void)fd;(void)addr;(void)len;target(p);return 0;}
static LONG __send(LONG fd,const char*b,LONG len,LONG flags,struct SocketBase*p){(void)fd;(void)b;(void)flags;target(p);return len;}
static LONG __recv(LONG fd,char*b,LONG len,LONG flags,struct SocketBase*p){
 (void)fd;(void)flags;struct socket *so=target(p);int id=so->id;
 if(atomic_exchange(&pause_recv,0)){
  atomic_store(&recv_entered,1);
  while(!atomic_load(&release_recv)){} /* test-controlled overlap, no OS wait modeled */
  assert(so->id==id&&so->so_refcnt==1);
 }
 if(len)memset(b,0x42,len);return len;
}
static LONG __sendto(LONG fd,const char*b,LONG len,LONG flags,struct sockaddr*addr,LONG alen,struct SocketBase*p){(void)addr;(void)alen;return __send(fd,b,len,flags,p);}
static LONG native_recvfrom(LONG fd,caddr_t b,LONG len,LONG flags,caddr_t addr,LONG*alen,struct SocketBase*p){(void)addr;*alen=16;return __recv(fd,b,len,flags,p);}
static LONG native_shutdown(LONG fd,LONG how,struct SocketBase*p){(void)fd;(void)how;target(p);return 0;}
static LONG native_getsockopt(LONG fd,LONG lev,LONG name,caddr_t b,ULONG*len,struct SocketBase*p){(void)fd;(void)lev;(void)name;target(p);if(*len>=4){*(int*)b=0;*len=4;}return 0;}
static LONG native_setsockopt(LONG fd,LONG lev,LONG name,caddr_t b,ULONG len,struct SocketBase*p){(void)fd;(void)lev;(void)name;(void)b;(void)len;target(p);return 0;}
static LONG native_getsockname(LONG fd,caddr_t a,ULONG*l,struct SocketBase*p){(void)fd;target(p);if(*l>=16){memset(a,0,16);*l=16;}return 0;}
static LONG native_getpeername(LONG fd,caddr_t a,ULONG*l,struct SocketBase*p){return native_getsockname(fd,a,l,p);}
static LONG __IoctlSocket(LONG fd,ULONG cmd,caddr_t b,struct SocketBase*p){(void)fd;assert(cmd==FIONBIO);struct socket*so=target(p);ObtainSemaphore(&syscall_semaphore);if(*(int*)b)so->so_state|=SS_NBIO;else so->so_state&=~SS_NBIO;ReleaseSemaphore(&syscall_semaphore);return 0;}
#ifndef NATIVE_POLL_ENGINE_TEST
static LONG __WaitSelect(ULONG n,fd_set*r,fd_set*w,fd_set*e,struct timeval*t,ULONG*sig,struct SocketBase*p){
 (void)sig;assert(n==1);target(p);p->selitems.db_Addr=calloc(1,10);
 assert(!t||(t->tv_sec>=0&&t->tv_usec>=0));
 e->bits=0;r->bits=0;return w->bits?1:0;
}
#endif
static void sbselqueue(struct sockbuf *buffer, struct SocketBase *context);
#ifndef NATIVE_POLL_ENGINE_TEST
static void sbselqueue(struct sockbuf *buffer, struct SocketBase *context){(void)buffer;(void)context;}
static LONG __WaitNativePoll(ULONG count,struct timeval *timeout,struct SocketBase *context,
 int (*scan)(struct SocketBase*,APTR,int*,int*),APTR data){
 int ready,items;(void)count;(void)timeout;assert(!scan(context,data,&ready,&items));return ready;
}
#endif
#define __FDH_READ 1
#define __FDH_WRITE 2
#define __FDH_CLOSE 3
#define __FDH_IOCTL 4
#define __FDH_FCNTL 5
#define __FDH_DUP 6
static atomic_int fd_library_refs;
static struct Library *OpenLibrary(const char*name,ULONG version){assert(!strcmp(name,"fd.library")&&version==51);atomic_fetch_add(&fd_library_refs,1);return FDBase;}
static void CloseLibrary(struct Library*base){assert(base==FDBase);assert(atomic_fetch_sub(&fd_library_refs,1)>0);}
typedef struct {void *fcb;} fdesc;
static fdesc *__getfdesc(int fd){(void)fd;return NULL;}
static int __getfirstfd(int minimum){return minimum;}
static int close(int fd){(void)fd;assert(0);return -1;}
