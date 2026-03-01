/* Prototypes for functions defined in rtsock.c
 */

int route_usrreq(struct socket * so,
                 int req,
                 struct mbuf * m,
                 struct mbuf * nam,
                 struct mbuf * control);

int STKARGFUN route_output(struct mbuf * m,
                           struct socket * so);

void rt_setmetrics(u_long which,
                  register struct rt_metrics * in,
                  register struct rt_metrics * out);

void m_copyback(struct	mbuf *,	register int, register int, caddr_t);

void rt_missmsg(int, register struct sockaddr *, struct sockaddr *,
		struct sockaddr *, struct sockaddr *, int, int);

void rt_ifmsg(struct ifnet * ifp);

void rt_newaddrmsg(int cmd,
                   struct ifaddr * ifa,
                   int error,
                   struct rtentry * rt);

struct walkarg;
int rt_walk(register struct radix_node * rn,
            register int (* f)(),
            struct walkarg * w);

