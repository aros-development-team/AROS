/*
 * $Id: ContextFactory.java,v 1.5 2002/06/06 15:02:07 valis Exp $
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

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.url.*;
import org.openlaszlo.iv.flash.xml.*;

import org.w3c.dom.Node;
import java.io.*;
import java.util.*;

/**
 * Factory class for creating various types of context from other objects.
 *
 * @author James Taylor
 */

public abstract class ContextFactory
{
    // Standard Contexts

    /**
     * Creates StandardContext from specified data and specified row.
     *
     * @param data   specified strings array
     * @param row    take values from this specified row
     * @return StandardContext without parent (null parent)
     * @exception IVException
     * @see #createContext(Context,String[][],int)
     */
    public static Context createContext( String[][] data, int row ) throws IVException
    {
        return createContext( null, data, row );
    }

    /**
     * Creates StandardContext from specified data and specified row
     * <P>
     * Format of the data is the following:<BR>
     * <PRE>
     * name0  , name1  , name2  , ...
     * value00, value01, value02, ...
     * value10, value11, value12, ...
     * ....
     * </PRE>
     * <BR>
     * Created context will contain values from specified row
     *
     * @param parent parent of return context
     * @param data   specified strings array
     * @param row    take values from this specified row
     * @return StandardContext with parent
     * @exception IVException
     */
    public static Context createContext( Context parent, String[][] data, int row ) throws IVException
    {
        StandardContext context = new StandardContext( parent );

        setStandardContextData( context, data, row );

        return context;
    }

    /**
     * Creates StandardContext from specified data
     *
     * @param data   specified strings array
     * @return StandardContext without parent
     * @exception IVException
     * @see #createContext(Context,String[][])
     */
    public static Context createContext( String[][] data ) throws IVException
    {
        return createContext( null, data );
    }

    /**
     * Creates StandardContext from specified data
     * <P>
     * Format of the data is the following:<BR>
     * <PRE>
     * foo0 , ... , name  , value  ,  fooN, ...
     * ...  , ... , name0 , value0 ,  ... , ...
     * ...  , ... , name1 , value1 ,  ...
     * .....
     * </PRE>
     * <BR>
     * Created context will contain values with names from corresponding columns
     *
     * @param parent parent of return context
     * @param data   specified strings array
     * @return StandardContext with parent
     * @exception IVException
     */
    public static Context createContext( Context parent, String[][] data ) throws IVException
    {
        StandardContext context = new StandardContext( parent );

        setStandardContextData( context, data );

        return context;
    }

    // XML Contexts

    public static Context createContext( Node n )
    {
        return createContext( null, n );
    }

    public static Context createContext( Context parent, Node n ) {
        return XMLContext.newXMLContext(parent, n);
    }

    // Bean Contexts

    public static Context createContext( Map o )
    {
        return createContext( null, o );
    }

    public static Context createContext( Context parent, Map o )
    {
        return new BeanContext( parent, o );
    }

    /**
     * Reads datasource from specified url, detects the type of this context text or url (so far)
     * and creates LineReader for text or IVUrl for xml.
     * <P>
     * I believe it has to be rewritten in future, it's too ugly!
     *
     * @param surl      either url to datasource or datasource itself (perfixed with #)
     * @param flashFile flash file which location is used to resolve relative urls
     * @return LineReader for text datasources or IVUrl for xml datasources
     * @exception IVException
     * @exception java.io.IOException
     */
    public static Object readContext( String surl,
                                      FlashFile flashFile ) throws IVException, java.io.IOException
    {
        return DataSourceHelper.readContextData(surl, flashFile);
    }

