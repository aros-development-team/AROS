/* *****************************************************************************
 * CompilationManager.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.cm;
import java.io.InputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FilterOutputStream;
import java.io.FileInputStream;
import java.io.PrintWriter;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.Serializable;
import java.util.*;
import org.openlaszlo.compiler.*;
import org.openlaszlo.server.LPS;
import org.openlaszlo.server.Configuration;
import org.openlaszlo.servlets.KrankListener;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.utils.LZHttpUtils;
import org.openlaszlo.cache.Cache;
import org.apache.log4j.*;

/** A <code>CompilationManager</code> is responsible for maintaining
 * the correspondence between source and object files.  It's
 * responsible for dependency analysis, caching, and selective recompilation.
 *
 * The main entry point to the Compilationmanager code is getObjectStream
 *
 * A CompilationManager is constructed with a source directory, where it
 * looks for source files, and a cache directory, where it places
 * compiled object files and cached (dependency) information.  It can be
 * instructed to always recompile files, never recompile them so long
 * as they exist, or use dependency information that it creates during
 * the course of a file compilation to determine whether a file is out
 * of date.  See the documentation for getProperty() for a description
 * of how to select between these behaviors.
 *
 * The compilation manager currently uses itself to represent the
 * cache, and methods that access the cache are synchronized.  This
 * would have to change to support multiple readers.
 *
 * Methods that trigger recompilation are synchronized: it's safe for
 * multiple threads to contain references to a compilation manager if
 * they are only using it to compile.  Accessor methods should only be
 * called single-threaded.  Two compilation managers shouldn't be
 * pointed at the same cache directory.
 */
public class CompilationManager extends Cache {
    /** Logger. */
    private static Logger mLogger = Logger.getLogger(CompilationManager.class);

    /** See the constructor. */
    protected File mSourceDirectory;
    /** See the constructor. */
    protected File mCacheDirectory;
    /** Cache for compiled media */
    protected CompilerMediaCache mMediaCache;
    /** See getProperties. */
    protected Properties mProperties = null;

    public static KrankListener sKrankListener = null;

    protected File mLPSJarFile = null;

    public static final String RECOMPILE = "lzrecompile";

    /**
     * Creates a new <code>CompilationManager</code> instance.
     *
     * @param sourceDirectory a <code>File</code> naming a directory,
     * that is used as a base for resolving relative names that are
     * passed to getObjectData.
     *
     * @param cacheDirectory a <code>File</code> naming a directory.
     * The <code>CompilationManager</code> places object files and
     * dependency-tracking information here, to avoid unnecessary
     * subsequent recompilation.
     *
     */
    public CompilationManager(File sourceDirectory, File cacheDirectory, Properties props) 
        throws IOException {
        super("cache", cacheDirectory, props);
        this.mSourceDirectory = sourceDirectory;
        this.mCacheDirectory = cacheDirectory;
        try {
            cacheDirectory.mkdirs();
        } catch (SecurityException se) {
        }
        if (!cacheDirectory.exists()) {
            throw new FileNotFoundException(cacheDirectory.getAbsolutePath() + " does not exist");
        }
        if (!cacheDirectory.canRead()) {
            throw new IOException("can't read " + cacheDirectory.getAbsolutePath());
        }
        String p = cacheDirectory.getAbsolutePath() + File.separator + "media";
        this.mMediaCache = new CompilerMediaCache(new File(p), props);
        this.mProperties = props;

        String jd = LPS.getProperty("compMgr.lps.jar.dependency", "true");
        if ("true".equals(jd)) {
            mLPSJarFile = LPS.getLPSJarFile();
        }
    }

    /** 
     * Returns the media cache for the compilation manager
     */
    public CompilerMediaCache getCompilerMediaCache() {
        return mMediaCache;
    }

    /** Sets the source directory.  This is the directory within which
     * source files will be searched for.
     * @param sourceDirectory a File
     */
    public void setSourceDirectory(File sourceDirectory) {
        this.mSourceDirectory = sourceDirectory;
    }

