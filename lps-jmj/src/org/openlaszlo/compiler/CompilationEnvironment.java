/* ****************************************************************************
 * CompilationEnvironment.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;

import java.io.*;
import java.util.*;
import org.jdom.Element;
import org.apache.log4j.*;
import org.openlaszlo.server.LPS;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.utils.ComparisonMap;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.xml.internal.XMLUtils;

/** Encapsulates all the context that script compilation needs to
 * refer to.  Instances of this class are threaded through the calls
 * to instances of CompilerNode.
 *
 * Also contains utility functions for compiling to a file.
 */
public class CompilationEnvironment {
    private final Properties mProperties;
    public static final String SWFVERSION_PROPERTY = "runtime";
    static final String PROXIED_PROPERTY           = "lzproxied";
    static final String DEBUG_PROPERTY             = "debug";
    static final String PROFILE_PROPERTY           = "profile";
    static final String KRANK_PROPERTY             = "krank";
    static final String VALIDATE_PROPERTY          = "validate";
    static final String TEMPLATE_PROPERTY          = "template";
    // Log all debug.write messages back to the server
    static final String LOGDEBUG_PROPERTY           = "logdebug";
    public static final String REMOTEDEBUG_PROPERTY = "remotedebug";
    static final String EMBEDFONTS_PROPERTY         = "embedfonts";

    /** The root file being compiled.  This is used to resolve
     * relative pathnames. */
    protected File mApplicationFile = null;

    // Use a local symbol generator so that we recycle method
    // names for each new view, to keep the constant pool
    // small.
    final SymbolGenerator methodNameGenerator = new SymbolGenerator("$m");

    /** Output is written here.
     *
     * When we support multiple targets, this can be generalized to an
     * interface that SWFWriter implements.*/
    private SWFWriter mSWFWriter;
    /** Main program output when generating a loadable
     * library. Otherwise same as mSWFWriter */
    private SWFWriter mMainSWFWriter;

    private final FileResolver mFileResolver;
    boolean mRecursiveCompilation = true;

    private final CompilationErrorHandler mCompilerErrors =
        new CompilationErrorHandler();

    /** A ViewSchema object, to allow adding of user defined classes
     * during compilation.
     */
    private final ViewSchema mSchema;

    /**
     * CompilerMediaCache
     */
    private CompilerMediaCache mMediaCache = null;

    /** A cache of a compiled validator */
    private org.iso_relax.verifier.Verifier cachedVerifier = null;

    /** {canonical filenames} for libraries that have been imported;
     * used to prevent recursive processing and including the same
     * library more than once. */
    private final Set mImportedLibraryFiles = new HashSet();

    private final Set mVisitedLibraryFiles = new HashSet();

    /** Keep track of all named resources, so we can check if a view references a defined resource. */
    private final Set mResourceNames = new HashSet();

    private final Parser mParser;
    private Canvas mCanvas = null;

    /** Keep a list of assigned global id's, so we can warn when one
     * is redefined */
    // TODO: [07-18-03 hqm] We will compare all id's case
    // insensitively (using ComparisonMap), because actionscript is
    // not case sensitive.  But in the future, we should preserve
    // case.
    private final Map idTable = new ComparisonMap();

    /** Holds a set of unresolved references to resources, so we can
        check for undefined (possibly forward) references after all
        app sources have been parsed.
        Map contains resource-id => Element
    */
    private Map resourceReferences = new HashMap();

    /** Cache of the FontInfo information for each class. Computed by
        ViewCompiler walking a class' superclass chain from the base
        class.
    */
    private HashMap classFontInfoTable = new HashMap();
    private FontInfo mDefaultFontInfo;

    /** Default text view width */
    private static boolean mDefaultTextWidthInitialized = false;
    private static int mDefaultTextWidth = 100;

    /** Default SWF version to compile to */
    private String mDefaultSWFVersion = LPS.getProperty("compiler.runtime.default", "swf6");

