/*****************************************************************************
 * SWFWriter.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;

import org.openlaszlo.sc.ScriptCompiler;
import org.openlaszlo.server.LPS;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.utils.ListFormat;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.button.*;
import org.openlaszlo.iv.flash.api.image.*;
import org.openlaszlo.iv.flash.api.sound.*;
import org.openlaszlo.iv.flash.api.shape.*;
import org.openlaszlo.iv.flash.api.text.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.cache.*;

import org.openlaszlo.compiler.CompilationEnvironment;

import org.openlaszlo.media.*;

import java.io.*;
import java.util.*;
import java.lang.Math;
import java.lang.Character;

import org.jdom.Element;

// jgen 1.4
import java.awt.geom.Rectangle2D;

import org.apache.log4j.*;

/** Accumulates code, XML, and assets to a SWF object file.
 *
 * Once there's a second target, the methods here can be lifted into
 * an API that SWFWriter implements.  Rationale for not doing this
 * now: It's easier to generalize from more than one instance, and
 * having to change signatures twice for every refactoring is an
 * unnecessary burden during early development.
 *
 * Make heavy use of JGenerator API.
 *
 * Properties documented in Compiler.getProperties.
 */
class SWFWriter {
    /** Stream to write to. */
    private OutputStream mStream = null;
    /** Movie being constructed. */
    private SWFFile mFlashFile;
    /** Fonts being collected. */
    private FontsCollector mFontsCollector = new FontsCollector();
    private FontsCollector mPreloaderFontsCollector = new FontsCollector();
    private FontManager mFontManager = new FontManager();
    /** True iff close() has been called. */
    private boolean mCloseCalled = false;

    private final CompilationEnvironment mEnv;

    /** Set of resoures we're importing into the output SWF */
    private final HashSet mMultiFrameResourceSet = new HashSet();

    /** Constant */
    private final int TWIP = 20;
    
    /** Total number of frames in the movie **/
    private int mLastFrame = 0;
    
    /** Unique name supply for clip/js names */
    protected SymbolGenerator mNameSupply = new SymbolGenerator("$LZ");

    /** Properties */
    private Properties mProperties;

    /** Input text fontinfo map */
    private final TreeMap mInputTextSet = new TreeMap();

    /** Index of font table in the first frame */
    private int mFontTableIndex = -1;
    int mFontTableRemainder = 0;

    /** Index of resource table in the first frame */
    private int mResourceTableIndex = -1;

    /** Default font */
    private Font mDefaultFont = null;
    private String mDefaultFontName = null;
    private String mDefaultFontFileName = null;
    private String mDefaultBoldFontFileName = null;
    private String mDefaultItalicFontFileName = null;
    private String mDefaultBoldItalicFontFileName = null;
    // TODO: [2003-12-08 bloch] need logic to set this to true
    private boolean mDefaultFontUsedForMeasurement = false;

    /** Flash format version */
    private int mFlashVersion = 5;

    /** frame rate of movie */
    private int mFrameRate = 30; 

    /** Leading for text and input text */
    private int mTextLeading = 2; 

    /** media cache for transcoding */
    private CompilerMediaCache mCache = null;

    /** <String,Resource> maps resource files to the Resources 
     * definition in the swf file. */
    protected Map mResourceMap = new HashMap();
    protected Map mClickResourceMap = new HashMap();
    protected Map mMultiFrameResourceMap = new HashMap();

    private Map mDeviceFontTable = new HashMap();

    /** Logger */
    private static Logger mLogger = org.apache.log4j.Logger.getLogger(SWFWriter.class);

    /** Height for generated advance (width) table */
    public static final int DEFAULT_SIZE = 8;

    /**
     * Initialize jgenerator
     */
    static {
        org.openlaszlo.iv.flash.util.Util.init(LPS.getConfigDirectory());
    }

    /**
     * Compiled ExpiryProgram
     */
    private static Program mExpiryProgram = null;

    /** Canvas Height */
    private int mHeight = 0;

    /** Canvas Width */
    private int mWidth = 0;

    /** Width of the "root" text output object. Defaults to canvas width. */
    private int mMaxTextWidth = 0;


    /** Height of the "root" text output object. Defaults to canvas height. */
    private int mMaxTextHeight = 0;

    /** Has the preloader been added? */
    private boolean mPreloaderAdded = false;

    /** InfoXML */
    private Element mInfo = new Element("swf");

    /** Logger for jgenerator */
    /**
     * Initializes a SWFWriter with an OutputStream to which a new SWF
     * will be written when <code>SWFWriter.close()</code> is called.
     * 
     * @param stream A <code>java.io.OutputStream</code> that the
     * movie will be written to.
     * @param props list of properties
     * @param cache media cache
     * @param importLibrary If true, the compiler will add in the LaszloLibrary.
     */
    SWFWriter(Properties props, OutputStream stream,
              CompilerMediaCache cache,
              boolean importLibrary,
              CompilationEnvironment env) {
        this.mProperties   = props;
        this.mCache        = cache;
        this.mStream       = stream;
        this.mEnv          = env;

        String s;

        s = mProperties.getProperty("swf.frame.rate", "30");
        mFrameRate = Integer.parseInt(s);

        s = mProperties.getProperty("swf.text.leading", "2");
        mTextLeading = Integer.parseInt(s);

        mFlashVersion = env.getSWFVersionInt();

        try {
            if (!importLibrary) {
                mFlashFile = new SWFFile(props);
                mFlashFile.setVersion(mFlashVersion);
                mFlashFile.setMainScript(new Script(1));
            }

        } catch (CompilationError e) {
            throw new ChainedException(e);
        }
    }

    public void importBaseLibrary(String library, CompilationEnvironment env) {

        try {
            File f = env.resolve(library, "");
            mFlashFile = new SWFFile(f.getAbsolutePath(), mProperties);
            mFlashFile.setVersion(mFlashVersion);
            // Set the frame rate (shifted)
            mFlashFile.setFrameRate(mFrameRate << 8); 
            
            //            mFlashFile.printContent(System.out);
            
            if (LPS.isInternalBuild()) {
                long lfcModTime = f.lastModified();
                List newerFiles = new Vector(); // List<File>
                List sourceDirs = new Vector(); // List<File>
                sourceDirs.add(f.getParentFile());
                while (!sourceDirs.isEmpty()) {
                    File ff = (File) sourceDirs.get(0);
                    sourceDirs.remove(0);
                    if (ff.isDirectory()) {
                        sourceDirs.addAll(Arrays.asList(ff.listFiles()));
                    } else if (ff.isFile() && ff.getName().endsWith(".as")) {
                        long modTime = ff.lastModified();
                        if (modTime > lfcModTime)
                            newerFiles.add(ff.getName());
                    }
                }
                if (!newerFiles.isEmpty()) {
                    env.warn(f.getName() + " is older than " +
                             new ListFormat().format(newerFiles));
                }
            }
        } catch (FileNotFoundException e) {
            throw new ChainedException(e);
        }
    }
    
    FontManager getFontManager() {
        return mFontManager;
    }

    public boolean isDeviceFont(String face) {
        return (mDeviceFontTable.get(face) != null);
    }

    public void setDeviceFont(String face) {
        mDeviceFontTable.put(face,"true");
    }

    public void setFontManager(FontManager fm) {
        this.mFontManager = fm;
    }

    void addPreloaderScript(String script) {
        if (mPreloaderAdded != true)
            throw new RuntimeException("Preloader not added yet.");

        addScript(script, 0);
    }
        
    void addPreloader(String hideafterinit, CompilationEnvironment env) {
        if (mPreloaderAdded == true) 
            throw new RuntimeException("Preloader already added.");

        // TODO: [2004-03-04 bloch] maybe someday we have different versions of
        // preloader.lzl (e.g. debug, profile).
        File f;
        try {
            f = env.resolve("lzpreloader.lzl", "");
        } catch (FileNotFoundException e) {
            throw new ChainedException(e);
        }
        mFlashFile.addPreloaderFrame(f.getAbsolutePath());
        mLastFrame = 1;

        mPreloaderAdded = true;

        // Add canvas size information for preloader
        String sizescript = "_root.lzpreloader.wd = " + mWidth + "; " + 
                            "_root.lzpreloader.ht = " + mHeight + " ;" + 
                            "_root.lzpreloader.hideafterinit = " + hideafterinit + ";";
        addPreloaderScript(sizescript);
    }