    /** Clear the cache. 
     * @return true if full removal of cache was successful, otherwise false.
     */
    public boolean clearCacheDirectory() {
        return mMediaCache.clearCache() && clearCache();
    }
    
    /** Miscellaneous properties.
     *
     * <dl>
     * <dt>recompile=always
     * <dd>Always recompile object files, regardless of whether source
     * files have changed.
     * <dt>recompile=never
     * <dd>Never recompile object files.
     * <dt>recompile=check (default)
     * <dd>Recompile an object file if a source file that it depends
     * on has changed.
     * </dl>
     *
     * Additionally, if <code>getProperties().getProperty("compiler."
     * + <var>key</var>) == <var>value</var></code>, then files are
     * compiled with a compiler such that
     * <code>compiler.getProperty(<var>key</var>) ==
     * <var>value</var></code>.
     * @return a Properties
     */
    public Properties getProperties() {
        return mProperties;
    }
    
    public void setProperty(String key, String value) {
        mProperties.setProperty(key, value);
    }

    protected void afterCacheRead(Object metaData) {
        CachedInfo ci = (CachedInfo)metaData;
        DependencyTracker dt = ci.getDependencyTracker();

        // update webapp path of cache files
        dt.updateWebappPath();

        // set application options in configuration
        Canvas canvas = ci.getCanvas();
        LPS.configuration.setApplicationOptions
            (canvas.getFilePath(), canvas.getSecurityOptions());
    }

    /**
     * Returns a File containing the compiled form of the file 
     * named by pathname, suitable play on the client.  
     *
     * @param pathname a <code>String</code> value.  If
     * <var>pathname</var> is relative, it is resolved relative to the
     * CompilationManager's <var>sourceDirectory</var>.
     * @param props params for dependency tracker and compiler
     * @return the compiled File object.
     * @exception CompilationError if an error occurs.
     */
    public synchronized File getObjectFile(String pathname, Properties props)
        throws CompilationError, IOException
    {
        mLogger.debug("getObjectFile for " + pathname.toString());
        return getItem(pathname, props).getFile();
    }

    /**
       Abort the krank thread if it is running.
     */
    public synchronized static void abortKrankProcess () {
        KrankListener kt = sKrankListener;
        mLogger.info("called abortKrankProcess()");
        // try and halt any previous listener that is sitting around hogging port 4444
        if (kt != null) {
            kt.setState(KrankListener.ABORTED);
            try {
                // [todo: hqm 10-1-2003 for some reason
                // thread.interrupt() seems to have no effect on the
                // running krank thread. Thread.stop() stops it
                // alright, but is deprecated by Sun
                kt.closeSocket();
                kt.interrupt();
            } catch (Exception e) {
                mLogger.debug("abortKrankProcess exception while killing thread: "+e.getMessage());
            }
        }

    }

    /**
     * Returns an InputStream containing the compiled form of the file 
     * named by pathname, suitable play on the client.  
     *
     * If the krank flag is set, run a krank workflow thread including
     * starting the krank listener on port 4444
     * 
     * @param pathname a <code>String</code> value.  If
     * <var>pathname</var> is relative, it is resolved relative to the
     * CompilationManager's <var>sourceDirectory</var>.
     * @param props params for dependency tracker and compiler
     * @return the compiled File object.
     * @exception CompilationError if an error occurs.
     */
    public synchronized InputStream getObjectStream(String pathname, Properties props)
        throws CompilationError, IOException
    {
        // +++ We really want the actual original source file path , from the HttpRequest
        // is that what we are getting? check the log for this message:
        boolean kranking = "true".equals(props.get("krank"));
        if (kranking) {
            startKrankWorkflow(pathname, props);
        }
        return getItem(pathname, props).getStream();
    }

