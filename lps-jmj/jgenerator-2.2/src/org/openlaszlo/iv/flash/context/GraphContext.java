/*
 * $Id: GraphContext.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
 *
 * ==========================================================================
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

import java.util.*;

/**
 * Abstract generator context which supports path expressions.
 *
 * PathContexts allow for expressions which can evaluate expressions to lists
 * of contexts, not just Strings.
 *
 * @author James Taylor
 */

public abstract class GraphContext extends Context
{
    public abstract List getValueList( String path );

    public abstract String getValue( String path );

    protected List getValueListFromParent( String path )
    {
        Context parent = getParent();

        // Find closest parent which is a GraphContext and call its
        // getValueList method.

        while ( parent != null )
        {

            if ( parent instanceof GraphContext )
            {
                return ( ( GraphContext ) parent ).getValueList( path );
            }
            else
            {
                parent = parent.getParent();
            }
        }

        // If no parent is a GraphContext, return an empty list.

        return null;
    }

   /**
     * Sort given List of contexts using given path 'sortby' as a key
     *
     * @param list List to sort
     * @param sortby Path expression to sort on
     * @param ascending if true then sort in ascending order, otherwise in descending
     * @return sorted copy of list
     */

    public static List sortValueList( List list,
                                      final String sortby,
                                      final boolean ascending )
    {
        // create list to sort into

        ArrayList newList = new ArrayList( list );

        // sort

        Collections.sort( newList, new java.util.Comparator()
            {
                public int compare( Object o1, Object o2 )
                {
                    String val1 = ( ( GraphContext ) o1 ).getValue( sortby );
                    String val2 = ( ( GraphContext ) o2 ).getValue( sortby );

                    if( ascending )
                    {
                        return val1.compareTo( val2 );
                    }
                    else
                    {
                        return val2.compareTo( val1 );
                    }
                }
            }
        );

        return newList;
    }

}
