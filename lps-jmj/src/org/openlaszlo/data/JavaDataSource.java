/* ****************************************************************************
 * JavaDataSource.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

/*
 * The Apache Software License, Version 1.1
 *
 *
 * Copyright(c) 2002 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation(http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "XML-RPC" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */
package org.openlaszlo.data;

import java.io.*;
import java.lang.reflect.*;
import java.util.*;
import java.net.MalformedURLException;
import javax.servlet.http.*;
import org.apache.xmlrpc.*;
import org.openlaszlo.server.LPS;
import org.openlaszlo.server.Option;
import org.openlaszlo.xml.*;
import org.openlaszlo.xml.internal.*;
import org.openlaszlo.utils.*;
import org.openlaszlo.remote.*;
import org.openlaszlo.media.MimeType;
import org.apache.log4j.*;

/**
 * Data source for java direct calls.
 */
public class JavaDataSource extends DataSource
{
    private static Logger mLogger  = Logger.getLogger(JavaDataSource.class);

    private static String OBJECTS = "__lzobj";

    HashMap mInstancePrototypes = new HashMap();
    HashMap mStaticPrototypes = new HashMap();

    static final String UNKNOWN = "unknown";
    static final String SESSION = "session";
    static final String WEBAPP  = "webapp";
    static final String NONE    = "none";

    static final int SCOPE_UNKNOWN = -1;
    static final int SCOPE_SESSION = 0;
    static final int SCOPE_WEBAPP  = 1;
    static final int SCOPE_NONE  = 2;

    HashMap mVoidMap = null;
    ObjectData DEFAULT_VOID;

    public JavaDataSource() {
        try {
            DEFAULT_VOID = new ObjectData
                (XMLRPCCompiler.compileResponse(0, "void", 
                                                LPS.mSWFVersionNumDefault));
        } catch (IOException e) {
            mLogger.error("exception: " + e.getMessage(), e);
            throw new RuntimeException(e.getMessage());
        }
    }

    /**
     * @return unique name of this data source
     */
    public String name() 
    {
        return "java";
    }

    /**
     * Sends system information to client.
     * 
     * @throws DataSourceException if there was a problem retrieving or sending
     * the data.
     */
    public Data getData(String app, HttpServletRequest req, 
                        HttpServletResponse res, long lastModifiedTime)
        throws DataSourceException {
        mLogger.debug("getData");

        int swfversion = LPS.getSWFVersionNum(req);

        if (! req.getMethod().equals("POST"))
            return compileFault("request must be POST", swfversion);

        String url;
        try {
            url = getURL(req);
        } catch (MalformedURLException e) {
            return compileFault("malformed url", e, swfversion);
        }

        String cname = getClassName(url);
        if (cname == null)
            return compileFault("invalid class or bad url: " + url, swfversion);

        if (! isClassOk(cname, req.getServletPath())) 
            return compileFault("forbidden class: " + cname, swfversion);

        Class targetClass = getClass(cname);
        if (targetClass == null)
            return compileFault("no such class " + cname, swfversion);

        int scope = getScope(req);
        if (scope == SCOPE_UNKNOWN)
            return compileFault("no scope request parameter", swfversion);

        String postbody = req.getParameter("lzpostbody");
        if (postbody == null || postbody.equals(""))
            return compileFault("no post body", swfversion);

        // one of 'pojo' or 'javabean'
        String objectReturnType = req.getParameter("objectreturntype");
        if (objectReturnType == null || "".equals(objectReturnType)) {
            objectReturnType = "pojo";
        }

        if (mLogger.isDebugEnabled()) {
            mLogger.debug("class name: " + cname);
            mLogger.debug("object return type: " + objectReturnType);
            mLogger.debug("POST body:\n" + postbody);
        }

        XmlRpcRequest xr = new LZXmlRpcRequestProcessor()
            .processRequest(new ByteArrayInputStream(postbody.getBytes()));

        try {
            return execute(req, res, targetClass, scope, objectReturnType,
                           swfversion, xr.getMethodName(), xr.getParameters());
        } catch (IOException e) {
            return compileFault("exception executing " + xr.getMethodName(), e,
                                swfversion);
        }
    }

    /**
     * Check to see if class is allowed.
     */
    public boolean isClassOk(String cname, String path) {
        return LPS.configuration.optionAllows(path, "security", cname, false);
    }