    /**
     * Start a thread which does the krank workflow; listen on TCP port for xml serializtion,
     * and when that completes, compile the kranked swf file and leave it in the source directory
     * with a .lzo suffix.
     * @param pathname path to source file
     * @param props params for dependency tracker and compiler
     */
    protected void startKrankWorkflow (String pathname, Properties props) {
        File srcfile = new File(pathname);
        // Get just the prefix of the file basename:  /bar/baz/foo.lzx => foo
        String fname = new File(pathname).getName();
        String prefix;
        int idx = fname.indexOf(".");
        if (idx != -1) {
            prefix = fname.substring(0, idx);
        } else {
            prefix = fname;
        }

        // Where is the source directory?
        mLogger.debug("getObjectStream pathname= " + pathname.toString());
        mLogger.debug("krank prefix= "+prefix);

        // Java requires variables that are referenced from inside
        // a closure (the KrankListener runner thread below) to be
        // declared final. Why? 

        File xmlFile = new File(srcfile.getParent(), prefix + "obj__.xml");
        File krankedSWFfilecopy = new File(srcfile.getParent(), prefix + "__.swf");

        String myfileName = pathname;
        Properties myprops = (Properties) props.clone();
        mLogger.debug("myprops = "+myprops.toString());
        String myprefix = prefix;

        // returns path with no suffix,  like "c:/lps-krank/test/simple"
        File basepath = new File(srcfile.getParent(), myprefix);
        File targetSWF = new File(basepath.getAbsolutePath()+".lzo");
        File targetSWFgz = new File(basepath.getAbsolutePath()+".lzo.gz");

        try {
            targetSWF.delete();
        } catch (SecurityException e) {
            mLogger.error("Could not delete old .lzo file at" + targetSWF.getAbsolutePath());
            throw new CompilationError("Could not delete old .lzo file at" + targetSWF.getAbsolutePath());
        }

        try {
            krankedSWFfilecopy.delete();
        } catch (SecurityException e) {
            mLogger.error("Could not delete old swf file at" + krankedSWFfilecopy.getAbsolutePath());
            throw new CompilationError("Could not delete old swf file at" + krankedSWFfilecopy.getAbsolutePath());
        }


        try {
            targetSWFgz.delete();
        } catch (SecurityException e) {
            mLogger.error("Could not delete old gzipped .lzo file at" + targetSWFgz.getAbsolutePath());
            throw new CompilationError("Could not delete gzipped .lzo file at" + targetSWFgz.getAbsolutePath());
        }

        try {
            xmlFile.delete();
        } catch (SecurityException e) {
            mLogger.error("Could not delete old xml data file at" + xmlFile.getAbsolutePath());
            throw new CompilationError("Could not delete old xml data file at" + xmlFile.getAbsolutePath());
        }

        mLogger.debug("Krank xml data file  is " + xmlFile.getAbsolutePath());

        // kill any old listener hanging around;
        abortKrankProcess();

        // Remove gzip encoding property, so we can get our
        // hands on an unzipped swf to give to the kranker
        myprops.remove("Content-Encoding"); 
        File krankedSWF;
        try {
            krankedSWF  = getObjectFile(myfileName, myprops);
        } catch (IOException e) {
            throw new CompilationError("IOException could not locate kranked swf file for "+myfileName+" "+ myprops+": "+e.getMessage());
        }

        sKrankListener = new KrankListener(
            prefix,
            xmlFile,
            krankedSWF,
            krankedSWFfilecopy,
            basepath,
            targetSWF,
            targetSWFgz,
            myprops);

        sKrankListener.start();
    }

    /**
     * @return the canvas for this app
     */
    public synchronized Canvas getCanvas(String pathname)
        throws CompilationError, IOException {
        return getCanvas(pathname, new Properties());
    }

    /**
     * Return the canvas associated with the given LZX file
     *
     * @return the canvas
     * @param pathname path to the LZX file
     * @param props props for dependency tracker and compiler
     * @throws CompilationError if there is a compilation error
     * in the file
     */
    public synchronized Canvas getCanvas(String pathname, Properties props)
        throws CompilationError, IOException {

        mLogger.debug("getCanvas for " + pathname.toString());
        CachedInfo info = (CachedInfo)getItem(pathname, props).getMetaData();
        return info.getCanvas();
    }

