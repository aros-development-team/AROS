#ifndef OOP_PROXY_H
#define OOP_PROXY_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Include file for proxy class
    Lang: english
*/

extern ULONG __OOPI_Proxy;

#define IID_Proxy "Proxy"
#define CLID_Proxy "proxyclass"

#define ProxyAttrBase (__IProxy)


enum {
    aoProxy_RealObject = 0, /* The object we are a proxy for */
    aoProxy_Port,
    num_Proxy_Attrs
};

#define aProxy_RealObject	(ProxyAttrBase + aoProxy_RealObject)
#define aProxy_Port		(ProxyAttrBase + aoProxy_Port)


#endif /* OOP_PROXY_H */
