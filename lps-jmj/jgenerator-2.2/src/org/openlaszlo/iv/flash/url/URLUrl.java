/*
 * $Id: URLUrl.java,v 1.4 2002/04/16 12:40:16 ptalbot Exp $
 *
 * ==========================================================================
 *
 * The JGenerator Software License, Version 1.0
 *
 * Copyright (c) 2000 Dmitry Skavish (skavish@usa.net). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution, if
 *    any, must include the following acknowlegement:
 *    "This product includes software developed by Dmitry Skavish
 *     (skavish@usa.net, http://www.flashgap.com/)."
 *    Alternately, this acknowlegement may appear in the software itself,
 *    if and wherever such third-party acknowlegements normally appear.
 *
 * 4. The name "The JGenerator" must not be used to endorse or promote
 *    products derived from this software without prior written permission.
 *    For written permission, please contact skavish@usa.net.
 *
 * 5. Products derived from this software may not be called "The JGenerator"
 *    nor may "The JGenerator" appear in their names without prior written
 *    permission of Dmitry Skavish.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL DMITRY SKAVISH OR THE OTHER
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

package org.openlaszlo.iv.flash.url;

import java.io.*;
import java.net.*;
import java.util.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;


/**
 * Implementation of regular url
 *
 * @author Dmitry Skavish
 */
public class URLUrl extends IVUrl {

    private URL url;
    private URLConnection conn;
    private long lastModified = 0;
    private long lastConnect = 0;

    /**
     * Creates URLUrl from URL
     *
     * @param url    specified URL
     * @exception IVException
     */
    public URLUrl( URL url ) throws IVException {
        this.url = url;
    }

    public String getParameter( String name ) {
        if( parms == null ) {
            try {
                parse( getName() );
            } catch( IVException e ) {
                Log.logRB(e);
            }
            if( parms == null ) {
                parms = new Hashtable();
            }
        }
        return super.getParameter(name);
    }

    public String getName() {
        return url.toExternalForm();
    }

    public String getRef() {
        return url.getRef();
    }

    public long lastModified() {
        return lastModified;
    }

    public InputStream getInputStream() throws IOException {
        connect();
        Log.logRB( Resource.RETRIEVINGCONTENT, new Object[] {getName()} );
        return conn.getInputStream();
    }

    public void refresh() {
        try {
            connect();
        } catch( IOException e ) {
            Log.logRB( e );
        }
    }

    private synchronized void connect() throws IOException {
        long now = System.currentTimeMillis();
        if( lastConnect == 0 || lastConnect+500 < now ) {
            Log.logRB( Resource.CONNECTINGTO, new Object[] {getName()} );
            String auth = setProxy();
            conn = url.openConnection();
            if ( auth!=null )
                conn.setRequestProperty( "Proxy-Authorization", auth );
            conn.connect();
            lastModified = conn.getLastModified();
            lastConnect = System.currentTimeMillis();
        }
    }

    private String setProxy() {
        String auth = null;
        String useProxy = PropertyManager.getProperty( "org.openlaszlo.iv.flash.http.proxy.enable" );
        if ( "true".equalsIgnoreCase( useProxy ) )
        {
            String proxy = PropertyManager.getProperty( "org.openlaszlo.iv.flash.http.proxy.host" );
            String proxyport = PropertyManager.getProperty( "org.openlaszlo.iv.flash.http.proxy.port" );
            String myUserName = PropertyManager.getProperty( "org.openlaszlo.iv.flash.http.proxy.username" );
            String myPassword = PropertyManager.getProperty( "org.openlaszlo.iv.flash.http.proxy.password" );
            if( proxy != null)
            {
                Properties props = System.getProperties();
                props.put( "proxySet", "true" );
                props.put( "proxyHost", proxy );
                if ( proxyport != null )
                    props.put( "proxyPort", proxyport );
                else
                    props.put( "proxyPort", "80" );
                if ( myUserName!=null && myUserName.trim().length()>0 )
                {
                    String authString = myUserName + ":" +
                                        ( ( myPassword!=null && myPassword.trim().length()>0 ) ? myPassword : "" );
                    auth = "Basic " + new sun.misc.BASE64Encoder().encode(authString.getBytes());
                }
            }
        }
        return auth;
    }
}
