/* *****************************************************************************
 * Main.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import org.openlaszlo.server.LPS;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.sc.ScriptCompiler;
import java.io.*;
import java.util.*;
import org.apache.log4j.*;
import org.apache.log4j.spi.*;
import org.apache.log4j.varia.NullAppender;

public class Main {
    private static String[] USAGE = {
        "Usage: lzc [OPTION]... FILE...",
        "",
        "Options:",
        "-D<name>=<value>",
        "  Set the name/var property to value (See Compiler.getProperties).",
        "-D<name>",
        "  Short for -Dname=true.",
        "-v",
        "  Write progress information to standard output.",
        "--mcache on|off",
        "  Turns on/off media cache.  Default is off.",
        "--onerror [throw|warn]",
        "  Action to take on compilation errors.  Defaults to warn.",
        "--help",
        "  Prints this message.",
        "--keepscriptcache",
        "  Doesn't flush script cache before compiling.",
        "",
        "Output options:",
        "--runtime=[swf5|swf6|swf7]",
        "  Compile to swf5, swf6, or swf7.",
        "--dir outputdir",
        "  Output directory.",
        "-k | --krank",
        "  Add krank information to the output object.",
        "-g | --debug",
        "  Add debugging information into the output object.",
        "-p | --profile",
        "  Add profiling information into the output object.",
        "",
        "Logging options:",
        "-l<loglevel>",
        "  Logging level (See org.apache.log4j.Level)",
        "-l<loggerName>=<loglevel>",
        "  Logging level (See org.apache.log4j.Level)",
        "-lp file",
        "  Log4j properties files",
        "--log logfile",
        "  Specify logfile (output still goes to console as well)",
        "--schema",
        "  Writes the schema to standard output.",
        "--script",
        "  Writes JavaScript to standard output."
    };

    /** Compiles each file base.ext to the output file base.swf,
     * writing progress information to standard output.  This method
     * is intended for testing the compiler.
     *
     * <p>See the usage string or execute <code>lzc --help</code>
     * to see a list of options.
     *
     * @param args the command line arguments
     */
    public static void main(String args[])
        throws IOException
    {
        lzc(args, null, null, null);
    }

    /** This method implements the behavior described in main
     * but also returns an integer error code.
     */
    public static int lzc(String args[], String logFile, 
            String outFileName, String outDir)
        throws IOException
    {
        Logger logger = Logger.getRootLogger();
        List files = new Vector();
        boolean millis = false;
        

        // Preprocess args
        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("m")) {
                millis = true;
            }
        }
        // Configure logging
        logger.setLevel(Level.ERROR);
        PatternLayout layout;
        if (millis) {
            layout = new PatternLayout("%r %m%n");
        } else {
            layout = new PatternLayout("%r %m%n");
        }
        logger.removeAllAppenders();
        if (logFile == null) {
            logger.addAppender(new ConsoleAppender(layout));
        } else {
            logger.addAppender(new FileAppender(layout, logFile, false));
        }
    
        Compiler compiler = new Compiler();
        CompilerMediaCache cache;
        String cacheDir = LPS.getWorkDirectory() + File.separator + "cache" + File.separator + "cmcache";
        cache = new CompilerMediaCache(new File(cacheDir), new Properties());

        // Make sure we always retranscode media
        //cache.setProperty("forcetranscode", "true");
        compiler.setMediaCache(cache);

        String scacheDir = LPS.getWorkDirectory() + File.separator + "scache";
        ScriptCompiler.initScriptCompilerCache(new File(scacheDir), new Properties());
        boolean flushScriptCache = true;

        for (int i = 0; i < args.length; i++) {
            String arg = args[i].intern();
            if (arg.startsWith("-")) {
                if (arg == "-v") {
                    logger.setLevel(Level.ALL);
                } else if (arg == "-lp") {
                    PropertyConfigurator.configure(args[++i]);
                    String lhome = System.getProperty("LPS_HOME");
                    if (lhome == null || lhome.equals("")) {
                        lhome = System.getenv("LPS_HOME");
                    }
                    LPS.setHome(lhome);
                } else if (arg == "--schema") {
                    compiler.SchemaLogger.setLevel(Level.ALL);
                } else if (arg == "--keepscriptcache") {
                    flushScriptCache = false;
                } else if (arg == "--onerror") {
                    String value = args[++i];
                    CompilationError.ThrowCompilationErrors =
                        "throw".equals(value);
                } else if (arg.startsWith("--runtime=")) {
                    String value = arg.substring("--runtime=".length());
                    if (value.equals("swf5") || value.equals("swf6") || value.equals("swf7")) {
                      compiler.setProperty(CompilationEnvironment.SWFVERSION_PROPERTY, value);
                    } else {
                      System.err.println("Invalid value for --runtime");
                      return 1;
                    }
                } else if (arg == "--script") {
                    Logger.getLogger(org.openlaszlo.sc.ScriptCompiler.class)
                        .setLevel(Level.ALL);
                } else if (arg == "-mcache" || arg == "--mcache") {
                    String value = args[++i];
                    if (value.equals("on")) {
                        cache.getProperties().setProperty("forcetranscode", "false");
                    } else if (value.equals("off")) {
                        cache.getProperties().setProperty("forcetranscode", "true");
                    }
                } else if (arg == "-log" || arg == "--log") {
                    String log = args[++i];
                    logger.removeAllAppenders();
                    logger.addAppender(new FileAppender(layout, log, false));
                } else if (arg == "-dir" || arg == "--dir") {
                    outDir = args[++i];
                } else if (arg.startsWith("-D")) {
                    String key = arg.substring(2);
                    String value = "true";
                    int offset = key.indexOf('=');
                    if (offset >= 0) {
                        value = key.substring(offset + 1).intern();
                        key = key.substring(0, offset);
                    }
                    compiler.setProperty(key, value);
                    LPS.setProperty(key, value);
                } else if (arg.startsWith("-l")) {
                    Logger thisLogger = logger;
                    String level = arg.substring(2);
                    if (level.indexOf('=') > -1) {
                        String key = level.substring(0, level.indexOf('='));
                        level = level.substring(level.indexOf('=')+1);
                        thisLogger = Logger.getLogger(key);
                    }
                    if (level != "" && level != null) {
                        thisLogger.setLevel(Level.toLevel(level));
                    }
                } else if (arg == "-k" || arg == "--krank") {
                    compiler.setProperty(CompilationEnvironment.KRANK_PROPERTY, "true");
                } else if (arg == "-g" || arg == "--debug") {
                    compiler.setProperty(CompilationEnvironment.DEBUG_PROPERTY, "true");
                } else if (arg == "-p" || arg == "--profile") {
                    compiler.setProperty(CompilationEnvironment.PROFILE_PROPERTY, "true");
                } else if (arg == "--help") {
                    for (int j = 0; j < USAGE.length; j++) {
                        System.err.println(USAGE[j]);
                    }
                    return 0;
                } else {
                    System.err.println("Usage: lzc [OPTION]... file...");
                    System.err.println("Try `lzc --help' for more information.");
                    return 1;
                }
                continue;
            }
            String sourceName = args[i];
            files.add(sourceName);
        }

        if (flushScriptCache) {
            ScriptCompiler.clearCacheStatic();
        }

        LPS.initialize();

        for (Iterator iter = files.iterator(); iter.hasNext(); ) {
            String sourceName = (String) iter.next();
            if (files.size() > 1)
                System.err.println("Compiling " + sourceName);
            compile(compiler, logger, sourceName, outFileName, outDir);
        }
        return 0;
    }

    static private void compile(Compiler compiler, Logger logger,
                 String sourceName, String outFileName, String outDir)
    {
        File sourceFile = new File(sourceName);
        if (outFileName == null && outDir == null) {
            outFileName = FileUtils.getBase(sourceName) + ".swf";
        } else if (outFileName == null) {
            outFileName = outDir + File.separator + 
                FileUtils.getBase(sourceFile.getName()) + ".swf";
        } 
        File objectFile = new File(outFileName);
        try {
            compiler.compile(sourceFile, objectFile, new Properties());
        } catch (CompilationError e) {
            logger.error("Compilation errors occurred:");
            logger.error(e.getMessage());
        } catch (IOException e) {
            logger.error("IO exception: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