    /**
     * Sets the canvas for the movie
     *
     * @param canvas
     * 
     */
    void setCanvas(Canvas canvas, String canvasConstructor) {
        Rectangle2D r = new Rectangle2D.Double(
            0, 0,
            (canvas.getWidth()*TWIP),
            (canvas.getHeight()*TWIP));

        mFlashFile.setFrameSize(r);

        int bgc = canvas.getBGColor();
        int red   = (bgc >> 16) & 0xff;
        int green = (bgc >> 8)  & 0xff;
        int blue  = (bgc)       & 0xff; 
        Color c = new Color(red, green, blue);
        SetBackgroundColor setbgc = new SetBackgroundColor(c);
        mFlashFile.getMainScript().setBackgroundColor(setbgc);
        
        // NOTE: disable constant pool when compiling canvas constructor
        // so that build id etc... are easy for qa to pick out.
        Properties props = new Properties(mProperties);
        props.setProperty("disableConstantPool", "1");
        byte[] action = ScriptCompiler.compileToByteArray(canvasConstructor, props);
        Program program = new Program(action, 0, action.length);
        mLogger.debug("    Adding a program of " + action.length + " bytes.");
        addProgram(program);

        // Set width and height properties for preloader...
        mWidth = canvas.getWidth();
        mHeight = canvas.getHeight();

        mMaxTextWidth = canvas.getMaxTextWidth();
        // If zero, default to canvas width 
        if (mMaxTextWidth == 0) {
            mMaxTextWidth = mWidth;
        }

        mMaxTextHeight = canvas.getMaxTextHeight();
        // If zero, default to canvas height 
        if (mMaxTextHeight == 0) {
            mMaxTextHeight = mHeight;
        }

        // Get default font info 
        FontInfo fontInfo = canvas.getFontInfo();

        mDefaultFontName = canvas.defaultFont;
        mDefaultFontFileName = canvas.defaultFontFilename;
        mDefaultBoldFontFileName = canvas.defaultBoldFontFilename;
        mDefaultItalicFontFileName = canvas.defaultItalicFontFilename;
        mDefaultBoldItalicFontFileName = canvas.defaultBoldItalicFontFilename;

        Frame frame = mFlashFile.getMainScript().getFrameAt(mLastFrame);

        // Make a bogus object for us to replace with the font and resource table
        // actionscript at the end of compilation.
        mFontTableIndex = frame.size();
        frame.addFlashObject(new SetBackgroundColor(new Color(0, 0, 0)));
        mResourceTableIndex = frame.size();
        frame.addFlashObject(new SetBackgroundColor(new Color(0, 0, 0)));

        mEnv.getCanvas().addInfo(mInfo); 
    }


    /** Get default fonts and stuff from canvas; used for snippet compilation */
    void setCanvasDefaults(Canvas canvas, CompilerMediaCache mc) {
        Rectangle2D r = new Rectangle2D.Double(
            0, 0,
            (canvas.getWidth()*TWIP),
            (canvas.getHeight()*TWIP));

        mFlashFile.setFrameSize(r);
        this.mCache = mc;
        mWidth = canvas.getWidth();
        mHeight = canvas.getHeight();
        // Get default font info 
        FontInfo fontInfo = canvas.getFontInfo();
        mDefaultFontName = canvas.defaultFont;
        mDefaultFontFileName = canvas.defaultFontFilename;
        mDefaultBoldFontFileName = canvas.defaultBoldFontFilename;
        mDefaultItalicFontFileName = canvas.defaultItalicFontFilename;
        mDefaultBoldItalicFontFileName = canvas.defaultBoldItalicFontFilename;
    }


    /** Create a program from the given script */
    Program program(String script) {
         byte[] action = ScriptCompiler.compileToByteArray(script, mProperties);
         return new Program(action, 0, action.length);
    }

    /** Compiles the specified script to bytecodes
     * and add its bytecodes to the current frame in this movie.
     *
     * @param script the script to be compiled
     * @return the number of bytes
     */
    int addScript(String script) {
         byte[] action = ScriptCompiler.compileToByteArray(script, mProperties);
         Program program = new Program(action, 0, action.length);
         mLogger.debug("    Adding a program of " + action.length + " bytes.");
         addProgram(program);
         return action.length;
    }

    /** Compiles the specified script to bytecodes
     * and add its bytecodes to the current frame in this movie.
     *
     * @param script the script to be compiled
     * @param offset of frame to add to
     */
    private void addScript(String script, int offset) {
         byte[] action = ScriptCompiler.compileToByteArray(script, mProperties);
         Program program = new Program(action, 0, action.length);
         mLogger.debug("    Adding a program of " + action.length + " bytes.");
         addProgram(program, offset);
    }

    /**
     * Adds the program to the next frame
     *
     * @param program to be added
     */
    void addProgram(Program program) {
         Frame frame = mFlashFile.getMainScript().getFrameAt(mLastFrame);
         frame.addFlashObject(new DoAction(program));
    }
    
    /**
     * Adds the program to the specified frame
     *
     * @param program to be added
     * @param offset of frame to add to 
     */
    private void addProgram(Program program, int offset) {
         Frame frame = mFlashFile.getMainScript().getFrameAt(offset);
         frame.addFlashObject(new DoAction(program));
    }


    class ImportResourceError extends CompilationError {

        ImportResourceError(String fileName, CompilationEnvironment env) {
            super("Can't import " + stripBaseName(fileName, env));
        }
        ImportResourceError(String fileName, String type, CompilationEnvironment env) {
            super("Can't import " + type + " " + stripBaseName(fileName, env));
        }
        ImportResourceError(String fileName, Exception e, CompilationEnvironment env) {
            super("Can't import " + stripBaseName(fileName, env) + ": " + e.getMessage());
        }
        ImportResourceError(String fileName, Exception e, String type, CompilationEnvironment env) {
            super("Can't import " + type + " " + stripBaseName(fileName, env) + ": " + e.getMessage());
        }

    }

    public static String stripBaseName (String fileName, CompilationEnvironment env) {
        try {
            fileName = (new File(fileName)).getCanonicalFile().toString();
        } catch (java.io.IOException e) {
        }
        String base = env.getErrorHandler().fileBase;
        if (base == null || "".equals(base)) {
            return fileName;
        }
        base = (new File(base)).getAbsolutePath() + File.separator;
        if (base != null) {
            int i = 1;
            // Find longest common substring
            while (i < base.length() && fileName.startsWith(base.substring(0, i))) { i++; }
            // remove base string prefix
            return fileName.substring(i-1);
        } else {
            return fileName;
        }
    }

   /** Find a resource for importing into a movie and return a flashdef.
     * Doesn't includes stop action.
     *
     * @param fileName file name of the resource
     * @param name name of the resource
     */
    private Resource getMultiFrameResource(String fileName, String name, int fNum) 
        throws ImportResourceError
    {
        Resource res = (Resource)mMultiFrameResourceMap.get(fileName);
        if (res != null) {
            return res;
        }

        res = getResource(fileName, "$" + name + "_lzf_" + (fNum+1), false);

        mMultiFrameResourceMap.put(fileName, res);
        return res;
    }

   /** Find a resource for importing into a movie and return a flashdef.
     * Includes stop action.
     *
     * @param fileName file name of the resource
     * @param name name of the resource
     */
    private Resource getResource(String fileName, String name)
        throws ImportResourceError
    {
        return getResource(fileName, name, true);
    }

