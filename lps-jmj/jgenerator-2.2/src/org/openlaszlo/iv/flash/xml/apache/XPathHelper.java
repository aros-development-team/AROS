/*
 * $Id: XPathHelper.java,v 1.5 2002/07/18 06:02:22 skavish Exp $
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

import java.io.*;
import java.net.*;
import java.util.*;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.Properties;

import org.apache.xerces.parsers.DOMParser;
import org.apache.xpath.*;
import org.apache.xml.utils.*;
import org.apache.xpath.objects.*;
import org.w3c.dom.*;
import org.w3c.dom.traversal.*;
import org.xml.sax.SAXException;
import org.xml.sax.InputSource;

import javax.xml.parsers.*;
import javax.xml.transform.*;

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.url.*;

import org.openlaszlo.iv.flash.cache.*;

import org.xml.sax.EntityResolver;
import org.xml.sax.InputSource;

/**
 * XML Helper class for apache implementation
 * <P>
 * Provides various methods for XML stuff
 *
 * @author Dmitry Skavish
 */
public class XPathHelper {

    private static Hashtable xpaths = new Hashtable();
    private static Hashtable prefix_resolvers = new Hashtable();

    public synchronized static PrefixResolver getPrefixResolver( Node node ) {
        if( prefix_resolvers.size() > 300 ) prefix_resolvers.clear();

        PrefixResolver pr = (PrefixResolver) prefix_resolvers.get(node);
        if( pr == null ) {
            // Create an object to resolve namespace prefixes.
            // XPath namespaces are resolved from the input context node's document element
            // if it is a root node, or else the current context node (for lack of a better
            // resolution space, given the simplicity of this sample code).
            pr = new PrefixResolverDefault(
                        (node.getNodeType() == Node.DOCUMENT_NODE)?
                            ((Document) node).getDocumentElement() :
                            node
                );
            prefix_resolvers.put(node, pr);
        }

        return pr;
    }

    /**
     * Compiles specified string into XPath.
     * <P>
     * Tries to retrieve compiled XPath expression from cache,
     * if it's not in the cache then compiles give string into XPath
     * and caches in the cache.
     *
     * @param expr   string representing XPath expression
     * @return compiled XPath expression
     * @exception TransformerException
     */
    public synchronized static XPath getXPath( Node node, String expr ) throws TransformerException {
        if( xpaths.size() > 300 ) xpaths.clear();

        XPath xpath = (XPath) xpaths.get( expr );
        if( xpath == null ) {
            xpath = new XPath(expr, null, getPrefixResolver(node), XPath.SELECT, null);
            xpaths.put( expr, xpath );
        }
        return xpath;
    }

    /**
     * Evaluates specified XPath expression in the specified xml node.
     *
     * @param xpathContext
     * @param node   xml node to evaluated on
     * @param expr   xpath expression
     * @return result of xpath execution
     * @exception TransformerException
     */
    public static XObject evalXPath( XPathContext xpathContext, Node node, String expr )
        throws TransformerException
    {
        return evalXPath( xpathContext, node, getXPath(node, expr) );
    }

    /**
     * Evaluates specified XPath expression in the specified xml node.
     *
     * @param xpathContext
     * @param node   xml node to evaluated on
     * @param xpath  xpath expression
     * @return result of xpath execution
     * @exception TransformerException
     */
    public static XObject evalXPath( XPathContext xpathContext, Node node, XPath xpath )
        throws TransformerException
    {
        // Execute the XPath, and have it return the result
        // return xpath.execute(xpathSupport, contextNode, prefixResolver);
        int ctxtNode = xpathContext.getDTMHandleFromNode(node);

        return xpath.execute(xpathContext, ctxtNode, getPrefixResolver(node));
    }

    /**
     * Evaluates XPath to list of nodes.
     *
     * @param xpathContext
     * @param node   xml node to evaluated on
     * @param expr   XPath expression
     * @return result of xpath execution - list of nodes
     * @exception TransformerException
     */
    public static NodeList selectNodeList( XPathContext xpathContext, Node node, String expr )
        throws TransformerException
    {
        return getNodeList( evalXPath(xpathContext, node, expr) );
    }

    /**
     * Evaluates XPath to list of nodes.
     *
     * @param xpathContext
     * @param node   xml node to evaluated on
     * @param xpath  XPath expression
     * @return result of xpath execution - list of nodes
     * @exception TransformerException
     */
    public static NodeList selectNodeList( XPathContext xpathContext, Node node, XPath xpath )
        throws TransformerException
    {
        return getNodeList( evalXPath(xpathContext, node, xpath) );
    }

