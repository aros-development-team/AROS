/* Prototypes for functions defined in
tcp_subr.c
 */

void tcp_init(void);

struct tcpiphdr * tcp_template(struct tcpcb * tp);

void tcp_respond(struct tcpcb * tp,
                register struct tcpiphdr * ti,
                register struct mbuf * m,
                tcp_seq ack,
                tcp_seq seq,
                int flags);

struct tcpcb * tcp_newtcpcb(struct inpcb * inp);

struct tcpcb * tcp_drop(register struct tcpcb * tp,
                        int error);

struct tcpcb * tcp_close(register struct tcpcb * tp);

void tcp_drain(void);

void tcp_notify(register struct inpcb * inp,
               int error);

void tcp_ctlinput(int cmd,
                 struct sockaddr * sa,
                 register struct ip * ip);

void tcp_quench(struct inpcb * inp, int error);