    /**
     * @return a String containing XML info about this app
     */
    public synchronized String getInfoXML(String pathname, Properties props) 
        throws CompilationError, IOException {

        if (pathname == null) {
             return "";
        }

        Properties altProps = (Properties)props.clone();
        Item item = getItem(pathname, props);
        String enc = props.getProperty(LZHttpUtils.CONTENT_ENCODING);
        long size = item.getSize();
        if (enc == null) {
            enc = "";
        }

        boolean isDebug = "true".equals(props.getProperty("debug"));
        boolean debugExists = isDebug;
        boolean nondebugExists = !isDebug;
        boolean debugUptodate = false;
        boolean nondebugUptodate = false;
        Properties alt = null;

        if (isDebug) {
            alt = (Properties)props.clone();
            alt.setProperty("debug", "false");
            nondebugExists = (getItem(computeKey(pathname, alt)) != null);
        } else {
            alt = (Properties)props.clone();
            alt.setProperty("debug", "true");
            debugExists = (getItem(computeKey(pathname, alt)) != null);
        }

        if (debugExists) {
            Properties p;
            if (!isDebug) {
                item = getItem(computeKey(pathname, alt));
                p = (Properties)alt.clone();
            } else {
                p = (Properties)props.clone();
            }
            CachedInfo info = (CachedInfo)item.getMetaData();
            if (info != null) {
                DependencyTracker tracker = info.getDependencyTracker();
                debugUptodate = tracker.isUpToDate(p);
            }
        } 

        if (nondebugExists) {
            Properties p;
            if (isDebug) {
                item = getItem(computeKey(pathname, alt));
                p = (Properties)alt.clone();
            } else {
                item = getItem(computeKey(pathname, props));
                p = (Properties)props.clone();
            }
            CachedInfo info = (CachedInfo)item.getMetaData();
            if (info != null) {
                DependencyTracker tracker = info.getDependencyTracker();
                nondebugUptodate = tracker.isUpToDate(p);
            }
        }

        long gzsize = 0;
        long ungzsize = 0;

        // Only swf5 has alternate compressed file version,
        // swf6 and greater has internal file compression.
        String version = props.getProperty(CompilationEnvironment.SWFVERSION_PROPERTY);
        if ("swf5".equals(version)) {
            if (enc.equals("gzip")) {
                altProps.setProperty(LZHttpUtils.CONTENT_ENCODING, "");
                ungzsize = getItem(pathname, altProps).getSize();
                gzsize = size;
            } else {
                altProps.setProperty(LZHttpUtils.CONTENT_ENCODING, "gzip");
                gzsize = getItem(pathname, altProps).getSize();
                ungzsize = size;
            }
        } else {
            ungzsize = size;
        }

        return "<info size=\"" + ungzsize + 
               "\" gz-size=\"" + gzsize + 
               "\" debug=\"" + isDebug + 
               "\" encoding=\"" + enc + 
               "\" debug-exists=\"" + debugExists + 
               "\" debug-up-to-date=\"" + debugUptodate + 
               "\" nondebug-exists=\"" + nondebugExists + 
               "\" nondebug-up-to-date=\"" + nondebugUptodate +
               "\" runtime=\"" + version +  
               "\" />";
    }

    /**
     * Return the last modified time associated with the given LZX file
     *
     * @return the last modified time in utc
     * @param pathname path to the LZX file
     * @param props params for dependency tracker and compiler
     * @throws CompilationError if there is a compilation error
     * in the file
     */
    public synchronized long getLastModified(String pathname, Properties props)
        throws CompilationError, IOException {
        mLogger.debug("getLastModified for " + pathname.toString());
        // This is the last modified time from the item in the cache
        File file = new File(getItem(pathname, props).getPathName());
        return file.lastModified();
    }
    
