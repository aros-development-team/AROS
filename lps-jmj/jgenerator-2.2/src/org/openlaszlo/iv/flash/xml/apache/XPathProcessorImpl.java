/*
 * $Id: XPathProcessorImpl.java,v 1.3 2002/02/24 02:10:19 skavish Exp $
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

import org.openlaszlo.iv.flash.xml.*;

import java.util.*;
import javax.xml.transform.TransformerException;

import org.w3c.dom.*;
import org.apache.xpath.*;

/**
 * XPath processor
 * <P>
 * Provides various methods for XPath processing
 *
 * @author Dmitry Skavish
 */
public class XPathProcessorImpl implements XPathProcessor {

    /**
     * Evaluates XPath to list of nodes.
     *
     * @param expr   XPath expression
     * @param node   xml node to evaluated on
     * @return result of xpath execution - iterator on nodes
     * @exception Exception
     */
    public Iterator selectNodeList(String expr, Node node) throws TransformerException {
        //final NodeList list = XPathAPI.selectNodeList(node, expr);
        final NodeList list = XPathHelper.selectNodeList(new XPathContext(), node, expr);
        final int n = list.getLength();
        return new Iterator() {
                int cur = 0;
                public boolean hasNext() {
                    return cur < n;
                }
                public Object next() {
                    if( !hasNext() ) throw new NoSuchElementException();
                    return list.item(cur++);
                }
                public void remove() {
                }
            };
    }

    /**
     * Evaluates XPath to one node.
     *
     * @param expr   XPath expression
     * @param node   xml node to be evaluated on
     * @return xml node
     * @exception Exception
     */
    public Node selectSingleNode(String expr, Node node) throws TransformerException {
        //return XPathAPI.selectSingleNode(node, expr);
        return XPathHelper.selectSingleNode(new XPathContext(), node, expr);
    }
}