    /** 
     * This code borrows from Apache's XMLRPC
     * org.apache.xmlrpc.Invoker.execute() method.
     */
    public Data execute(HttpServletRequest req, HttpServletResponse res, 
                        Class targetClass, int scope, String objectReturnType, 
                        int swfversion, String methodName, 
                        Vector params) throws IOException {

        if (mLogger.isDebugEnabled()) {
            StringBuffer p = new StringBuffer(" ");
            for (int i=0; i < params.size(); i++) {
                p.append(params.get(i)).append(" ");
            }
            mLogger.debug("execute(" + methodName
                          + ", [" + p.toString() + "],context)");
        }

        Class[] argClasses = null;
        Object[] argValues = null;
        
        String _doreq = req.getParameter("doreq");
        String _dores = req.getParameter("dores");
        boolean doreq = (_doreq != null && _doreq.equals("1"));
        boolean dores = (_dores != null && _dores.equals("1"));

        int reqPos    = -1; // position of request in parameter list
        int resPos    = -1; // position of response in parameter list 

        int httpCount = 0;  // count of request and response passed in

        int paramCount = (params != null ? params.size() : 0);

        // if request exists, place request following the last parameter
        if (doreq) {
            mLogger.debug("adding request to method");
            reqPos = paramCount;
            httpCount++;
        }

        // if response exists, place response after it, else
        // place it following the last parameter
        if (dores) {
            mLogger.debug("adding response to method");
            resPos = (reqPos != -1 ? reqPos + 1 : paramCount);
            httpCount++;
        }

        // create arrays for argument classes and values.
        int count = paramCount + httpCount;
        argClasses = new Class[count];
        argValues = new Object[count];

        if (doreq) {
            argClasses[reqPos] = HttpServletRequest.class;
            argValues[reqPos] = req;
        }

        if (dores) {
            argClasses[resPos] = HttpServletResponse.class;
            argValues[resPos] = res;
        }

        if (params != null) {
            for (int i = 0; i < params.size(); i++) {
                argValues[i] = params.elementAt(i);
                if (argValues[i] instanceof Integer) {
                    argClasses[i] = Integer.TYPE;
                } else if (argValues[i] instanceof Double) {
                    argClasses[i] = Double.TYPE;
                } else if (argValues[i] instanceof Boolean) {
                    argClasses[i] = Boolean.TYPE;
                } else {
                    argClasses[i] = argValues[i].getClass();
                }
            }
        }

        Method method = null;
        Object invokeTarget = targetClass;

        String op = req.getParameter("op");
        if (op != null) {

            String cname = targetClass.getName();
            String oname = req.getParameter("oname");
            if (scope == SCOPE_NONE) oname= NONE;
            if (oname == null || "".equals(oname))
                return compileFault("no oname parameter", swfversion);

            if (scope == SCOPE_NONE) {
                if ("get".equals(op) || "create".equals(op)) {
                    return getStaticPrototype(cname, swfversion);
                }
            }

            if ( scope == SCOPE_SESSION || scope == SCOPE_WEBAPP ) {

                if ("get".equals(op)) {
                    return getInstancePrototype(cname, oname, scope, req, 
                                                swfversion);
                }

                if ("create".equals(op)) {
                    return createInstancePrototype(cname, oname, scope, 
                                                   argClasses, argValues, req, 
                                                   swfversion);
                }

                if ("destroy".equals(op)) {
                    return destroyInstance(oname, scope, req, swfversion);
                }

                if ("invoke".equals(op)) {
                    invokeTarget = getInvokeTarget(oname, scope, req);
                    if (invokeTarget == null) {
                        return compileFault("could not find " + getScopeName(scope) 
                                            + " instance " + oname, swfversion);
                    }
                }
            }
        }

        // The last element of the XML-RPC method name is the Java
        // method name.
        int dot = methodName.lastIndexOf('.');
        if (dot > -1 && dot + 1 < methodName.length()) {
            methodName = methodName.substring(dot + 1);
        }

        if (mLogger.isDebugEnabled()) {
            mLogger.debug("Searching for method: " + methodName +
                          " in class " + targetClass.getName());
            if (argClasses.length != 0) {
                for (int i = 0; i < argClasses.length; i++) {
                    mLogger.debug("Parameter " + i + ": " + argValues[i]
                                  + " (" + argClasses[i] + ')');
                }
            }
        }

        try {
            method = getMethod(targetClass, methodName, argClasses);
        } catch(NoSuchMethodException e) {
            mLogger.error("NoSuchMethodException", e);
            return compileFault(e.getMessage(), e, swfversion);
        } catch(SecurityException e) {
            mLogger.error("SecurityException", e);
            return compileFault(e.getMessage(), e, swfversion);
        }

        // Make all public methods callable except those defined in
        // java.lang.Object.
        if (method.getDeclaringClass() == Object.class) {
            return compileFault("Can't call methods in java.lang.Object", swfversion);
        }

        // invoke
        Object returnValue = null;
        try {
            returnValue = method.invoke(invokeTarget, argValues);
        } catch (IllegalAccessException e) {
            mLogger.error("IllegalAccessException", e);
            return compileFault(e.getMessage(), e, swfversion);
        } catch (IllegalArgumentException e) {
            mLogger.error("IllegalArgumentException", e);
            return compileFault(e.getMessage(), e, swfversion);
        } catch (InvocationTargetException e) {
            mLogger.error("InvocationTargetException", e);
            Throwable cause = e.getCause();
            if (cause != null) return compileFault(cause, swfversion);
            return compileFault(e.getMessage(), e, swfversion);
        } catch (Exception e) {
            mLogger.error("Exception", e);
            return compileFault(e.getMessage(), e, swfversion);
        } catch (Error e) {
            mLogger.error("Error", e);
            return compileFault(e.getMessage(), e, swfversion);
        }

        try {
            Class returnType = method.getReturnType();
            if (returnType == Void.TYPE) {
                return getVoid(swfversion);
            } else if (returnType == DataEncoder.class && returnValue != null) {
                return new EncoderData((DataEncoder)returnValue);
            }

            return new ObjectData
                (LZReturnObject.createObject(returnValue, objectReturnType, 
                                             swfversion));
        } catch (IOException e) {
            mLogger.error("IOException", e);
            return compileFault(e.getMessage(), e, swfversion);
        }
    }


