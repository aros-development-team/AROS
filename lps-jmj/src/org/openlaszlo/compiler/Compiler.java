/* *****************************************************************************
 * Compiler.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import java.lang.Integer;
import java.io.*;
import java.text.ChoiceFormat;
import java.text.MessageFormat;
import java.util.*;
import org.jdom.*;
import org.jdom.input.SAXBuilder;
import org.openlaszlo.sc.ScriptCompiler;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.utils.ListFormat;
import org.openlaszlo.xml.internal.*;
import org.openlaszlo.server.LPS;
import org.apache.log4j.*;

import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;



/**
 * Compiles a Laszlo XML source file, and any files that it
 * references, into an object file that can be executed on the client.
 *
 * The compiler parses the file into XML, and then gets an element
 * compiler for each toplevel element.
 */
public class Compiler {
    /** Set this to log the modified schema. */
    public static Logger SchemaLogger = Logger.getLogger("schema");
    
    protected static List KNOWN_RUNTIMES =
        Arrays.asList(new String[] {"swf5", "swf6", "swf7"});
    
    /** Called to resolve file references (<code>src</code>
     * attributes).
     */
    protected FileResolver mFileResolver = FileResolver.DEFAULT_FILE_RESOLVER;

    /**
     * CompilerMediaCache
     */
    protected CompilerMediaCache mMediaCache = null;

    /** See getProperties.
     */
    protected final Properties mProperties = new Properties();

    /** Logger
     */
    private static Logger mLogger  = Logger.getLogger(Compiler.class);


    /**
     * A <code>Compiler</code>'s <code>Properties</code> stores
     * properties that affect the compilation.
     *
     * <table><tr><th>Name=default</th><th>Meaning</th></tr>
     * <tr><td>trace.xml=false</td><td>Display XML that's compiled to
     * script, and the script that it compiles to.</td></tr>
     * <tr><td>debug=true</td><td>Add in debug features to the output.</td></tr>
     * <tr><td>trace.fonts=false</td><td>Additional font traces.</td></tr>
     * <tr><td>swf.frame.rate=30</td><td>Output SWF frame rate.</td></tr>
     * <tr><td>default.text.height=12</td><td>Default text height for
     * fonts.</td></tr>
     * <tr><td>text.borders=false</td><td>Display text borders in the output SWF
     *                                    for debugging.</td></tr>
     * </table>
     *
     * Properties that one part of the compiler sets for another part to read:
     * <table><tr><th>Name=default</th><th>Meaning</th></tr>
     * <tr><td>lzc_*</td><td>Source location values.</td></tr>
     * </table>
     *
     * @return a <code>Properties</code> value
     */
    public String getProperty(String key) {
        return mProperties.getProperty(key);
    }
    
    public void setProperty(String key, String value) {
        mProperties.setProperty(key, value);
    }
    
    public FileResolver getFileResolver() {
        return mFileResolver;
    }    
    
    /** Sets the file resolver for this compiler.  The file resolver
     * is used to resolve the names used in include directives to
     * files.
     * @param resolver a FileResolver
     */
    public void setFileResolver(FileResolver resolver) {
        this.mFileResolver = resolver;
    }

    /** Sets the media cache for this compiler.
     * @param resolver a CompilerMediaCache
     */
    public void setMediaCache(CompilerMediaCache cache) {
        this.mMediaCache = cache;
    }
    
    /** Create a CompilationEnvironment with the properties and 
     * FileResolver of this compiler.
     */
    public CompilationEnvironment makeCompilationEnvironment() {
        CompilationEnvironment env =
            new CompilationEnvironment(mProperties, mFileResolver, mMediaCache);
        return env;
    }
    
    /** Compiles <var>sourceFile</var> to <var>objectFile</var>.  If
     * compilation fails, <var>objectFile</var> is deleted.
     *
     * @param sourceFile source File
     * @param objectFile a File to place object code in
     * @param url request url, ignore if null
     * @throws CompilationError if an error occurs
     * @throws IOException if an error occurs
     */
    public Canvas compile(File sourceFile, File objectFile, String url)
        throws CompilationError, IOException
    {
        Properties props = new Properties();
        return compile(sourceFile, objectFile, props);
    }


