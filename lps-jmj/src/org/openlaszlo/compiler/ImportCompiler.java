/* *****************************************************************************
 * ImportCompiler.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import java.io.*;
import java.util.*;
import org.jdom.Document;
import org.jdom.Element;
import org.openlaszlo.xml.internal.XMLUtils;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.sc.ScriptCompiler;
import org.openlaszlo.utils.ChainedException;
import org.apache.log4j.*;
/** Compiler for <code>import</code> elements.
 *
 * @author  Henry Minsky
 */
class ImportCompiler extends ToplevelCompiler {
    final static String HREF_ANAME = "href";
    final static String NAME_ANAME = "name";
    
    private static Logger mLogger  = Logger.getLogger(ImportCompiler.class);

    ImportCompiler(CompilationEnvironment env) {
        super(env);
    }

    static boolean isElement(Element element) {
        return element.getName().equals("import");
    }

    /** Return the difference of two strings.
     *  stringDiff("foo/bar/baz.lzx", "foo/bar") ==> /baz.lzx
     */
    static String stringDiff(String s1, String s2) {
        int sl1 = s1.length();
        int sl2 = s2.length();
        return s1.substring(sl2+1, sl1);
    }

    public void compile(Element element) throws CompilationError
    {
        String href = XMLUtils.requireAttributeValue(element, HREF_ANAME);
        String libname = XMLUtils.requireAttributeValue(element, NAME_ANAME);
        String stage = XMLUtils.requireAttributeValue(element, "stage");

        Element module = LibraryCompiler.resolveLibraryElement(
            element, mEnv, mEnv.getImportedLibraryFiles(),
            mEnv.getBooleanProperty(mEnv.VALIDATE_PROPERTY));
        if (module != null) {
            // check for conflict in the value of the "proxied"
            // attribute declared on the <import> tag vs the
            // <library> tag.

            String libproxied = module.getAttributeValue("proxied", "inherit");
            String importproxied = element.getAttributeValue("proxied", "inherit");

            if ((importproxied.equals("true") && libproxied.equals("false")) ||
                (importproxied.equals("false") && libproxied.equals("true"))){
                mEnv.warn("The value of the 'proxied' attribute on this library, '"+libproxied+"', conflicts with the import element value of '"+importproxied+"'", element);
            }
                
            // We're not compiling this into the current app, we're
            // building a separate binary library object file for it.
            File appdir = mEnv.getApplicationFile().getParentFile();
            File file = mEnv.resolveReference(element, HREF_ANAME);
            // If the filename is a directory "dir", we need to make it "dir/library.lzx"
            File libsrcfile = LibraryCompiler.resolveLibraryName(file);
            String adjustedhref = libsrcfile.getPath();

            if (appdir != null) {
                adjustedhref = FileUtils.relativePath(libsrcfile.getPath(), appdir.getPath());
            }


            // If we added a "library.lzx" to the path, set it properly in the XML
            element.setAttribute(HREF_ANAME, adjustedhref);

            // I'm scared of the CompilationManager, just generate the output file
            // directly for now.
            String libfile = libsrcfile.getName();
            String libprefix = mEnv.getLibPrefix();
            String objfilename = libprefix + File.separator + libfile + ".swf";
                
            mLogger.info("calling compilerLibrary libsrcfile="+libsrcfile+" objfile="+objfilename);

            try {
                FileUtils.makeFileAndParentDirs(new File(objfilename));
            } catch (java.io.IOException e) {
                throw new CompilationError(element, e);
            }

            compileLibrary(libsrcfile, objfilename, module);

            // Emit code into main app to instantiate a LzLibrary
            // object, which implements the load() method.
            ViewCompiler vc =  new ViewCompiler(mEnv);

            // Override the href with a pointer to the library object-file build directory
            String objpath = mEnv.getLibPrefixRelative() + File.separator + libfile + ".swf";
            element.setAttribute("href", objpath);
            vc.compile(element);
        }
    }

    void updateSchema(Element element, ViewSchema schema, Set visited) {
        element = LibraryCompiler.resolveLibraryElement(element, mEnv, visited, false);
        if (element != null) {
            super.updateSchema(element, schema, visited);
        }
    }

    /**
     * Compile a standalone binary library file with no canvas.
     */
    void compileLibrary(File infile, String outfile, Element element) throws CompilationError{
        // copy fields from mEnv to new Environment
        CompilationEnvironment env =  new CompilationEnvironment(mEnv);
        CompilationErrorHandler errors = env.getErrorHandler();
        env.setApplicationFile(infile);
        Properties props = (Properties) env.getProperties().clone();
        byte[] action;

        try {

            OutputStream ostream = new FileOutputStream(outfile);

            try {
                SWFWriter writer = new SWFWriter(env.getProperties(), ostream,
                                                 env.getMediaCache(), false, env);
                env.setSWFWriter(writer);
                // Set the main SWFWriter so we can output resources
                // to the main app
                env.setMainSWFWriter(mEnv.getGenerator());

                // TODO [hqm 2004-10-05] For now we'll never include fonts in the library file.
                // At some later date we can decide if we want to do that, and if so, we'll have to
                // make the compiler use a separate FontsCollector when it descends into the library.
                env.setEmbedFonts(false);

                // copy the fontmanager from old env to new one.
                writer.setFontManager(mEnv.getGenerator().getFontManager());
                writer.setCanvasDefaults(mEnv.getCanvas(), mEnv.getMediaCache());

                writer.openSnippet();

                // allows snippet code to call out to LzInstantiateView in the main app:
                // var LzInstantiateView = _level0.LzInstantiateView;
                env.compileScript("var "+VIEW_INSTANTIATION_FNAME+" = _level0."+VIEW_INSTANTIATION_FNAME, element);

                // TODO [hqm 2004-10-02] There ought to be an API call
                // to reset the LzInstantiateView machinery. In the
                // meantime we need to do this:
                //env.compileScript("_level0.canvas.initialsubviews = []", element);

                for (Iterator iter = element.getChildren().iterator();
                     iter.hasNext(); ) {
                    Element child = (Element) iter.next();
                    if (!NodeModel.isPropertyElement(child)) {
                        //mLogger.debug("snippet calling CompileElement "+child);
                        Compiler.compileElement(child, env);
                    }
                }

                ViewCompiler.checkUnresolvedResourceReferences (env);
                writer.closeSnippet();
            } finally {
                ostream.close();
            }

            /** Do something useful with any compilation errors/warnings

            See Compiler.compile()
            */

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
        } catch (IOException e) {
            throw new CompilationError(element, e);
        }

    }
}
