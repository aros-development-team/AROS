/*
 * $Id: FOToSWF.java,v 1.3 2002/02/24 02:10:19 skavish Exp $
 *
 * ==========================================================================
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

package org.openlaszlo.iv.flash.fop;

import org.openlaszlo.iv.flash.util.*;

import org.openlaszlo.iv.flash.api.*;

import org.apache.fop.apps.*;
import org.apache.fop.messaging.MessageHandler;
import org.apache.fop.viewer.*;

import org.xml.sax.*;
import org.apache.xerces.parsers.*;

import java.io.*;
import java.net.*;
import java.util.*;

/**
 * A command line launcher using FOPHelper to convert FO input to SWF output.
 * Mostly ripped out of "SWFStarter" right now.
 *
 * @author James Taylor
 * @author Johan "Spocke" Sörlin
 */

class FOToSWF
{
    private InputSource xslFOInput;
    private OutputStream swfOutputStream;

    /**
     * Changes the XSLFO InputSource, this input source is
     * requierd in order to generate a SWF movie.
     *
     * @param input_source inputsource pointing to the XSLFO file/stream
     */

    public void setXSLFOInputSource( InputSource input_source )
    {
        this.xslFOInput = input_source;
    }

    /**
     * Changes the output stream, this stream is where the output SWF movie
     * will be stored.
     *
     * @param output_stream stream to write finished SWF movie to
     */

    public void setOutputStream( OutputStream output_stream )
    {
        this.swfOutputStream = output_stream;
    }

    /**
     * Generates the SWF movie from the passed inputparameters using FOP.
     */

    public void generate( ) throws IVException, IOException, FOPException
    {
        FOPHelper fh = new FOPHelper();

        fh.setXSLFOInputSource( this.xslFOInput );

        fh.render();

        Script s = fh.getScript();

        FlashFile f = FlashFile.newFlashFile( );

        s.setMain();

        f.setMainScript( s );

        f.setFrameSize( fh.getMaxPageBounds() );

        FlashOutput fob = f.generate( );

        BufferedOutputStream bos = new BufferedOutputStream( this.swfOutputStream );
        bos.write( fob.getBuf( ), 0, fob.getSize( ) );
        bos.flush( );
        bos.close( );
    }

    /**
     * Console main method, this method is executed when running in console mode.
     *
     * @param args application arguments
     */

    public static void main( String args[] )
    {
        FOToSWF starter = new FOToSWF();
        String temp;
        boolean inputFilePassed = false;

        try
        {
            Util.init( );
            Log.setLogToConsole( );

            // Check num arguments
            if ( args.length < 3 )
            {
                printHelp( "Requierd arguments missing." );
            }

            // Get XSL-FO parameter
            temp = getParameterByName( args, "-fo" );
            if ( temp != null )
            {
                starter.setXSLFOInputSource( new InputSource( temp ) );
                inputFilePassed = true;
            }

            // If no input file, pass error

            if ( ! inputFilePassed )
            {
                printHelp( "Input file missing, use argument \"-fo <infile>\"." );
            }

            // Get SWF output path

            temp = getParameterByName( args, "-swf" );
            if ( temp != null )
            {
                starter.setOutputStream( new FileOutputStream( temp ) );
            }
            else
            {
                printHelp( "Output file missing, use argument \"-swf <outfile>\"." );
            }

            // Do the generation

            starter.generate( );

            System.exit( 0 );
        }
        catch ( Exception e )
        {
            e.printStackTrace();
        }
    }


    // * * Private class methods

    /**
     * Returns a parameter value by a parameter name.
     *
     * @param args application arguments
     * @param name name of parameter to get
     * @return parameter value or null if it wasn't found
     */
    private static String getParameterByName( String args[], String name )
    {
        for ( int i = 0; i < args.length; i++ )
        {
            if ( args[ i ].equals( name ) )
            {
                if ( ( i + 1 ) < args.length )
                    return args[ i + 1 ];
                else
                    return null;
            }
        }

        return null;
    }

    /**
     * Checks the existance of a parameter by a parameter name.
     *
     * @param args application arguments
     * @param name name of parameter to get
     * @return true - it exists, false - it doesn't exist
     */
    private static boolean checkParameterByName( String args[], String name )
    {
        for ( int i = 0; i < args.length; i++ )
        {
            if ( args[ i ].equals( name ) )
                return true;
        }

        return false;
    }

    /**
     * Prints the usage text.
     *
     * @param error_msg Error message to write with usage text
     */
    private static void printHelp( String error_msg )
    {
        System.out.println( "USAGE" );
        System.out.println( "" );
        System.out.println( "SWFStarter [options] -fo infile -swf outfile" );
        System.out.println( "" );
        System.out.println( "  [INPUT]" );
        System.out.println( "   -fo  infile       xsl:fo input file" );
        System.out.println( "" );
        System.out.println( "  [OUTPUT]" );
        System.out.println( "   -swf outfile      input will be rendered as swf format into the outfile" );
        System.out.println( "" );
        System.out.println( "  [Examples]" );
        System.out.println( "   SWFStarter -fo myfile.fo myfile.swf" );
        System.out.println( "" );
        System.out.println( "ERROR: " + error_msg );
        System.out.println( "" );

        System.exit( -1 );
    }
}
