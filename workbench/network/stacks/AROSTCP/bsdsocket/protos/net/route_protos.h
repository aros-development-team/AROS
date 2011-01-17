void rtinitheads(void);
void rtalloc(struct route * ro);
struct rtentry * rtalloc1(struct sockaddr * dst, int report);
void rtfree(struct rtentry * rt);
void rtredirect(struct sockaddr * dst, struct sockaddr * gateway,
               struct sockaddr * netmask, int flags,
               struct sockaddr * src, struct rtentry ** rtp);
int rtioctl(int req, caddr_t data);
struct ifaddr * ifa_ifwithroute(int flags, struct sockaddr * dst,
                                struct sockaddr * gateway);
int rtrequest(int req, struct sockaddr * dst, struct sockaddr * gateway,
              struct sockaddr * netmask, int flags, struct rtentry ** ret_nrt);
void rt_maskedcopy(struct sockaddr * src, struct sockaddr * dst,
                  struct sockaddr * netmask);
int rtinit(struct ifaddr * ifa, int cmd, int flags);

