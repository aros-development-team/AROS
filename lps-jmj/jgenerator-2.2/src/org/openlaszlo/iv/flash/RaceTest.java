/*
 * $Id: RaceTest.java,v 1.3 2002/02/24 02:10:19 skavish Exp $
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

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.context.*;

/**
 * Command-line (offline) generator
 */
public final class RaceTest {

    public static FlashFile parse( String name ) throws IVException, FileNotFoundException {
        return FlashFile.parse( name );
    }

    public static void process( FlashFile file, Context context ) throws IVException {
        file.processFile(context);
    }

    public static FlashOutput generate( FlashFile file ) throws IVException {
        return file.generate();
    }

    public static void help() {
        System.err.println( "JGenerator Race Condition Test Version "+Util.getVersion() );
        System.err.println( "Copyright (c) Dmitry Skavish, 2000. All rights reserved." );
        System.err.println( "" );
        System.err.println( "Usage: racetest [options] <filename.swt>" );
        System.err.println( "" );
        System.err.println( "Options:" );
        System.err.println( "    -help                  displays usage text" );
        System.err.println( "    -param <name> <value>  specifies a named parameter" );
        System.err.println( "    -threads <num>         number of created threads (50 default)" );
        System.err.println( "    -iters <num>           number of iterations in each thread (1 default)" );
        System.err.println( "    -verbose               verbose output" );
        System.err.println( "    -save                  save output" );
        System.err.println( "" );
        System.exit(1);
    }

    public static void err( String msg ) {
        System.err.println( msg );
        help();
    }

    public static void main( String[] args ) {

        Util.init();
        Log.setLogToConsole();

        String outFileName = null;
        String inFileName = null;
        int threads = 50;
        int iters = 1;
        boolean verbose = false;
        boolean save = false;
        StandardContext context = new StandardContext();
        // parse options
        int l = args.length-1;
        for( int i=0; i<=l; i++ ) {
            if( args[i].equals("-help") ) {
                help();
            } else if( args[i].equals("-param") ) {
                if( i+2 > l ) err( "Error declaring parameter" );
                String name = args[++i];
                String value = args[++i];
                if( value.charAt(0) == '"' && value.charAt( value.length()-1 ) == '"' ) {
                    value = value.substring(1, value.length()-1);
                }
                context.setValue(name, value);
            } else if( args[i].equals("-threads") ) {
                if( i+1 > l ) err( "Number of threads is not specified" );
                threads = Util.toInt( args[++i], 50 );
            } else if( args[i].equals("-iters") ) {
                if( i+1 > l ) err( "Number of iterations is not specified" );
                iters = Util.toInt( args[++i], 1 );
            } else if( args[i].equals("-verbose") ) {
                verbose = true;
            } else if( args[i].equals("-save") ) {
                save = true;
            } else {
                inFileName = args[i];
                if( i != l ) err( "Too many parameters" );
            }
        }

        if( inFileName == null ) err( "Input file is not specified" );
        if( outFileName == null ) {
            if( inFileName.endsWith(".swt") ) {
                outFileName = inFileName.substring(0, inFileName.length()-3)+"swf";
            } else {
                outFileName = inFileName+".swf";
            }
        }

        final String myFileName = inFileName;
        final Context myContext = context;
        final boolean myVerbose = verbose;
        final int myIters = iters;
        final FlashOutput[] fobs = new FlashOutput[threads];
        FlashFile file2 = null;
        try { file2 = parse( myFileName ); } catch( Exception e ) { Log.log(e); }
        final FlashFile file1 = file2;
        for( int i=0; i<threads; i++ ) {
            final int ii = i;
            Thread st = new Thread() {
                public void run() {
                    for( int j=0; j<myIters; j++ ) {
                        fobs[ii] = process( file1, myContext, ii, myVerbose );
//                        fobs[ii] = process( myFileName, myContext, ii, myVerbose );
                    }
                }
            };
            st.start();
        }

        // compare all fobs
        if( verbose ) System.out.println( "Comparing fobs ... " );
        boolean f = true;
        FlashOutput fob = getFob( fobs, 0 );
        for( int i=1; i<threads; i++ ) {
            FlashOutput fb = getFob( fobs, i );
            if( !compare( fob, fb ) ) {
                if( verbose ) System.out.println( "#0 and #"+i+" are not equal" );
                f = false;
            } else {
                if( verbose ) System.out.println( "#0 and #"+i+" are equal" );
            }
        }
        if( f ) {
            System.out.println( "Comparing ok" );
        } else {
            System.out.println( "Comparing failed" );
        }
        if( save ) {
            System.out.println( "Saving ..." );
            for( int i=0; i<threads; i++ ) {
                FlashOutput fb = getFob( fobs, i );
                try {
                    BufferedOutputStream bos = new BufferedOutputStream( new FileOutputStream( i+"_"+outFileName ) );
                    bos.write( fb.getBuf(), 0, fb.getSize() );
                    bos.close();
                } catch( Exception e ) {
                    Log.log(e);
                }
            }
        }
//        System.exit(0);
    }

    private static FlashOutput process( FlashFile file1, Context myContext, int ii, boolean v ) {
         try {
             long startTime = System.currentTimeMillis();
             if( v ) System.out.println( "Copying thread #"+ii );
             FlashFile file = file1.copyFile();
             if( v ) System.out.println( "Processing thread #"+ii );
             process(file,myContext);
             if( v ) System.out.println( "Generating thread #"+ii );
             FlashOutput fob = generate(file);
             if( v ) System.out.println( "Done thread #"+ii+" processing time is: "+(System.currentTimeMillis()-startTime)+"ms" );
             return fob;
         } catch( IVException e ) {
             Log.log(e);
         } catch( RuntimeException e ) {
             Log.logRB(Resource.UNKNOWNERROR, e);
         }
         return null;
    }

    private static FlashOutput process( String myFileName, Context myContext, int ii, boolean v ) {
         try {
             long startTime = System.currentTimeMillis();
             if( v ) System.out.println( "Parsing thread #"+ii );
             FlashFile file = parse( myFileName );
             if( v ) System.out.println( "Processing thread #"+ii );
             process(file,myContext);
             if( v ) System.out.println( "Generating thread #"+ii );
             FlashOutput fob = generate(file);
             if( v ) System.out.println( "Done thread #"+ii+" processing time is: "+(System.currentTimeMillis()-startTime)+"ms" );
             return fob;
         } catch( FileNotFoundException e ) {
             Log.logRB( Resource.FILENOTFOUND, new Object[] {myFileName} );
         } catch( IVException e ) {
             Log.log(e);
         } catch( RuntimeException e ) {
             Log.logRB(Resource.UNKNOWNERROR, e);
         }
         return null;
    }

    private static boolean compare( FlashOutput fob1, FlashOutput fob2 ) {
        int size = fob1.getSize();
        if( size != fob2.getSize() ) return false;
        byte[] buf1 = fob1.getBuf();
        byte[] buf2 = fob2.getBuf();
        for( int i=0; i<size; i++ ) {
            if( buf1[i] != buf2[i] ) return false;
        }
        return true;
    }

    private static FlashOutput getFob( FlashOutput[] fobs, int i ) {
        while( fobs[i] == null ) {
            try {
                Thread.sleep(10);
            } catch( InterruptedException e ) {}
        }
        return fobs[i];
    }

}
