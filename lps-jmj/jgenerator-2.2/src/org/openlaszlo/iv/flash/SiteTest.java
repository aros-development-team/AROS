/*
 * $Id: SiteTest.java,v 1.2 2002/02/15 23:44:27 skavish Exp $
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

package org.openlaszlo.iv.flash;

import java.io.*;
import java.util.*;
import java.net.*;

/**
 */
public final class SiteTest {

    public static void help() {
        System.err.println( "Site test v1.0" );
        System.err.println( "Copyright (c) Dmitry Skavish, 2000. All rights reserved." );
        System.err.println( "" );
        System.err.println( "Usage: sitetest [options] <url>" );
        System.err.println( "" );
        System.err.println( "Options:" );
        System.err.println( "    -help                  displays usage text" );
        System.err.println( "    -users <num>           number of users (20 default)" );
        System.err.println( "    -verbose               verbose output" );
        System.err.println( "" );
        System.exit(1);
    }

    public static void err( String msg ) {
        System.err.println( msg );
        help();
    }

    public static void main( String[] args ) throws MalformedURLException {

        String url = null;
        int users = 50;
        boolean verbose = false;
        // parse options
        int l = args.length-1;
        for( int i=0; i<=l; i++ ) {
            if( args[i].equals("-help") ) {
                help();
            } else if( args[i].equals("-users") ) {
                if( i+1 > l ) err( "Number of users is not specified" );
                users = Integer.parseInt( args[++i] );
            } else if( args[i].equals("-verbose") ) {
                verbose = true;
            } else {
                url = args[i];
                if( i != l ) err( "Too many parameters" );
            }
        }

        if( url == null ) err( "Url is not specified" );

        start = System.currentTimeMillis();

        final boolean myVerbose = verbose;
        for( int i=0; i<users; i++ ) {
            Thread st = new User( url );
            st.start();
        }

        new Thread() {
            public void run() {
                try {
                    for(;;) {
                        Thread.sleep(5000);
                        int size;
                        long time;
                        int num;
                        synchronized( SiteTest.class ) {
                            size = totalSize;
                            time = totalTime;
                            num = number;
                        }
                        System.err.println( "----------------------------------------------------------------------" );
                        System.err.println( "total size: "+size+", total time: "+time+"ms, number: "+num );
                        System.err.println( "avg size: "+(size/num)+", avg time: "+(time/num)+"ms" );
                        double nps = (num*1000.0)/(System.currentTimeMillis()-start);
                        System.err.println( "requests per second: "+nps );
                    }
                } catch( InterruptedException e ) {
                }
            }
        }.start();

//        System.exit(0);
    }

    private static long start = 0;
    private static int totalSize = 0;
    private static long totalTime = 0;
    private static int number = 0;

    public synchronized static void addData( int size, long time ) {
        totalSize += size;
        totalTime += time;
        number++;
    }

    public static class User extends Thread {
        private URL url;
        private byte[] buffer = new byte[4096*4];

        public User( String urlStr ) throws MalformedURLException {
            url = new URL( urlStr );
        }

        public void run() {
            for(;;) {
                try {
                    long time = System.currentTimeMillis();
                    URLConnection conn = url.openConnection();
                    conn.connect();
                    InputStream is = conn.getInputStream();
                    int thisSize = 0;
                    int size;
                    while( (size=is.read(buffer, 0, buffer.length))>0 ) {
                        thisSize += size;
                    }
                    time = System.currentTimeMillis() - time;
                    addData( thisSize, time );
                    is.close();
                } catch( IOException e ) {
                    err( e.getMessage() );
                }
            }
        }
    }
}
