package org.openlaszlo.iv.flash.js;

import java.io.*;
import java.lang.reflect.*;
import org.mozilla.javascript.*;
import org.mozilla.javascript.serialize.*;
import org.mozilla.javascript.tools.shell.Environment;

/**
 * This class provides for sharing functions across multiple threads.
 * This is of particular interest to server applications.
 *
 * @author Norris Boyd
 * @author Dmitry Skavish
 */
public class Global extends ImporterTopLevel {

    public Global( Context cx ) {
        this(cx, null);
    }

    public Global( Context cx, org.openlaszlo.iv.flash.context.Context genContext ) {
        // Define some global functions particular to the shell. Note
        // that these functions are not part of ECMA.
        super(cx);

        this.genContext = genContext;

        String[] names = { "print", "println", "version", "load",
                           "loadClass", "defineClass", "spawn", "sync",
                           "serialize", "deserialize", "getVar", "setVar" };
        try {
            defineFunctionProperties(names, Global.class, ScriptableObject.DONTENUM);
        } catch (PropertyException e) {
            throw new Error();  // shouldn't occur.
        }
        defineProperty(privateName, this, ScriptableObject.DONTENUM);

        // Set up "environment" in the global scope to provide access to the
        // System environment variables.
        Environment.defineClass(this);
        Environment environment = new Environment(this);
        defineProperty("environment", environment,
                       ScriptableObject.DONTENUM);

        history = (NativeArray) cx.newArray(this, 0);
        defineProperty("history", history, ScriptableObject.DONTENUM);
    }

    /**
     * Print the string values of its arguments.
     *
     * This method is defined as a JavaScript function.
     * Note that its arguments are of the "varargs" form, which
     * allows it to handle an arbitrary number of arguments
     * supplied to the JavaScript function.
     *
     */
    public static Object print(Context cx, Scriptable thisObj,
                               Object[] args, Function funObj)
    {
        PrintStream out = getInstance(thisObj).getOut();
        for (int i=0; i < args.length; i++) {
            // Convert the arbitrary JavaScript value into a string form.
            String s = Context.toString(args[i]);
            out.print(s);
        }
        return Context.getUndefinedValue();
    }

    /**
     * Println the string values of its arguments.
     *
     * This method is defined as a JavaScript function.
     * Note that its arguments are of the "varargs" form, which
     * allows it to handle an arbitrary number of arguments
     * supplied to the JavaScript function.
     *
     */
    public static Object println(Context cx, Scriptable thisObj,
                               Object[] args, Function funObj)
    {
        PrintStream out = getInstance(thisObj).getOut();
        for (int i=0; i < args.length; i++) {
            // Convert the arbitrary JavaScript value into a string form.
            String s = Context.toString(args[i]);
            out.print(s);
        }
        out.println();
        return Context.getUndefinedValue();
    }

    /**
     * Returns variable from associated generator context
     */
    public static Object getVar(Context cx, Scriptable thisObj, Object[] args, Function funObj) {
        org.openlaszlo.iv.flash.context.Context myContext = (org.openlaszlo.iv.flash.context.Context) getInstance(thisObj).genContext;
        //System.out.println("getVar: myContext="+myContext+", args.length="+args.length);
        if( args.length > 0 && myContext != null ) {
            String value = myContext.getValue(Context.toString(args[0]));
            if( value == null ) {
                return Context.getUndefinedValue();
            } else {
                return value;
            }
        } else {
            return Context.getUndefinedValue();
        }
    }

    /**
     * Sets variable to associated standard generator context
     */
    public static Object setVar(Context cx, Scriptable thisObj, Object[] args, Function funObj) {
        org.openlaszlo.iv.flash.context.Context myContext = (org.openlaszlo.iv.flash.context.Context) getInstance(thisObj).genContext;
        if( args.length > 1 && myContext instanceof org.openlaszlo.iv.flash.context.StandardContext ) {
            org.openlaszlo.iv.flash.context.StandardContext myContext1 = (org.openlaszlo.iv.flash.context.StandardContext) myContext;
            String name = Context.toString(args[0]);
            String value = Context.toString(args[1]);
            myContext1.setValue(name, value);
        }
        return Context.getUndefinedValue();
    }

