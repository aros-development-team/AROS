/******************************************************************************
 * LZPostMethod.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.utils;

import org.apache.commons.httpclient.HttpState;
import org.apache.commons.httpclient.HttpConnection;
import org.apache.commons.httpclient.methods.PostMethod;

/**
 * Special post method class that overrides the unfortunate cookie processing in
 * the httpclient 2.0-rc1 library.
 */
public class LZPostMethod extends PostMethod {
    protected void addCookieRequestHeader(HttpState s, HttpConnection c) {

    }

    protected void processResponseHeaders(HttpState state,
        HttpConnection conn) {
    }
}
