/*
 * $Id: DataSourceHelper.java,v 1.5 2002/04/26 05:23:11 skavish Exp $
 *
 * ===========================================================================
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

package org.openlaszlo.iv.flash.util;

import java.io.*;
import java.util.*;

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.url.*;
import org.openlaszlo.iv.flash.xml.*;

import org.w3c.dom.*;

/**
 * Helper class for reading any kind of datasources.
 *
 * @author Dmitry Skavish
 * @see XMLDataSource
 * @see DataSource
 */
public class DataSourceHelper {

    /**
     * Reads datasource from given url, detects it's type: xml or text
     * and creates either LineReader (text) or else (xml)<P>
     * Datasources can be specified by url or inline. If url starts
     * with '#' then this is inline datasource which is completely given
     * in the url string.
     *
     * @param surl      url or inline datasource
     * @param flashFile current flash file from which this datasource is requested
     * @return either LineReader (plain datasource) or IVUrl
     * @exception IOException
     * @exception IVException
     */
    public static Object readContextData( String surl, FlashFile flashFile )
        throws IVException, IOException
    {
        if( surl == null || surl.length() == 0 ) {
            throw new IOException( "null datasource" );
        }

        Object dsrc = null;

        if( surl.charAt(0) == '#' ) {
            if( surl.charAt(1) == '<' ) {
                // xml datasource
                byte[] bytes = flashFile.getEncoding() != null?
                                    surl.substring(1).getBytes(flashFile.getEncoding()):
                                    PropertyManager.defaultEncoding != null?
                                        surl.substring(1).getBytes(PropertyManager.defaultEncoding):
                                        surl.substring(1).getBytes();
                dsrc = new BufferedUrl( new FlashBuffer(bytes) );
            } else {
                // text datasource
                dsrc = new NativeLineReader(surl);
            }
        } else {
            int idx = surl.indexOf(';');
            if( idx == -1 ) {
                dsrc = new BufferedUrl( surl, flashFile );
            } else {
                StringTokenizer st = new StringTokenizer(surl, ";");
                IVVector urls = new IVVector();
                while( st.hasMoreTokens() ) {
                    IVUrl url = IVUrl.newUrl( st.nextToken(), flashFile );
                    urls.addElement(url);
                }
                dsrc = new MultipleUrlsReader(urls, flashFile);
            }
        }

        if( dsrc instanceof BufferedUrl ) {
            BufferedUrl burl = (BufferedUrl) dsrc;
            byte[] buf = burl.getFlashBuffer().getBuf();
            if( !(buf.length > 3 && (buf[0] == '<' || (buf[0] == -1 && buf[1] == -2 && buf[2] == '<'))) ) {
                dsrc = Util.getUrlReader(flashFile, burl);
            }
        }

        return dsrc;
    }

    /**
     * Return context data.<p>
     * Reads datasource from given url, detects it's type: xml or text
     * and creates either xml Node or array of strings.<P>
     * Datasources can be specified by url or inline. If url starts
     * with '#' then this is inline datasource which is completely given
     * in the utl string.
     *
     * @param surl      url or inline datasource
     * @param flashFile current flash file from which this datasource is requested
     * @return either Node or String[][]
     * @exception IVException
     * @exception IOException
     */
    public static Object getContextData( String surl, FlashFile flashFile )
        throws IVException, IOException
    {
        Object dsrc = readContextData(surl, flashFile);

        if( dsrc instanceof LineReader ) {
            DataSource ds = new DataSource( (LineReader) dsrc );
            String[][] data = ds.getData();
            return data;
        } else {
            try {
                return XMLHelper.getNode( (IVUrl) dsrc );
            } catch( Exception e ) {    // otherwise it requires xml libs to be in classpath
                if( e instanceof IOException ) throw (IOException) e;
                throw new IVException(e);
            }
        }

    }

}