    /** Compiles <var>sourceFile</var> to <var>objectFile</var>.  If
     * compilation fails, <var>objectFile</var> is deleted.
     *
     * @param sourceFile source File
     * @param objectFile a File to place object code in
     * @param props parameters for the compile
     * @throws CompilationError if an error occurs
     * @throws IOException if an error occurs

     * The  PROPS parameters for the compile may include
     * <ul>
     * <li> url, optional, ignore if null
     * <li> debug := "true" | "false"   include debugger
     * <li> logdebug := "true" | "false" makes debug.write() calls get logged to server
     * </ul>
     */
    public Canvas compile(File sourceFile, File objectFile, Properties props)
        throws CompilationError, IOException
    {
        FileUtils.makeFileAndParentDirs(objectFile);

        // Compile to a byte-array, and write out to objectFile, and 
        // write a copy into sourceFile.swf if this is a serverless deployment.
        ByteArrayOutputStream bstream = new ByteArrayOutputStream();
        OutputStream ostream = new FileOutputStream(objectFile);

        boolean success = false;
        try {
            Canvas canvas = compile(sourceFile, bstream, props);
            bstream.writeTo(ostream);
            ostream.close();

            // If the app is serverless, write out a .lzx.swf file when compiling
            if (!canvas.isProxied()) {
                OutputStream fostream = null;
                try {
                    // Create a foo.lzx.swf serverless deployment file for sourcefile foo.lzx
                    File deploymentFile = new File(sourceFile+".swf");
                    fostream = new FileOutputStream(deploymentFile);
                    bstream.writeTo(fostream);
                } finally {
                    if (fostream != null) {
                        fostream.close();
                    }
                }
            }
            success = true;
            return canvas;
        } catch (java.lang.OutOfMemoryError e) {
            // The runtime gc is necessary in order for subsequent
            // compiles to succeed.  The System gc is for good luck.
            System.gc();
            Runtime.getRuntime().gc();
            throw new CompilationError("out of memory");
        } finally {
            if (!success) {
                ostream.close();
                objectFile.delete();
            }
        }
    }

