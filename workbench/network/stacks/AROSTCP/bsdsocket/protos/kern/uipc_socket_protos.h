/* Prototypes for functions defined in uipc_socket.c
 */

#if 0
#include <sys/socket.h>
#define __BSD_VISIBLE 1
#include <sys/uio.h>
#endif

int socreate(int dom,
             struct socket ** aso,
             register int type,
             int proto);

int sobind(struct socket * so,
           struct mbuf * nam);

int solisten(register struct socket * so,
             int backlog);

void sofree(register struct socket * so);

int soclose(register struct socket * so);

int soabort(struct socket * so);

int soaccept(register struct socket * so,
             struct mbuf * nam);

int soconnect(register struct socket * so,
              struct mbuf * nam);

int sodisconnect(register struct socket * so);

int sosend(register struct socket * so,
           struct mbuf * addr,
           struct uio * uio,
           struct mbuf * top,
           struct mbuf * control,
           int flags);

int soreceive(register struct socket * so,
              struct mbuf ** paddr,
              struct uio * uio,
              struct mbuf ** mp0,
              struct mbuf ** controlp,
              int * flagsp);

int soshutdown(register struct socket * so,
               register int how);

void sorflush(register struct socket * so);

int sosetopt(register struct socket * so,
             int level,
             int optname,
             struct mbuf * m0);

int sogetopt(register struct socket * so,
             int level,
             int optname,
             struct mbuf ** mp);

void sohasoutofband(register struct socket * so);

