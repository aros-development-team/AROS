/* *****************************************************************************
 * Parser.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import java.io.*;
import java.lang.*;
import java.util.*;
import org.apache.log4j.Logger;
import org.iso_relax.verifier.*;
import org.jdom.Attribute;
import org.jdom.Document;
import org.jdom.Element;
import org.jdom.JDOMException;
import org.jdom.Namespace;
import org.jdom.input.JDOMFactory;
import org.jdom.input.SAXBuilder;
import org.jdom.input.SAXHandler;
import org.jdom.output.SAXOutputter;
import org.jdom.output.XMLOutputter;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.XMLFilter;
import org.xml.sax.helpers.XMLFilterImpl;
import org.apache.commons.collections.LRUMap;
import org.openlaszlo.server.*;
import org.openlaszlo.utils.*;
import org.openlaszlo.xml.internal.*;

import javax.xml.transform.TransformerConfigurationException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.sax.SAXResult;
import javax.xml.transform.sax.SAXTransformerFactory;
import javax.xml.transform.sax.TransformerHandler;
import javax.xml.transform.stream.StreamSource;

import org.xml.sax.SAXException; 

/** Parses and validates an XML file.  XML elements are annotated with
 * their source locations.
 *
 * A new parser should be used for each compilation, but shared across
 * all XML file reads within a compilation.  This assures that the
 * parser caches are active during compilation, but are not reused
 * for subsequent compilations when the file may have been modified.
 */
public class Parser {
    private static Logger mLogger = Logger.getLogger(Parser.class);
    private static Logger mPostTransformationLogger =
        Logger.getLogger("postTransformation");
    private static Logger mPreValidationLogger =
        Logger.getLogger("preValidation");
    static public Namespace sNamespace =
        Namespace.getNamespace("http://www.laszlosystems.com/2003/05/lzx");
    
    protected FileResolver resolver = FileResolver.DEFAULT_FILE_RESOLVER;
    
    /** Map(File, Document) */
    protected final Map fileCache = new HashMap();
    /** List<String>. Pathnames in messages are reported relative to
     * one of these. */
    List basePathnames = new Vector();

    static final String[] sParseSpecials = {
        "console", "app", "xslt", "template", "name", "tasks", "div", "img", "src"};
    
    // Stylesheet templates and generators for updating old
    // namespaces, and adding namespace declarations.
    private static javax.xml.transform.Templates sPreprocessorTemplates;
    private static SAXTransformerFactory sSaxFactory;
    private static long sPreprocessorLastModified;
    
    protected static synchronized SAXTransformerFactory getSaxFactory() {
        String stylePath = LPS.HOME().replace('\\', '/') + "/" +
            "WEB-INF" + "/" + 
            "lps" + "/" + 
            "schema"  + "/" + "preprocess.xsl";
        File styleFile = new File(stylePath);
        long lastModified = styleFile.lastModified();
        
        if (sSaxFactory != null && sPreprocessorLastModified == lastModified)
            return sSaxFactory;
        
        // name the class instead of using
        // TransformerFactory.newInstance(), to insure that we get
        // saxon and thereby work around a failure on Tomcat 5 w/ jdk
        // 1.4.2 Linux, and w/ Sun 1.4.1_05
        javax.xml.transform.TransformerFactory factory = 
            new com.icl.saxon.TransformerFactoryImpl();
        
        javax.xml.transform.Templates templates = null;
        try {
            templates = factory.newTemplates(
                new StreamSource("file:///" + stylePath));
        } catch (TransformerConfigurationException e) {
            throw new ChainedException(e);
        }
        
        if (!factory.getFeature(SAXTransformerFactory.FEATURE))
            throw new RuntimeException(
                "TransformerFactory doesn't implement SAXTransformerFactory");
        
        SAXTransformerFactory saxFactory = (SAXTransformerFactory) factory;
        
        sSaxFactory = saxFactory;
        sPreprocessorTemplates = templates;
        sPreprocessorLastModified = lastModified;
        return saxFactory;
    }
    
