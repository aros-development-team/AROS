/*
 * $Id: IVUrl.java,v 1.9 2002/07/15 02:15:03 skavish Exp $
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
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.util.*;

/**
 * Abstract generator url
 * <p>
 * Usage example:<br>
 * <pre><code>
 * // creates url to file 'data.txt' in the same directory where 'file' is
 * IVUrl url = IVUrl.newUrl( "data.txt", file );
 *
 * // creates url to http://server/getdata.cgi
 * IVUrl url = IVUrl.newUrl( "http://server/getdata.cgi?parm=text" );
 *
 * </code></pre>
 */
public abstract class IVUrl {

    public static final String ENCODING = "genc";
    //private static IVVector handlers = new IVVector();

    protected Hashtable parms;

    /**
     * Creates new url from specified string
     *
     * @param surl   absolute url
     * @return generator url
     * @exception IVException
     */
    public static IVUrl newUrl( String surl ) throws IVException {
        return newUrl( surl, null );
    }

    /**
     * Creates new url relative from specified file
     *
     * @param surl   relative or absolute url
     * @return generator url
     * @exception IVException
     */
    public static IVUrl newUrl( String surl, FlashFile flashFile ) throws IVException {
        if( surl.startsWith("fgjdbc:///") ) {
            return new JDBCUrl( surl );
        } else if( surl.startsWith("fgjava:///") ) {
            return new JavaUrl( surl );
        } else if( surl.startsWith("fgjs:///") ) {
            // return new JSUrl( surl, flashFile );
            try {
                return (IVUrl) Util.newInstance(
                                "org.openlaszlo.iv.flash.url.JSUrl",
                                new Class[] {String.class, FlashFile.class},
                                new Object[] {surl, flashFile});
            } catch( Exception e ) {
                throw new IVException(e);
            }
        } else if( surl.startsWith("fgfilter:///") ) {
            return new FilterUrl( surl, flashFile );
        } else if ( surl.startsWith("ftp://") || surl.startsWith("fgftp://") ) {
            try {
                return (IVUrl) Util.newInstance(
                               "org.openlaszlo.iv.flash.url.FTPUrl",
                                new Class[] { String.class, Boolean.class },
                                new Object[] { surl, new Boolean(surl.startsWith("fgftp://")) });
            } catch( Exception e ) {
                throw new IVException(e);
            }
        } else {
            try {
                URL url = new URL(surl);
                return new URLUrl( url );
            } catch( MalformedURLException e ) {
                return new FileUrl( surl, flashFile );
            }
        }
    }

    /**
     * Returns name of the url.
     *
     * @return name of this url
     */
    public abstract String getName();

    /**
     * Returns input stream for this url
     *
     * @return url input stream
     * @exception Exception
     */
    public abstract InputStream getInputStream() throws IOException;

    /**
     * Returns parameter of the url by name
     * <p>
     * Parameters are standard url parameters like 'parm' in: http://server/getdata.cgi?parm=text
     *
     * @param name   parameter name
     * @return parameter value or null
     */
    public String getParameter( String name ) {
        if( parms == null ) return null;
        return (String) parms.get(name.toLowerCase());
    }

    /**
     * Returns timestamp of last time this url was modified
     *
     * @return last modified time
     */
    public long lastModified() {
        return 0;
    }

    /**
     * Refreshes this url. Updates last modified.
     */
    public void refresh() {
    }

    /**
     * Returns encoding taken from parameters of this url if it
     * was specified.
     *
     * @return specified in this url encoding or null
     */
    public String getEncoding() {
        return getParameter(ENCODING);
    }

    /**
     * Returns true if this url can provide data in name,value format right away
     * without parsing input stream. If returns true must implement {@link #getData} too.
     * @see #getData
     */
    public boolean hasDataReady() {
        return false;
    }

    /**
     * Returns data in name,value format ready to be used.
     * @see #hasDataReady
     */
    public String[][] getData() throws IOException {
        return null;
    }

    public String toString() {
        return getName();
    }

    /**
     * Returns url reference (the part of url after #)
     *
     * @return reference
     */
    public String getRef() {
        return null;
    }

    /**
     * Parses parameters of url-string
     * <p>
     * Parameters are pairs: name=value, separated by ampersand
     *
     * @param s      url
     */
    protected void parse( String s ) throws IVException {
        int idx = s.indexOf('?');
        if( idx >= 0 ) parse(s, idx);
    }

    /**
     * Parses parameters of url-string begining after the specified index (usually index of '?')
     * <p>
     * Parameters are pairs: name=value, separated by ampersand
     *
     * @param s      url
     */
    protected void parse( String s, int idx ) {
        parms = Util.parseUrlParms(s, idx);
    }

    /**
     * Converts specified two-dimensional array of strings into InputStream
     *
     * @param data   specified array
     * @return input stream
     */
    protected InputStream arrayToStream( String[][] data ) {
        StringBuffer sb = new StringBuffer( data.length*data[0].length*10 );
        for( int i=0; i<data.length; i++ ) {
            int ilen = data[i].length;
            for( int j=0; j<ilen; j++ ) {
                String s = data[i][j];
                if( s == null ) sb.append("\"\"");
                else {
                    sb.append("\"");
                    sb.append(s);
                    sb.append("\"");
                }
                if( j+1 < ilen ) sb.append(',');
            }
            sb.append('\n');
        }
        String str = new String( sb );
        return new ByteArrayInputStream( str.getBytes() );
    }

}
