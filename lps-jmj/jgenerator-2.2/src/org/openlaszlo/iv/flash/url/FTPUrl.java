/*
 * $Id: FTPUrl.java,v 1.3 2002/04/18 14:06:01 ptalbot Exp $
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

import com.enterprisedt.net.ftp.*;

/**
 * Implementation of ftp:// or fgftp:// url
 *
 * @author Patrick Talbot
 */
public class FTPUrl extends IVUrl {

    private FTPClient client;
    private String username;
    private String pwd;
    private int port;

    private URL url;
    private long lastConnect = 0;

    /**
     * Creates FTPUrl from URLString
     *
     * @param urlString String specified URL String
     * @param fgftp Boolean flag set if the urlString must be processed before creating the URL
     * @exception IVException
     */
    public FTPUrl( String urlString, Boolean fgftp ) throws IVException {
        if ( fgftp.booleanValue() )
        {
            // because fgftp use | as a separator between username and password :
            urlString = urlString.replace( '|', ':' );
            // get rid of the fg before the ftp protocol name :
            urlString = urlString.substring(2, urlString.length());
        }
        try {
            url = new URL( urlString );
        } catch (MalformedURLException e ) {
            throw new IVException(e.getClass().getName() + "\r\n" + e.getMessage());
        }

        this.port = url.getPort();
        if (port < 0 )
            port = 21;
        username = "anonymous";
        pwd = "";
        String infos = url.getUserInfo();
        if ( infos!=null && !infos.trim().equals("") )
        {
            int index = infos.indexOf(":");
            if (index < 0 )
                username = infos;
            else
            {
                username = infos.substring( 0, index );
                pwd = infos.substring( index+1, infos.length());
            }
        }
    }

    public String getName() {
        return url.toExternalForm();
    }

    public InputStream getInputStream() throws IOException {
        connect();
        Log.logRB( Resource.RETRIEVINGCONTENT, new Object[] { getName() } );
        byte[] readBytes;
        try {
            readBytes = client.get( url.getPath() );
            client.quit();
        } catch (FTPException ex) {
            throw new IOException( ex.getClass().getName() + "\n" + ex.getMessage() );
        }
        return (InputStream) new ByteArrayInputStream( readBytes );
    }

    private synchronized void connect() throws IOException {
        long now = System.currentTimeMillis();
        if( lastConnect == 0 || lastConnect+500 < now ) {

            Log.logRB( Resource.CONNECTINGTO, new Object[] { getName() } );
            try {

                client = new FTPClient( url.getHost(), port );
                client.login( username, pwd );
                client.setType( FTPTransferType.BINARY );

            } catch (FTPException ex) {
                throw new IOException( ex.getClass().getName() + "\n" + ex.getMessage() );
            }
            lastConnect = System.currentTimeMillis();
        }
    }

}
