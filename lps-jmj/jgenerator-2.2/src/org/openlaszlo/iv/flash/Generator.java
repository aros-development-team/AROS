/*
 * $Id: Generator.java,v 1.10 2002/05/29 04:32:42 skavish Exp $
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

import org.openlaszlo.iv.flash.api.*;
//import org.openlaszlo.iv.flash.player.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.commands.*;
import org.openlaszlo.iv.flash.context.*;

/**
 * Command-line (offline) generator.
 */
public final class Generator {

    private static final int GIF_IMAGE  = 0;
    private static final int JPEG_IMAGE = 1;

    private int imageType = -1;
    private String outImageName;

    private String outFileName;
    private String inFileName;

    public void doCommandLine( String[] args, boolean preview ) {

        String dumpFileName = null;

        long startTime = -1;
        boolean logInit = false;

        CommandExecutor executor = new OfflineCommandExecutor();
        Context context = new CommandContext( executor );

        GenericCommand setenv_cmd = null;
        String encoding = null;
        boolean compressOutput = false;

        // parse options
        int l = args.length-1;
        for( int i=0; i<=l; i++ ) {
            if( args[i].equals("-help") ) {
                help();
            } else if( args[i].equals("-log") ) {
                err("Option -log is deprecated, please use -log4j instead");
                //Log.setLogToFile();
                //logInit = true;
            } else if( args[i].equals("-log4j") ) {
                logInit = true;
            } else if( args[i].equals("-compress") ) {
                compressOutput = true;
            } else if( args[i].equals("-verbose") ) {
                if( i == l ) err("Verbose level is not specified");
                String level = args[++i];
                if( level.equalsIgnoreCase("fatal") ) Log.setFatalLevel();
                else if( level.equalsIgnoreCase("error") ) Log.setErrorLevel();
                else if( level.equalsIgnoreCase("warn") ) Log.setWarnLevel();
                else if( level.equalsIgnoreCase("info") ) Log.setInfoLevel();
                else if( level.equalsIgnoreCase("debug") ) Log.setDebugLevel();
                else {
                    err("Unknown verbose level "+level);
                    Log.setInfoLevel();
                }
            /*} else if( preview && args[i].equals("-logfile") ) {
                if( i == l ) err( "Log file is not specified" );
                String logFileName = args[++i];
                new File(logFileName).delete();
                Log.setLogToFile(logFileName);
                logInit = true;*/
            } else if( preview && args[i].equals("-d") ) {
                if( i == l ) err( "Debug level is not specified" );
                i++;    // ignore
            } else if( args[i].equals("-swf") ) {
                if( i == l ) err( "Output file is not specified" );
                outFileName = args[++i];
            } else if( args[i].equals("-gif") ) {
                if( i == l ) err( "Output GIF file is not specified" );
                outImageName = args[++i];
                imageType = GIF_IMAGE;
            } else if( args[i].equals("-jpg") ) {
                if( i == l ) err( "Output JPEG file is not specified" );
                outImageName = args[++i];
                imageType = JPEG_IMAGE;
            } else if( args[i].equals("-dump") ) {
                if( i == l ) err( "Dump file is not specified" );
                dumpFileName = args[++i];
            } else if( args[i].equals("-time") ) {
                startTime = System.currentTimeMillis();
            } else if( args[i].equals("-setenv1") ) {
                if( i == l ) err( "Datasource for -setenv1 is not specified" );
                if( setenv_cmd != null ) err( "Datasource is already specified" );
                setenv_cmd = new SetEnvironmentCommand();
                setenv_cmd.addParameter( "datasource", args[++i] );
            } else if( args[i].equals("-setenv2") ) {
                if( i == l ) err( "Datasource for -setenv1 is not specified" );
                if( setenv_cmd != null ) err( "Datasource is already specified" );
                setenv_cmd = new SetEnvironment2Command();
                setenv_cmd.addParameter( "datasource", args[++i] );
            } else if( args[i].equals("-encoding") ) {
                if( i == l ) err( "Encoding is not specified" );
                encoding = args[++i];
            } else if( args[i].equals("-param") ) {
                if( i+2 > l ) err( "Error declaring parameter" );
                String name = args[++i];
                String value = args[++i];
                if( value.length() > 0 && value.charAt(0) == '"' && value.charAt( value.length()-1 ) == '"' ) {
                    value = value.substring(1, value.length()-1);
                }
                ((StandardContext)context).setValue(name, Util.processEscapes(value) );
            } else if( preview && args[i].equals("-t") ) {
                if( i == l ) err( "Input file is not specified" );
                inFileName = args[++i];
            } else {
                inFileName = args[i];
                if( i != l ) err( "Too many parameters" );
            }
        }

        if( inFileName == null ) err( "Input file is not specified" );
        if( outFileName == null && imageType == -1 && dumpFileName == null ) {
            if( inFileName.endsWith(".swt") ) {
                outFileName = inFileName.substring(0, inFileName.length()-3)+"swf";
            } else {
                outFileName = inFileName+".swf";
            }
        }

        if( !logInit ) {
            Log.setLogToConsole();
        }

        FlashFile file = null;
        if( encoding != null && Util.isDefault(encoding) ) {
            encoding = null;
        }

        try {

            try {
                file = FlashFile.parse(inFileName, imageType!=-1, encoding);
            } catch( FileNotFoundException e ) {
                Log.logRB(Resource.FILENOTFOUND, new Object[] {inFileName}, e);
            }

            if( file != null ) {
                // set parsed file to command executor
                executor.setFlashFile(file);

                if( dumpFileName != null ) dump( file, dumpFileName );
                else {
                    // load environment
                    if( setenv_cmd != null ) {
                        try {
                            FakeContext fakeContext = new FakeContext(context);
                            setenv_cmd.doCommand(file, fakeContext, file.getMainScript(), 0);
                            Context myContext = fakeContext.getContext();
                            myContext.setParent(context);   // it's supposedly already done
                            context = myContext;
                        } catch( Exception e ) {
                            Log.logRB( new IVException(Resource.ERRDOCMD, new Object[] {
                                        file.getFullName(), "", "0", setenv_cmd.getCommandName() }, e));
                        }
                    }

                    file.processFile(context);

                    if( outFileName != null ) {
                        try {
                            file.setCompressed(compressOutput);
                            FlashOutput fob = file.generate();
                            BufferedOutputStream bos = new BufferedOutputStream( new FileOutputStream( outFileName ) );
                            bos.write( fob.getBuf(), 0, fob.getSize() );
                            bos.close();
                        } catch( IOException e ) {
                            Log.logRB(Resource.ERRWRITINGFILE, new Object[] {outFileName}, e);
                        }
                    }
                }

            }

            /*
            if( imageType != -1 ) {
                Player player = new Player(file);
                player.play(outImageName);
            }*/

        } catch( Exception e ) {
            Log.log(e);
        } catch( Throwable e ) {
            Log.logRB(Resource.UNKNOWNERROR, e);
        }

        if( startTime != -1 ) {
            System.err.println( "Processing time is: "+(System.currentTimeMillis()-startTime)+"ms" );
        }

    }

