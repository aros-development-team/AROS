/* Prototypes for functions defined in
raw_ip.c
 */

void rip_input(void *args, ...);

int rip_output(void *args,...);

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