    public Parser() {
    }
    
    public void setResolver(FileResolver resolver) {
        this.resolver = resolver;
    }
    
    /** Returns the pathname to use in user error messages.  This is
     * the shortest pathname relative to a directory on the search
     * path for this application.
     */
    public String getUserPathname(String pathname) {
        String sourceDir = new File(pathname).getParent();
        if (sourceDir == null)
            sourceDir = "";
        sourceDir = sourceDir.replace(File.separatorChar, '/');
        String best = pathname.replace(File.separatorChar, '/');
        int bestLength = StringUtils.split(best, "/").length;
        for (Iterator iter = basePathnames.iterator(); iter.hasNext(); ) {
            String item = (String) iter.next();
            String base = item.replace(File.separatorChar, '/');
            try {
                String candidate = FileUtils.adjustRelativePath(
                    new File(pathname).getName(),
                    base,
                    sourceDir);
                int candidateLength =
                    StringUtils.split(candidate, "/").length;
                if (candidateLength < bestLength) {
                    best = candidate;
                    bestLength = candidateLength;
                }
            } catch (FileUtils.RelativizationError e) {
                // If it can't be relativized, it simply doesn't produce
                // a candidate, and we do nothing.
            }
        }
        return best;
    }
    
    /** Reads an XML document and adds source location information to
     * the elements. */
    protected Document read(File file)
        throws JDOMException, IOException
    {
        // See if we've already read the file.  This is an
        // optimization, and also assures that the same content is
        // used across passes.  We don't need to (and shouldn't) check
        // the date, since the cache is an instance variable, and each
        // compiler invocation uses a fresh parser.
        File key = file.getCanonicalFile();
        if (fileCache.containsKey(key)) {
            return (Document) fileCache.get(key);
        }
        
        // Use a custom subclass of SAXBuilder to build a JDOM tree
        // containing custom Elements which contain source file
        // location info (ElementWithLocationInfo).
        
        // The following variables are used to add source location
        // that reflects the input name, while the system identifier
        // has been made absolute.
        final String pathname = file.getPath();
        final String messagePathname = getUserPathname(pathname);

        // This is a ContentHandler which adds source location info to
        // our own custom class of jdom.Element
        class SourceLocatorHandler extends org.jdom.input.SAXHandler {
            org.xml.sax.Locator locator;
            int startLineNumber;
            int startColumnNumber;
            Element currentElement;
            
            SourceLocatorHandler() throws IOException {}
            
            SourceLocatorHandler(JDOMFactory factory)
                throws IOException
            {
                super(factory);
            }

            public void characters(char[] ch, int start, int length)
                throws SAXException
            {
                startLineNumber = locator.getLineNumber();
                startColumnNumber = locator.getColumnNumber();
                super.characters(ch, start, length);
            }

            public void endElement(String namespaceURI, String localName,
                                   String qName)
                throws SAXException
            {
                // You can only call this.getCurrentElement() before
                // super.endElement

                // Save source location info for reporting compilation errors
                saveEndLocation(this.getCurrentElement(),
                                pathname,
                                messagePathname,
                                locator.getLineNumber(),
                                locator.getColumnNumber());

                super.endElement(namespaceURI, localName, qName);
            }

            public void setDocumentLocator(org.xml.sax.Locator locator) {
                this.locator = locator;
            }

            public void startElement(String namespaceURI, String localName,
                                     String qName,
                                     org.xml.sax.Attributes atts)
                throws SAXException
            {
                super.startElement(namespaceURI, localName, qName, atts);
                    
                // You can only call this.getCurrentElement() after
                // super.startElement

                // Save source location info for reporting compilation errors
                saveStartLocation(this.getCurrentElement(),
                                  pathname,
                                  messagePathname,
                                  locator.getLineNumber(),
                                  locator.getColumnNumber());
            }
        }

        /* We need a SAXBuilder that uses our custom Factory and
           ContentHandler classes, but the stock
           org.jdom.input.SAXBuilder has no API for setting which
           ContentHandler is used.

           To get what we need, we create a subclass of SAXBuilder
           and override the createContentHandler method to
           instantiate our custom handler with our custom factory . 
        */
        class SourceLocatorSAXBuilder extends SAXBuilder {
            SourceLocatorSAXBuilder (String saxDriverClass) {
                super(saxDriverClass);
            }

            SourceLocatorSAXBuilder () {
                super();
            }

            // We need to create our own special contentHandler,
            // and you *must* pass the factory to the SaxHandler
            // constructor, or else you get a default JDOM
            // factory, which is not what you want!
            protected org.jdom.input.SAXHandler createContentHandler() {
                try {
                    return new SourceLocatorHandler(factory);
                } catch (IOException e) {
                    throw new ChainedException(e);
                }
            }
        }

        //SAXBuilder builder = new SourceLocatorSAXBuilder("org.apache.crimson.parser.XMLReaderImpl");
        SAXBuilder builder = new SourceLocatorSAXBuilder();
        builder.setFactory(new SourceLocatorJDOMFactory());

        // ignore DOCTYPE declarations TODO [2004-25-05 ows]: parse
        // entity references from internal declarations, and either
        // warn about external declarations or add them to the
        // dependency information.  If the latter, use a library to
        // cache and resolve non-file sources against a catalog file.
        builder.setEntityResolver(
            new org.xml.sax.helpers.DefaultHandler() {
                public InputSource resolveEntity(String publicId, String systemId) {
                    return new InputSource(new StringReader(""));
                }
            });
        
        // Parse the document
        java.io.Reader reader = FileUtils.makeXMLReaderForFile(
            pathname, "UTF-8");
        InputSource source = new InputSource(reader);
        source.setPublicId(messagePathname);
        source.setSystemId(pathname);
        Document doc = builder.build(source);
        reader.close();
        fileCache.put(key, doc);
        return doc;
    }
    
