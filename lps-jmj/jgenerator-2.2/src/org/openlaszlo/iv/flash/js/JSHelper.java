package org.openlaszlo.iv.flash.js;

import java.io.*;
import java.net.URL;
import java.net.MalformedURLException;
import java.util.*;
import java.lang.reflect.*;
import org.mozilla.javascript.*;
import org.mozilla.javascript.tools.ToolErrorReporter;


import org.openlaszlo.iv.flash.util.*;

/**
 * @author Norris Boyd
 */
public class JSHelper {

    private static Context init( String args[], PrintStream out, org.openlaszlo.iv.flash.context.Context genContext ) {
        Context cx = Context.enter();
        // Create the top-level scope object.
        global = getGlobal(genContext);
        global.setOut(out);
        errorReporter = new IVErrorReporter(false);
        cx.setErrorReporter(errorReporter);
        cx.setLanguageVersion(Context.VERSION_1_5);
        cx.setOptimizationLevel(-1);

        // define "arguments" array in the top-level object
        if( args == null ) args = new String[0];
        Object[] array = args;
        Scriptable argsObj = cx.newArray(global, args);
        global.defineProperty("arguments", argsObj,
                              ScriptableObject.DONTENUM);
        return cx;
    }

    /**
     *  Execute the given arguments
     */
    public static int execFile(String fileName, String args[], PrintStream out) {

        Context cx = init(args, out, null);

        processFile(cx, global, fileName);

        cx.exit();

        return exitCode;
    }

    /**
     *  Execute the given arguments
     */
    public static int execFile(String fileName, String args[], PrintStream out, org.openlaszlo.iv.flash.context.Context genContext) {

        Context cx = init(args, out, genContext);

        processFile(cx, global, fileName);

        cx.exit();

        return exitCode;
    }

    /**
     *  Execute the given arguments
     */
    public static int execString( String js_text, String args[],
                            PrintStream out, org.openlaszlo.iv.flash.context.Context genContext )
    {
        Context cx = init(args, out, genContext);

        StringReader in = new StringReader(js_text);
        String source = js_text.length()>20? js_text.substring(0,20)+"...": js_text;
        evaluateReader(cx, global, in, source, 1);

        cx.exit();
        return exitCode;
    }

    public static Global getGlobal() {
        return getGlobal(null);
    }

    public static Global getGlobal(org.openlaszlo.iv.flash.context.Context genContext) {
        if (global == null) {
            try {
                global = new Global(Context.enter(), genContext);
            } finally {
                Context.exit();
            }
        }
        if( global != null ) {
            global.genContext = genContext;
        }
        return global;
    }

    public static void processFile(Context cx, Scriptable scope,
                                   String filename)
    {
        Reader in = null;
        // Try filename first as URL
        try {
            URL url = new URL(filename);
            InputStream is = url.openStream();
            in = new BufferedReader(new InputStreamReader(is));
        }  catch (MalformedURLException mfex) {
            // fall through to try it as a file
            in = null;
        } catch (IOException ioex) {
            Context.reportError(IVErrorReporter.getMessage(
                "msg.couldnt.open.url", filename, ioex.toString()));
            exitCode = EXITCODE_FILE_NOT_FOUND;
            return;
        }

        if (in == null) {
            // Try filename as file
            try {
                in = new FileReader(filename);
                filename = new java.io.File(filename).getCanonicalPath();
            } catch (FileNotFoundException ex) {
                Context.reportError(IVErrorReporter.getMessage(
                    "msg.couldnt.open",
                    filename));
                exitCode = EXITCODE_FILE_NOT_FOUND;
                return;
            } catch (IOException ioe) {
                Log.logRB(Resource.JSERROR, ioe);
            }
        }
        // Here we evalute the entire contents of the file as
        // a script. Text is printed only if the print() function
        // is called.
        evaluateReader(cx, scope, in, filename, 1);
    }