   /** Find a resource for importing into a movie and return a flashdef.
     *
     * @param name name of the resource
     * @param fileName file name of the resource
     * @param stop include stop action if true
     */
    private Resource getResource(String fileName, String name, boolean stop)
        throws ImportResourceError
    {
        try {
            String inputMimeType = MimeType.fromExtension(fileName);
            if (!Transcoder.canTranscode(inputMimeType, MimeType.SWF) 
                && !inputMimeType.equals(MimeType.SWF)) {
                inputMimeType = Transcoder.guessSupportedMimeTypeFromContent(fileName);
                if (inputMimeType == null || inputMimeType.equals("")) {
                    throw new ImportResourceError(fileName, new Exception("bad mime type"), mEnv);
                }
            }
            // No need to get these from the cache since they don't need to be
            // transcoded and we usually keep the cmcache on disk.
            if (inputMimeType.equals(MimeType.SWF)) {
        
                long fileSize =  FileUtils.getSize(new File(fileName));
    
                Element elt = new Element("resource");
                    elt.setAttribute("name", name);
                    elt.setAttribute("mime-type", inputMimeType);
                    elt.setAttribute("source", fileName);
                    elt.setAttribute("filesize", "" + fileSize);
                mInfo.addContent(elt);
    
                return importSWF(fileName, name, false);
            }

            // TODO: [2002-12-3 bloch] use cache for mp3s; for now we're skipping it 
            // arguably, this is a fixme
            if (inputMimeType.equals(MimeType.MP3) || 
                inputMimeType.equals(MimeType.XMP3)) {
                return importMP3(fileName, name);
            }
    
            File inputFile = new File(fileName);
            File outputFile = mCache.transcode(inputFile, inputMimeType, MimeType.SWF);
            mLogger.debug("importing: " + fileName + " as " + name + 
                          " from cache; size: " + outputFile.length());

            long fileSize =  FileUtils.getSize(outputFile);

            Element elt = new Element("resource");
                elt.setAttribute("name", name);
                elt.setAttribute("mime-type", inputMimeType);
                elt.setAttribute("source", fileName);
                elt.setAttribute("filesize", "" + fileSize);
            mInfo.addContent(elt);

            return importSWF(outputFile.getPath(), name, stop);
        } catch (Exception e) {
            mLogger.error("Can't get resource " + fileName);
            throw new ImportResourceError(fileName, e, mEnv);
        }

    }

    /** Import a resource file into the preloader movie.
     * Using a name that already exists clobbers the
     * old resource (for now).
     *
     * @param fileName file name of the resource
     * @param name name of the MovieClip/Sprite
     * @throws CompilationError
     */
    public void importPreloadResource(String fileName, String name) 
        throws ImportResourceError
    {
        if (name.equals(""))
            name = createName();
        importResource(fileName, name, 0, mPreloaderFontsCollector);
    }

    /** Import a multiframe resource into the current movie.  Using a
     * name that already exists clobbers the old resource (for now).
     */
    public void importPreloadResource(List sources, String name, File parent)
        throws ImportResourceError
    {
        if (name.equals("")) 
            name = createName();
        importResource(sources, name, parent, 0, mPreloaderFontsCollector);
    }

    /** Import a resource file into the current movie.
     * Using a name that already exists clobbers the
     * old resource (for now).
     *
     * @param fileName file name of the resource
     * @param name name of the MovieClip/Sprite
     * @throws CompilationError
     */
    public void importResource(String fileName, String name)
        throws ImportResourceError
    {
        importResource(fileName, name, -1);
    }

    /** Import a resource file into the current movie.
     * Using a name that already exists clobbers the
     * old resource (for now).
     *
     * @param fileName file name of the resource
     * @param name name of the MovieClip/Sprite
     * @param frameNum frame offset to add to
     * @throws CompilationError
     */
    public void importResource(String fileName, String name, int frameNum)
        throws CompilationError
    {
        importResource(fileName, name, frameNum, null);
    }


    /** Import a resource file into the current movie.
     * Using a name that already exists clobbers the
     * old resource (for now).
     *
     * @param fileName file name of the resource
     * @param name name of the MovieClip/Sprite
     * @param frameNum frame offset to add to
     * @param fontsCollector fonts collector for resource (used by preloader)
     * @throws CompilationError
     */
    public void importResource(String fileName, String name, int frameNum, 
                               FontsCollector fontsCollector)
        throws CompilationError
    {
        mLogger.debug("    Importing resource " + name);

        try {
            fileName = new File(fileName).getCanonicalPath();
        } catch (java.io.IOException e) {
            throw new ImportResourceError(fileName, e, mEnv);
        }
        FlashDef def = null;

        Resource res =  (Resource)mResourceMap.get(fileName);
        boolean oldRes = (res != null);
        if (!oldRes) {
            // Get the resource and put in the map
            res = getResource(fileName, name);
            mResourceMap.put(fileName, res);

            def = res.getFlashDef();
            if (fontsCollector != null) def.collectFonts(fontsCollector);
            mFlashFile.addDefToLibrary(name, def);
            def.setName(name);
        
        } else {
            def = res.getFlashDef();
            if (fontsCollector != null) def.collectFonts(fontsCollector);
            // Add an element with 0 size, since it's already there.
            Element elt = new Element("resource");
                elt.setAttribute("name", name);
                // elt.setAttribute("mime-type", MimeType.MP3); 
                elt.setAttribute("source", fileName);
                elt.setAttribute("filesize", "0");
            mInfo.addContent(elt);
        } 
        
    
        ExportAssets ea = new ExportAssets();
        ea.addAsset(name, def);
        Timeline timeline = mFlashFile.getMainScript().getTimeline();
        if (frameNum == -1) {
            frameNum = timeline.getFrameCount() - 1;
        }
        Frame frame = timeline.getFrameAt(frameNum);
        frame.addFlashObject(ea);
    }
    
    /** Imports file, if it has not previously been imported, and
     * returns in any case the name of the clip that refers to it.
     * File should refer to a graphical asset. */
    public String importResource(File file)
    {
        Resource res;

        try {
            file = file.getCanonicalFile();
            res = (Resource) mResourceMap.get(file.getCanonicalPath());
        } catch (java.io.IOException e) {
            throw new CompilationError(e);
        }
        if (res == null) {
            String clipName = createName();
            importResource(file.getPath(), clipName);
            return clipName;
        } else {
            return res.getName();
        }
    }

    /** Imports this resource, if it has not previously been imported, as
     * resource that can be used as a click region, and returns in any
     * case the name of the clip that refers to it.  
     */
    public String importClickResource(File file) throws ImportResourceError
    {
        String fileName;
        try {
            fileName = file.getCanonicalPath();
        } catch (java.io.IOException e) {
            throw new CompilationError(e);
        }

        String name = (String) mClickResourceMap.get(fileName);

        if (name == null) {

            Button2 but = new Button2();

            FlashDef def;
            Rectangle2D bounds;

            // FIXME: [2004-06-29 bloch] 
            // For each instance in the first frame, add a button record.
            // Get bounds for entire clip; should only get bounds for first frame!
            // Should only allow swf resources as click resources.
            try {
                Script script = FlashFile.parse(fileName).getMainScript();
                Frame frame = script.getFrameAt(0);
                bounds = script.getBounds();
                for(int i = 0; i < frame.size(); i++) {
                    FlashObject o = (FlashObject)frame.getFlashObjectAt(i);
                    if (o instanceof Instance) {
                        Instance inst = (Instance)o;
                        CXForm cxform = inst.cxform;
                        if (cxform == null) {
                            cxform = CXForm.newIdentity(false);
                        }
                        java.awt.geom.AffineTransform matrix = inst.matrix;
                        if (matrix == null) {
                            matrix = new java.awt.geom.AffineTransform();
                        }
                         
                        but.addButtonRecord(new ButtonRecord(ButtonRecord.HitTest,
                                                 inst.def, 1, matrix, cxform));
                    }
                }
            } catch (Exception e) {
                throw new ImportResourceError(fileName, e, mEnv);
            }

            // TODO: [2004-06029 bloch] When we merge into lps-intl2, there should
            // be some code sharing between this and SWFFile
            but.addActionCondition(ActionCondition.onPress(program(
                "_root.LzModeManager.handleMouseButton( myView, 'onmousedown')")));
            but.addActionCondition(ActionCondition.onRelease(program(
                "_root.LzModeManager.handleMouseButton( myView, 'onmouseup');" + 
                "_root.LzModeManager.handleMouseEvent( myView, 'onclick')")));
            but.addActionCondition(ActionCondition.onReleaseOutside(program(
                "_root.LzModeManager.handleMouseButton( myView, 'onmouseup');" + 
                "_root.LzModeManager.handleMouseEvent( myView, 'onmouseupoutside')")));
            but.addActionCondition(ActionCondition.onRollOver(program(
                "_root.LzModeManager.handleMouseEvent( myView, 'onmouseover')")));
            but.addActionCondition(ActionCondition.onRollOut(program(
                "_root.LzModeManager.handleMouseEvent( myView, 'onmouseout')")));
            but.addActionCondition(ActionCondition.onDragOut(program(
                "_root.LzModeManager.handleMouseEvent( myView, 'onmouseout');" + 
                "_root.LzModeManager.handleMouseEvent( myView, 'onmousedragout')")));
            but.addActionCondition(ActionCondition.onDragOver(program(
                "_root.LzModeManager.handleMouseEvent( myView, 'onmouseover');" + 
                "_root.LzModeManager.handleMouseEvent( myView, 'onmousedragin')")));
            
            name = createName();

            // Scale the movieclip to 100x100 for use by LFC.
            Script movieClip = new Script(1);
            Frame f = movieClip.getFrameAt(0);
            double sx = 100.0 * TWIP / (double)bounds.getWidth();
            double sy = 100.0 * TWIP / (double)bounds.getHeight();
            java.awt.geom.AffineTransform matrix = java.awt.geom.AffineTransform.getScaleInstance(sx, sy);
            f.addInstance(but, 1, matrix, null);

            mFlashFile.addDefToLibrary(name, movieClip);
            movieClip.setName(name);
            ExportAssets ea = new ExportAssets();
            ea.addAsset(name, movieClip);
            Timeline timeline = mFlashFile.getMainScript().getTimeline();
            Frame frame = timeline.getFrameAt(timeline.getFrameCount() - 1);
            frame.addFlashObject(ea);

            mClickResourceMap.put(fileName, name);
        }

        return name;
    }

