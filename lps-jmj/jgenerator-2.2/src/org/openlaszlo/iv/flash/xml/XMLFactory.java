/*
 * $Id: XMLFactory.java,v 1.3 2002/03/22 19:08:58 awason Exp $
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

import org.openlaszlo.iv.flash.context.*;

import org.openlaszlo.iv.flash.util.*;

import javax.xml.parsers.*;
import java.lang.reflect.*;
import org.w3c.dom.Node;

/**
 * XML Factory
 *
 * @author Dmitry Skavish
 */
public abstract class XMLFactory {

    private static XMLFactory instance;

    /**
     * Returns implementation specific DocumentBuilder
     *
     * @return implementation specific DocumentBuilder
     * @exception ParserConfigurationException
     */
    public abstract DocumentBuilder getDocumentBuilder() throws ParserConfigurationException;

    /**
     * Creates new implementation specific XMLContext
     *
     * @param parent parent context
     * @param node   node representing the created context
     * @return implementation specific XMLContext
     */
    public abstract XMLContext newXMLContext( Context parent, Node node );

    /**
     * Returns implementation specific XPathProcessor
     *
     * @return implementation specific XPathProcessor
     */
    public abstract XPathProcessor getXPathProcessor();

    /**
     * Returns implementation specific instance of XMLFactory
     *
     * @return implementation specific instance of XMLFactory
     */
    public static XMLFactory getFactory() {
        return instance;
    }

    static {
        DocumentBuilderFactory docFactory = null;

        String factoryName = PropertyManager.getProperty("javax.xml.parsers.DocumentBuilderFactory", null);
        if( factoryName != null ) {
            try {
                Class clazz = Class.forName(factoryName);
                docFactory = (DocumentBuilderFactory) clazz.newInstance();
            } catch( Exception e ) {
            }
        }

        if( docFactory == null ) {
            try {
                docFactory = DocumentBuilderFactory.newInstance();
            } catch( RuntimeException e ) {
            }
            if (docFactory == null) {
                try {
                    Class clazz = Class.forName("org.openlaszlo.iv.flash.xml.apache.DocumentBuilderFactoryImpl");
                    docFactory = (DocumentBuilderFactory) clazz.newInstance();
                } catch( Exception e ) {
                    Log.logRB(e);
                }
            }
        }

        if (docFactory != null) {

            String name = docFactory.getClass().getName();
            if( name.startsWith("org.apache.xerces") ) {
                // change default apache factory with ours, because default one very strict to errors
                try {
                    Class clazz = Class.forName("org.openlaszlo.iv.flash.xml.apache.DocumentBuilderFactoryImpl");
                    DocumentBuilderFactory docFactory2 = (DocumentBuilderFactory) clazz.newInstance();
                    docFactory = docFactory2;
                    name = docFactory.getClass().getName();
                } catch( Exception e ) {
                }
            }

            docFactory.setExpandEntityReferences(false);
            docFactory.setIgnoringComments(true);
            docFactory.setNamespaceAware(true);
            docFactory.setValidating(false);

            // Use XML factory if specified
            String xmlFactoryName = PropertyManager.getProperty("org.openlaszlo.iv.flash.XMLFactory", null);
            if (xmlFactoryName != null) {
                try {
                    instance = constructFactory(xmlFactoryName, docFactory);
                } catch( Exception e ) {
                    Log.logRB(e);
                }
            }
            // Otherwise find a factory to match the DocumentBuilderFactory
            else if( name.startsWith("org.openlaszlo.iv.flash.xml.apache.") ) {
                try {
                    instance = constructFactory("org.openlaszlo.iv.flash.xml.apache.XMLFactoryImpl", docFactory);
                } catch( Exception e ) {
                    Log.logRB(e);
                }
            } else if( name.startsWith("com.caucho.") ) {
                // instance = new org.openlaszlo.iv.flash.xml.caucho.XMLFactoryImpl(docFactory);
                try {
                    instance = constructFactory("org.openlaszlo.iv.flash.xml.caucho.XMLFactoryImpl", docFactory);
                } catch( Exception e ) {
                    Log.logRB(e);
                }
            } else {
                instance = new org.openlaszlo.iv.flash.xml.generic.XMLFactoryImpl(docFactory);
            }
        }
    }

    private static XMLFactory constructFactory(String factoryName, DocumentBuilderFactory docFactory) throws Exception {
        Class factory = Class.forName(factoryName);
        Constructor constr = factory.getConstructor(new Class[] {DocumentBuilderFactory.class});
        return (XMLFactory) constr.newInstance(new Object[] {docFactory});
    }
}


