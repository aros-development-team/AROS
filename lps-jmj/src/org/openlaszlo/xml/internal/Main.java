/* *****************************************************************************
 * Main.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.xml.internal;

import java.io.*;
import org.apache.log4j.*;
import org.apache.log4j.spi.*;
import org.apache.log4j.varia.NullAppender;

import org.jdom.input.SAXBuilder;
import org.jdom.Document;
import org.jdom.Element;
import org.jdom.JDOMException;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.xml.*;

/**
 * Log4j appender that simply keeps state as to
 * whether or not there has been an error, warning, or fatal
 * log message
 */
class ErrorChecker extends NullAppender {
    public boolean hadError = false;
    public void doAppend(LoggingEvent e) {
        super.doAppend(e);
        if (e.level == Level.WARN ||
            e.level == Level.FATAL ||
            e.level == Level.ERROR) {
            hadError = true;
        }
    }
}

/**
 * Test class for media converter
 */
public class Main {

    /** 
     * Usage: java org.openlaszlo.xml.Main in out
     *
     * Converts input xml to output swf.
     *
     * @param args the command line arguments
     */
    public static void main(String args[]) {
        lzdc(args);
    }

   /**
     * Allow execution from the command line
     *
     * @param args[] 2 arguments - [XML input document],
     *  [SWF output document]
     */

    public static int lzdc(String[] args) {

        int exitStatus = 0;

        // Configure logging
        Logger logger = Logger.getRootLogger();
        logger.setLevel(Level.ERROR);

        logger.addAppender(new ConsoleAppender(
           new PatternLayout("%r msecs [%p] - %m%n" )));

        ErrorChecker errorChecker = new ErrorChecker();
        logger.addAppender(errorChecker);

        int swfversionNum = 5;

        try {
            if (args.length < 2) {
                System.err.println("Usage: lzdc [-v] [-i] from to");
                return -1;
            }

            for (int i = 0; i < args.length; i++) {
                String arg = args[i].intern();
                if (arg == "-v") {
                    logger.setLevel(Level.ALL);
                } else if (arg == "-i") {
                    logger.setLevel(Level.INFO);
                } else if (arg == "-5") {
                    swfversionNum = 5;
                } else if (arg == "-6") {
                    swfversionNum = 6;
                } else if (arg == "-7") {
                    swfversionNum = 7;
                }
            }

            String fromFileName = args[args.length-2];
            String toFileName = args[args.length-1];

            String xmlstring = FileUtils.readFileString(new File(fromFileName), "UTF-8");
            // Create and load properties
            logger.info("Reading XML from " + fromFileName);

            InputStream is = DataCompiler.compile(xmlstring, swfversionNum);
            FileOutputStream w = new FileOutputStream(toFileName);

            logger.info("Writing to " + toFileName);
            
            //de.buildFromDocument(doc);
            FileUtils.send(is, w);
            w.flush();

            logger.info("Done");

        } catch (DataCompilerException e) {
            exitStatus = -1;
            System.err.println("DataCompilerException: " + e.getMessage());
            e.printStackTrace();
        } catch (IOException e) {
            exitStatus = -1;
            System.err.println("IO exception: " + e.getMessage());
            e.printStackTrace();
        }

        if (errorChecker.hadError)
            exitStatus = -1;

        return exitStatus;
    }
}
