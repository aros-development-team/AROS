/*
 * $Id: XMLDataSource.java,v 1.1 2002/02/15 23:44:28 skavish Exp $
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

package org.openlaszlo.iv.flash.xml;

import java.io.*;
import java.util.*;

import org.w3c.dom.Node;
import org.xml.sax.SAXException;

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.url.*;
import org.openlaszlo.iv.flash.util.*;

/**
 * XML datasource.
 * <P>
 * Helps in reading and parsing xml data
 *
 * @author Dmitry Skavish
 */
public class XMLDataSource {

    private Node node;

    public XMLDataSource() {}

    public XMLDataSource( String surl, FlashFile flashFile ) throws IVException, IOException, SAXException {
        if( surl == null || surl.length() == 0 ) {
            throw new IOException( "null datasource" );
        }

        if( surl.charAt(0) == '#' || surl.charAt(0) == '=' ) {
            byte[] bytes = flashFile.getEncoding() != null? surl.substring(1).getBytes(flashFile.getEncoding()):
                                                            surl.substring(1).getBytes();
            node = XMLHelper.parse( new ByteArrayInputStream( bytes ) );
        } else {
            IVUrl url = IVUrl.newUrl( surl, flashFile );
            node = XMLHelper.getNode( url );
        }
    }

    public Node getNode() {
        return node;
    }
}
