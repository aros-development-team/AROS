/*
 * $Id: GenUtil.java,v 1.4 2002/02/25 19:50:55 valis Exp $
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
 * @deprecated It is now prefered to use the API directly since it has gotten
 *             easier to use, and supports more advanced features. This class
 *             only provides an interface to the most basic capabilities of
 *             JGenerator.
 *
 * The class provides high-level API to the JGenerator<br>
 * <br>
 * Usage:<br>
 * <pre>
 *   // install directory "c:\\jgenerator-v1.1.3" and log to console
 *   GenUtil.init( "c:\\jgenerator-v1.1.3", false );
 *   ...
 *   try {
 *     // parse file
 *     Object flashFile = GenUtil.parse( "d:\\templates\\cooltemplate.swt" );<br>
 *
 *     // define parameters. if your parameters are case unsensitive, you have to set
 *     // property iv.flash.varCaseSensitive in iv.properties file to false and add the
 *     // parameters' names in uppercase
 *     Hashtable parms = new Hashtable();
 *     parms.put( "DATASRC", "fgjdbc:///?driver=oracle.jdbc.driver.OracleDriver&url=jdbc:oracle:thin:username/password@lifu:1521:rs8i&query=select+*+from+flashtest+where+id>1" );<br>
 *
 *     // process file with given parameters
 *     GenUtil.process( flashFile, parms );<br>
 *
 *     // generate file
 *     InputStream is = GenUtil.generate( flashFile );<br>
 *
 *     // write to disk
 *     OutputStream out = new FileOutputStream( "d:\\movies\\coolmovie.swf" );
 *     int size;
 *     byte[] buffer = new byte[4096];
 *     while( (size=is.read(buffer, 0, buffer.length))>0 ) out.write(buffer, 0, size);
 *     out.close();
 *  } catch( IVException e ) {
 *    Log.logRB( e );
 *  }
 *
 * </pre>
 */
public final class GenUtil {

    /**
     * Parse specified .swt file and return instance of FlashFile object
     *
     * @param name name of .swt file to parse. name may be absolute or relative. in last case it's
     *             relative from the current directory.
     * @return instance of FlashFile.
     * @exception IVException thrown if some generator problems occured
     * @exception FileNotFoundException thrown if the given file cannot be found
     * @author Dmitry Skavish
     * @version 1.0
     */
    public static Object parse( String name ) throws IVException, FileNotFoundException {
        return FlashFile.parse( name );
    }

    /**
     * Process given FlashFile object using given user parameters
     *
     * @param file FlashFile object obtained from parse method.
     * @param parms Hashtable of parameters. If property iv.flash.varCaseSensitive in iv.properties file
     *              set to false then parameter names <b>have</b> to be in uppercase.
     * @exception IVException thrown if some generator problems occured
     * @author Dmitry Skavish
     * @version 1.0
     */
    public static void process( Object file, Hashtable parms ) throws IVException {
        Context context = new StandardContext(parms);
        ((FlashFile)file).processFile(context);
    }

    /**
     * Generate .swf file from given FlashFile
     *
     * @param file FlashFile object ibtained from process method
     * @return InputStream (ByteArrayInputStream)
     * @author Dmitry Skavish
     * @version 1.0
     */
    public static InputStream generate( Object file ) throws IVException {
        FlashOutput fout = ((FlashFile)file).generate();
        return new ByteArrayInputStream(fout.getBuf(), 0, fout.getSize());
    }

    /**
     * Initialization. Has to be called once before all the others methods.
     *
     * @param installDir specifies absolute path to the directory where JGenerator is installed
     * @param logToFile  if true logging will be to log file (the one listed in the iv.properties file)
     *                   otherwise to console
     * @author Dmitry Skavish
     * @version 1.0
     */
    public static void init( String installDir, boolean logToFile ) {
        Util.init(installDir);
        if( !logToFile ) {
            Log.setLogToConsole();
        }
    }

}
