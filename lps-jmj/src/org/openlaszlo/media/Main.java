/* *****************************************************************************
 * Main.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.media;

import org.openlaszlo.utils.FileUtils;

import java.io.*;
import org.apache.log4j.*;
import org.apache.log4j.spi.*;
import org.apache.log4j.varia.NullAppender;

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
     * Usage: java org.openlaszlo.media.Main in out
     *
     * Converts input ttf to output fft.
     *
     * @param args the command line arguments
     */
    public static void main(String args[]) {
        lzmc(args);
    }

    /** This method implements the behavior described in main
     * but also returns an integer error code.
     */
    public static int lzmc(String args[]) {

        int exitStatus = 0;

        // Configure logging
        Logger logger = Logger.getRootLogger();
        logger.setLevel(Level.ERROR);

        logger.addAppender(new ConsoleAppender(
           new PatternLayout("%r msecs [%p] - %m%n" )));

        ErrorChecker errorChecker = new ErrorChecker();
        logger.addAppender(errorChecker);

        try {
            for (int i = 0; i < args.length; i++) {
                String arg = args[i].intern();
                if (arg == "-v") {
                    logger.setLevel(Level.ALL);
                } else if (arg == "-i") {
                    logger.setLevel(Level.INFO);
                }
            }

            if (args.length < 2) {
                System.err.println("Usage: lzmc [-v] [-i] from to");
                return -1;
            }

            String fromFileName = args[args.length-2];
            String toFileName = args[args.length-1];
            String fromType = MimeType.fromExtension(fromFileName);
            String toType   = MimeType.fromExtension(toFileName);

            // Assume it's a font.
            if (fromType.equalsIgnoreCase("UNKNOWN")) {
                fromType = FontType.TTF;
                toType = FontType.FFT;
            }

            logger.info("Converting from " + fromType + " to " + toType);
            File         from = new File(fromFileName);
            OutputStream to   = new FileOutputStream(toFileName);
            InputStream  cvt  = Transcoder.transcode(from, fromType, toType);
            logger.info("Finished conversion");
            FileUtils.send(cvt, to);
            to.flush();
            to.close();
            logger.info("Finished writing file");
        } catch (TranscoderException e) {
            exitStatus = -1;
            System.err.println("Transcoder Exception:" + e.getMessage());
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
