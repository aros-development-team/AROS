/*
 * $Id: DocumentBuilderImpl.java,v 1.1 2002/02/15 23:44:28 skavish Exp $
 *
 * The Apache Software License, Version 1.1
 *
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Xerces" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation and was
 * originally based on software copyright (c) 1999, Sun Microsystems, Inc.,
 * http://www.sun.com.  For more information on the Apache Software
 * Foundation, please see <http://www.apache.org/>.
 */


package org.openlaszlo.iv.flash.xml.apache;

import java.io.IOException;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.DOMImplementation;
import org.w3c.dom.DocumentType;

import org.xml.sax.XMLReader;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;
import org.xml.sax.EntityResolver;
import org.xml.sax.ErrorHandler;
import org.xml.sax.helpers.DefaultHandler;

import org.apache.xerces.parsers.DOMParser;
import org.apache.xerces.dom.DOMImplementationImpl;

/**
 * Standard DocumentBuilderImpl from apache.
 * <p>
 * Modifications for JGenerator:<br>
 *   - ignore fatal errors in parser<br>
 *   - pool of DOM parser with locking and releasing<br>
 *
 * @author Dmitry Skavish
 * @author Rajiv Mordani
 * @author Edwin Goei
 */
public class DocumentBuilderImpl extends DocumentBuilder {

    /** Size of parser's pool */
    private static final int MAX_PARSERS = 5;

    /* Xerces features */
    private static final String XERCES_FEATURE_PREFIX = "http://apache.org/xml/features/";
    private static final String CREATE_ENTITY_REF_NODES_FEATURE = "dom/create-entity-ref-nodes";
    private static final String INCLUDE_IGNORABLE_WHITESPACE = "dom/include-ignorable-whitespace";
    private static final String CONTINUE_AFTER_FATAL_ERROR = "continue-after-fatal-error";
    private static final String VALIDATING_PARSER = "http://xml.org/sax/features/validation";
    private static final String NAMESPACE_AWARE = "http://xml.org/sax/features/namespaces";

    private DocumentBuilderFactory dbf;

    private EntityResolver er = null;
    private ErrorHandler eh = null;

    /* Parsers' pool */
    private DOMParser[] parsers = new DOMParser[MAX_PARSERS];
    private boolean[] locks     = new boolean[MAX_PARSERS];

    private boolean namespaceAware = false;
    private boolean validating = false;

    DocumentBuilderImpl(DocumentBuilderFactory dbf)
        throws ParserConfigurationException
    {
        this.dbf = dbf;
        this.validating = dbf.isValidating();
        this.namespaceAware = dbf.isNamespaceAware();
    }

    /**
     * Non-preferred: use the getDOMImplementation() method instead of this
     * one to get a DOM Level 2 DOMImplementation object and then use DOM
     * Level 2 methods to create a DOM Document object.
     */
    public Document newDocument() {
        return new org.apache.xerces.dom.DocumentImpl();
    }

    public DOMImplementation getDOMImplementation() {
        return DOMImplementationImpl.getDOMImplementation();
    }

    public Document parse(InputSource is) throws SAXException, IOException {
        if( is == null ) {
            throw new IllegalArgumentException("InputSource cannot be null");
        }

        DOMParser parser = lockDOMParser();
        try {
            parser.parse(is);
            Document doc = parser.getDocument();
            return doc;
        } finally {
            releaseDOMParser(parser);
        }
    }

    public boolean isNamespaceAware() {
        return namespaceAware;
    }

    public boolean isValidating() {
        return validating;
    }

    public void setEntityResolver( org.xml.sax.EntityResolver er ) {
        this.er = er;
    }

    public void setErrorHandler( org.xml.sax.ErrorHandler eh ) {
        // If app passes in a ErrorHandler of null, then ignore all errors
        // and warnings
        this.eh = (eh == null) ? new DefaultHandler() : eh;
    }

    /**
     * Create DOM parser
     *
     * @return created parser
     * @exception ParserConfigurationException
     */
    private DOMParser createDOMParser() throws SAXException {
        DOMParser domParser = new DOMParser();

        domParser.setFeature(VALIDATING_PARSER, false);

        // "namespaceAware" ==  SAX Namespaces feature
        domParser.setFeature(NAMESPACE_AWARE, namespaceAware);

        // Set various parameters obtained from DocumentBuilderFactory
        domParser.setFeature(XERCES_FEATURE_PREFIX+INCLUDE_IGNORABLE_WHITESPACE,
                             !dbf.isIgnoringElementContentWhitespace());
        domParser.setFeature(XERCES_FEATURE_PREFIX+CREATE_ENTITY_REF_NODES_FEATURE,
                             !dbf.isExpandEntityReferences());
        domParser.setFeature(XERCES_FEATURE_PREFIX+CONTINUE_AFTER_FATAL_ERROR, true );

        // XXX No way to control dbf.isIgnoringComments() or
        // dbf.isCoalescing()

        if( er != null ) domParser.setEntityResolver(er);
        if( eh != null ) domParser.setErrorHandler(eh);

        return domParser;
    }

    private synchronized DOMParser lockDOMParser() throws SAXException {
        for(;;) {
            for( int i=0; i<MAX_PARSERS; i++ ) {
                if( !locks[i] ) {
                    if( parsers[i] == null ) {
                        parsers[i] = createDOMParser();
                    }
                    locks[i] = true;
                    return parsers[i];
                }
            }
            try {
                wait();
            } catch( InterruptedException e ) {
            }
        }
    }

    private synchronized void releaseDOMParser( DOMParser p ) {
        for( int i=0; i<MAX_PARSERS; i++ ) {
            if( parsers[i] == p ) {
                locks[i] = false;
                notify();
                break;
            }
        }
    }

}