    /** Constructs an instance.
     * @param properties compilation properties
     * @param writer output is sent here
     */
    CompilationEnvironment(Properties properties, FileResolver resolver, CompilerMediaCache mcache) {
        this.mSchema = new ViewSchema();
        // lzc depends on the properties being shared, because it sets
        // them after creating the environment
        this.mProperties = properties;
        this.mFileResolver = resolver;
        this.mParser = new Parser();
        this.mParser.setResolver(resolver);
        this.mMediaCache = mcache;
        // Default property values
        this.setProperty(VALIDATE_PROPERTY, true);
    }

    /** Copy fields from an existing CompilationEnvironment.
     */
    CompilationEnvironment(CompilationEnvironment srcEnv) {
        this.mProperties = new Properties(srcEnv.getProperties());
        this.mFileResolver = srcEnv.getFileResolver();
        this.mParser = new Parser();
        this.mParser.setResolver(this.mFileResolver);
        // Default property values
        this.setProperty(VALIDATE_PROPERTY, true);
        this.mSchema = srcEnv.getSchema();
        this.mCanvas = srcEnv.getCanvas();
    }
    
    /** Use this constructor for unit testing.  The Compiler uses the
     * constructor that takes a FileResolver. */
    public CompilationEnvironment() {
        this(new Properties(), FileResolver.DEFAULT_FILE_RESOLVER, null);
    }
    
    void setApplicationFile(File file) {
        mApplicationFile = file;
        mCompilerErrors.setFileBase(file.getParent());
        if (file.getParent() == null) {
            mParser.basePathnames.add(0, "");
        } else {
            mParser.basePathnames.add(0, file.getParent());
        }
        // TODO: [12-26-2002 ows] Consolidate this list with the one
        // in FileResolver.
        mParser.basePathnames.add(LPS.getComponentsDirectory());
        mParser.basePathnames.add(LPS.getFontDirectory());
        mParser.basePathnames.add(LPS.getLFCDirectory());
    }

    // For an app named /path/to/myapp.lzx, returns /path/to/build/myapp
    public String getLibPrefix() {
        File appfile = getApplicationFile();
        String appname = appfile.getName();

        String basename = FileUtils.getBase(appname);

        String parent = appfile.getParent();
        if (parent == null) {
            parent = ".";
        }

        String path = parent + File.separator + "build" + File.separator + basename;
        return path;
    }

    // For an app named /path/to/myapp.lzx, returns build/myapp
    public String getLibPrefixRelative() {
        File appfile = getApplicationFile();
        String appname = appfile.getName();

        String basename = FileUtils.getBase(appname);

        String path = "build" + File.separator + basename;
        return path;
    }

    public File getApplicationFile() {
        return mApplicationFile;
    }

    public void setMediaCache(CompilerMediaCache cache) {
        this.mMediaCache = cache;
    }
    public CompilerMediaCache getMediaCache() {
        return this.mMediaCache;
    }

    public void setDefaultFontInfo(FontInfo fi) {
        mDefaultFontInfo = fi;
    }

    public FontInfo getDefaultFontInfo() {
        return mDefaultFontInfo;
    }

    /** Add canvas info.  It is an error to call this before calling
     * setCanvas (hand will currently result in a null reference
     * exception). */
    public void addClassFontInfo(String classname, FontInfo info) {
        classFontInfoTable.put(classname, info);
    }

    public FontInfo getClassFontInfo(String classname) {
        FontInfo cached = (FontInfo) classFontInfoTable.get(classname);
        return cached;
    }

    public boolean getEmbedFonts() {
        return this.getBooleanProperty(EMBEDFONTS_PROPERTY);
    }

    public void setEmbedFonts(boolean embed) {
        this.setProperty(EMBEDFONTS_PROPERTY, embed);
    }

    public void setSWFWriter(SWFWriter writer) {
        assert mSWFWriter == null;
        this.mSWFWriter = writer;
        this.mMainSWFWriter = writer;
    }
    
    public void setMainSWFWriter(SWFWriter writer) {
        assert mMainSWFWriter == null || mMainSWFWriter == mSWFWriter;
        this.mMainSWFWriter = writer;
    }
    
    public ViewSchema getSchema() {
        return mSchema;
    }

