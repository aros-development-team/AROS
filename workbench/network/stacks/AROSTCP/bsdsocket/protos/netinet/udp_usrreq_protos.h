/* Prototypes for functions defined in
udp_usrreq.c
 */

void udp_init(void);

void udp_input(void *args, ...);

struct mbuf * udp_saveopt(caddr_t p,
                          register int size,
                          int type);

void udp_notify(register struct inpcb * inp,
		int error);

void udp_ctlinput(int cmd,
                 struct sockaddr * sa,
                 void *arg);

int udp_output(void *args, ...);

int udp_usrreq(struct socket * so,
               int req,
               struct mbuf * m,
               struct mbuf * addr,
               struct mbuf * control);

void udp_detach(struct inpcb * inp);