    protected Document preprocess(final Document sourceDoc)
        throws java.io.IOException, org.jdom.JDOMException
    {
        // Fills location information from the metasource attribute.
        class SourceLocatorHandler extends org.jdom.input.SAXHandler {
            SourceLocatorHandler() throws IOException {}

            SourceLocatorHandler(JDOMFactory factory)
                throws IOException
            {
                super(factory);
            }

            public void endElement(String namespaceURI, String localName,
                                   String qName)
                throws SAXException
            {
                ElementWithLocationInfo element =
                    (ElementWithLocationInfo) this.getCurrentElement();
                Attribute attr = element.getAttribute(
                    SourceLocatorSAXOutputter.SOURCEINFO_ATTRIBUTE_NAME);
                if (attr != null) {
                    SourceLocator locator = SourceLocator.fromString(attr.getValue());
                    element.initSourceLocator(locator);
                    element.removeAttribute(attr);
                }
                super.endElement(namespaceURI, localName, qName);
            }
        }

        // Create a transformer that implements the 'preprocess'
        // transformation.
        TransformerHandler handler;
        try {
            handler = getSaxFactory().
                newTransformerHandler(sPreprocessorTemplates);
        } catch (TransformerConfigurationException e) {
            throw new ChainedException(e);
        }
        
        SAXHandler resultHandler =
            new SourceLocatorHandler(new SourceLocatorJDOMFactory());
        SAXResult result =
            new javax.xml.transform.sax.SAXResult(resultHandler);
        handler.setResult(result);
        
        SourceLocatorSAXOutputter outputter = new SourceLocatorSAXOutputter();
        outputter.setWriteMetaData(true);
        outputter.setContentHandler(handler);
        outputter.output(sourceDoc);
        
        Document resultDoc = resultHandler.getDocument();
        
        if (mPostTransformationLogger.isDebugEnabled()) {
            org.jdom.output.XMLOutputter xmloutputter =
                new org.jdom.output.XMLOutputter();
            mPostTransformationLogger.debug(
                xmloutputter.outputString(resultDoc));
        }
        
        return resultDoc;
    }
    
