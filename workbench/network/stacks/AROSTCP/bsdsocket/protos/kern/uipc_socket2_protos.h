/* Prototypes for functions defined in uipc_socket2.c
 */

void soisconnecting(register struct socket * so);

void soisconnected(register struct socket * so);

void soisdisconnecting(register struct socket * so);

void soisdisconnected(register struct socket * so);

struct socket * sonewconn(struct socket * , int );

void soqinsque(register struct socket * head,
              register struct socket * so,
              int q);

int soqremque(register struct socket * so,
              int q);

void socantsendmore(struct socket * so);

void socantrcvmore(struct socket * so);

void sbselqueue(struct sockbuf * sb,
               struct SocketBase * cp);

int sbwait(struct sockbuf * sb,
           struct SocketBase * cp);

int sb_lock(struct sockbuf * sb,
            struct SocketBase * cp);

void sowakeup(register struct socket * so,
             register struct sockbuf * sb);

void sorwakeup(struct socket *so);

void sowwakeup(struct socket *so);

void soevent(struct socket *so, u_long event);

int soreserve(register struct socket * so,
              u_long sndcc,
              u_long rcvcc);

int sbreserve(struct sockbuf * sb,
              u_long cc);

void sbrelease(struct sockbuf * sb);

void sbappend(struct sockbuf * sb,
             struct mbuf * m);

void sbcheck(register struct sockbuf * sb);

void sbappendrecord(register struct sockbuf * sb,
                   register struct mbuf * m0);

void sbinsertoob(register struct sockbuf * sb,
                register struct mbuf * m0);

int sbappendaddr(register struct sockbuf * sb,
                 struct sockaddr * asa,
                 struct mbuf * m0,
                 struct mbuf * control);

int sbappendcontrol(struct sockbuf * sb,
                    struct mbuf * m0,
                    struct mbuf * control);

void sbcompress(register struct sockbuf * sb,
               register struct mbuf * m,
               register struct mbuf * n);

void sbflush(register struct sockbuf * sb);

void sbdrop(register struct sockbuf * sb,
           register int len);

void sbdroprecord(register struct sockbuf * sb);