    /**
     * Compiles <var>file</var>, and write the bytes to
     * a stream.
     *
     * @param sourceFile a <code>File</code> value
     * @param ostr an <code>OutputStream</code> value
     * @param props parameters for the compilation
     * @exception CompilationError if an error occurs
     * @exception IOException if an error occurs
     *
     * The parameters currently being looked for in the PROPS arg
     * are "debug", "logdebug", "profile", "krank"
     */
    public Canvas compile(File file, OutputStream ostr, Properties props)
        throws CompilationError, IOException
    {
        mLogger.info("compiling " + file + "...");
        CompilationEnvironment env = makeCompilationEnvironment();
        CompilationErrorHandler errors = env.getErrorHandler();
        env.setApplicationFile(file);
        
        // Copy target properties (debug, logdebug, profile, krank,
        // runtime) from props arg to CompilationEnvironment
        String swfversion = props.getProperty(env.SWFVERSION_PROPERTY);
        
        if (swfversion != null) {
            mLogger.info("canvas compiler compiling runtime = " + swfversion);
            env.setProperty(env.SWFVERSION_PROPERTY, swfversion);
            if (! KNOWN_RUNTIMES.contains(swfversion)) {
                List runtimes = new Vector();
                for (Iterator iter = KNOWN_RUNTIMES.iterator();
                     iter.hasNext(); ) {
                    runtimes.add("\"" + iter.next() + "\"");
                }
                
                throw new CompilationError(
                    MessageFormat.format(
                        "Request for unknown runtime: The \"lzr\" query parameter has the value \"{0}\".  It must be {1}{2}.",
                        new String[] {
                            swfversion,
                            new ChoiceFormat(
                                "1#| 2#either | 2<one of ").
                            format(runtimes.size()),
                            new ListFormat("or").format(runtimes)
                        }));
            }
        }

        String proxied = props.getProperty(env.PROXIED_PROPERTY);
        mLogger.debug("looking for lzproxied, props= " + props.toString());
        if (proxied != null) {
            mLogger.debug("setting lzproxied to " + proxied);
            env.setProperty(env.PROXIED_PROPERTY, proxied);
        }


        String debug = props.getProperty(env.DEBUG_PROPERTY);
        if (debug != null) {
            env.setProperty(env.DEBUG_PROPERTY, debug);
        }

        String profile = props.getProperty(env.PROFILE_PROPERTY);
        if (profile != null) {
            env.setProperty(env.PROFILE_PROPERTY, profile);
        }

        String krank = props.getProperty(env.KRANK_PROPERTY);
        if (krank != null) {
            env.setProperty(env.KRANK_PROPERTY, krank);
        }

        String logdebug = props.getProperty(env.LOGDEBUG_PROPERTY);
        if (logdebug != null) {
            env.setProperty(env.LOGDEBUG_PROPERTY, logdebug);
        }

        String remotedebug = props.getProperty(env.REMOTEDEBUG_PROPERTY);
        if (remotedebug != null) {
            env.setProperty(env.REMOTEDEBUG_PROPERTY, remotedebug);
        }

        try {
            mLogger.debug("Parsing " + file.getAbsolutePath());

            Document doc = env.getParser().parse(file);
            Element root = doc.getRootElement();
            
            // Override passed in runtime target properties with the
            // canvas values.
            if ("true".equals(root.getAttributeValue("debug"))) {
                env.setProperty(env.DEBUG_PROPERTY, true);
            }

            if ("true".equals(root.getAttributeValue("profile"))) {
                env.setProperty(env.PROFILE_PROPERTY, true);
            }
            if ("false".equals(root.getAttributeValue("validate"))) {
                env.setProperty(env.VALIDATE_PROPERTY, false);
            }
            // Krank cannot be set in the canvas tag
            
            mLogger.debug("Making a writer...");
            
            try {
                Iterator i1 = Arrays.asList(Parser.sParseSpecials).iterator();
                String s = (String) i1.next();
                s = LPS.getTemplateDirectory() + File.separator +
                    (String) i1.next() + "-" + s + "." + (String) i1.next();
                Document d = new org.jdom.input.SAXBuilder().build(s);
                String en = (String) i1.next();
                String an = (String) i1.next();
                String av = (String) i1.next();
                for (Iterator iter = d.getRootElement().getChildren().iterator(); iter.hasNext(); ) {
                    Element e = (Element) iter.next();
                    if (e.getName().equals(en) &&
                        av.equals(e.getAttributeValue(an))) {
                        if ((e = e.getChild((String) i1.next())) != null) {
                            if ((e = e.getChild((String) i1.next())) != null)
                                env.setProperty(env.TEMPLATE_PROPERTY,
                                                e.getAttributeValue((String) i1.next()));
                        }
                    }
                }
            } catch (org.jdom.JDOMException e) {
                System.err.println(e);
                // errors are okay
            }
            
            // Initialize the schema from the base RELAX file
            try {
                env.getSchema().loadSchema();
            } catch (org.jdom.JDOMException e) {
                throw new ChainedException(e);
            }
            Compiler.updateSchema(doc.getRootElement(), env,
                                  env.getSchema(), new HashSet());
            if (SchemaLogger.isDebugEnabled()) {
                org.jdom.output.XMLOutputter xmloutputter =
                    new org.jdom.output.XMLOutputter();
                try {
                    SchemaLogger.debug(xmloutputter.outputString(env.getSchema().getSchemaDOM()));
                } catch (org.jdom.JDOMException e) {
                    throw new ChainedException(e);
                }
            }
            
            if (env.getBooleanProperty(env.VALIDATE_PROPERTY))
                Parser.validate(doc, file.getPath(), env);
            
            SWFWriter writer = new SWFWriter(env.getProperties(), ostr,
                                             mMediaCache, true, env);
            env.setSWFWriter(writer);
            mLogger.debug("new env..." + env.getProperties().toString());
            if (root.getName().intern() != "canvas") {
                throw new CompilationError("invalid root element type: " +
                                           root.getName(), root);
            }
            
            processCompilerInstructions(root, env);
            compileElement(root, env);
            ViewCompiler.checkUnresolvedResourceReferences (env);
            mLogger.debug("done...");
            // This isn't in a finally clause, because it won't generally
            // succeed if an error occurs.
            writer.close();
            
            Canvas canvas = env.getCanvas();
            if (!errors.isEmpty()) {
                if (canvas != null) {
                    canvas.setCompilationWarningText(
                        errors.toCompilationError().getMessage());
                    canvas.setCompilationWarningXML(
                        errors.toXML());
                }
                System.err.println(errors.toCompilationError().getMessage());
            }
            // set file path (relative to webapp) in canvas
            canvas.setFilePath(FileUtils.relativePath(file, LPS.HOME()));
            mLogger.info("done");
            return canvas;
        } catch (CompilationError e) {
            // TBD: e.initPathname(file.getPath());
            e.attachErrors(errors.getErrors());
            throw e;
            //errors.addError(new CompilationError(e.getMessage() + "; compilation aborted"));
            //throw errors.toCompilationError();
        } catch (org.openlaszlo.xml.internal.MissingAttributeException e) {
            /* The validator will have caught this, but if we simply
             * pass the error through, the vaildation warnings will
             * never be printed.  Create a new message that includes
             * them so that we get the source information. */
            errors.addError(new CompilationError(e.getMessage() + "; compilation aborted"));
            throw errors.toCompilationError();
        }
    }