    /** Reads the XML document, and modifies it by replacing each
     * include element by the root of the expanded document named by
     * the include's href attribute. Pathames are resolved relative to
     * the element's source location.  If a pathname resolves to the
     * current file or a file in the set that's passed as the second
     * argument, a compilation error is thrown.  A copy of the set of
     * pathnames that additionally includes the current file is passed
     * to each recursive call.  (Since the set has stacklike behavior,
     * a cons list would be appropriate, but I don't think Java has
     * one.)
     *
     * This is a helper function for parse(), which is factored out
     * so that expandIncludes can be apply it recursively to included
     * files. */
    protected Document readExpanded(File file, Set currentFiles)
        throws IOException, JDOMException
    {
        File key = file.getCanonicalFile();
        if (currentFiles.contains(key)) {
            throw new CompilationError(file + " includes itself.");
        }
        Set newCurrentFiles = new HashSet(currentFiles);
        newCurrentFiles.add(key);
        Document doc = read(file);
        Element root = doc.getRootElement();
        expandIncludes(root, newCurrentFiles);
        return doc;
    }
    
    /** Replaces include statements by the content of the included
     * file.
     *
     * This is a helper function for readExpanded, which is factored
     * out so that readExpanded can apply it recursively to included
     * files. */
    protected void expandIncludes(Element element, Set currentFiles)
        throws IOException, JDOMException
    {
        // Do this in two passes: collect the replacements, and then
        // make them.  This is necessary because it's impossible to
        // modify a list that's being iterated.
        //
        // Replacements is a map of Element -> Maybe Element. The key
        // is an Element instead of an index into the content, because
        // deletions will alter the index. Replacements could be a
        // list [(Element, Maybe Element)], but since there's no Pair
        // type the map gets to use prefab types.
        Map replacements = new HashMap();
        for (Iterator iter = element.getChildren().iterator();
             iter.hasNext(); ) {
            Element child = (Element) iter.next();
            if (child.getName().equals("include")) {
                String base = new File(getSourcePathname(element)).getParent();
                String type = XMLUtils.getAttributeValue(child, "type", "xml");
                String href = child.getAttributeValue("href");
                if (href == null) {
                    throw new CompilationError("The <include> element requires an \"href\" attribute.", child);
                }
                File target = resolver.resolve(href, base);
                // An include whose href is a directory implicitly
                // includes the library.lzx file in that directory.
                if (type.equals("xml") && target.isDirectory()) {
                    target = new File(target, "library.lzx");
                }
                if (type.equals("text")) {
                    replacements.put(child,
                                     new org.jdom.Text(
                                         FileUtils.readFileString(target, "UTF-8")));
                } else if (type.equals("xml")) {
                    // Pass the target, not the key, so that source
                    // location information is correct.
                    Document doc = read(target);
                    // If it's a top-level library, the compiler will
                    // process it during the compilation phase.  In
                    // that case change it to a <library href=""/>
                    // element, where the href is the <include>'s
                    // href, so that LibraryCompiler can resolve it.
                    //
                    // Otherwise replace the <include> element with
                    // the included file.
                    if (CompilerUtils.isAtToplevel(child) &&
                        LibraryCompiler.isElement(doc.getRootElement())) {
                        // Modify the existing element instead of
                        // creating a new one, so that source location
                        // information is preserved.
                        child.setName(doc.getRootElement().getName());
                    } else {
                        doc = readExpanded(target, currentFiles);
                        replacements.put(child, doc.getRootElement());
                        // The replacement below destroys this tree,
                        // so insure that the next call to read gets a
                        // fresh one.
                        File key = target.getCanonicalFile();
                        fileCache.remove(key);
                    }
                } else {
                    throw new CompilationError("include type must be xml or text");
                }
            } else {
                expandIncludes(child, currentFiles);
            }
        }
        // Now make the replacements.
        List contents = element.getContent();
        for (Iterator iter = replacements.entrySet().iterator();
             iter.hasNext(); ) {
            Map.Entry entry = (Map.Entry) iter.next();
            int index = contents.indexOf(entry.getKey());
            Object value = entry.getValue();
            contents.set(index, value);
        }
    }