    synchronized ObjectData getVoid(int swfversion) {
        if (mVoidMap == null) {
            mVoidMap = new HashMap();
        }
        ObjectData voidobj;
        try {
            Integer _swfversion = new Integer(swfversion);
            voidobj = (ObjectData)mVoidMap.get(_swfversion);
            if (voidobj == null) {
                mLogger.debug("creating void for swf " + swfversion);
                voidobj = new ObjectData
                    (XMLRPCCompiler.compileResponse(0, "void", swfversion));
                mVoidMap.put(_swfversion, voidobj);
            }
        } catch (Exception e) {
            mLogger.warn("using default void", e);
            voidobj = DEFAULT_VOID;
        }
        return voidobj;
    }

    /**
     * for scope == SCOPE_NONE (static objects).
     */
    Data getStaticPrototype(String cname, int swfversion) {
        if (mLogger.isDebugEnabled()) {
            mLogger.debug("getStaticPrototype(" + cname + ")");
        }
        return getPrototype(cname, SCOPE_NONE, swfversion);
    }


    /**
     * Return prototype for session and webapp if object was
     * previously created.
     * Note: swfversion not used but may be if we have different versions of
     * object cached.
     */
    //--------------------------------------------------------------------
    // TODO [2005-02-16 pkang]: think about what happens if swf version
    // requested is different from the instance prototype we have cached.
    //--------------------------------------------------------------------
    Data getInstancePrototype(String cname, String oname, int scope, 
                              HttpServletRequest req, int swfversion) {

        if (mLogger.isDebugEnabled()) {
            mLogger.debug("getInstancePrototype(" + cname + "," + oname + ")");
        }

        if (getJavaObject(req.getSession(false), scope, oname) != null) {
            return (Data)mInstancePrototypes.get(cname);
        }
        return compileFault("could not find " + getScopeName(scope) +
                            " instance " + oname, swfversion);
    }


    /**
     * creates a client swf object (remoteobject create)
     */
    Data createInstancePrototype(String cname, String oname, int scope, 
                                 Class[] argClasses, Object[] argValues,
                                 HttpServletRequest req, int swfversion) {

        if (mLogger.isDebugEnabled()) {
            mLogger.debug("createInstancePrototype(" + cname + "," + oname + ")");
        }

        // create session here, if it doesn't exist
        HttpSession session = req.getSession(true);

        String _clobber = req.getParameter("clobber");
        boolean clobber = (_clobber != null && "1".equals(_clobber));

        Object o = null;
        if (! clobber ) {
            o = getJavaObject(session, scope, oname);
            if (o != null) {
                if (mLogger.isDebugEnabled()) {
                    mLogger.debug("not clobbering existing " +
                                  getScopeName(scope) +
                                  " object " + oname );
                }
                return getPrototype(cname, scope, swfversion);
            }
        }

        // send back client object.
        try {
            o = getServerInstance(cname, argClasses, argValues);
            setJavaObject(session, scope, oname, o);
        } catch (Exception e) {
            return compileFault("could not create instance of " + cname, e,
                                swfversion);
        }

        return getPrototype(cname, scope, swfversion);
    }