    /**
     * Get and set the language version.
     *
     * This method is defined as a JavaScript function.
     */
    public static double version(Context cx, Scriptable thisObj,
                                 Object[] args, Function funObj)
    {
        double result = (double) cx.getLanguageVersion();
        if (args.length > 0) {
            double d = cx.toNumber(args[0]);
            cx.setLanguageVersion((int) d);
        }
        return result;
    }

    /**
     * Load and execute a set of JavaScript source files.
     *
     * This method is defined as a JavaScript function.
     *
     */
    public static void load(Context cx, Scriptable thisObj,
                            Object[] args, Function funObj)
    {
        for (int i=0; i < args.length; i++) {
            JSHelper.processFile(cx, thisObj, cx.toString(args[i]));
        }
    }

    /**
     * Load a Java class that defines a JavaScript object using the
     * conventions outlined in ScriptableObject.defineClass.
     * <p>
     * This method is defined as a JavaScript function.
     * @exception IllegalAccessException if access is not available
     *            to a reflected class member
     * @exception InstantiationException if unable to instantiate
     *            the named class
     * @exception InvocationTargetException if an exception is thrown
     *            during execution of methods of the named class
     * @exception ClassDefinitionException if the format of the
     *            class causes this exception in ScriptableObject.defineClass
     * @exception PropertyException if the format of the
     *            class causes this exception in ScriptableObject.defineClass
     * @see org.mozilla.javascript.ScriptableObject#defineClass
     */
    public static void defineClass(Context cx, Scriptable thisObj,
                                   Object[] args, Function funObj)
        throws IllegalAccessException, InstantiationException,
               InvocationTargetException, ClassDefinitionException,
               PropertyException
    {
        Class clazz = getClass(args);
        ScriptableObject.defineClass(thisObj, clazz);
    }

    /**
     * Load and execute a script compiled to a class file.
     * <p>
     * This method is defined as a JavaScript function.
     * When called as a JavaScript function, a single argument is
     * expected. This argument should be the name of a class that
     * implements the Script interface, as will any script
     * compiled by jsc.
     *
     * @exception IllegalAccessException if access is not available
     *            to the class
     * @exception InstantiationException if unable to instantiate
     *            the named class
     * @exception InvocationTargetException if an exception is thrown
     *            during execution of methods of the named class
     * @exception JavaScriptException if a JavaScript exception is thrown
     *            during execution of the compiled script
     * @see org.mozilla.javascript.ScriptableObject#defineClass
     */
    public static void loadClass(Context cx, Scriptable thisObj,
                                 Object[] args, Function funObj)
        throws IllegalAccessException, InstantiationException,
               InvocationTargetException, JavaScriptException
    {
        Class clazz = getClass(args);
        if (!Script.class.isAssignableFrom(clazz)) {
            throw Context.reportRuntimeError(IVErrorReporter.getMessage(
                "msg.must.implement.Script"));
        }
        Script script = (Script) clazz.newInstance();
        script.exec(cx, thisObj);
    }

    private static Class getClass(Object[] args)
        throws IllegalAccessException, InstantiationException,
               InvocationTargetException
    {
        if (args.length == 0) {
            throw Context.reportRuntimeError(IVErrorReporter.getMessage(
                "msg.expected.string.arg"));
        }
        String className = Context.toString(args[0]);
        try {
            return Class.forName(className);
        }
        catch (ClassNotFoundException cnfe) {
            throw Context.reportRuntimeError(IVErrorReporter.getMessage(
                "msg.class.not.found",
                className));
        }
    }

    public static void serialize(Context cx, Scriptable thisObj,
                                 Object[] args, Function funObj)
        throws IOException
    {
        if (args.length < 2) {
            throw Context.reportRuntimeError(
                "Expected an object to serialize and a filename to write " +
                "the serialization to");
        }
        Object obj = args[0];
        String filename = cx.toString(args[1]);
        FileOutputStream fos = new FileOutputStream(filename);
        Scriptable scope = ScriptableObject.getTopLevelScope(thisObj);
        ScriptableOutputStream out = new ScriptableOutputStream(fos, scope);
        out.writeObject(obj);
        out.close();
    }

