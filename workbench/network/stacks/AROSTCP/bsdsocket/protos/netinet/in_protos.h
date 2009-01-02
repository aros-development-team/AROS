/* Prototypes for functions defined in
in.c
 */

struct in_addr in_makeaddr(u_long net,
                           u_long host);

u_long in_netof(struct in_addr in);

void in_sockmaskof(struct in_addr in,
                  register struct sockaddr_in * sockmask);

u_long in_lnaof(struct in_addr in);

int in_localaddr(struct in_addr in);

int in_canforward(struct in_addr in);

int in_control(struct socket * so,
               int cmd,
               caddr_t data,
               register struct ifnet * ifp);

void in_ifscrub(register struct ifnet * ifp,
               register struct in_ifaddr * ia);

int in_ifinit(register struct ifnet * ifp,
              register struct in_ifaddr * ia,
              struct sockaddr_in * sin,
              int scrub);

struct in_ifaddr * in_iaonnetof(u_long net);

int in_broadcast(struct in_addr in);

