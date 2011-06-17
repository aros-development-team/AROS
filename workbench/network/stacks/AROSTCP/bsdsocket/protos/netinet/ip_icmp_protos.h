/* Prototypes for functions defined in
ip_icmp.c
 */

void icmp_error(struct mbuf * n,
               int type,
               int code,
               struct in_addr dest);

void icmp_input(void *arg, ...);

void icmp_reflect(struct mbuf * m);

struct in_ifaddr * ifptoia(struct ifnet * ifp);

void icmp_send(register struct mbuf * m,
              struct mbuf * opts);

n_time iptime(void);