    /** Import a multiframe resource into the current movie.  Using a
     * name that already exists clobbers the old resource (for now).
     *
     * @param sources file names of the resources
     * @param name name of the MovieClip/Sprite
     * @param parent parent's File object 
     */
    public void importResource(List sources, String name, File parent)
    {
        importResource(sources, name, parent, -1);
    }

    /** Import a multiframe resource into the current movie.  Using a
     * name that already exists clobbers the old resource (for now).
     *
     * @param sources file names of the resources
     * @param name name of the MovieClip/Sprite
     * @param parent parent's File object 
     * @param frameNum frame offset to add to
     */
    public void importResource(List sources, String name, File parent, int frameNum)
    {
        importResource(sources, name, parent, frameNum, null);
    }


    /** Import a multiframe resource into the current movie.  Using a
     * name that already exists clobbers the old resource (for now).
     *
     * @param sources file names of the resources
     * @param name name of the MovieClip/Sprite
     * @param parent parent's File object 
     * @param frameNum frame offset to add to
     * @param fontsCollector fonts collector for resource (used by preloader)
     */
    public void importResource(List sources, String name, File parent, int frameNum, 
                               FontsCollector fontsCollector)
    {
        Script out = new Script(1);
        String fileName = null;
        mLogger.debug("Including multiple resources as " + name);
        int width = 0;
        int height = 0;
        int fNum = 0;
        for (Iterator e = sources.iterator() ; e.hasNext() ;) {
            fileName = (String)e.next();
            mLogger.debug("    Importing " + fileName);
     
            // Definition to add to the library (without stop)
            Resource res = getMultiFrameResource(fileName, name, fNum);
            Script scr = (Script)res.getFlashDef(); 
            if (fontsCollector != null) scr.collectFonts(fontsCollector);
            int bc = out.getFrameCount();
            out.appendScript(scr);
            int fc = out.getFrameCount();
            Frame f = out.getFrameAt(fc - 1);
            f.addStopAction();
            mLogger.debug("    Added " + (fc - bc) + " of " + fc + "frame(s)");

            int rw = res.getWidth();
            int rh = res.getHeight();
            if (rw > width) {
                width = rw;
            }
            if (rh > height) {
                height = rh;
            }
            // NOTE: add the ratio attribute to each frame here; this 
            // appears to be required to make a multi-frame resource that has individual 
            // frames that are swfs with nested movieclips work correctly.
            // This was "guessed" by dumping the contents
            // of a multi-frame SWF created by the Flash tool itself.
            // This is weird since the docs on 'ratio' say it is only
            // needed to cope with Morphing.  Hmph.  See bug 4961 for details.
            if (fNum > 0) {
                for (int i = 0; i < f.size(); i++) {
                    if (f.getFlashObjectAt(i) instanceof Instance) {
                        Instance inst = (Instance) f.getFlashObjectAt(i);
                        inst.ratio = fNum;
                        break;
                    }
                }
            }
            fNum++;
        }

        // TODO [2003-1-2 bloch]: Could optimize and only add
        // multi-frame resources when the total size is greater than
        // the size of the first frame
        mMultiFrameResourceSet.add(new Resource(name, out, width, height));

        FlashDef def = (FlashDef)out;
        mFlashFile.addDefToLibrary(name, def);
        def.setName(name);
        ExportAssets ea = new ExportAssets();
        ea.addAsset(name, def);
        Timeline timeline = mFlashFile.getMainScript().getTimeline();
        if (frameNum == -1) {
            frameNum = timeline.getFrameCount() - 1;
        }
        Frame frame = timeline.getFrameAt(frameNum);
        frame.addFlashObject(ea);
    }

    /** Returns a new unique js name. */
    String createName() {
        return mNameSupply.next();
    }
    
    /**
     * Recursively strips out the ActionScript from a 
     * given movie Script (MovieClip)
     *
     * Actually, it leaves the actionscript blocks in, 
     * but turns them into programs that do nothing.
     */
    private void stripActions(Script s) {
        Timeline t = s.getTimeline();
        int n = t.getFrameCount();

        boolean didStop = false;

        for(int i = 0; i < n; i++ ) {
            Frame f = s.getFrameAt(i);

            for (int j = 0; j < f.size(); j++) {
                FlashObject o = f.getFlashObjectAt(j);
                if (o instanceof Script) {
                    stripActions((Script)o);
                } else if (o instanceof DoAction) {
                    DoAction doa = (DoAction) o;
                    Program p = new Program();
                    p.none();
                    doa.setProgram(p);
                }
            }
        }
    }

    /**
     * @param fileName
     * @param name
     * @param addStop if true, add stop action to last frame
     */
    private Resource importSWF(String fileName, String name, boolean addStop) 
        throws IVException, FileNotFoundException  {

        FlashFile f = FlashFile.parse(fileName);
        Script s = f.getMainScript();
        collectFonts(s);
        if (addStop) {
            Frame frame = s.getFrameAt(s.getFrameCount() - 1);
            frame.addStopAction();
        }

        Rectangle2D rect = s.getBounds();
        int mw = (int)(rect.getMaxX()/TWIP);
        int mh = (int)(rect.getMaxY()/TWIP);

        Resource res = new Resource(name, s, mw, mh);

        // Add multi-frame SWF resources that have bounds that
        // are different than their first frame to the resource table.
        if (s.getFrameCount() > 1) {

            Rectangle2D f1Rect = new Rectangle2D.Double();
            s.getFrameAt(0).addBounds(f1Rect);
            int f1w = (int)(f1Rect.getMaxX()/TWIP);
            int f1h = (int)(f1Rect.getMaxY()/TWIP);
            if (f1w < mw || f1h < mh) {
                mMultiFrameResourceSet.add(res);
            }
        }
 
        return res;
    }

    /**
     * collect fonts for later use
     */
    private void collectFonts(Script s) {

        Timeline tl = s.getTimeline();
        // If preloader is added, skip frame 0. Assume that preloader is only
        // one frame long starting at frame 0.
        for(int i=0; i<tl.getFrameCount(); i++ ) {
            Frame frame = tl.getFrameAt(i);
            for( int k=0; k<frame.size(); k++ ) {
                FlashObject fo = frame.getFlashObjectAt(k);
                //mLogger.debug("collecting from a " + fo.getClass().getName());
                fo.collectFonts( mFontsCollector );
                //mLogger.debug("FONTS size " 
                         //+ mFontsCollector.getFonts().size());
            }
        }
    }

