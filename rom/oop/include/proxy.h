#ifndef OOP_PROXY_H
#define OOP_PROXY_H

/*
    Copyright 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Include file for proxy class
    Lang: english
*/

extern ULONG __OOPI_Proxy;

#define IID_Proxy "Proxy"
#define CLID_Proxy "proxyclass"

#define ProxyBase (__OOPI_Proxy)

#define IsProxyAttr(attr) \
    (((attr) & ~(METHOD_MASK)) == (__OOPI_Proxy))


enum {
    AO_Proxy_RealObject = 0, /* The object we are a proxy for */
    AO_Proxy_Port,
    NUM_A_Proxy
};

#define A_Proxy_RealObject	(ProxyBase + AO_Proxy_RealObject)
#define A_Proxy_Port		(ProxyBase + AO_Proxy_Port)


#endif /* OOP_PROXY_H */