    public static synchronized int getDefaultTextWidth() {
        if (!mDefaultTextWidthInitialized) {
            mDefaultTextWidthInitialized = true;
            String dws = LPS.getProperty("compiler.defaultTextWidth", "100");
            try {
                int dw = Integer.parseInt(dws);
                mDefaultTextWidth = dw;
            } catch (NumberFormatException e) {
                Logger.getLogger(CompilationEnvironment.class)
                    .error("could not parse property value for compiler.defaultTextWidth: " + dws);
            }
        }
        return mDefaultTextWidth;
    }

    public CompilationErrorHandler getErrorHandler() {
        return mCompilerErrors;
    }

    public void warn(CompilationError e) {
        mCompilerErrors.addError(e);
    }

    public void warn(Throwable e, Element element) {
        mCompilerErrors.addError(new CompilationError(element, e));
    }

    public void warn(String msg) {
        warn(new CompilationError(msg));
    }

    public void warn(String msg, Element element) {
        warn(new CompilationError(msg, element));
    }

    public Canvas getCanvas() {
        return mCanvas;
    }

    public void setCanvas(Canvas canvas, String constructorScript) {
        if (mCanvas != null)
            throw new RuntimeException("canvas set twice");
        mCanvas = canvas;
        try {
            getGenerator().setCanvas(canvas, constructorScript);
            if (getProperty(TEMPLATE_PROPERTY) != null)
                getGenerator().addTextAsset(getProperty(TEMPLATE_PROPERTY), null);
        } catch (org.openlaszlo.sc.CompilerException e) {
            throw new CompilationError(e);
        }
        mRecursiveCompilation = getGenerator().mFontTableRemainder != 123;
    }

    public void addId(String name, Element e) {
        idTable.put(name, e);
    }

    public Element getId(String name) {
        return (Element) idTable.get(name);
    }

    public void addResourceReference(String name, Element elt) {
        resourceReferences.put(name, elt);
    }

    public Map resourceReferences() {
        return resourceReferences;
    }


    /** Returns the SWF writer that compilation within this
     * environment writes to.
     * @return the object writer
     */
    SWFWriter getGenerator() {
        return mSWFWriter;
    }

    /** Returns the SWF writer for the main program compilation when 
     * compiling a loadable library.
     * @return the object writer
     */
    SWFWriter getResourceGenerator() {
        return mMainSWFWriter;
    }

    /** Returns the file resolver used in this environment.
     * @return the object writer
     */
    FileResolver getFileResolver() {
        return mFileResolver;
    }

    Set getImportedLibraryFiles() {
        return mImportedLibraryFiles;
    }

    Set getResourceNames() {
        return mResourceNames;
    }

    Parser getParser() {
        return mParser;
    }

    /** Returns the Properties object used in this environment.
     * @return the properties
     */
    Properties getProperties() {
        return mProperties;
    }

    String getProperty(String name) {
        return mProperties.getProperty(name);
    }

    String getProperty(String name, String defval) {
        return mProperties.getProperty(name, defval);
    }

    void setProperty(String name, String value) {
        mProperties.setProperty(name, value);
    }

    /** Return target Flash version (5, 6, ...) **/
    public String getSWFVersion() {
        return getProperty(SWFVERSION_PROPERTY, mDefaultSWFVersion);
    }

    public String getSWFVersion(String defaultVersion) {
        return getProperty(SWFVERSION_PROPERTY, defaultVersion);
    }

    public int getSWFVersionInt() {
        if ("swf6".equals(getSWFVersion())) {
            return 6;
        } else if ("swf7".equals(getSWFVersion())) {
            return 7;
        } else {
            return 5;
        }
    }

    boolean getBooleanProperty(String name) {
        return "true".equals(mProperties.getProperty(name));
    }

    void setProperty(String name, boolean value) {
        setProperty(name, value ? "true" : "false");
    }

    /** Compiles <var>script</var> to bytecodes, and adds them to the
     * output file.
     * @param script a script
     */
    void compileScript(String script) {
        try {
            int size = getGenerator().addScript(script);
            Element info = new Element("block");
            info.setAttribute("size", "" + size);
            mCanvas.addInfo(info);
        } catch (org.openlaszlo.sc.CompilerException e) {
            throw new CompilationError(e);
        }
    }