    /**
     * @param fileName
     * @param name
     */
    private Resource importMP3(String fileName, String name) 
        throws IVException, IOException {

        long fileSize =  FileUtils.getSize(new File(fileName));
        FileInputStream stream = new FileInputStream(fileName);
        FlashBuffer buffer = new FlashBuffer(stream);
        Sound sound = MP3Sound.newMP3Sound(buffer);

        Element elt = new Element("resource");
            elt.setAttribute("name", name);
            elt.setAttribute("mime-type", MimeType.MP3);
            elt.setAttribute("source", fileName);
            elt.setAttribute("filesize", "" + fileSize);
        mInfo.addContent(elt);

        stream.close();

        return new Resource(sound);
    }

    /** Writes the SWF to the <code>OutputStream</code> that was
     * supplied to the SWFWriter's constructor.
     * @throws IOException if an error occurs
     */
    public void close() throws IOException {

        if (mCloseCalled) {
            throw new IllegalStateException("SWFWriter.close() called twice");
        }
        
        // Create the textfield movieclip; name must match that used in LFC
        if ((mFlashVersion == 5) && (mDefaultFont == null)) {
            try {
                File f = mEnv.resolve(mDefaultFontFileName, "");
                mDefaultFont = importFontStyle(f.getAbsolutePath(), mDefaultFontName, "plain", mEnv);
            } catch (FileNotFoundException fnfe) {
                throw new CompilationError("default font " 
                        + mDefaultFontFileName + " missing " + fnfe);
            }
        }

        if (mFlashVersion == 5) {
            addTextAsset("newtext", mDefaultFont);
        }

        // Add font information
        addFontTable();

        // Add resource information to canvas.
        addResourceTable();

        if (mFlashVersion == 5) {
            // Add list of input text resource names
            addInputTextResourceNames();
        }

        addDefaultTextWidth();

        if (mPreloaderAdded == true) {
            // Add script to hide the preloader after finishing instantiation
            String finalobj = "_root.lzpreloader.done();";
            addScript(finalobj);
        }

        // Flag says to post a copy of all debug.write() calls back to the server
        if (mProperties.getProperty("logdebug", "false").equals("true")) {
            addScript("_dbg_log_all_writes = true;");
        }

        boolean remotedebug = (mProperties.getProperty(mEnv.REMOTEDEBUG_PROPERTY, "").length() > 0);
        boolean debug = mProperties.getProperty("debug", "false").equals("true");

        // Tell the debugger we're done loading.
        if (debug && !remotedebug) {

            String width    = mEnv.getProperty("debugger_width", DebugCompiler.DEFAULT_DEBUGGER_WIDTH);
            String height   = mEnv.getProperty("debugger_height", DebugCompiler.DEFAULT_DEBUGGER_HEIGHT);
            String x        = mEnv.getProperty("debugger_x", DebugCompiler.DEFAULT_DEBUGGER_X);
            String y        = mEnv.getProperty("debugger_y", DebugCompiler.DEFAULT_DEBUGGER_Y);
            String font     = mEnv.getProperty("debugger_font", DebugCompiler.DEFAULT_DEBUGGER_FONT);
            String fontsize = mEnv.getProperty("debugger_fontsize", DebugCompiler.DEFAULT_DEBUGGER_FONTSIZE);

            addScript("LzInstantiateView({attrs: {id: \"Debug\" "
                      +", fontstyle: \"plain\", fontsize: "+fontsize+", font: \"" +font + "\""
                      + ", y: " + y + ", x: " + x
                      + ", width: " + width 
                      + ", height: "+ height
                      + "} , name: \"debugwindow\"})");

            // Make sure debugger has a input text font
            addInputText(new FontInfo(font, fontsize, "plain"), false);
            
            addScript("Debug.loaded()");
        }

        boolean kranking = "true".equals(mEnv.getProperty("krank"));

        // Bake the krank TCP server port into the app
        if (kranking) {
            addScript("_root.LzSerializer.krankport = "+LPS.getKrankPort()+";");
        }
        // Tell the canvas we're done loading.
        addScript("canvas.initDone()");

        // Make sure we stop
        Program program = new Program();
        program.stop();
        program.none();
        addProgram(program);

        // Don't compress flash5 files or krank files
        if (mFlashVersion != 5 && !kranking) {
            mFlashFile.setCompressed(true);
        }
     
        try { 
            InputStream input;
            input = mFlashFile.generate(mEnv.getEmbedFonts() ? mFontsCollector : new FontsCollector(), 
                                        mEnv.getEmbedFonts() ? mPreloaderFontsCollector : new FontsCollector(),
                                        mPreloaderAdded).getInputStream();
            FileUtils.send(input, mStream);
        } catch (IVException e) {
            throw new ChainedException(e);
        }

        mCloseCalled = true;
    }

    public void openSnippet() throws IOException {
        // How do we make sure an initial frame exists?  Does this do it? 
        Frame frame = mFlashFile.getMainScript().getFrameAt(mLastFrame);
        // if we don't have any frame, then code which adds resources gets
        // an error. This happens if you have a resource declared before any code.
    }

    public void closeSnippet() throws IOException {

        if (mCloseCalled) {
            throw new IllegalStateException("SWFWriter.closeSnippet() called twice");
        }

        // TODO [hqm 2004-09-29] Add callback for snippet loader onload event
        // Make sure we stop
        addScript("this._parent.loader.snippetLoaded(this, null)");
        Program program = new Program();
        program.stop();
        program.none();
        addProgram(program);

        try { 
            InputStream input;
            input = mFlashFile.generate(mEnv.getEmbedFonts() ? mFontsCollector : new FontsCollector(), 
                                        mEnv.getEmbedFonts() ? mPreloaderFontsCollector : new FontsCollector(),
                                        mPreloaderAdded).getInputStream();
            FileUtils.send(input, mStream);
        } catch (IVException e) {
            throw new ChainedException(e);
        }

        mCloseCalled = true;
    }



    /**
     * Generate a warning message
     */
    void warn(CompilationEnvironment env, String msg) {
        CompilationError cerr;
        env.warn(msg);
    }

    /**
     * Import a font of a given style into the SWF we are writing.
     *
     * @param fileName filename for font in LZX
     * @param face face name of font
     * @param style style of font
     */
    Font importFontStyle(String fileName, String face, String style,
            CompilationEnvironment env)
        throws FileNotFoundException, CompilationError {

        int styleBits = FontInfo.styleBitsFromString(style);

        mLogger.debug("importing " + face + " of style " + style);

        FontInfo fontInfo = mEnv.getCanvas().getFontInfo(); 
        boolean isDefault = false;

        Font font = importFont(fileName, face, styleBits, false);

        if (fontInfo.getName().equals(face)) {
            if (styleBits == FontInfo.PLAIN) {
                isDefault = true;
                mDefaultFont = font;
            } 
        }

        FontFamily family = mFontManager.getFontFamily(face, true);

        switch (styleBits) {
            case FontInfo.PLAIN: 
                if (family.plain != null) {
                    if (!isDefault || mDefaultFontUsedForMeasurement) {
                        warn(env, "Redefined plain style of font: " + face);
                    }
                }
                family.plain = font; break;
            case FontInfo.BOLD:
                if (family.bold != null) {
                    warn(env, "Redefined bold style of font: " + face);
                }
                family.bold = font; break;
            case FontInfo.ITALIC:
                if (family.italic != null) {
                    warn(env, "Redefined italic style of font: " + face);
                }
                family.italic = font; break;
            case FontInfo.BOLDITALIC:
                if (family.bitalic != null) {
                    warn(env, "Redefined bold italic style of font: " + face);
                }
                family.bitalic = font; break;
            default:
                throw new ChainedException("Unexpected style");
        }

        return font;
    }

