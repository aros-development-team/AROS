/*
 * $Id: Log.java,v 1.1 2002/02/24 02:11:24 skavish Exp $
 *
 * ===========================================================================
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

package org.openlaszlo.iv.flash.util;

import org.openlaszlo.iv.flash.api.*;
import java.io.*;
import java.net.*;
import java.util.*;

import org.apache.log4j.*;
import org.apache.log4j.spi.*;
import org.apache.log4j.helpers.*;

/**
 * Logger
 * <P>
 * After replacing jgenerator native logging system with log4j
 * all the jgenerator classes like FileLogger, ConsoleLogger etc. are gone.
 * But I decided to keep this class Log to avoid changing a lot of code
 * which uses it.
 * <P>
 * Instead I changed its implementation to make it a wrapper for log4j.
 * <P>
 * log4j has several level methods, like 'debug', 'warn' etc., but jgenerator Log
 * has only one method 'log' which takes the level of a message from its
 * resource key's first character. I preserved this schema, so now if you use
 * this class's log method it will call corresponding log4j method based
 * on the first character of the message key, otherwise you can call any of
 * log4j methods directly or using new Log methods with the same signature.
 * <P>
 * The default initialization procedure:
 * <UL>
 * <LI>get property org.openlaszlo.iv.flash.log4j.configuration from iv.property file
 * <LI>if it exists and contains valid url or file name (either relative to jgen home or absolute)
 * then use this url to initialize log4j (it can be either xml or property based configuration)
 * <LI>if the property does not exist or is not valid then use standard log4 initialization procedure,
 * i.e. read system property log4j.configuration etc
 * </UL>
 *
 * @author Dmitry Skavish
 */
public class Log {

    private static final String FQCN = Log.class.getName();

    private static Logger logger;
    private static final String filePattern = "%d [%p] - %m%n";
    private static final String consolePattern = "%r [%p] - %m%n";

    static {

        String log4j_conf = PropertyManager.getProperty("org.openlaszlo.iv.flash.log4j.configuration");

        if( log4j_conf != null ) {
            URL url = null;
            try {
                url = new URL(log4j_conf);
            } catch( MalformedURLException e ) {
                File file = Util.getSysFile(log4j_conf);
                try {
                    url = file.toURL();
                } catch( MalformedURLException ee ) {
                }
            }

            // configure log4j
            if( url != null ) {
                OptionConverter.selectAndConfigure(url, null, LogManager.getLoggerRepository());
            }
        }

        logger = Logger.getLogger("org.openlaszlo.iv.flash.Generator");
        logger.setResourceBundle(Resource.getInstance());
    }

    /**
     * Gets current logger
     *
     * @return current logger
     */
    public static Logger getLogger() {
        return logger;
    }

    /**
     * Sets new logger
     * <P>
     * If you set new logger you probably also need to set ResourceBundle to it
     *
     * @param logger new logger
     */
    public static void setLogger( Logger logger ) {
        Log.logger = logger;
    }

    /**
     * Sets log to console
     */
    public static void setLogToConsole() {
        logger.removeAllAppenders();
        logger.addAppender(new ConsoleAppender(new PatternLayout(consolePattern)));
    }

    /**
     * Sets log to file
     * <P>
     * File specified in property org.openlaszlo.iv.flash.logFile
     *
     * @deprecated logging system is configured now using log4j configuration file
     */
 /*   public static void setLogToFile() {
        String fileName = PropertyManager.getProperty("org.openlaszlo.iv.flash.logFile", "logs/generator.log");
        File file = Util.getSysFile(fileName);
        fileName = file.getAbsolutePath();
        setLogToFile(fileName);
    }
*/
    /**
     * Sets log to the specified file
     *
     * @deprecated logging system is configured now using log4j configuration file
     */
/*    public static void setLogToFile( String fileName ) {
        try {
            logger.removeAllAppenders();
            logger.addAppender(new FileAppender(new PatternLayout(filePattern), fileName, true));
        } catch( IOException e ) {
            setLogToConsole();
            log(e);
        }
    }
*/
    /**
     * Logs IVException
     * <P>
     * IVException contains message with parameters inside
     * and probably nested exception
     *
     * @param e      exception to log
     */
    public static void logRB( IVException e ) {
        log(e);
    }

    public static void log( IVException e ) {
        String key = e.getMessageKey();
        Level level = getMessageLevel(key);
        if( !logger.isEnabledFor(level) ) return;

        logger.log(FQCN, level, e.getLocalizedMessage(), e);
    }

    /**
     * Logs Exception
     *
     * @param e      exception to log
     */
    public static void logRB( Throwable t ) {
        log(t);
    }

    public static void log( Throwable t ) {
        if( t instanceof IVException ) {
            log( (IVException) t );
        } else {
            error(t.getLocalizedMessage(), t);
        }
    }

    /**
     * Logs exception as message specified by its resource key
     *
     * @param key    resource key
     * @param p      array of parameters
     * @param t      exception to log
     */
    public static void logRB( String key, Object[] p, Throwable t ) {
        _logif(key, p, t);
    }

    /**
     * Logs exception as message specified by its resource key
     *
     * @param key    resource key
     * @param p      array of parameters
     */
    public static void logRB( String key, Object[] p ) {
        _logif(key, p, null);
    }

    /**
     * Logs exception as message specified by its resource key
     *
     * @param key    resource key
     * @param p      array of parameters
     */
    public static void logRB( String key ) {
        _logif(key, null, null);
    }

    /**
     * Logs exception as message specified by its resource key
     *
     * @param key    resource key
     * @param p      array of parameters
     */
    public static void logRB( String key, Throwable t ) {
        _logif(key, null, t);
    }

    public static void infoRB( String key, Object[] parms ) {
        _logif(Level.INFO, key, parms, null);
    }