    /**
     * Evaluates XPath to one node.
     *
     * @param xpathContext
     * @param node   xml node to be evaluated on
     * @param expr   XPath expression
     * @return xml node
     * @exception TransformerException
     */
    public static Node selectSingleNode( XPathContext xpathContext, Node node, String expr )
        throws TransformerException
    {
        NodeList nl = selectNodeList(xpathContext, node, expr);
        if( nl.getLength() > 0 ) return nl.item(0);
        return null;
    }

    /**
     * Evaluates XPath to one node.
     *
     * @param xpathContext
     * @param node   xml node to be evaluated on
     * @param xpath   XPath expression
     * @return xml node
     * @exception TransformerException
     */
    public static Node selectSingleNode( XPathContext xpathContext, Node node, XPath xpath )
        throws TransformerException
    {
        NodeList nl = selectNodeList(xpathContext, node, xpath);
        if( nl.getLength() > 0 ) return nl.item(0);
        return null;
    }

    /**
     * Converts result of XPath evaluation to nodelist.
     *
     * @param xo     result of xpath evaluation
     * @return nodelist
     * @exception TransformerException
     */
    public static NodeList getNodeList( XObject xo ) throws TransformerException {
        return xo.nodelist();
    }

    /**
     * Returns String representation of specified xml node.
     *
     * @param node   the specified xml node
     * @return string representation of the node
     */
    public static String getNodeData( Node node ) {
        switch( node.getNodeType() ) {
            case Node.DOCUMENT_FRAGMENT_NODE:
            case Node.DOCUMENT_NODE:
            case Node.ELEMENT_NODE: {
              /*for (Node child = node.getFirstChild(); null != child;
                    child = child.getNextSibling())*/
                Node child = node.getFirstChild();
                if( child != null ) return getNodeData( child );
            }
            break;
            case Node.TEXT_NODE:
            case Node.CDATA_SECTION_NODE:
                return node.getNodeValue();
            case Node.ATTRIBUTE_NODE:
                return node.getNodeValue();
            case Node.PROCESSING_INSTRUCTION_NODE :
                break;
            default:
                break;
        }
        return "";
    }

    public static String getXObjectData( XObject xo ) {
        switch( xo.getType() ) {
            case XObject.CLASS_UNKNOWN:
                return null;
            case XObject.CLASS_BOOLEAN:
            case XObject.CLASS_STRING:
            case XObject.CLASS_NULL:
            case XObject.CLASS_NUMBER:
            case XObject.CLASS_UNRESOLVEDVARIABLE:
            case XObject.CLASS_RTREEFRAG:
                return xo.toString();
                //break;
            case XObject.CLASS_NODESET:
                try {
                    NodeList nl = xo.nodelist();
                    StringBuffer sb = new StringBuffer();
                    int l = nl.getLength();
                    if( l == 0 ) return null;
                    for( int i=0; i<l; i++ ) {
                        Node node = nl.item(i);
                        sb.append(getNodeData(node));
                    }
                    return sb.toString();
                } catch( javax.xml.transform.TransformerException e ) {
                }
        }
        return null;
        //return xo.toString();
    }

    public static void getNodeData(Node node, StringBuffer buf) {
        switch( node.getNodeType() ) {
            case Node.DOCUMENT_FRAGMENT_NODE:
            case Node.DOCUMENT_NODE:
            case Node.ELEMENT_NODE:
                {
                    for( Node child = node.getFirstChild(); null != child;
                       child = child.getNextSibling() ) {
                        buf.append('<');
                        buf.append(node.getNodeName());
                        buf.append('>');
                        getNodeData(child, buf);
                        buf.append("</");
                        buf.append(node.getNodeName());
                        buf.append('>');
                    }
                }
                break;
            case Node.TEXT_NODE :
            case Node.CDATA_SECTION_NODE :
                buf.append(node.getNodeValue());
                break;
            case Node.ATTRIBUTE_NODE :
                buf.append(node.getNodeValue());
                break;
            case Node.PROCESSING_INSTRUCTION_NODE :
                // warning(XPATHErrorResources.WG_PARSING_AND_PREPARING);
                break;
            default :
                // ignore
                break;
        }
    }
}