    /**
     * Get client prototype.
     */
    synchronized Data getPrototype(String cname, int scope, int swfversion) {
        try {
            Data data;
            if (scope == SCOPE_NONE) {
                data = (Data)mStaticPrototypes.get(cname);
            } else {
                data = (Data)mInstancePrototypes.get(cname);
            }

            if (data == null) {
                mLogger.debug("creating client prototype for " + cname);
                byte[] b = LZClientObject.createObject(cname, getScopeName(scope),
                                                       swfversion);
                data = new ObjectData(b);
                if (scope == SCOPE_NONE) {
                    mStaticPrototypes.put(cname, data);
                } else {
                    mInstancePrototypes.put(cname, data);
                }
            }

            return data;
        } catch (Exception e) {
            return compileFault("could not get " + cname +
                                " in scope " + getScopeName(scope), e, 
                                swfversion);
        }
    }



    /**
     * removes client swf object from session attribute or webapp attribute.
     * Note: swfversion not used but may be if we have different versions of
     * object cached.
     */
    //--------------------------------------------------------------------
    // TODO [2005-02-16 pkang]: think about what happens if swf version
    // requested is different from the instance prototype we have cached.
    //--------------------------------------------------------------------
    Data destroyInstance(String oname, int scope, HttpServletRequest req, 
                         int swfversion) {

        if (mLogger.isDebugEnabled()) {
            mLogger.debug("destroyInstance(" + oname + ", " + scope);
        }
        removeJavaObject(req.getSession(false), scope, oname);
        return getVoid(swfversion);
    }

    /*
     * Get the invocation target for the session or webapp scope.
     */
    Object getInvokeTarget(String oname, int scope, HttpServletRequest req) {
        return getJavaObject(req.getSession(false), scope, oname);
    }


    String getClassName(String url) {
        if (url.startsWith("java://")) return url.substring(7);
        return null;
    }

    Class getClass(String cname) {
        try {
            return Class.forName(cname);
        } catch (ClassNotFoundException e) {
            mLogger.error("class not found: " + cname);
        }
        return null;
    }



    /**
     * pt1 and pt2 are assumed not to be null. If param types are 0, array length
     * is 0.
     */
    boolean paramTypesMatch(Class[] pt1, Class[] pt2) {
        if (pt1.length != pt2.length) 
            return false; 

        for (int i=0; i < pt1.length; i++) {
            if ( ! pt1[i].isAssignableFrom(pt2[i]) ) {
                return false;
            }
        }
        return true;
    }

    /**
     * Create instance by calling constructor.
     */
    Object getServerInstance(String classname, Class[] argClasses, Object[] argValues) 
        throws Exception {

        if (mLogger.isDebugEnabled()) {
            mLogger.debug("getServerInstance(" + classname + ")");
        }

        Class cl = Class.forName(classname);

        Constructor constructor = null;
        Constructor[] carr = cl.getConstructors();
        for (int i=0; i < carr.length; i++) {
            Class[] ptarr = carr[i].getParameterTypes();
            if ( paramTypesMatch(ptarr, argClasses) ) {
                constructor = carr[i];
                break;
            }
        }

        if (constructor == null) {
            StringBuffer buf = new StringBuffer();
            for (int i=0; i < argClasses.length; i++) {
                if (i != 0) buf.append(",");
                buf.append(argClasses[i].toString());
            }
            String msg = "no such constructor found: " + classname + 
                "(" + buf.toString() + ")";
            mLogger.error(msg);
            throw new Exception(msg);
        }

        return constructor.newInstance(argValues);
    }



    
    /**
     * Like Class.getMethod() except we allow params that are derived.
     */
    Method getMethod(Class cl, String methodName, Class[] params) 
        throws NoSuchMethodException {

        // targetClass.getMethod(methodName, argClasses);

        Method[] classMethods = cl.getMethods();
        for (int m=0; m < classMethods.length; m++) {

            // find matching method
            if ( ! methodName.equals(classMethods[m].getName()) ) continue;

            // make sure params match
            Class[] classParams = classMethods[m].getParameterTypes();
            if (classParams.length == 0 && params == null)
                return classMethods[m];

            if (classParams.length == params.length) {
                boolean ok = true;
                for (int p=0; p < classParams.length; p++) {
                    if ( ! classParams[p].isAssignableFrom(params[p]) ) {
                        ok = false;
                        break;
                    }
                }
                if (ok) return classMethods[m];
            }
        }

        StringBuffer buf = new StringBuffer();
        for (int i=0; i < params.length; i++) {
            if (i != 0) buf.append(", ");
            buf.append(params[i].toString());
        }
        throw new NoSuchMethodException(methodName + "(" + buf.toString() + "): no such method");
    }