    /** Reads an XML file, expands includes, and validates it.
     */
    public Document parse(File file)
      throws CompilationError
    {
        String pathname = file.getPath();
        try {
            Document doc = readExpanded(file, new HashSet());
            // Apply the stylesheet
            doc = preprocess(doc);
            return doc;
        } catch (IOException e) {
            CompilationError ce = new CompilationError(e);
            ce.initPathname(pathname);
            throw ce;
        } catch (JDOMException e) {
            String solution = SolutionMessages.findSolution(e.getMessage(), SolutionMessages.PARSER);
            CompilationError ce = new CompilationError(e, solution);
            throw ce;

        } 
    }
    
    /** Cache of compiled schema verifiers.  Type Map<String,
     * org.iso_relax.verifier.Schema>, where the key is the string
     * serialization of the schema.  */
    private static LRUMap mSchemaCache = new LRUMap();
    
    /** Validates an XML file against the Laszlo schema. Errors are
     * thrown as the text of a CompilationError.
     *
     * @param doc DOM of the source code to be validated
     * @param pathname pathname of the source file, for error reporting
     * @param env the compilation environment
     * @throws CompilationError if a validation error occurs
     */
    public static void validate(Document doc, String pathname,
                                final CompilationEnvironment env) 
    {
        final CompilationErrorHandler errorHandler = env.getErrorHandler();
        final ViewSchema viewschema = env.getSchema();

        if (mPreValidationLogger.isDebugEnabled()) {
            org.jdom.output.XMLOutputter debugOutputter =
                new org.jdom.output.XMLOutputter();
            mPreValidationLogger.debug("Pathname: " + pathname);
            mPreValidationLogger.debug(debugOutputter.outputString(doc));
        }
        try {
            org.iso_relax.verifier.Schema schema;

            // Look up a cached version of the compiled verifier for this env
            Verifier verifier = env.getCachedVerifier();

            if (verifier == null) {
                // System.err.println("env verifier cache miss: " + pathname);

                // Look up schema XML signature in common pool of compiled verifiers
                Object value;
                String schemaXMLstring = new XMLOutputter().outputString(viewschema.getSchemaDOM());
                String key = schemaXMLstring;

                synchronized (mSchemaCache) {
                    schema = (org.iso_relax.verifier.Schema) mSchemaCache.get(key);
                }
                
                if (schema == null) {
                    StringReader schemaXMLSource = new StringReader(schemaXMLstring);
                    InputSource schemaInputSource = new InputSource(schemaXMLSource);

                    // System.err.println("validator pool cache miss: " + pathname + " " + key.length());
                    VerifierFactory factory = VerifierFactory.newInstance(
                        "http://relaxng.org/ns/structure/1.0");
                    schema = factory.compileSchema(schemaInputSource);
                    synchronized (mSchemaCache) {
                        mSchemaCache.put(key, schema);
                    }
                } else {
                    // System.err.println("validator pool cache hit: " + pathname + " " + key.length());
                }
                
                // Cache this in CompilationEnvironment
                verifier = schema.newVerifier();
                env.setCachedVerifier(verifier);
            } else {
                // System.err.println("env verifier cache hit: " + pathname);
            }
            
            VerifierHandler handler = verifier.getVerifierHandler();

            verifier.setErrorHandler(
                new org.xml.sax.ErrorHandler() {
                        protected void reportError(org.xml.sax.SAXParseException e)
                        {
                            String msg = e.getMessage();
                            msg = StringUtils.replace(msg, " from namespace \"" + sNamespace.getURI() + "\"", "");
                            CompilationError cerr = new CompilationError(msg);
                            cerr.initPathname(e.getPublicId());
                            cerr.setLineNumber(e.getLineNumber());
                            cerr.setColumnNumber(e.getColumnNumber());
                            if (msg.endsWith("not allowed in this context"))
                                cerr.setSolution("Check whether it is spelled correctly, and whether a class with this name exists.");
                            // TODO [2003-3-17 hqm]: find and add more
                            // solution message here]
                            env.warn(cerr);
                        }
                        public void fatalError(org.xml.sax.SAXParseException e) {
                            reportError(e);
                        }
                        public void error(org.xml.sax.SAXParseException e) {
                            reportError(e);
                        }
                        public void warning(org.xml.sax.SAXParseException e) {
                            reportError(e);
                        }
                    }
                );
            
            /* Walk the DOM tree out as SAX events, as best we can. We
             * can't just use org.jdom.output.SAXOutputter because it
             * doesn't know how to convert the source location info from
             * our custom Elements into SAX Events. 
             */

            SourceLocatorSAXOutputter outputter =
                new SourceLocatorSAXOutputter();

            /* Sets our validator (event handler) to be the handler
             * for the SAX Outputter  */
            outputter.setContentHandler(handler);
            /* Feed the the source file DOM, via the SAX event
             * generator, to the validator. */
            outputter.output(doc);

        } catch (VerifierConfigurationException e) {
            throw new ChainedException(e);
        } catch (JDOMException e) {
            throw new ChainedException(e);
        } catch (StackOverflowError e) {
            env.warn("The validator had a fatal error, but proceeding with compilation", doc.getRootElement());
        } catch (SAXException e) {
            String solution = SolutionMessages.findSolution(e.getMessage(), SolutionMessages.PARSER);
            CompilationError err = new CompilationError(e, solution);
            err.setFileBase(errorHandler.fileBase);
            err.initPathname(pathname);
            throw err;
        } catch (IOException e) {
            throw new ChainedException(e);
        }
    }