    /**
     * Get the cached item.  Recompile or convert encoding as needed.
     *
     * @param pathname path to the LZX file
     * @param props params for compiler
     */
    public synchronized Item getItem(String pathname, Properties props) 
        throws IOException {

        Serializable key = computeKey(pathname, props);
        String enc = props.getProperty(LZHttpUtils.CONTENT_ENCODING);
        boolean lockit = false;
        Item item = findItem(key, enc, lockit);
        // Set up the properties, for dependency checking
        Properties compilationProperties = (Properties) props.clone();
        if (!isItemUpToDate(item, pathname, compilationProperties)) {
            
            Properties props2 = (Properties) props.clone();
            if (enc == null || enc.equals("")) {
                props2.setProperty(LZHttpUtils.CONTENT_ENCODING, "gzip");
            } else {
                props2.setProperty(LZHttpUtils.CONTENT_ENCODING, "");
            }
            Serializable key2 = computeKey(pathname, props2);
            Item item2 = getItem(key2);
            if (item2 != null && isItemUpToDate(item2, pathname, props2)) {
                convertItemEncoding(item2, item, pathname, enc, compilationProperties);
            } else {
                compileItem(item, pathname, compilationProperties);
            }
        } 
        updateCache(item);
        return item;
    }

    /**
     * Take source item, un-encode it, and then re-encode and store in dest item
     * with the specified encoding
     * @param src - source item
     * @param dest - dest item
     * @param pathname - pathname for destination item
     * @param enc - encoding for dest
     * @param props - properties for compile
     *
     * For now, hardcoded to only support gzip.
     */
    public synchronized void convertItemEncoding(Item src, Item dest, 
        String pathname, String enc, Properties props) {

        File srcFile = src.getFile();
        File destFile = new File(dest.getPathName());
        CachedInfo srcInfo = (CachedInfo)src.getMetaData();
        try {

            dest.markDirty();

            File tempFile = null;
            FileInputStream tempFileStream = null;
            try {
                tempFile = File.createTempFile("lzc-", null);
                mLogger.debug("Temporary file is " + tempFile.getAbsolutePath());

                mLogger.debug("Re-encoding from " + srcFile.toString() + " to " 
                                                  + tempFile.toString());

                if (enc != null && enc.equals("gzip")) {
                    // Encode from uncompressed to gzip
                    FileUtils.encode(srcFile, tempFile, enc);
                } else {
                    // Decode from gzip to uncompressed.
                    FileUtils.decode(srcFile, tempFile, "gzip");
                }

                tempFileStream = new FileInputStream(tempFile);

                dest.update(tempFileStream);

                DependencyTracker dependencyTracker = new DependencyTracker(props);
                dependencyTracker.copyFiles(srcInfo.getDependencyTracker(), srcFile);
                dependencyTracker.addFile(destFile);
    
                // FIXME: [2003-07-21 bloch] the factoring of cm.CacheInfo and cache.CacheInfo
                // has never been right.  Someday, if there are more subclasses of Cache that
                // use the [gs]etMetaData methods, then this should really be fixed.  (bug #1778).
                CachedInfo info = new CachedInfo(dependencyTracker, srcInfo.getCanvas(), enc);
                dest.getInfo().setLastModified(destFile.lastModified());
                dest.update(info);
                dest.markClean();

            } finally {
                if (tempFileStream != null) {
                    FileUtils.close(tempFileStream);
                }
                if (tempFile != null) {
                    tempFile.delete();
                }

                // Cleanup now
                mLogger.debug("starting gc");
                System.gc();
                mLogger.debug("finished gc");
            }


        } catch (IOException ioe) {
            CompilationError e = new CompilationError(ioe);
            e.initPathname(pathname);
            throw e;
        } 
    }

