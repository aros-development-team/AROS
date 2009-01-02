/* Prototypes for functions defined in rtsock.c
 */


void rt_setmetrics(u_long which,
                  register struct rt_metrics * in,
                  register struct rt_metrics * out);

void m_copyback(struct	mbuf *,	register int, register int, caddr_t);

void rt_missmsg(int, register struct sockaddr *, struct sockaddr *,
		struct sockaddr *, struct sockaddr *, int, int);

struct walkarg;
int rt_walk(register struct radix_node * rn,
            register int (* f)(),
            struct walkarg * w);