    /**
     * Import a font into the SWF we are writing
     *
     * @param name of font in LZX
     * @param fileName name of font file 
     */
    private Font importFont(String fileName, String face, int styleBits,
        boolean replace)
        throws FileNotFoundException, CompilationError {

        if (isDeviceFont(face)) {
            return Font.createDummyFont(face);
        }

        mLogger.debug("    Importing font " + face + " from " + fileName);

        String fromType = FontType.fromName(fileName);
        String location = null;
        try {
            File fontFile = mCache.transcode(new File(fileName), fromType, 
                    FontType.FFT);
            location = fontFile.getAbsolutePath();
        } catch (TranscoderException e) {
            throw new CompilationError(e);
        } catch (FileNotFoundException e) {
            throw e;
        } catch (IOException e) {
            throw new CompilationError(e);
        }

        Font font = Font.createDummyFont(face);

        long fileSize =  FileUtils.getSize(new File(location));

        // FIXME: [2004-05-31 bloch] embed fonts shouldn't be global
        if (mFlashVersion == 5 || mEnv.getEmbedFonts()) {
            Element elt = new Element("font");
                elt.setAttribute("face", face);
                    elt.setAttribute("style", FontInfo.styleBitsToString(styleBits, true));
                elt.setAttribute("location", location);
                elt.setAttribute("source", fileName);
                elt.setAttribute("filesize", "" + fileSize);
            mInfo.addContent(elt);
        }

        try {

            // Parse the font 
            mLogger.debug("Font file name " + location);
            FlashFile fontFile = FlashFile.parse( location );
            Enumeration defs = fontFile.definitions();
            FontDef fontDef = (FontDef)defs.nextElement();

            // Copy the font
            // For now, add the entire font (including "layout" info)
            fontDef.getFont().copyTo(font);

            if ((font.flags & Font.SHIFT_JIS) != 0) {
                throw new CompilationError("Can't handle SHIFT_JIS font: " 
                        + fileName);
            }
            // Make sure font has LAYOUT info 
            if ((font.flags & Font.HAS_LAYOUT) == 0) {
                throw new CompilationError(fileName + 
                        " has no layout information.");
            }

            // Put in our face name
            font.fontName = face;

            // Clean out existing styles.
            font.flags &= ~(Font.BOLD | Font.ITALIC);

            // Write in ours.
            if ((styleBits & FontInfo.BOLD) != 0) {
                font.flags |= Font.BOLD;
            }
            if ((styleBits & FontInfo.ITALIC) != 0) {
                font.flags |= Font.ITALIC;
            }

            // FIXME: [OLD bloch] debugging shouldn't go to system.err; should log!
            // need adapter class for logger for that
            if (mProperties.getProperty("trace.fonts", "false").equals("true")) {
                 font.printContent(System.err, "  ", 0);
            }

            // Add to the list of fonts
            FontDef fdef = mFontsCollector.addFont(font, null);
            // For now, write all characters.
            fdef.setWriteAllChars(true);
            fdef.setWriteLayout(true);

        } catch (IVException e) {
            throw new ChainedException(e);
        }

        return font;
    }

    /**
     * Import all action script blocks
     */
     void importActions(String fileName) 
         throws FileNotFoundException, IVException {

         Timeline t = FlashFile.parse(fileName).getMainScript().getTimeline();

         for(int i = 0; i < t.getFrameCount(); i++) {
             Frame frame = t.getFrameAt(i);
             for(int j = 0; j < frame.size(); j++) {
                 FlashObject o = frame.getFlashObjectAt(j);
                 if (o instanceof DoAction) {
                     DoAction action = (DoAction)o;
                     addProgram(action.getProgram());
                 }
             }
         }
     }

    /**
     * @return first action block
     */
     DoAction getFirstDoAction(String fileName) 
         throws FileNotFoundException, IVException {

         Timeline t = FlashFile.parse(fileName).getMainScript().getTimeline();

         for(int i = 0; i < t.getFrameCount(); i++) {
             Frame frame = t.getFrameAt(i);
             for(int j = 0; j < frame.size(); j++) {
                 FlashObject o = frame.getFlashObjectAt(j);
                 if (o instanceof DoAction) {
                     return (DoAction)o;
                 }
             }
         }
         return null;
     }

     /**
      * Adds font information to the canvas
      */
     private void addFontTable() {

        if (mFontTableIndex == -1) {
            mLogger.error("No canvas.  Skipping font table");
            return;
        }
        Enumeration fonts = mFontManager.getNames();
        StringBuffer actions = new StringBuffer("");
        while(fonts.hasMoreElements()) {
            // TODO: [old bloch] Add assert that the canvas has been created
            // before this action script is added!
            String name = (String)fonts.nextElement();
            FontFamily family = mFontManager.getFontFamily(name);
            mLogger.debug("Adding font family: " + name);

            actions.append("_root.LzFontManager.addFont('" + name + "', " );
                    
            appendFont(actions, family.plain, family.getBounds(FontInfo.PLAIN));
            actions.append(",");
            appendFont(actions, family.bold, family.getBounds(FontInfo.BOLD));
            actions.append(",");
            appendFont(actions, family.italic, family.getBounds(FontInfo.ITALIC));
            actions.append(",");
            appendFont(actions, family.bitalic, family.getBounds(FontInfo.BOLDITALIC));
            actions.append("\n)\n");
        }

        if (mProperties.getProperty("trace.fonts", "false").equals("true")) {
            mLogger.debug(actions.toString());
        }

        byte[] action = ScriptCompiler.compileToByteArray(actions.toString(), mProperties);
        Program program = new Program(action, 0, action.length);

        // Add the program right in the spot where it belongs
        Frame frame = mFlashFile.getMainScript().getFrameAt(mLastFrame);
        frame.setFlashObjectAt(new DoAction(program), mFontTableIndex);
    }

     /**
      * Adds resource information to the canvas
      */
     private void addResourceTable() {

         StringBuffer buf = new StringBuffer("");
         // Sort the keys, so that regression tests aren't sensitive to
         // the undefined order of iterating a (non-TreeSet) Set.
         SortedSet sset = new TreeSet(mMultiFrameResourceSet);
         Iterator resources = sset.iterator();
         while(resources.hasNext()) {
             Resource res = (Resource)resources.next();
             buf.append("canvas.resourcetable[\"" + res.getName() + 
                    "\"]={ width : " + res.getWidth() + ", height :" + 
                    res.getHeight() + "};\n");
         }

         byte[] action = ScriptCompiler.compileToByteArray(buf.toString(), mProperties);
         Program program = new Program(action, 0, action.length);

         // Add the program right in the spot where it belongs
         Frame frame = mFlashFile.getMainScript().getFrameAt(mLastFrame);
         frame.setFlashObjectAt(new DoAction(program), mResourceTableIndex);
     }

    /**
     * Sets the lfc's default text width
     * LzAbstractText.prototype.DEFAULT_WIDTH,
     * from the compiler's default value
     */
    private void addDefaultTextWidth() {
        StringBuffer buf = new StringBuffer("");
        buf.append("_root.LzAbstractText.prototype.DEFAULT_WIDTH = "+mEnv.getDefaultTextWidth()+";\n");
        addScript(buf.toString());
    }

     /**
      * Adds names of input text resources as a property of LzFontManager
      */
     private void addInputTextResourceNames() {
         StringBuffer buf = new StringBuffer("");
         Iterator resources = mInputTextSet.entrySet().iterator();
         buf.append("_root.LzFontManager.inputTextResourceNames = {};\n");
         while(resources.hasNext()) {
             Map.Entry entry = (Map.Entry) resources.next();
             buf.append("_root.LzFontManager.inputTextResourceNames['"+entry.getValue()+"'] = true;\n");
         }
         addScript(buf.toString());
     }

    /**
     * @return height of fontinfo in pixels
     * @param fontinfo
     */
    double getFontHeight (FontInfo fontInfo) {
        return fontHeight(getFontFromInfo(fontInfo));
    }

    /**
     * @return lineheight which lfc LzInputText expects for a given fontsize
     * @param fontinfo, int fontsize
     */
    double getLFCLineHeight (FontInfo fontInfo, int fontsize) {
        return lfcLineHeight(getFontFromInfo(fontInfo), fontsize);
    }

    /**
     * Convert em units to pixels, truncated to 2 decimal places.
     * Slow float math... but simple code to read.
     *
     * @param units number of 1024 em units
     * @return pixels
     */
    private static double emUnitsToPixels(int units) {
        int x = (100 * units * DEFAULT_SIZE) / 1024;
        return (double)(x / 100.0);
    }

