/*
 * $Id: XMLContextImpl.java,v 1.2 2002/02/24 02:10:19 skavish Exp $
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

package org.openlaszlo.iv.flash.xml.caucho;


import org.openlaszlo.iv.flash.xml.*;
import org.openlaszlo.iv.flash.context.*;
import org.openlaszlo.iv.flash.util.*;

import java.util.*;

import javax.xml.transform.TransformerException;

import org.w3c.dom.*;
import org.w3c.dom.traversal.NodeIterator;

import com.caucho.xpath.*;

/**
 * Resin XML context
 * <P>
 * Represents one xml node
 *
 * @author Dmitry Skavish
 */

public class XMLContextImpl extends XMLContext {

    /**
     * Creates xml context with specified parent and xml node.
     *
     * @param parent parent context
     * @param node xml node represented by this context
     */

    public XMLContextImpl( Context parent, Node node ) {
        super(parent, node);
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

        //System.out.println( "XMLContext.getValue("+path+")" );
        try {
            String value = XPath.evalString(path, node);

            if( value != null && value.length() > 0 /*&& !isUndefined( xo )*/ ) {
                return value;
            }
        } catch( com.caucho.xpath.XPathException e ) {
            // ignore this exception, it usually means that it is not an
            // xpath, but just a variable
        } catch( Exception e ) {
            Log.logRB(e);
        }

        return getValueFromParent( path );
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

        try {
            Iterator it = XPath.select(path, node);
            if( it != null ) {
                ArrayList list = new ArrayList();
                while( it.hasNext() ) {
                    Node n = (Node) it.next();
                    list.add(new XMLContextImpl(this, n));
                }

                return list;
            }
        } catch( com.caucho.xpath.XPathException e ) {
        }

        // The path either evaluated to nothing ( an unresolvable path ) or an
        // atomic type. Neither of these suit our purposes ( I think ) so for
        // now defer to the parent.

        return getValueListFromParent( path );
    }

}

