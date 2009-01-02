/* Prototypes for functions defined in
raw_ip.c
 */

void STKARGFUN rip_input(struct mbuf * m);

int STKARGFUN rip_output(register struct mbuf * m,
               struct socket * so);

int rip_ctloutput(int op,
                  struct socket * so,
                  int level,
                  int optname,
                  struct mbuf ** m);

int rip_usrreq(register struct socket * so,
               int req,
               struct mbuf * m,
               struct mbuf * nam,
             /*struct mbuf * rights,*/
               struct mbuf * control);