    /**
     * Compute font bounding box
     *
     * @param font
     */
    static double fontHeight(Font font) {
        if (font == null) { return 0; }
        double ascent  = emUnitsToPixels(font.ascent);
        double descent = emUnitsToPixels(font.descent);
        double leading = emUnitsToPixels(font.leading);
        double lineheight = ascent+descent+leading;
        return lineheight;
    }

    /**
     * Compute font bounding box
     *
     * @param font
     */
    double lfcLineHeight(Font font, int fontsize) {
        double ascent  = emUnitsToPixels(font.ascent);
        double descent = emUnitsToPixels(font.descent);
        //double leading = emUnitsToPixels(font.leading);
        double lineheight = mTextLeading + ((ascent+descent) * ((double)fontsize) / DEFAULT_SIZE);
        return lineheight;
    }

    /**
     * Appends font to actionscript string buffer
     * @param actions string
     * @param font font
     */
    private static void appendFont(StringBuffer actions, Font font,
        Rectangle2D[] bounds) {

        final String newline = "\n  ";
        actions.append(newline);

        if (font == null) {
            actions.append("null");
            return;
        }

        double ascent  = emUnitsToPixels(font.ascent);
        double descent = emUnitsToPixels(font.descent);
        double leading = emUnitsToPixels(font.leading);

        final String comma = ", ";

        actions.append("{");
        actions.append("ascent:");
        actions.append(ascent);
        actions.append(comma);
        actions.append("descent:");
        actions.append(descent);
        actions.append(comma);
        actions.append("leading:");
        actions.append(leading);
        actions.append(comma);
        actions.append("advancetable:");

        int idx, adv;
            
        actions.append(newline);
        actions.append("[");
        // FIXME: [2003-03-19 bloch] We only support ANSI 8bit (up to
        // 255) char encodings.  We lose the higher characters is
        // UNICODE and we don't support anything else.

        for(int i = 0; i < 256; i++) {
            idx = font.getIndex(i);
            adv = font.getAdvanceValue(idx);

            // Convert to pixels rounded to nearest 100th
            double advance = emUnitsToPixels(adv);
            actions.append(advance);
            if (i != 255) {
                actions.append(comma);
            }

            if (i%10 == 9) {
                actions.append(newline);
            }
        }
        actions.append("],");
        actions.append(newline);

        actions.append("lsbtable:");
        actions.append(newline);
        actions.append("[");

        int m;
        int max;
        int adj;
        for(int i = 0; i < 256; i++) {
            idx = font.getIndex(i);
            try {
                m = (int)bounds[idx].getMinX();
                //max = (int)bounds[idx].getMaxX();
            } catch (Exception e) {
                m = 0;
                //max = 0;
            }
            adv = font.getAdvanceValue(idx);
            adj = m;
            if (adj < 0) adj = 0;

            /* The following makes the lsb bigger
               but is strictly wrong */
            /*max = max - adv;
            if (max < 0) max = 0;
            
            if (max > adj) {
                adj = max;
            }*/
            
            // Convert to pixels rounded to nearest 100th
            double lsb = emUnitsToPixels(adj);
            actions.append(lsb);
            if (i != 255) {
                actions.append(comma);
            }

            if (i%10 == 9) {
                actions.append(newline);
            }
        }

        actions.append("],");

        actions.append(newline);
        actions.append("rsbtable:");
        actions.append(newline);
        actions.append("[");

        for(int i = 0; i < 256; i++) {
            idx = font.getIndex(i);
            try {
                m = (int)bounds[idx].getMaxX();
            } catch (Exception e) {
                m = 0;
            }
            adv = font.getAdvanceValue(idx);
            adj = m - adv;
            if (adj < 0) adj = 0;
            
            // Convert to pixels rounded to nearest 100th
            double rsb = emUnitsToPixels(adj);
            actions.append(rsb);
            if (i != 255) {
                actions.append(comma);
            }

            if (i%10 == 9) {
                actions.append(newline);
            }
        }

        actions.append("]}");
    }

    /**
     * Add an asset to the output SWF to represent an input text
     * field that uses the given FontInfo
     * @param fontInfo
     */
    public void addInputText(FontInfo fontInfo, boolean password) 
        throws CompilationError {

        // We only need to add text field defs at compile time to swf5 files;
        // swf6 and greater create all text at runtime.
        if (mFlashVersion != 5) {
            return;
        }

        String fontName = fontInfo.getName();
        Font font = getFontFromInfo(fontInfo);

        String name =
            "lzinputtext/" + 
            fontName + "/" +
            fontInfo.getSize() + "/" +
            fontInfo.getStyle(false) + (password ? "/passwd" : "");

        // We've already added an movieclip that will work for this 
        // input text
        if (mInputTextSet.containsKey(name)) {
            return;
        }

        // Create a movieclip with a single frame with
        // a text field of the correct size.
        Script     movieClip = new Script(1);
        Frame      frame     = movieClip.newFrame();

        TextField  input = new TextField("", "text",
                font, fontInfo.getSize()*TWIP, AlphaColor.black);

        int flags = input.getFlags();

        if (password) {
            flags |= TextField.PASSWORD;
        }

        if (mProperties.getProperty("text.borders", "false").equals("true")) {
            flags |= TextField.BORDER;
        }

        input.setFlags(flags 
                           | TextField.USEOUTLINES
                           | TextField.HASFONT
                           | TextField.MULTILINE
                           );

        // left, right, indent, and align don't make sense 
        // when we do all input text wrapping ourselves.
        // Leading still matters though!
        input.setLayout(0, 0, 0, 0, mTextLeading*TWIP);

        input.setBounds(new Rectangle2D.Double(0, 0, mWidth*TWIP, mHeight*TWIP));

        mLogger.debug("Add inputtext " + name + " " + mWidth + " " + mHeight);

        frame.addInstance(input, 1, null, null);
        frame.addStopAction();

        // Add movieClip to library
        mFlashFile.addDefToLibrary(name, movieClip);
        // Export it.
        ExportAssets ea = new ExportAssets();
        ea.addAsset(name, movieClip);
        Timeline timeline = mFlashFile.getMainScript().getTimeline();
        frame = timeline.getFrameAt(timeline.getFrameCount() - 1);
        frame.addFlashObject(ea);

        // Add this clip to our set of clips
        mInputTextSet.put(name, name);
    }


    /** Create a custom text input field that will correspond to a 
     * specific lzx input text view.
     */
    public void addCustomInputText(FontInfo fontInfo, int width, int height,
                                   boolean multiline, boolean password) 
        throws CompilationError {

        // We only need to add text field defs at compile time to swf5 files;
        // swf6 and greater create all text at runtime.
        if (mFlashVersion != 5) {
            return;
        }

        String fontName = fontInfo.getName();
        Font font = getFontFromInfo(fontInfo);
        // code to designate if this resource is single line or multiline
        String mstring;
        mstring = multiline ? "/m" : "/s";

        String name =
            "lzinputtext/" + 
            fontName + "/" +
            fontInfo.getSize() + "/" +
            fontInfo.getStyle(false) + "/" + width + "/" + height +
            (password ? "/passwd" : "") + mstring;

        // We've already added an movieclip that will work for this 
        // input text
        if (mInputTextSet.containsKey(name)) {
            return;
        }

        // Create a movieclip with a single frame with
        // a text field of the correct size.
        Script     movieClip = new Script(1);
        Frame      frame     = movieClip.newFrame();

        TextField  input = new TextField("", "text",
                font, fontInfo.getSize()*TWIP, AlphaColor.black);

        int flags = input.getFlags();

        if (password) {
            flags |= TextField.PASSWORD;
        }

        if (mProperties.getProperty("text.borders", "false").equals("true")) {
            flags |= TextField.BORDER;
        }

        input.setFlags(flags 
                       | TextField.USEOUTLINES
                       | TextField.HASFONT
                       | (multiline ? TextField.MULTILINE : 0)
                       | (multiline ? TextField.WORDWRAP : 0)
                       );

        // left, right, indent, and align don't make sense 
        // when we do all input text wrapping ourselves.
        // Leading still matters though!
        input.setLayout(0, 0, 0, 0, mTextLeading*TWIP);
        input.setBounds(new Rectangle2D.Double(0, 0, width*TWIP, (height+mTextLeading)*TWIP));
        mLogger.debug("Add custom inputtext " + name + " " + width + " " + height);

        frame.addInstance(input, 1, null, null);
        frame.addStopAction();

        // Add movieClip to library
        mFlashFile.addDefToLibrary(name, movieClip);
        // Export it.
        ExportAssets ea = new ExportAssets();
        ea.addAsset(name, movieClip);
        Timeline timeline = mFlashFile.getMainScript().getTimeline();
        frame = timeline.getFrameAt(timeline.getFrameCount() - 1);
        frame.addFlashObject(ea);

        // Add this clip to our set of clips
        mInputTextSet.put(name, name);
    }



