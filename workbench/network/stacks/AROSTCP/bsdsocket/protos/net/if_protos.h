/* Prototypes for functions defined in if.c
 */

void ifinit(void);

void if_attach(struct ifnet * ifp);

struct ifaddr * ifa_ifwithaddr(register struct sockaddr * addr);

struct ifaddr * ifa_ifwithdstaddr(register struct sockaddr * addr);

struct ifaddr * ifa_ifwithnet(struct sockaddr * addr);

struct ifaddr * ifa_ifwithaf(register int af);

struct ifaddr * ifaof_ifpforaddr(struct sockaddr * addr,
                                 register struct ifnet * ifp);

void link_rtrequest(int cmd,
                   struct rtentry * rt,
                   struct sockaddr * sa);

void if_down(register struct ifnet * ifp);

void if_qflush(register struct ifqueue * ifq);

void if_slowtimo(void);

struct ifnet * ifunit(register char * name);

int ifioctl(struct socket * so,
            int cmd,
            caddr_t data);

void ifupdown(struct ifnet *ifp, int up);

int ifconf(int cmd,
           caddr_t data);

