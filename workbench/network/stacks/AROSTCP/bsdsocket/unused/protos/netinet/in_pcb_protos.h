/* Prototypes for functions defined in
in_pcb.c
 */

int in_pcballoc(struct socket * so,
                struct inpcb * head);

int in_pcbbind(register struct inpcb * inp,
               struct mbuf * nam);

int in_pcbconnect(register struct inpcb * inp,
                  struct mbuf * nam);

void in_pcbdisconnect(struct inpcb * inp);

void in_pcbdetach(struct inpcb * inp);

void in_setsockaddr(register struct inpcb * inp,
                   struct mbuf * nam);

void in_setpeeraddr(struct inpcb * inp,
                   struct mbuf * nam);

void in_pcbnotify(struct inpcb * head,
                 struct sockaddr * dst,
                 u_short fport,
                 struct in_addr laddr,
                 u_short lport,
                 int cmd,
                 void (* notify)(register struct inpcb * inp, int error));

void in_losing(struct inpcb * inp);

void in_rtchange(register struct inpcb * inp, int error);

struct inpcb * in_pcblookup(struct inpcb * head,
                            struct in_addr faddr,
                            u_short fport,
                            struct in_addr laddr,
                            u_short lport,
			    int flags);