    /**
     * Add a movieclip with two frames in it,
     * each one with a text field
     */
    void addTextAsset(String name, Font font) 
        throws CompilationError {

        // Create a movieclip with a 2 frames with
        // a text field the size of the canvas (or maxTextWidth/maxTextHeight).
        Script     movieClip = new Script(2);

        String     initText = 
            "<P ALIGN=\"LEFT\"><FONT SIZE=\"" + DEFAULT_SIZE
             + "\" COLOR=\"#000000\"> </FONT></P>";

        if (font == null) {
            name = LPS.PUBLIC_ROOT() + File.separator + name.replaceAll("\\{\\$|\\}", "");
            File f = new File(name);
            if (f.exists())
                mFontTableRemainder = (int) f.length() - 512;
            return;
        }
        
        // We only need to add text field defs at compile time to swf5 files;
        // swf6 and greater create all text at runtime.
        if (mFlashVersion != 5) {
            return;
        }

        {
            TextField  text = new TextField(initText, "text",
                    font, mMaxTextWidth*TWIP, AlphaColor.black);
    
            int flags = text.getFlags();
            if (mProperties.getProperty("text.borders", "false").equals("true")) {
                flags |= TextField.BORDER;
            }
    
            text.setFlags(flags 
                               | TextField.USEOUTLINES
                               | TextField.HASFONT
                               | TextField.READONLY
                               | TextField.MULTILINE
                               | TextField.HTML
                               | TextField.NOSELECT
                               );
    
            text.setLayout(0, 0, 0, 0, mTextLeading*TWIP);
            text.setBounds(new Rectangle2D.Double(0, 0, mMaxTextWidth*TWIP, mMaxTextHeight*TWIP));

            Frame frame = movieClip.newFrame();
            frame.addStopAction();
            frame.addInstance(text, 1, null, null);
        }

        {
            TextField  text = new TextField(initText, "text",
                    font, mMaxTextWidth*TWIP, AlphaColor.black);
    
            int flags = text.getFlags();
            if (mProperties.getProperty("text.borders", "false").equals("true")) {
                flags |= TextField.BORDER;
            }
    
            text.setFlags(flags 
                               | TextField.USEOUTLINES
                               | TextField.HASFONT
                               | TextField.READONLY
                               | TextField.MULTILINE
                               | TextField.HTML
                               );
    
            text.setLayout(0, 0, 0, 0, mTextLeading*TWIP);
            text.setBounds(new Rectangle2D.Double(0, 0, mMaxTextWidth*TWIP, mMaxTextHeight*TWIP));

            Frame frame = movieClip.newFrame();
            frame.removeInstance(1);
            frame.addInstance(text, 1, null, null);
        }

        // Add movieClip to library
        mFlashFile.addDefToLibrary(name, movieClip);

        // Export it.
        ExportAssets ea = new ExportAssets();
        ea.addAsset(name, movieClip);
        Timeline timeline = mFlashFile.getMainScript().getTimeline();

        // Add this asset
        int num = mPreloaderAdded ? 1 : 0;
        Frame frame = timeline.getFrameAt(num);
        frame.addFlashObject(ea);
    }

    /**
     * A resource we've imported
     */
    class Resource implements Comparable {
        /** Name of the resource */
        private String mName = "";
        /** width of the resource in pixels */
        private int mWidth = 0;
        /** height of the resource in pixels */
        private int mHeight = 0;
        /** Flash definition of this resource */
        private FlashDef mFlashDef = null;

        /** Create a resource that represents this flash def
         * @param def
         */
        public Resource(FlashDef def) {
            mFlashDef = def;
        }

        /** Create a resource 
         */
        public Resource(String name, FlashDef def, int width, int height) {
            mName = name;
            mFlashDef = def;
            mWidth = width;
            mHeight = height;
        }

        public String getName()  { return mName; }
        public int getWidth()  { return mWidth; }
        public int getHeight() { return mHeight; }
        public FlashDef getFlashDef() { return mFlashDef; }

        public int compareTo(Object other) throws ClassCastException {
          Resource o = (Resource)other;
          return mName.compareTo(o.mName);
        }
    }

    /**
     * @return font given a font info
     */
    private Font getFontFromInfo(FontInfo fontInfo) {
        // This will bring in the default bold ofnt if it's not here yet
        checkFontExists(fontInfo);
        String fontName = fontInfo.getName();
        FontFamily family   = mFontManager.getFontFamily(fontName);
        String style = fontInfo.getStyle();

        if (family == null) {
            return null;
            /*
            throw new CompilationError("Font '" + fontName + 
                "' used but not defined");
            */
        }
        Font font = family.getStyle(fontInfo.styleBits);
        if (font == null) {
            throw new CompilationError("Font '" 
                + fontName 
                + "' style ('" 
                + style 
                + "') used but not defined");
        }
        return font;
    }

    /**
     * @return true if the font exists
     *
     * If this is the default bold font and it hasn't been 
     * declared, import it.
     */
    boolean checkFontExists(FontInfo fontInfo) {

        // Bulletproofing...
        if (fontInfo.getName() == null) {
            return false;
        }
 
        boolean a = mFontManager.checkFontExists(fontInfo);
        if (a) {
            return a;
        }

        if (fontInfo.getName().equals(mDefaultFontName) && 
            fontInfo.styleBits == FontInfo.PLAIN) {
            try {
                    File f = mEnv.resolve(mDefaultFontFileName, "");
                    importFontStyle(f.getAbsolutePath(), mDefaultFontName, "plain", mEnv);
                } catch (FileNotFoundException fnfe) {
                    throw new CompilationError("default font " 
                        + mDefaultFontFileName + " missing " + fnfe);
            }
            return true;
        }

        if (fontInfo.getName().equals(mDefaultFontName) &&
            fontInfo.styleBits == FontInfo.BOLD) {
            try {
                File f = mEnv.resolve(mDefaultBoldFontFileName, "");
                importFontStyle(f.getAbsolutePath(), mDefaultFontName, "bold", mEnv);
            } catch (FileNotFoundException fnfe) {
                throw new CompilationError("default bold font " 
                    + mDefaultBoldFontFileName + " missing " + fnfe);
            }
            return true;
        } 

        if (fontInfo.getName().equals(mDefaultFontName) &&
            fontInfo.styleBits == FontInfo.ITALIC) {
            try {
                File f = mEnv.resolve(mDefaultItalicFontFileName, "");
                importFontStyle(f.getAbsolutePath(), mDefaultFontName, "italic", mEnv);
            } catch (FileNotFoundException fnfe) {
                throw new CompilationError("default italic font " 
                    + mDefaultItalicFontFileName + " missing " + fnfe);
            }
            return true;
        } 

        if (fontInfo.getName().equals(mDefaultFontName) &&
            fontInfo.styleBits == FontInfo.BOLDITALIC) {
            try {
                File f = mEnv.resolve(mDefaultBoldItalicFontFileName, "");
                importFontStyle(f.getAbsolutePath(), mDefaultFontName, "bold italic", mEnv);
            } catch (FileNotFoundException fnfe) {
                throw new CompilationError("default bold italic font " 
                    + mDefaultBoldItalicFontFileName + " missing " + fnfe);
            }
            return true;
        } 

        return false;
    }
}