    public static void main( String[] args ) {
        Util.init();

        Generator gen = new Generator();
        gen.doCommandLine(args, false);

        System.exit(0);
    }

    private static void dump( FlashFile file, String fileName ) throws IOException {
        FileOutputStream fout = new FileOutputStream(fileName);
        PrintStream out = new PrintStream(fout, true);
        file.printContent( out );
        Enumeration defs = file.definitions();
        out.println( "Definitions:" );
        while( defs.hasMoreElements() ) {
            FlashDef def = (FlashDef) defs.nextElement();
            def.printContent( out, "" );
        }
    }

    public static void help() {
        System.err.println( "JGenerator Version "+Util.getVersion() );
        System.err.println( "Copyright (c) JZox, Inc. 2000-2002. All rights reserved." );
        System.err.println( "" );
        System.err.println( "Usage: jgenerate [options] <filename.swt>" );
        System.err.println( "" );
        System.err.println( "Options:" );
        System.err.println( "    -swf <filename.swf>    output as Flash movie" );
        //System.err.println( "    -jpg <filename.jpg>    output as JPEG image" );
        //System.err.println( "    -gif <filename.gif>    output as GIF image" );
        System.err.println( "    -param <name> <value>  specifies a named parameter" );
        System.err.println( "    -setenv1 <url>         specifies flash environment (Name,Value or XML)" );
        System.err.println( "    -setenv2 <url>         specifies flash environment (Name1,Name2... or XML)" );
        System.err.println( "    -encoding <encoding>   specifies default encoding of all datasources" );
        System.err.println( "    -dump <filename>       dump template content into specified file" );
        //System.err.println( "    -log                   redirects output to system log file" );
        //System.err.println( "    -logfile <filename>    redirects output to specified log file" );
        System.err.println( "    -log4j                 use log4j configuration for output" );
        System.err.println( "    -verbose <level>       verbose level: fatal, error, warn, info, debug" );
        System.err.println( "    -time                  print processing time" );
        System.err.println( "    -compress              compress output (Flash MX)" );
        System.err.println( "    -help                  displays usage text" );
        System.err.println( "" );
        System.exit(1);
    }

    public static void err( String msg ) {
        System.err.println( msg );
        help();
    }

    /**
     * Offline command executor.
     * <P>
     * Adds additional commands for offline version of generator
     */
    public class OfflineCommandExecutor extends CommandExecutor {

        /**
         * Sets output file name
         *
         * @param name   output file name
         * @return output file name
         */
        public String setOutputFile( Context context, String name ) {
            outFileName = name;
            return outFileName;
        }

        /**
         * Returns output file name
         *
         * @return output file name
         */
        public String getOutputFile( Context context ) {
            return outFileName;
        }

        /**
         * Returns input file name
         *
         * @return input file name
         */
        public String getInputFile( Context context ) {
            return inFileName;
        }
    }

}
