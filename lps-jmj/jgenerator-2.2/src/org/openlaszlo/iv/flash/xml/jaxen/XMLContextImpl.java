/*
 * $Id: XMLContextImpl.java,v 1.2 2002/06/06 15:02:07 valis Exp $
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

package org.openlaszlo.iv.flash.xml.jaxen;

import org.openlaszlo.iv.flash.context.Context;
import org.openlaszlo.iv.flash.context.XMLContext;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.jaxen.JaxenException;
import org.jaxen.dom.DOMXPath;
import org.jaxen.dom.DOMXPath;
import org.w3c.dom.DOMException;
import org.w3c.dom.Node;


/**
 * Jaxen XML context
 * <P>
 * Represents one xml node
 *
 * @author Andrew Wason
 */
public class XMLContextImpl extends XMLContext {

    /**
     * Creates xml context with specified parent and xml node.
     *
     * @param parent parent context
     * @param node xml node represented by this context
     */
    public XMLContextImpl(Context parent, Node node) {
        super(parent, node);
    }

    /**
     * Evaluates the  specified path (as XPath) in this context or nearest
     * xml parent.
     *
     * @param path containing XPath expression
     * @return String representation of result of xpath execution or empty string
     */
    public String getValue(String path) {
        try {
            DOMXPath xpath = new DOMXPath(path);
            Object value = xpath.evaluate(node);
            if (value != null) {
                if (value instanceof List) {
                    List values = (List)value;
                    if (values.size() == 0) {
                        // Fall through
                    }
                    else if (values.size() == 1) {
                        return getNodeData((Node)values.get(0));
                    }
                    else {
                        Iterator iter = values.iterator();
                        StringBuffer sb = new StringBuffer();
                        while (iter.hasNext())
                            sb.append(getNodeData((Node)iter.next()));
                        return sb.toString();
                    }
                }
                else
                    return value.toString();
            }
        } catch (DOMException e) {
        } catch (JaxenException e) {
        }

        return getValueFromParent(path);
    }

    private String getNodeData(Node node) {
        switch (node.getNodeType()) {
            case Node.DOCUMENT_FRAGMENT_NODE:
            case Node.DOCUMENT_NODE: {
                Node child = node.getFirstChild();
                if (child != null)
                    return getNodeData(child);
            }
            break;
            case Node.ELEMENT_NODE: {
                StringBuffer sb = new StringBuffer();
                for (Node child = node.getFirstChild(); child != null; child = child.getNextSibling())
                    sb.append(getNodeData(child));
                return sb.toString();
            }
            case Node.TEXT_NODE:
            case Node.CDATA_SECTION_NODE:
            case Node.ATTRIBUTE_NODE:
            case Node.PROCESSING_INSTRUCTION_NODE :
                return node.getNodeValue();
            default:
                break;
        }
        return "";
    }

    /**
     * Evaluates the  specified path (as XPath) in this context or nearest
     * xml parent. If the path evaluates to a nodeset, a list of contexts for
     * the nodes in that nodeset is returned, otherwise null
     *
     * @param path String containing XPath expression
     * @return List of contexts or null
     */
    public List getValueList(String path) {
        try {
            DOMXPath xpath = new DOMXPath(path);
            List nodes = xpath.selectNodes(node);

            int size = nodes.size();
            if (size > 0) {
                ArrayList contexts = new ArrayList(size);
                for (int i = 0; i < size; i++)
                    contexts.add(new XMLContextImpl(this, (Node)nodes.get(i)));
                return contexts;
            }
        } catch (JaxenException e) {
        }

        return getValueListFromParent(path);
    }
}
