/*
 * $Id: XMLFactoryImpl.java,v 1.3 2002/02/24 02:31:00 skavish Exp $
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

package org.openlaszlo.iv.flash.xml.apache;

import org.openlaszlo.iv.flash.xml.apache.functions.*;

import org.openlaszlo.iv.flash.xml.*;
import org.openlaszlo.iv.flash.context.*;

import org.w3c.dom.*;
import javax.xml.parsers.*;
import javax.xml.transform.TransformerException;

/**
 * XML Factory
 *
 * @author Dmitry Skavish
 */
public class XMLFactoryImpl extends XMLFactory {

    private DocumentBuilder builder;
    private XPathProcessor xpathProcessor;

    public XMLFactoryImpl( DocumentBuilderFactory factory ) throws ParserConfigurationException {
        builder = factory.newDocumentBuilder();

        // install new builtin functions
        org.apache.xpath.compiler.FunctionTable.installFunction("serialize-node", new FuncSerialize());
        org.apache.xpath.compiler.FunctionTable.installFunction("serialize", new FuncSerialize1());
        //org.apache.xpath.compiler.FunctionTable.installFunction("myposition", new FuncMyPosition());
    }

    public DocumentBuilder getDocumentBuilder() {
        return builder;
    }

    public XMLContext newXMLContext( Context parent, Node node ) {
        return new XMLContextImpl(parent, node);
    }

    public XPathProcessor getXPathProcessor() {
        if( xpathProcessor == null ) {
            xpathProcessor = new XPathProcessorImpl();
        }
        return xpathProcessor;
    }

}



