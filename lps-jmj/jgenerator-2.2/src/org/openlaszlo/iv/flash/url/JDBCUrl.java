/*
 * $Id: JDBCUrl.java,v 1.3 2002/02/24 02:10:19 skavish Exp $
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

package org.openlaszlo.iv.flash.url;

import java.io.*;
import java.util.*;
import java.net.*;
import java.sql.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;

/**
 * Implementation of fgjdbc:// url
 * <P>
 * Syntax is: fgjdbc:///?driver=org.gjt.mm.mysql.Driver&url=jdbc:mysql://domen:3306/dbName&
 *            userid=username&password=password&query=select columnName from Table
 * <p>
 * The other parameters are (not supported now):
 * <UL>
 * <LI>lock
 * <LI>cache
 * <LI>share
 * <LI>timeout
 * </UL>
 *
 * @author Dmitry Skavish
 */
public class JDBCUrl extends IVUrl {

    private String driver;
    private String url;
    private String userid;
    private String password;
    private String query;
    /*private String lock;
      private String cache;
      private String share;
      private String timeout;
    */

    public JDBCUrl( String surl ) throws IVException {
        parse( surl );
        driver   = getParameter("driver");
        url      = getParameter("url");
        userid   = getParameter("userid");
        password = getParameter("password");
        query    = getParameter("query");
        /*  lock     = getParameter("lock");
            cache    = getParameter("cache");
            share    = getParameter("share");
            timeout  = getParameter("timeout");
        */
        if( driver == null || url == null || query == null ) {
            throw new IVException(Resource.INVALURL, new Object[] {surl});
        }
    }

    public String getName() {
        return url+"@"+userid+"/"+query;
    }

    public InputStream getInputStream() throws IOException {
        return arrayToStream(getData(false));
    }

    public boolean hasDataReady() {
        return true;
    }

    public String[][] getData() throws IOException {
        return getData(true);
    }

    private String[][] getData( boolean processEscapes ) throws IOException {
        try {
            Class.forName(driver);
        } catch( Exception e ) {
            throw new IOException("driver not found");
        }

        String[][] data = null;

        Connection conn = null;
        try {
            Log.logRB(Resource.SQLQUERY, new Object[] {driver, url, userid, password, query});
            if( userid == null ) {
                conn = DriverManager.getConnection(url);
            } else {
                conn = DriverManager.getConnection(url, userid, password);
            }
            Statement stmt = conn.createStatement();

            ResultSet rs = stmt.executeQuery(query);
            ResultSetMetaData meta = rs.getMetaData();

            int numCols = meta.getColumnCount();
            String[] header = new String[numCols];
            for( int i=0; i<numCols; i++ ) {
                header[i] = meta.getColumnLabel(i+1);
            }

            IVVector lines = new IVVector();
            while( rs.next() ) {
                String[] line = new String[numCols];
                for( int i=0; i<numCols; i++ ) {
                    Object o = rs.getObject(i+1);
                    String s = res2string(o);
                    line[i] = processEscapes? Util.processEscapes(s): s;
                }
                lines.addElement(line);
            }

            data = new String[lines.size()+1][];
            data[0] = header;
            for( int i=0; i<lines.size(); i++ ) {
                data[i+1] = (String[]) lines.elementAt(i);
            }

            return data;
        } catch( Exception e ) {
            throw new IOException( e.getMessage() );
        } finally {
            try {
                if( conn != null )
                    conn.close();
            } catch( SQLException ee ) {
                throw new IOException(ee.getMessage());
            }
        }
    }

    private String res2string( Object o ) {
        if( o == null ) return "";
        if( o instanceof byte[] ) return new String((byte[])o);
        return o.toString();
    }
}
