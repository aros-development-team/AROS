/*
 * $Id: UrlDataSource.java,v 1.3 2002/04/26 05:23:11 skavish Exp $
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
import java.net.*;
import java.util.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.url.*;

/**
 * Text datasource.
 * <P>
 * This class reads and creates text datasource from:
 * <UL>
 * <LI>a string
 * <LI>an url
 * <LI>list of url, seperated by semicolon
 * </UL>
 *
 * @author Dmitry Skavish
 * @see XMLDataSource
 */
public class UrlDataSource extends DataSource {

    private IVUrl url;

    public UrlDataSource() {}

    /**
     * Creates datasource from specified string in the context of FlashFile.
     *
     * @param surl      string representing the datasource<BR>
     *                  can be one of the following:<BR>
     *                  <UL>
     *                  <LI>data itself if prefixed with '#'
     *                  <LI>url pointing to text datasource
     *                  <LI>several urls to text datasources, separated by semicolon
     *                  </UL>
     * @param flashFile flash file in the context of which this datasource is created<BR>
     *                  if datasource is given by relative url, then this flash file can be used
     *                  to resolve this relative url to an absolute one
     * @exception IVException thrown if there are some problems with parsing the datasource
     * @exception IOException thrown if there are some problems with reading the datasource
     */
    public UrlDataSource( String surl, FlashFile flashFile ) throws IVException, IOException {
        url = null;

        if( surl == null || surl.length() == 0 ) {
            throw new IOException( "null datasource" );
        }

        if( surl.charAt(0) == '#' || surl.charAt(0) == '=' ) {
            setReader( new NativeLineReader(surl) );
        } else {
            StringTokenizer st = new StringTokenizer(surl, ";");
            IVVector urls = new IVVector();
            while( st.hasMoreTokens() ) {
                url = IVUrl.newUrl( st.nextToken(), flashFile );
                urls.addElement(url);
            }
            if( urls.size() > 1 ) {
                url = null;
                setReader( new MultipleUrlsReader(urls, flashFile) );
            } else {
                if( !url.hasDataReady() ) {
                    setReader(Util.getUrlReader(flashFile, url));
                }
            }
        }
    }

    /**
     * Returns the datasource as two dimensional array of strings.
     *
     * @return datasource parsed to array of strings
     * @exception IOException throws if there are some problems with reading the datasource
     */
    public String[][] getData() throws IOException {
        if( url != null && url.hasDataReady() ) return url.getData();
        return super.getData();
    }
}