    void saveStartLocation (Element elt,
                            String pathname,
                            String messagePathname,
                            int startLineNumber,
                            int startColumnNumber) {
        SourceLocator info = ((ElementWithLocationInfo) elt).locator;
        info.setPathname(pathname, messagePathname);
        info.startLineNumber   = startLineNumber;
        info.startColumnNumber = startColumnNumber;
    }

    void saveEndLocation (Element elt,
                          String pathname,
                          String messagePathname,
                          int endLineNumber,
                          int endColumnNumber) {
        SourceLocator info = ((ElementWithLocationInfo) elt).locator;
        info.setPathname(pathname, messagePathname);
        info.endLineNumber   = endLineNumber;
        info.endColumnNumber = endColumnNumber;
    }
    
    /* Implement source location, on top of metadata */
    static final int LINENO = 1;
    static final int COLNO = 2;
    
    static String getSourcePathname(Element elt) {
        SourceLocator info = ((ElementWithLocationInfo) elt).locator;
        return info.pathname;
    }

    static String getSourceMessagePathname(Element elt) {
        SourceLocator info = ((ElementWithLocationInfo) elt).locator;
        return info.messagePathname;
    }

    static Integer getSourceLocation(Element elt, int coord, boolean start) {
        if (elt == null) {
            return null;
            // +++ should we throw an error if elt == null?
        } 

        SourceLocator info = ((ElementWithLocationInfo) elt).locator;


        if (coord == LINENO) {
            return new Integer(start ? info.startLineNumber : info.endLineNumber);
        } else {
            return new Integer(start ? info.startColumnNumber : info.endColumnNumber);
        }
    }
    
    static Integer getSourceLocation(Element elt, int coord) {
        return getSourceLocation(elt, coord, true);
    }


    class SourceLocatorJDOMFactory extends org.jdom.input.DefaultJDOMFactory {
                
        public SourceLocatorJDOMFactory () {
            super();
        }

        public Element element(String name) {
            return new ElementWithLocationInfo(name);
        }
                
        public Element element(String name, Namespace namespace) {
            return new ElementWithLocationInfo(name, namespace);
        }

        public Element element(String name, String uri) {
            return new ElementWithLocationInfo(name, uri);
        }

        public Element element(String name, String prefix, String uri) {
            return new ElementWithLocationInfo(name, prefix, uri);
        }
    }
}
