/*
 * $Id: JavaUrl.java,v 1.3 2002/02/24 02:10:19 skavish Exp $
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

import java.lang.reflect.*;
import java.io.*;
import java.util.*;
import java.net.*;
import java.sql.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;

/**
 * Implementation of fgjava:// url
 * <P>
 * Syntax is: fgjava:///classname?parm1=value1&....
 * <P>
 * The class has to implement method:
 * <code><pre>
 *     InputStream getStream( Hashtable parms );
 * </pre></code>
 *
 * @author Dmitry Skavish
 */
public class JavaUrl extends IVUrl {

    private Class clazz;

    public JavaUrl( String surl ) throws IVException {
        parse( surl );
    }

    protected void parse( String s ) throws IVException {
        int idx = s.indexOf('?');
        if( idx >= 0 && idx < 10 ) {
            throw new IVException(Resource.INVALURL, new Object[] {s});
        }

        String className;
        if( idx == -1 ) {
            className = s.substring(10);
        } else {
            className = s.substring( 10, idx );
        }
        try {
            clazz = Class.forName(className);
        } catch( Exception e ) {
            throw new IVException(Resource.INVALURL, new Object[] {s}, e);
        }

        if( idx >= 0 ) parse(s, idx);
    }

    public String getName() {
        return clazz.getName();
    }

    public InputStream getInputStream() throws IOException {
        InputStream is = null;
        try {
            Method getStream = clazz.getMethod("getStream", new Class [] { Hashtable.class } );
            is = (InputStream) getStream.invoke( null, new Object[] { parms } );
        } catch( Exception e ) {
            throw new IOException( e.getLocalizedMessage() );
        }
        // fgjava: urls are allowed to return a null stream
        // http://www.macromedia.com/go/15196
        if (is == null)
            throw new IOException("null input stream");
        return is;
    }

}