    void compileScript(String script, Element elt) {
        try {
            int size = getGenerator().addScript(script);
            Element info = new Element("block");
            info.setAttribute("pathname", Parser.getSourceMessagePathname(elt) );
            info.setAttribute("lineno", ""+Parser.getSourceLocation(elt, Parser.LINENO));
            info.setAttribute("tagname", elt.getName());
            if (elt.getAttribute("id") != null)
                info.setAttribute("id", elt.getAttributeValue("id"));
            if (elt.getAttribute("name") != null)
                info.setAttribute("name", elt.getAttributeValue("name"));
            info.setAttribute("size", "" + size);
            mCanvas.addInfo(info);
        } catch (org.openlaszlo.sc.CompilerException e) {
            throw new CompilationError(elt, e);
        }
    }

    /** Holds the cached schema verifier */
    org.iso_relax.verifier.Verifier getCachedVerifier () {
        return cachedVerifier;
    }

    /** Use this to cache the schema verifier (call with null to flush the cache) */
    void setCachedVerifier (org.iso_relax.verifier.Verifier verifier) {
        cachedVerifier = verifier;
    }

    /**
     * @return a unique name in the SWF
     */
    String uniqueName() {
        return mSWFWriter.createName();
    }

    File resolve(String name, String base)
        throws FileNotFoundException
    {
        return mFileResolver.resolve(name, base);
    }

    /** Resolve the value of the named attribute, relative to the
     * source location of the element.
     */
    File resolveReference(Element element, String aname)
        throws CompilationError
    {
        String base = new File(Parser.getSourcePathname(element)).getParent();
        String href =  XMLUtils.requireAttributeValue(element, aname);

        try {
            return mFileResolver.resolve(href, base);
        } catch (FileNotFoundException e) {
            throw new CompilationError(element, e);
        }
    }

    /** Resolve the value of the parent node
     */
    File resolveParentReference(Element element)
        throws CompilationError
    {
        return new File(Parser.getSourcePathname(element.getParent()));
    }

    /** Resolve the value of the "src" attribute, relative to the
     * source location of the element.
     */
    File resolveReference(Element elt)
        throws CompilationError
    {
        return resolveReference(elt, "src");
    }

    /** If the argument is a relative URL with no host, return an URL
     * that resolves to the same address relative to the destDir as
     * the argument does relative to sourceDir.  Otherwise return the
     * argument unchanged. */
    static String adjustRelativeURL(String string, File sourceDir,
                                    File destDir)
    {
        try {
            java.net.URL url = new java.net.URL(string);
            if (!url.getHost().equals("")) {
                // It's on a different host.  Don't resolve it.
                return string;
            }
            if (url.getPath().startsWith("/")) {
                // It's an absolute path.  Don't resolve it.
                return string;
            }
            String path;
            try {
                path = FileUtils.adjustRelativePath(
                    url.getPath(),
                    FileUtils.toURLPath(sourceDir),
                    FileUtils.toURLPath(destDir));
            } catch (FileUtils.RelativizationError e) {
                throw new CompilationError(e);
            }
            if (url.getQuery() != null) {
                path += "?" + url.getQuery();
            }
            return new java.net.URL(
                url.getProtocol(), url.getHost(), url.getPort(), path)
                .toExternalForm();
        } catch (java.net.MalformedURLException e) {
            return string;
        }
    }

    /** If the argument is a relative URL with no host, return an URL
     * that resolves to the same address relative to the main source
     * file as the argument does relative to the file that contains
     * elt.  Otherwise return the argument unchanged. */
    String adjustRelativeURL(String string, Element elt) {
        File appdir = mApplicationFile.getParentFile();
        File localdir = new File(Parser.getSourcePathname(elt)).getParentFile();
        if (appdir == null) {
            appdir = new File(".");
        }
        if (localdir == null) {
            localdir = new File(".");
        }
        return adjustRelativeURL(string, appdir, localdir);
    }
}
