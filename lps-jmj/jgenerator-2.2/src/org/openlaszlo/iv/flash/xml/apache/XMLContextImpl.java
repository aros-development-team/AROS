/*
 * $Id: XMLContextImpl.java,v 1.4 2002/07/18 06:02:22 skavish Exp $
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

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.util.*;

import org.openlaszlo.iv.flash.xml.XMLHelper;
import org.openlaszlo.iv.flash.context.*;

import java.util.*;

import javax.xml.transform.TransformerException;

import org.w3c.dom.*;
import org.w3c.dom.traversal.NodeIterator;

import org.apache.xpath.*;
import org.apache.xpath.objects.XObject;

/**
 * Apache XML context
 * <P>
 * Represents one xml node
 *
 * @author Dmitry Skavish
 */

public class XMLContextImpl extends XMLContext {

    private XPathContext xpathContext;

    /**
     * Creates xml context with specified parent and xml node.
     *
     * @param parent parent context
     * @param node xml node represented by this context
     */

    public XMLContextImpl( Context parent, Node node ) {
        super(parent, node);
        this.xpathContext = new XPathContext();
    }

    /**
     * Evaluates the  specified path ( as XPath ) in this context or nearest
     * xml parent.
     *
     * @param string containing XPath expression
     * @return string representation of result of xpath execution or empty string
     */
    public String getValue( String path ) {
        // Evaluate the XPath in this context

        //System.out.println( "XMLContext(node="+node.getNodeName()+").getValue("+path+")" );
        try {
            XObject xo = XPathHelper.evalXPath(xpathContext, node, path);

            //System.out.println("xo class: "+xo.getClass().getName());
            String res = XPathHelper.getXObjectData(xo); //xo.toString();
            if( res != null ) {
                //System.out.println("   res='"+res+"'" );
                return res;
            }
        } catch( TransformerException e ) {
            // Log.log(e);
            // ignore this exception, it usually means that it is not an
            // xpath, but just a variable
        } catch( Exception e ) {
            Log.logRB(e);
        }

        //System.out.println("   (node="+node.getNodeName()+") go to parent with "+path+"" );
        String res = getValueFromParent( path );
        //System.out.println( "   res from parent='"+res+"'" );
        return res;
    }

    /**
     * Evaluates the  specified path ( as XPath ) in this context or nearest
     * xml parent. If the path evaluates to a nodeset, a list of contexts for
     * the nodes in that nodeset is returned, otherwise null
     *
     * @param string containing XPath expression
     * @returns list of contexts or null
     */
    public List getValueList( String path ) {
        // Evaluate the path into an XObject

        //System.out.println( "XMLContext.getValueList("+path+")" );
        try {
            //NodeList nlist = XPathAPI.selectNodeList(node, path);
            NodeList nlist = XPathHelper.selectNodeList(xpathContext, node, path);

            int l = nlist.getLength();
            if( l > 0 ) {
                ArrayList list = new ArrayList(l);
                //System.out.println("   items ("+l+"):");
                for( int i=0; i<l; i++ ) {
                    Node node = nlist.item(i);
                    //System.out.println("     node: "+node.getNodeName());
                    list.add( new XMLContextImpl( this, node ) );
                }

                return list;
            } else {
                // The path either evaluated to nothing ( an unresolvable path ) or an
                // atomic type. Neither of these suit our purposes ( I think ) so for
                // now defer to the parent.

                return getValueListFromParent( path );
            }
        } catch ( TransformerException e ) {
            // As usual, we don't want to bail just because an exception
            // occurs in a single path evaluation, so log and defer to parent.

            Log.logRB( e );
            return getValueListFromParent( path );
        }
    }

    /**
     * Compiles given string into XPath and execute it in this context or its parent.
     *
     * @param expr   supposedly XPath expression
     * @return result of XPath execution
     * @exception TransformerException
     */
/*    private XObject eval( String expr ) throws TransformerException {
        //XObject xo = XPathAPI.eval(node, expr);
        //System.out.println("XMLContext.eval("+node.getNodeName()+", "+expr+", type="+xo.getTypeString()+")=bebe");
        XPath xpath = XPathHelper.getXPath(node, expr);
        return eval( xpath );
    }
*/
    /**
     * Executes specified XPath in this context or nearest xml parent.
     *
     * @param xpath  compiled XPath expression
     * @return result of xpath execution
     * @exception TransformerException
     */
/*    private XObject eval( XPath xpath ) throws TransformerException {
        XObject xo = XPathHelper.evalXPath( xpathContext, node, xpath );

        return isUndefined( xo ) ? null : xo;
    }
*/
    /**
     * Check whether the specified XObject undefined or not.
     *
     * @param xo     XObject, usually results of XPath execution
     * @return true if specified xobject is undefined
     * @exception TransformerException
     */
/*    private boolean isUndefined( XObject xo ) throws TransformerException {
        if( xo == null ) return true;
        int type = xo.getType();

        System.out.println("isUndefined(type="+xo.getTypeString()+")="+xo);
        if( type == XObject.CLASS_NODESET )
            System.out.println("  length="+XPathHelper.getNodeList(xo).getLength());
        //return type == XObject.CLASS_UNKNOWN;
        return ( type == XObject.CLASS_UNKNOWN ||
                 ( type == XObject.CLASS_NODESET
                   && XPathHelper.getNodeList(xo).getLength() == 0 ) );
        return ( type == XObject.CLASS_UNKNOWN ||
                 ( type == XObject.CLASS_NODESET
                   && xo.nodeset().nextNode() == null ) );
    }*/
}