    public static Object evaluateReader(Context cx, Scriptable scope,
                                        Reader in, String sourceName,
                                        int lineno)
    {
        Object result = cx.getUndefinedValue();
        try {
            result = cx.evaluateReader(scope, in, sourceName, lineno, null);
        } catch (WrappedException we) {
            Log.logRB(Resource.JSERROR, we);
        } catch (EcmaError ee) {
            String msg = IVErrorReporter.getMessage(
                "msg.uncaughtJSException", ee.toString());
            exitCode = EXITCODE_RUNTIME_ERROR;
            if (ee.getSourceName() != null) {
                Context.reportError(msg, ee.getSourceName(),
                                    ee.getLineNumber(),
                                    ee.getLineSource(),
                                    ee.getColumnNumber());
            } else {
                Context.reportError(msg);
            }
        } catch (EvaluatorException ee) {
            // Already printed message.
            exitCode = EXITCODE_RUNTIME_ERROR;
        } catch (JavaScriptException jse) {
            // Need to propagate ThreadDeath exceptions.
            Object value = jse.getValue();
            if (value instanceof ThreadDeath)
                throw (ThreadDeath) value;
            exitCode = EXITCODE_RUNTIME_ERROR;
            Context.reportError(IVErrorReporter.getMessage(
                "msg.uncaughtJSException",
                jse.getMessage()));
        } catch (IOException ioe) {
            Log.logRB(Resource.JSERROR, ioe);
        } finally {
            try {
                in.close();
            } catch (IOException ioe) {
                Log.logRB(Resource.JSERROR, ioe);
            }
        }
        return result;
    }

/*    public static Node parseReader(Context cx, Scriptable scope, Reader in, String sourceName, int lineno) {
        try {
            TokenStream ts = new TokenStream(in, scope, sourceName, lineno);
            IRFactory irf = new IRFactory(ts, scope);
            Parser p = new Parser(irf);
            Node tree = (Node) p.parse(ts);
            if( tree == null ) {
                System.out.println(tree.toStringTree());
                return tree;
            }
            return null;
        } catch (WrappedException we) {
            Log.logRB(Resource.JSERROR, we);
        } catch (EcmaError ee) {
            String msg = IVErrorReporter.getMessage(
                "msg.uncaughtJSException", ee.toString());
            exitCode = EXITCODE_RUNTIME_ERROR;
            if (ee.getSourceName() != null) {
                Context.reportError(msg, ee.getSourceName(),
                                    ee.getLineNumber(),
                                    ee.getLineSource(),
                                    ee.getColumnNumber());
            } else {
                Context.reportError(msg);
            }
        } catch (JavaScriptException jse) {
            // Need to propagate ThreadDeath exceptions.
            Object value = jse.getValue();
            if (value instanceof ThreadDeath)
                throw (ThreadDeath) value;
            exitCode = EXITCODE_RUNTIME_ERROR;
            Context.reportError(IVErrorReporter.getMessage(
                "msg.uncaughtJSException",
                jse.getMessage()));
        } catch (IOException ioe) {
            Log.logRB(Resource.JSERROR, ioe);
        } finally {
            try {
                in.close();
            } catch (IOException ioe) {
                Log.logRB(Resource.JSERROR, ioe);
            }
        }
        return null;
    }
*/
    public static ScriptableObject getScope() {
        return global;
    }

    public static InputStream getIn() {
        return Global.getInstance(getGlobal()).getIn();
    }

    public static void setIn(InputStream in) {
        Global.getInstance(getGlobal()).setIn(in);
    }

    public static PrintStream getOut() {
        return Global.getInstance(getGlobal()).getOut();
    }

    public static void setOut(PrintStream out) {
        Global.getInstance(getGlobal()).setOut(out);
    }

    public static PrintStream getErr() {
        return Global.getInstance(getGlobal()).getErr();
    }

    public static void setErr(PrintStream err) {
        Global.getInstance(getGlobal()).setErr(err);
    }

    static protected IVErrorReporter errorReporter;
    static protected Global global;
    static protected int exitCode = 0;
    static private final int EXITCODE_RUNTIME_ERROR = 3;
    static private final int EXITCODE_FILE_NOT_FOUND = 4;
    //static private DebugShell debugShell;
}

