/*
 * $Id: BeanContext.java,v 1.5 2002/06/06 15:02:07 valis Exp $
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

package org.openlaszlo.iv.flash.context;

import org.apache.commons.jexl.Expression;
import org.apache.commons.jexl.ExpressionFactory;
import org.apache.commons.jexl.context.HashMapContext;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.Set;

/**
 * A Context that provides access to a graph of objects (beans and collections)
 * using Jexl (JSTL expression language + extenstions) syntax.
 *
 * Unlike the other contexts in JGen, this is not easily creatable from a
 * datasource, and is intended to be used where JGen is embedded in another
 * framework.
 *
 * @author <a href="james@jamestaylor.org">James Taylor</a>
 */
public class BeanContext extends GraphContext implements Map
{
    /** Key used to store root object when building a context for a list item */
    public static final String ROOT_ITEM_KEY = "root";

    /** The JexlContext */
    private HashMapContext jexlContext = new HashMapContext();

    /**
     * Creates empty context.
     */
    public BeanContext()
    {
    }

    /**
     * Creates context from specified hashtable.
     *
     * @param context parent context
     * @param values Map of initial values to populate context with
     */
    public BeanContext( Context context, Map values )
    {
        setParent( context );

        if ( values != null )
        {
            this.jexlContext.setVars( values );
        }
    }

    /**
     * Tries to resolve given string in this context as XPath expression,
     * using JPath. If the XPath expression selects an object, the result
     * of evaluating its 'toString' method is returned, otherwise the path
     * is passed along to the parent context (if defined).
     *
     * @param path XPath/JPath expression
     *
     * @return String representation of result of xpath execution or result
     *         from the parent or null
     */

    public String getValue( String path )
    {
        // Evaluate path against our context

        Object o = evaluatePath( path );

        // If o is null, that indicates that the path expression did not
        // select an object in the context. This is perfectly acceptable,
        // and we pass the path to the parent context.

        if ( o == null )
        {
            return getValueFromParent( path );
        }

        // If o was not null, we return it's string representation, which is
        // hopefully something meaningfull. But even if it is not the default
        // implementation of Object.toString() should give a value which will
        // allow the user to see that something is wrong with the path or
        // context.

        else
        {
            return o.toString();
        }
    }

    /**
     * Tries to resolve given string in this context as Jexl expression.
     * If the path expression evaluates to a non-empty list, a list of contexts
     * corresponding to each value in that list will be returned. Otherwise
     * the path is passed to the parent context. If there is no parent
     * context, null is returned.
     *
     * The wrapping contexts will have the single element 'root' which is the
     * object selected as a list item.
     *
     * @param path Jexl expression
     *
     * @return List of objects (wrapped in BeanContext) selected by path or
     *         result from the parent or null
     */
    public List getValueList( String path )
    {
        // Evaluate path against our context

        Object o = evaluatePath( path );

        // If o is null, that indicates that the path expression did not
        // select an object in the context. This is perfectly acceptable,
        // and we pass the path to the parent context.

        if ( o == null )
        {
            return getValueListFromParent( path );
        }

        // If o is a list, we build a list of contexts for each object it
        // contains. If o is a single object we'll just build a context list
        // with one item.

        ArrayList contextList = new ArrayList();

        if ( o instanceof List )
        {
            ListIterator iter = ( ( List ) o ).listIterator();

            while ( iter.hasNext() )
            {
                addToContextList( iter.next(), contextList );
            }
        }
        else
        {
            addToContextList( o, contextList );
        }

        return contextList;
    }

    /**
     * Build a BeanContext around and object and add it to the provided list.
     *
     * @param o Object to add
     * @param contextList List to add context to
     */
    private void addToContextList( Object o, List contextList )
    {
        BeanContext newContext = new BeanContext( this, null );

        newContext.put( ROOT_ITEM_KEY, o );

        contextList.add( newContext );
    }

    /**
     * Evaluate a path as a Jexl expression on the contained context.
     *
     * @param path Expression to evaluate
     * @return Object if found or null
     */
    private Object evaluatePath( String path )
    {
        try
        {
            // Parse the path into a Jexl Expression

            Expression expr = ExpressionFactory.createExpression( path );

            // Execute the expression against our context

            Object o = expr.evaluate( jexlContext );

            return o;
        }
        catch ( Exception e )
        {
            // Expression could not be parsed or evaluated as Jexl, so return
            // null which will cause the path to be passed up to that parent

            return null;
        }
    }

    // ------------------------------ interface Map, delegate to HashMapContext

    public int size()
    {
        return jexlContext.size();
    }

    public boolean isEmpty()
    {
        return jexlContext.isEmpty();
    }

    public boolean containsKey( Object key )
    {
        return jexlContext.containsKey( key );
    }

    public boolean containsValue( Object value )
    {
        return jexlContext.containsValue( value );
    }

    public Object get( Object key )
    {
        return jexlContext.get( key );
    }

    public Object put( Object key, Object value )
    {
        return jexlContext.put( key, value );
    }

    public Object remove( Object key )
    {
        return jexlContext.remove( key );
    }

    public void putAll( Map t )
    {
        jexlContext.putAll( t );
    }

    public void clear()
    {
        jexlContext.clear();
    }

    public Set keySet()
    {
        return jexlContext.keySet();
    }

    public Collection values()
    {
        return jexlContext.values();
    }

    public Set entrySet()
    {
        return jexlContext.entrySet();
    }

    public boolean equals( Object o )
    {
        return jexlContext.equals( o );
    }

    public int hashCode()
    {
        return jexlContext.hashCode();
    }

}