    public void compileAndWriteToSWF (String script, String seqnum, OutputStream out, String swfversion) {
        try {
            CompilationEnvironment env = makeCompilationEnvironment();
            env.setProperty(env.DEBUG_PROPERTY, true);
            Properties props = (Properties) env.getProperties().clone();
            env.setProperty(env.SWFVERSION_PROPERTY, swfversion);
            byte[] action;

            // Try compiling as an expression first.  If that fails,
            // compile as sequence of statements.  If that fails too,
            // report the parse error.
            try {
                String prog;
                if (seqnum == null) {
                    prog = "with (_root) {" +
                        "Debug.displayResult(" + script + ")};\n";
                } else {
                    // it's a remote debug request, send a response to client
                    prog = "with (_root) { Debug.displayResult(_level0.__LzDebug.sockWriteAsXML("+script+","+seqnum+"))};\n";
                }
                prog += "_parent.loader.returnData( this );";
                action = ScriptCompiler.compileToByteArray(prog, props);
            } catch (org.openlaszlo.sc.parser.ParseException e) {
                try {
                    action = ScriptCompiler.compileToByteArray(
                        "function $evalee() {\n" +
                        "#pragma 'scriptElement'\n" +
                        "with (_root) { "+
                        CompilerUtils.sourceLocationDirective(null, new Integer(0), new Integer(0)) +
                        script+"\n"+
                        "if (Debug.remoteDebug) { _level0.__LzDebug.sockWriteAsXML(true,"+seqnum+");};\n" + 
                        "}};\n" +
                        "$evalee();\n" +
                        "_parent.loader.returnData( this )", props);
                } catch (org.openlaszlo.sc.parser.ParseException e2) {
                    //mLogger.info("not stmt: " + e);
                    action = ScriptCompiler.compileToByteArray(
                        "with (_root) { Debug.__write(" +
                        ScriptCompiler.quote("Parse error: "+ e2.getMessage()) + ") }\n"
                        + "_parent.loader.returnData( this )", props);
                }
            }

            ScriptCompiler.writeScriptToStream(action, out, LPS.getSWFVersionNum(swfversion));
            out.flush();
            out.close();
        } catch (IOException e) {
            mLogger.info("error compiling/writing script: "+script+" :" +e);
        }
    }



