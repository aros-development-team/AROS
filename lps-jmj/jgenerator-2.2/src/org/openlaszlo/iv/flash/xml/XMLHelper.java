/*
 * $Id: XMLHelper.java,v 1.4 2002/04/02 21:50:35 awason Exp $
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
import java.net.*;
import java.util.*;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.Properties;

import org.w3c.dom.*;
import org.w3c.dom.traversal.*;
import org.xml.sax.SAXException;
import org.xml.sax.InputSource;

import javax.xml.parsers.*;
import javax.xml.transform.*;

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.url.*;
import org.openlaszlo.iv.flash.util.*;

import org.openlaszlo.iv.flash.cache.*;

import org.xml.sax.EntityResolver;
import org.xml.sax.InputSource;

/**
 * XMl Helper class.
 * <P>
 * Provides various methods for XML stuff
 *
 * @author Dmitry Skavish
 */
public class XMLHelper {

    private static XMLFactory factory = XMLFactory.getFactory();

    /**
     * Returns XMLFactory
     *
     * @return xml factory
     */
    public static XMLFactory getXMLFactory() {
        return factory;
    }

    /**
     * Returns DocumentBuilder or null
     *
     * @return DocumentBuilder or null
     */
    public static DocumentBuilder getDocumentBuilder() {
        try {
            return factory.getDocumentBuilder();
        } catch( ParserConfigurationException e ) {
            Log.logRB(e);
            return null;
        }
    }

    /**
     * Creates new empty xml document
     *
     * @return new empty xml document
     */
    public static Document newDocument() {
        return getDocumentBuilder().newDocument();
    }

    /**
     * Parses input stream of some xml data into xml document.
     *
     * @param is     xml input stream
     * @return xml document
     * @exception IOException
     * @exception SAXException
     */
    public static Document parse( InputStream is ) throws IOException, SAXException {
        return parse( new InputSource(is) );
    }

    /**
     * Parses input source of some xml data into xml document.
     *
     * @param isrc   xml input source
     * @return xml document
     * @exception IOException
     * @exception SAXException
     */
    public static Document parse( InputSource isrc ) throws IOException, SAXException {
        return getDocumentBuilder().parse( isrc );
    }

    /**
     * Parses xml document specified by url.
     * <P>
     * First tries to retrieve xml document from cache.
     * If it is not cached then reads it, parses it and caches (if needed).
     *
     * @param url    url to xml document
     * @return xml document
     * @exception IOException
     * @exception SAXException
     */
    public static Document getDocument( IVUrl url ) throws IOException, SAXException {
        Document doc = XMLCache.getXMLDocument( url );
        if( doc != null ) return doc;
        doc = parse( url.getInputStream() );
        XMLCache.addXMLDocument( url, doc );
        return doc;
    }

    /**
     * Parses xml document specified by url and resolves it to one node.
     * <P>
     * First tries to retrieve xml document from cache.
     * If it is not cached then reads it, parses it and caches (if needed).
     * Then if there is no any xpath expression in url returns Document itself as a xml node.
     * If there is some Xpath then evaluates this XPath to one xml node and returns it.
     * <P>
     * The url's format: <url itself>#<xpath expression which returns single node><BR>
     * Example: <CODE>http://myserver.com/mydata.xml#/doc/item/firstNode</CODE>
     *
     * @param url    url to xml document
     * @return xml node
     * @exception IOException
     * @exception SAXException
     */
    public static Node getNode( IVUrl url ) throws IOException, SAXException {
        Document doc = getDocument( url );
        String ref = url.getRef();
        if( ref == null ) return doc;
        Node node;
        try {
            node = getXMLFactory().getXPathProcessor().selectSingleNode(ref, doc);
        } catch( TransformerException e ) {
            throw new SAXException(e);
        }
        return node;
    }

}