    public static Object deserialize(Context cx, Scriptable thisObj,
                                     Object[] args, Function funObj)
        throws IOException, ClassNotFoundException
    {
        if (args.length < 1) {
            throw Context.reportRuntimeError(
                "Expected a filename to read the serialization from");
        }
        String filename = cx.toString(args[0]);
        FileInputStream fis = new FileInputStream(filename);
        Scriptable scope = ScriptableObject.getTopLevelScope(thisObj);
        ObjectInputStream in = new ScriptableInputStream(fis, scope);
        Object deserialized = in.readObject();
        in.close();
        return cx.toObject(deserialized, scope);
    }

    /**
     * The spawn function runs a given function or script in a different
     * thread.
     *
     * js> function g() { a = 7; }
     * js> a = 3;
     * 3
     * js> spawn(g)
     * Thread[Thread-1,5,main]
     * js> a
     * 3
     */
    public static Object spawn(Context cx, Scriptable thisObj, Object[] args,
                               Function funObj)
    {
        Scriptable scope = funObj.getParentScope();
        Runner runner;
        if (args.length != 0 && args[0] instanceof Function) {
            Object[] newArgs = null;
            if (args.length > 1 && args[1] instanceof Scriptable) {
                newArgs = cx.getElements((Scriptable) args[1]);
            }
            runner = new Runner(scope, (Function) args[0], newArgs);
        } else if (args.length != 0 && args[0] instanceof Script) {
            runner = new Runner(scope, (Script) args[0]);
        } else {
            throw Context.reportRuntimeError(IVErrorReporter.getMessage(
                "msg.spawn.args"));
        }
        Thread thread = new Thread(runner);
        thread.start();
        return thread;
    }

    /**
     * The sync function creates a synchronized function (in the sense
     * of a Java synchronized method) from an existing function. The
     * new function synchronizes on the <code>this</code> object of
     * its invocation.
     * js> var o = { f : sync(function(x) {
     *       print("entry");
     *       Packages.java.lang.Thread.sleep(x*1000);
     *       print("exit");
     *     })};
     * js> spawn(function() {o.f(5);});
     * Thread[Thread-0,5,main]
     * entry
     * js> spawn(function() {o.f(5);});
     * Thread[Thread-1,5,main]
     * js>
     * exit
     * entry
     * exit
     */
    public static Object sync(Context cx, Scriptable thisObj, Object[] args,
                               Function funObj)
    {
        if (args.length == 1 && args[0] instanceof Function) {
            return new Synchronizer((Function)args[0]);
        } else {
            throw Context.reportRuntimeError(IVErrorReporter.getMessage(
                "msg.spawn.args"));
        }
    }

    public InputStream getIn() {
        return inStream == null ? System.in : inStream;
    }

    public void setIn(InputStream in) {
        inStream = in;
    }

    public PrintStream getOut() {
        return outStream == null ? System.out : outStream;
    }

    public void setOut(PrintStream out) {
        outStream = out;
    }

    public PrintStream getErr() {
        return errStream == null ? System.err : errStream;
    }

    public void setErr(PrintStream err) {
        errStream = err;
    }

    static final String privateName = "org.openlaszlo.iv.flash.js.Global private";

    public static Global getInstance(Scriptable scope) {
        Object v = ScriptableObject.getProperty(scope,privateName);
        if (v instanceof Global)
            return (Global) v;
        return null;
    }

    NativeArray history;
    public InputStream inStream;
    public PrintStream outStream;
    public PrintStream errStream;
    public org.openlaszlo.iv.flash.context.Context genContext;
}

class Runner implements Runnable {

    Runner(Scriptable scope, Function func, Object[] args) {
        this.scope = scope;
        f = func;
        this.args = args;
    }

    Runner(Scriptable scope, Script script) {
        this.scope = scope;
        s = script;
    }

    public void run() {
        Context cx = Context.enter();

        try {
            if (f != null)
                f.call(cx, scope, scope, args);
            else
                s.exec(cx, scope);
        } catch (JavaScriptException e) {
            Context.reportError(IVErrorReporter.getMessage(
                "msg.uncaughtJSException",
                e.getMessage()));
        }

        cx.exit();
    }

    private Scriptable scope;
    private Function f;
    private Script s;
    private Object[] args;
}