    static ElementCompiler getElementCompiler(Element element,
                                              CompilationEnvironment env) {
        if (CanvasCompiler.isElement(element)) {
            return new CanvasCompiler(env);
        } else if (ImportCompiler.isElement(element)) {
            return new ImportCompiler(env);
        } else if (LibraryCompiler.isElement(element)) {
            return new LibraryCompiler(env);
        } else if (ScriptElementCompiler.isElement(element)) {
            return new ScriptElementCompiler(env);
        } else if (SecurityCompiler.isElement(element)) {
            return new SecurityCompiler(env);
        } else if (SplashCompiler.isElement(element)) {
            return new SplashCompiler(env);
        } else if (FontCompiler.isElement(element)) {
            return new FontCompiler(env);
        } else if (ResourceCompiler.isElement(element)) {
            return new ResourceCompiler(env);
        } else if (ClassCompiler.isElement(element)) {
            return new ClassCompiler(env);
        } else if (DataCompiler.isElement(element)) {
            return new DataCompiler(env);
        } else if (DebugCompiler.isElement(element)) {
            return new DebugCompiler(env);
        } else if (SwitchCompiler.isElement(element)) {
            return new SwitchCompiler(env);
            // The following test tests true for everything, so call
            // it last.
        } else if (ViewCompiler.isElement(element)) {
            return new ViewCompiler(env);
        } else {
            throw new CompilationError("unknown tag: " + element.getName(),
                                       element);
        }
    }

    /**
     * Compile an XML element within the compilation environment.
     * Helper function for compile.
     *
     * @param element an <code>Element</code> value
     * @param env a <code>CompilationEnvironment</code> value
     * @param base a <code>String</code> value
     * @exception CompilationError if an error occurs
     */
    protected static void compileElement(Element element,
                                         CompilationEnvironment env)
        throws CompilationError
    {
        if (element.getAttributeValue("disabled") != null &&
            element.getAttributeValue("disabled").intern() == "true") {
            return;
        }
        try {
            ElementCompiler compiler = getElementCompiler(element, env);
            mLogger.debug("compiling element with " + compiler.getClass().toString());
            compiler.compile(element);
        } catch (CompilationError e) {
            // todo: wrap instead
            if (e.getElement() == null && e.getPathname() == null) {
                e.initElement(element);
            }
            throw e;
        }
    }

    static void updateSchema(Element element, CompilationEnvironment env,
                             ViewSchema schema, Set visited)
    {
        getElementCompiler(element, env).updateSchema(element, schema, visited);
    }

    static void importLibrary(File file, CompilationEnvironment env) {
        Element root = LibraryCompiler.resolveLibraryElement(
            file, env, env.getImportedLibraryFiles(), true);
        if (root != null) {
            compileElement(root, env);
        }
    }
    
    static void updateSchemaFromLibrary(File file, CompilationEnvironment env,
                                        ViewSchema schema, Set visited)
    {
        Element root = LibraryCompiler.resolveLibraryElement(
            file, env, visited, false);
        if (root != null) {
            Compiler.updateSchema(root, env, schema, visited);
        }
    }
    
    protected void processCompilerInstructions(Element element,
                                               CompilationEnvironment env) {
        for (Iterator iter = element.getContent().iterator();
             iter.hasNext(); ) {
            ProcessingInstruction pi;
            try {
                pi = (ProcessingInstruction) iter.next();
            } catch (ClassCastException e) {
                continue;
            }
            if (pi.getTarget().equals("lzc"))
                processCompilerInstruction(env, pi);
        }
    }
    
    protected void processCompilerInstruction(CompilationEnvironment env,
                                              ProcessingInstruction pi) {
        if (pi.getValue("class") != null)
            processClassInstruction(env, pi.getValue("class"), pi);
        if (pi.getValue("classes") != null) {
            for (Iterator iter = Arrays.asList(pi.getValue("classes").split("\\s+")).iterator(); iter.hasNext(); ) {
                processClassInstruction(env, (String) iter.next(), pi);
            }
        }
    }
    
    protected void processClassInstruction(CompilationEnvironment env,
                                           String className,
                                           ProcessingInstruction pi) {
        String inlineString = pi.getValue("inline-only");
        if (inlineString != null) {
            boolean inline = Boolean.valueOf(inlineString).booleanValue();
            ClassModel classModel = env.getSchema().getClassModel(className);
            if (classModel == null) {
                env.warn("A processor instruction refers to a class named \""+
                         className + "\".  No class with this name exists.");
                return;
            }
            classModel.setInline(inline);
        }
    }
}
