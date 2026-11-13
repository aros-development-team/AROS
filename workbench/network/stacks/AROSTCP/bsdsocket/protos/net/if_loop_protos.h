void loattach(void);
void loconfig(void);
int looutput(struct ifnet *, 
	     register struct mbuf *, 
	     struct sockaddr *, 
	     register struct rtentry *);
void lortrequest(int, struct rtentry *, struct sockaddr *);
int loioctl(register struct ifnet *, int, caddr_t);