    /**
     * @return true if the item is up to date
     *
     * @param pathname a <code>String</code> value.  If
     * <var>pathname</var> is relative, it is resolved relative to the
     * CompilationManager's <var>sourceDirectory</var>.
     * @param props properties that affect the compile.
     * @return the bytes of the object file
     * @exception CompilationError if an error occurs
     *
     * The PROPS parameter may contain 
     * <ul>
     * <li>"Content-Encoding" =&gt; the encoding of the output (ignore if null)
     * </ul>
     * NOTE: can remove the recompile property from props 
     */
    public boolean isItemUpToDate(Item item, String pathname, Properties props) 
        throws CompilationError {

        File sourceFile = new File(mSourceDirectory, pathname);
        File objectFile = new File(item.getPathName());

        boolean recompile = props.getProperty(RECOMPILE) != null;
        props.remove(RECOMPILE);

        boolean upToDate = false;

        try {
            String recompileSwitch =
                mProperties.getProperty("recompile", "check").intern();

            if (pathname.endsWith(".lzo")) {
                return true; // we cannot recompile a kranked file automatically
            } else if (recompile) {
                mLogger.info("forcing a recompile");
                return false;
            } else if (recompileSwitch == "always") {
                return false;
            } else if (recompileSwitch == "never") {
                return objectFile.exists();
            } else if (recompileSwitch == "check") {
                CachedInfo info = (CachedInfo)item.getMetaData();
                DependencyTracker tracker = null;
                if (info != null) {
                    tracker = info.getDependencyTracker();
                }
                // Set up the properties, for dependency checking
                props = (Properties) props.clone();
                return (tracker != null) && tracker.isUpToDate(props);
            } else {
                throw new IllegalArgumentException(
                    "invalid value for 'recompile' property");
            }
        } catch (Exception ex) {
            CompilationError e = new CompilationError(ex);
            e.initPathname(sourceFile.getPath());
            throw e;
        }
    }

    /**
     * Compiles the file named by pathname and leaves the result 
     * in the cached item.  If the file can't be compiled and
     * debugCompilationErrors is true, returns an HTML document
     * describing the error; otherwise, passes the exception.
     *
     * @param pathname a <code>String</code> value.  If
     * <var>pathname</var> is relative, it is resolved relative to the
     * CompilationManager's <var>sourceDirectory</var>.
     * @param props properties that affect the compile.
     * @return the bytes of the object file
     * @exception CompilationError if an error occurs
     *
     * The PROPS parameter may contain 
     * <ul>
     * <li>"Content-Encoding" =&gt; the encoding of the output (ignore if null)
     * </ul>
     */
    public synchronized void compileItem(Item item, String pathname, 
            Properties compilationProperties)
        throws CompilationError
    {
        File sourceFile = new File(mSourceDirectory, pathname);
        File objectFile = new File(item.getPathName());

        try {

            item.markDirty();

            org.openlaszlo.compiler.Compiler compiler =
                new org.openlaszlo.compiler.Compiler();
            // For each property compiler.name=value, set a
            // compiler property name=value (that is, without
            // the "compiler." prefix)
            for (java.util.Enumeration e = getProperties().propertyNames();
                 e.hasMoreElements(); ) {
                String key = (String) e.nextElement();
                if (key.startsWith("compiler.")) {
                    compiler.setProperty(
                        key.substring("compiler.".length()),
                        getProperties().getProperty(key));
                }
            }
            DependencyTracker dependencyTracker =
                new DependencyTracker(compilationProperties);
            TrackingFileResolver resolver =
                new TrackingFileResolver(compiler.getFileResolver(),
                                         dependencyTracker);
            dependencyTracker.addFile(sourceFile);
            if (mLPSJarFile != null) {
                dependencyTracker.addFile(mLPSJarFile);
            }
            compiler.setFileResolver(resolver);
            compiler.setMediaCache(mMediaCache);

            Canvas canvas = null;

            File tempFile = null;
            FileInputStream tempFileStream = null;
            try {
                tempFile = File.createTempFile("lzc-", null);
                mLogger.debug("Temporary file is " + tempFile.getAbsolutePath());

                mLogger.debug("Compiling " + sourceFile.toString() + " to " 
                                           + tempFile.toString());
                canvas = compiler.compile(sourceFile, tempFile, 
                                          compilationProperties);

                mLogger.debug("Canvas size is " + canvas.getWidth() + " by " 
                                                + canvas.getHeight());

                tempFileStream = new FileInputStream(tempFile);

                item.update(tempFileStream);
                dependencyTracker.addFile(objectFile);
                String encoding = compilationProperties.getProperty(LZHttpUtils.CONTENT_ENCODING);
                // FIXME: [2003-07-21 bloch] the factoring of cm.CacheInfo and cache.CacheInfo
                // has never been right.  Someday, if there are more subclasses of Cache that
                // use the [gs]etMetaData methods, then this should really be fixed.  (bug #1778).
                CachedInfo info = new CachedInfo(dependencyTracker, canvas, encoding);
                item.getInfo().setLastModified(objectFile.lastModified());
                item.update(info);
                item.markClean();

                // Add security options for this application after compilation
                LPS.configuration.setApplicationOptions(FileUtils.relativePath(pathname, LPS.HOME()),
                                                        canvas.getSecurityOptions());

            } finally {
                if (tempFileStream != null) {
                    FileUtils.close(tempFileStream);
                }
                if (tempFile != null) {
                    tempFile.delete();
                }

                // Cleanup now
                mLogger.debug("starting gc");
                System.gc();
                mLogger.debug("finished gc");
            }
        } catch (IOException ioe) {
            CompilationError e = new CompilationError(ioe);
            e.initPathname(sourceFile.getPath());
            throw e;
        }
    }

