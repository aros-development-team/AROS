/* Prototypes for functions defined in
ip_input.c
 */

void ip_init(void);

void ipintr(void);

struct ip * ip_reass(register struct ipasfrag * ip,
                     register struct ipq * fp);

void ip_freef(struct ipq * fp);

void ip_enq(register struct ipasfrag * p,
	    register struct ipasfrag * prev);

void ip_deq(register struct ipasfrag * p);

void ip_slowtimo(void);

void ip_drain(void);

int ip_dooptions(struct mbuf * m);

struct in_ifaddr * ip_rtaddr(struct in_addr dst);

void save_rte(u_char * option,
	      struct in_addr dst);

struct mbuf * ip_srcroute(void);

void ip_stripoptions(register struct mbuf * m,
		     struct mbuf * mopt);

void ip_forward(struct mbuf * m,
		int srcrt);
