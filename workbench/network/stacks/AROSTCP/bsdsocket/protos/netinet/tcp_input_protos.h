/* Prototypes for functions defined in
tcp_input.c
 */

int tcp_reass(register struct tcpcb * tp,
              register struct tcpiphdr * ti,
              struct mbuf * m);

void STKARGFUN tcp_input(register struct mbuf * m,
			 int iphlen);

void tcp_dooptions(struct tcpcb * tp,
                  struct mbuf * om,
                  struct tcpiphdr * ti);

void tcp_pulloutofband(struct socket * so,
                      struct tcpiphdr * ti,
                      register struct mbuf * m);

void tcp_xmit_timer(register struct tcpcb * tp);

int tcp_mss(register struct tcpcb * tp,
            u_short offer);