    /**
     * @return a cache key for the given compile
     */
    static Serializable computeKey(String pathname, Properties props) {

        TreeSet  sorted = new TreeSet ();
        for (java.util.Enumeration e = props.propertyNames();
             e.hasMoreElements(); ) {
            String key = (String) e.nextElement();
            // Omit the recompile property
            if (key.equalsIgnoreCase(RECOMPILE))
                continue;

            String value = props.getProperty(key);
            StringBuffer buf = new StringBuffer();
            buf.append(key);
            buf.append(' ');
            buf.append(value);

            sorted.add(buf.toString());
        }

        StringBuffer buf = new StringBuffer(FileUtils.relativePath(pathname, LPS.HOME()));

        buf.append(' ');
        for (java.util.Iterator e = sorted.iterator(); e.hasNext(); ) {
            String str = (String) e.next();
            buf.append(str);
        }

        mLogger.debug("computeKey: " + buf.toString());
        return buf.toString();
    }

    /** Given the pathname of a file, return a file name
     * for an "source" version of this file that lives in the cache.
     * @param srcName path to file in source directory
     * @param webappPath real path for web application
     * @return a File
     */
    public File getCacheSourcePath (String srcName, String webappPath) {
        // todo: relative to src directory
        File src = new File(srcName);
        if (src.getParentFile() != null) {
            String baseParent = src.getParentFile().getAbsolutePath();
            if (baseParent != null &&
                baseParent.startsWith(mCacheDirectory.getAbsolutePath())) {
                return new File(srcName);
            }
        }
        // FIXME: [2003-01-21 pkang] webapp real path should match first part of
        // source name. CompilationManager and LZServlet will need overhaul to
        // support reading directly from war file.
        if (webappPath == null || ! srcName.startsWith(webappPath))
            throw new RuntimeException(srcName + " doesn't begin with " + 
                    webappPath);
        String fixedName = srcName.substring(webappPath.length());
        return new File(mCacheDirectory, fixedName);
    }
}


/** A FileResolver that tracks dependencies.
 *
 * @author Oliver Steele
 */
class TrackingFileResolver implements FileResolver {
    private final FileResolver mBaseResolver;
    private final DependencyTracker mDependencies;

    TrackingFileResolver(FileResolver baseResolver,
                         DependencyTracker tracker) {
        this.mBaseResolver = baseResolver;
        this.mDependencies = tracker;
    }
    
    /**
     * Implement the FileResolver interface.
     *
     * @param pathname a <code>String</code> value
     * @param base a <code>String</code> value
     * @return a <code>File</code> value
     * @exception FileNotFoundException if an error occurs
     */
    public File resolve(String pathname, String base)
        throws FileNotFoundException
    {
        File file = mBaseResolver.resolve(pathname, base);
        mDependencies.addFile(file);
        return file;
    }

    /** For debugging. */
    /*void writeResults() {
        System.out.println("depends on");
        for (java.util.Iterator e = mDependencies.iterator();
             e.hasNext(); ) {
            FileInfo fi = (FileInfo) e.next();
            System.out.println(fi.mPathname + " -> " + fi.mChecksum);
        }
        }*/
}