    /**
     * Reads datasource from specified url, assuming that it's in text (tabular) format and
     * returns two-dimensional array of Strings.
     * <p>
     * If the datasource is not in tabular format, then throw an exception
     *
     * @param surl      either url to datasource or datasource itself (perfixed with #)
     * @param flashFile flash file which location is used to resolve relative urls
     * @return two-dimensional array of Strings
     * @exception IVException
     * @exception java.io.IOException
     */
    public static String[][] readStandardContext( String surl,
                                                  FlashFile flashFile ) throws IVException, java.io.IOException
    {
        Object dsrc = readContext( surl, flashFile );
        if( !(dsrc instanceof LineReader)  ) {
            throw new IVException( Resource.EXPECTSTDCONTEXT );
        }

        DataSource ds = new DataSource( (LineReader) dsrc );
        return ds.getData();
    }

    /**
     * Reads datasource from given url, detects it's type: xml or text (so far)
     * and creates a Context of the corresponding type.<P>
     * Datasources can be specified by url or inline. If url starts
     * with '#' then this is inline datasource which is completely given
     * in the url string.
     *
     * @param parent will be used as parent for the created context ( null ok )
     * @param surl url or inline datasource
     * @param flashFile current flash file from which this datasource is
     *                  requested
     * @param useRowStyle if the datasource is tabular, determines whether
     *                    the row or column datasource style is used.
     * @return a context
     * @exception IVException
     * @exception IOException
     */

    public static Context createContext( Context parent,
                                         String surl,
                                         FlashFile flashFile,
                                         boolean useRowStyle ) throws IVException, java.io.IOException
    {
        // read context
        Object dsrc = readContext( surl, flashFile );

        if( dsrc instanceof LineReader )
        {
            // Standard ( tabular text ) datasource
            DataSource ds = new DataSource( (LineReader) dsrc );

            StandardContext context = new StandardContext( parent );

            if ( useRowStyle )
            {
                setStandardContextData( context, ds.getData(), 1 );
            }
            else
            {
                setStandardContextData( context, ds.getData() );
            }

            return context;
        }
        else
        {
            try {
                return XMLContext.newXMLContext(parent, XMLHelper.getNode((IVUrl)dsrc));
            } catch ( Exception e ) {  // otherwise it requires xml libraries to be in classpath
                throw new IVException( e );
            }
        }
    }

    /**
     * Sets specified data to the specified context
     * <P>
     * Format of the data is the following:<BR>
     * <PRE>
     * foo0 , ... , name  , value  ,  fooN, ...
     * ...  , ... , name0 , value0 ,  ... , ...
     * ...  , ... , name1 , value1 ,  ...
     * .....
     * </PRE>
     * <BR>
     *
     * @param context specified standard context
     * @param data    specified data
     * @exception IVException
     */

    public static void setStandardContextData( StandardContext context,
                                               String[][] data )
        throws IVException
    {
        int j, k;

        for ( j = 0; j < data[ 0 ].length && ! data[ 0 ][ j ].equalsIgnoreCase( "NAME" ); j++ );

        if( j == data[0].length )
        {
            throw new IVException( Resource.COLNOTFOUNDCMD, new Object[] {"", "NAME", ""} );
        }

        for( k = 0; k < data[ 0 ].length && ! data[ 0 ][ k ].equalsIgnoreCase( "VALUE" ); k++ );

        if( k == data[ 0 ].length )
        {
            throw new IVException( Resource.COLNOTFOUNDCMD, new Object[] {"", "VALUE", ""} );
        }

        for( int i = 1; i < data.length; i++ )
        {
            context.setValue( context.apply( data[i][j] ),
                              context.apply( data[i][k] ) );
        }
    }

    /**
     * Sets specified data at specified row to the specified context
     * <P>
     * Format of the data is the following:<BR>
     * <PRE>
     * name0  , name1  , name2  , ...
     * value00, value01, value02, ...
     * value10, value11, value12, ...
     * ....
     * </PRE>
     * <BR>
     *
     * @param context specified standard context
     * @param data    specified data
     * @param row     specified row
     * @exception IVException
     */

    public static void setStandardContextData( StandardContext context,
                                               String[][] data,
                                               int row )
        throws IVException
    {
        for ( int i = 0; i < data[ row ].length; i++ )
        {
            context.setValue( context.apply( data[ 0 ][ i ] ),
                              context.apply( data[ row ][ i ] ) );
        }
    }
}
