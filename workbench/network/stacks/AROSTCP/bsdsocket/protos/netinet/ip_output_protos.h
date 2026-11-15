/* Prototypes for functions defined in
ip_output.c
 */

int ip_output(void *args,...);

/* ip_insertoptions() is ip_output.c's own (static) */

int ip_optcopy(struct ip * ip,
               struct ip * jp);

int ip_ctloutput(int op,
                 struct socket * so,
                 int level,
                 int optname,
                 struct mbuf ** mp);

int ip_pcbopts(struct mbuf ** pcbopt,
               register struct mbuf * m);

struct ip_moptions;
void ip_freemoptions(register struct ip_moptions * imo);
int ip_setmoptions(int optname, struct ip_moptions ** imop, struct mbuf * m);
int ip_getmoptions(int optname, struct ip_moptions * imo, struct mbuf ** mp);