    String getScopeName(int scope) {
        if (SCOPE_SESSION == scope) return SESSION;
        if (SCOPE_WEBAPP  == scope) return WEBAPP;
        if (SCOPE_NONE    == scope) return NONE;
        return UNKNOWN;
    }

    int getScope(HttpServletRequest req) {
        String scope = req.getParameter("scope");
        if (scope == null) return SCOPE_UNKNOWN;
        if (SESSION.equals(scope)) return SCOPE_SESSION;
        if (WEBAPP.equals(scope))  return SCOPE_WEBAPP;
        if (NONE.equals(scope))    return SCOPE_NONE;
        return SCOPE_UNKNOWN;
    }


    Object getJavaObject(HttpSession session, int scope, String oname) {
        if (session == null) return null;
        Map map = null;
        if (scope == SCOPE_SESSION) {
            map = (Map)session.getAttribute(OBJECTS);
        } else if (scope == SCOPE_WEBAPP) {
            map = (Map)session.getServletContext().getAttribute(OBJECTS);
        }
        return ( map == null ? null : map.get(oname) );
    }


    void setJavaObject(HttpSession session, int scope, String oname, Object val){
        if (session == null) return;
        Map map = null;
        if (scope == SCOPE_SESSION) {
            map = (Map)session.getAttribute(OBJECTS);
            if ( map == null ) {
                map = new HashMap();
                session.setAttribute(OBJECTS, map);
            }
        } else if (scope == SCOPE_WEBAPP) {
            map = (Map)session.getServletContext().getAttribute(OBJECTS);
            if ( map == null ) {
                map = new HashMap();
                session.getServletContext().setAttribute(OBJECTS, map);
            }
        }
        if (map != null) map.put(oname, val);
    }

    void removeJavaObject(HttpSession session, int scope, String oname){
        if (session == null) return;
        Map map = null;
        if (scope == SCOPE_SESSION) {
            map = (Map)session.getAttribute(OBJECTS);
        } else if (scope == SCOPE_WEBAPP) {
            map = (Map)session.getServletContext().getAttribute(OBJECTS);
        }
        if (map != null) map.remove(oname);
    }


    /**
     * Compile fault exception message.
     */
    Data compileFault(Throwable e, int swfversion) {
        mLogger.error("compileFault", e);
        return compileFault(e.getMessage(), swfversion);
    }

    Data compileFault(String mesg, int swfversion) {
        return compileFault(mesg, null, swfversion);
    }

    /**
     * Compile fault response.
     */
    Data compileFault(String mesg, Throwable e, int swfversion) {
        if (e == null) {
            mLogger.error("compileFault mesg: " + mesg);
        } else {
            mLogger.error("compileFault mesg: " + mesg, e);
        }
        try {
            return new ObjectData
                ( XMLRPCCompiler.compileFault(XMLUtils.escapeXml(mesg), 
                                              swfversion) );
        } catch (Exception ex) {
            mLogger.error("Exception", ex);
            // this is an error since we can't build a fault response
            throw new Error(e.getMessage());
        }
    }

    public class EncoderData extends Data
    {
        DataEncoder mDataEncoder;
        long mSize;
        public EncoderData(DataEncoder dataEncoder) 
            throws IOException {
            mDataEncoder = dataEncoder;
            mSize = dataEncoder.getSize();
        }

        public String getMimeType() {
            return MimeType.SWF;
        }

        /**
         * @return the encoded XML
         */
        public InputStream getInputStream() 
            throws IOException {
            return mDataEncoder.getInputStream();
        }

        public long size() {
            return mSize;
        }
    }


    /**
     * A data object to hold session object.
     */
    public class ObjectData extends Data
    {
        byte[] mObject;
        public ObjectData(byte[] object) {
            mObject = object;
        }

        public String getMimeType() {
            return MimeType.SWF;
        }

        public InputStream getInputStream() 
            throws IOException {
            return new ByteArrayInputStream(mObject);
        }

        public long size() {
            return mObject.length;
        }
    }
}