    public static void infoRB( String key, Object[] parms, Throwable t ) {
        _logif(Level.INFO, key, parms, t);
    }

    public static void debugRB( String key, Object[] parms ) {
        _logif(Level.DEBUG, key, parms, null);
    }

    public static void debugRB( String key, Object[] parms, Throwable t ) {
        _logif(Level.DEBUG, key, parms, t);
    }

    public static void errorRB( String key, Object[] parms ) {
        _logif(Level.ERROR, key, parms, null);
    }

    public static void errorRB( String key, Object[] parms, Throwable t ) {
        _logif(Level.ERROR, key, parms, t);
    }

    public static void fatalRB( String key, Object[] parms ) {
        _logif(Level.FATAL, key, parms, null);
    }

    public static void fatalRB( String key, Object[] parms, Throwable t ) {
        _logif(Level.FATAL, key, parms, t);
    }

    public static void warnRB( String key, Object[] parms ) {
        _logif(Level.WARN, key, parms, null);
    }

    public static void warnRB( String key, Object[] parms, Throwable t ) {
        _logif(Level.WARN, key, parms, t);
    }

    //
    // methods which take a string not a resource bundle key
    //

    public static void info( Object msg ) {
        logger.info(msg);
    }

    public static void info( Object msg, Throwable t ) {
        logger.info(msg, t);
    }

    public static void debug( Object msg ) {
        logger.debug(msg);
    }

    public static void debug( Object msg, Throwable t ) {
        logger.debug(msg, t);
    }

    public static void error( Object msg ) {
        logger.error(msg);
    }

    public static void error( Object msg, Throwable t ) {
        logger.error(msg, t);
    }

    public static void fatal( Object msg ) {
        logger.fatal(msg);
    }

    public static void fatal( Object msg, Throwable t ) {
        logger.fatal(msg, t);
    }

    public static void warn( Object msg ) {
        logger.warn(msg);
    }

    public static void warn( Object msg, Throwable t ) {
        logger.warn(msg, t);
    }

     /**
     * Checks if a messages specified by given key is enabled under current log level
     *
     * @param key    message key
     * @return true if message has to be logged
     */
    public static Level getMessageLevel( String key ) {
        if( key == null || key.length() == 0 ) return Level.FATAL;
        char ch = key.charAt(0);
        switch(ch) {
            case '0': return Level.FATAL;
            case '1': return Level.ERROR;
            case '2': return Level.WARN;
            case '3': return Level.INFO;
            case '4': return Level.DEBUG;
            default: return Level.FATAL;
        }
    }

    public static void setFatalLevel() {
        logger.setLevel(Level.FATAL);
    }

    public static void setErrorLevel() {
        logger.setLevel(Level.ERROR);
    }

    public static void setWarnLevel() {
        logger.setLevel(Level.WARN);
    }

    public static void setInfoLevel() {
        logger.setLevel(Level.INFO);
    }

    public static void setDebugLevel() {
        logger.setLevel(Level.DEBUG);
    }

    /**
      * Checks if specified message is enabled under current level and logs the message if it is
      *
      * @param key    message key
      * @param parms  optional message parameters
      * @param t      optional exception
      */
    protected static void _logif( String key, Object[] parms, Throwable t ) {
        _logif(getMessageLevel(key), key, parms, t);
    }

    /**
      * Checks if specified level is enabled and logs the message if it is
      *
      * @param level  specified logger level
      * @param key    message key
      * @param parms  optional message parameters
      * @param t      optional exception
      */
    protected static void _logif( Level level, String key, Object[] parms, Throwable t ) {
        if( !logger.isEnabledFor(level) ) return;
        _log(level, key, parms, t);
    }

   /**
     * Logs specified message to current logger
     * <P>
     * This method does not call those methods in Logger which are supposed
     * to be used, but instead calls appenders directly thus avoiding double
     * checking for levels, however this can be easily replaced with standard
     * method, just swap the comments
     *
     * @param level  specified logger level
     * @param key    message key
     * @param parms  optional message parameters
     * @param t      optional exception
     */
    protected static void _log( Level level, String key, Object[] parms, Throwable t ) {
        String message = getMessage(key, parms);
        //logger.log(FQCN, level, message, t);
        logger.callAppenders(new LoggingEvent(FQCN, logger, level, message, t));
    }

    /**
     * Retrieves and formats a message given by its resource key
     * <P>
     * The messages is retrieved from resource bundle associated with current logger,
     * then formatted using java.util.MessageFormat
     *
     * @param key    resource key, if message is not found in the resource then the key is used as a message
     * @param parms  optional parameters
     * @return formatted message
     */
    public static String getMessage( String key, Object[] parms ) {
        return getMessage(logger.getResourceBundle(), key, parms);
    }

    /**
     * Retrieves and formats a message given by its resource key
     * <P>
     * The messages is retrieved from specified resource bundle or from standard jgenerator
     * bundle if the specified one is null, then the message is formatted using java.util.MessageFormat
     *
     * @param rb     specified resource bundle
     * @param key    resource key, if message is not found in the resource then the key is used as a message
     * @param parms  optional parameters
     * @return formatted message
     */
    public static String getMessage( ResourceBundle rb, String key, Object[] parms ) {
        if( rb == null ) {
            rb = Resource.getInstance();
        }

        String msg;
        try {
            msg = rb.getString(key);
        } catch( MissingResourceException e ) {
            msg = "No resource is associated with key \""+key+"\".";
            error(msg);
        } catch( Throwable t ) {
            msg = "Error retrieving resource by key \""+key+"\".";
            error(msg, t);
        }

        if( parms != null ) {
            msg = java.text.MessageFormat.format(msg, parms);
        }

        return msg;
    }

}
