/******************************************************************************
 * LZClientObject.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.remote;

import java.io.*;
import java.lang.reflect.*;
import javax.servlet.http.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.utils.*;
import org.openlaszlo.xml.internal.DataCommon;
import org.openlaszlo.xml.internal.DataContext;
import org.apache.log4j.*;

/**
 * Utility class to create object SWF based on a server object.
 */
public class LZClientObject
{
    public static Logger mLogger = Logger.getLogger(LZClientObject.class);

    /**
     */
    public static Program createObjectProgram(String classname, String scope, Class c) {
        if (mLogger.isDebugEnabled()) {
            mLogger.debug("createObjectProgram(" + classname + "," + "," + scope + "," 
                          + c.getName() + ")" );
        }

        int size = 4096;
        FlashBuffer body = new FlashBuffer(size);
        Program program = new Program(body);
        DataContext dc = new DataContext();

        //------------------------------------------------------------
        // Tells client data returned is an object 
        // this.__LZstub = true
        program.push("__LZstubload");
        program.push(1);


        // SOAP specific information
        // object.stubinfo
        program.push("stubinfo");
        {
            //------------------------------------------------------------
            // this.remoteClass = classname
            program.push("remoteClass");
            DataCommon.pushStringData(classname, body, dc);
        }
        program.push(1);
        body.writeByte(Actions.InitObject);


        // object.stub
        program.push("stub");
        {
            int count = 0;
            boolean onlyStatic = "staticobject".equals(scope);

            //------------------------------------------------------------
            // Create methods for object
            Method[] methods = c.getMethods();
            for (int i=0; i < methods.length; i++) {

                // Skip Object methods
                if (methods[i].getDeclaringClass() == Object.class) {
                    continue;
                }

                // Check if we only want static methods
                if (onlyStatic &&
                    ! Modifier.isStatic(methods[i].getModifiers())) {
                    continue;
                }

                // Skip toString method.
                String methodName = methods[i].getName();
                if ("toString".equals(methodName)) {
                    continue;
                }

                count++;

                //------------------------------------------------------------
                //
                DataCommon.pushStringData(methodName, body, dc);
                body.writeByte(Actions.DefineFunction);
                body.writeWord(5); // length of header (1 byte + 2 words (2bytes))
                body.writeByte(0); // function name string (if empty, it is
                // anonymous)
                body.writeWord(0); // number of parameters
                {
                    // Check to see if method's last two parameters are
                    // HttpServletRequest and HttpServletResponse.
                    Class[] params = methods[i].getParameterTypes();
                    int len = params.length;
                    boolean doreq = // next to last or last argument
                        (len > 1 && params[len-2] == HttpServletRequest.class) ||
                        (len > 0 && params[len-1] == HttpServletRequest.class);
                    boolean dores = // should be last argument
                        (len > 0 && params[len-1] == HttpServletResponse.class);

                    FlashBuffer fbuf = new FlashBuffer(500);
                    Program fprog = new Program(fbuf);

                    // arguments.callee.args gets set in the client and includes
                    // secure and secureport information.  
                    //
                    // var args = arguments.callee.args;
                    DataCommon.pushStringData("args", fbuf, dc);
                    {
                        DataCommon.pushStringData("arguments", fbuf, dc);
                        fprog.getVar();
                        DataCommon.pushStringData("callee", fbuf, dc);
                        fbuf.writeByte(Actions.GetMember);
                        DataCommon.pushStringData("args", fbuf, dc);
                        fbuf.writeByte(Actions.GetMember);
                    }
                    fprog.setVar();

                    // _root.LzRemote.invoke(
                    //     arguments,
                    //     delegate,
                    //     classname,
                    //     classname + "." + methodName,
                    //     { op: 'session', doreq..., dores... },
                    //     this.secure,
                    //     this.secureport
                    // );

                    // 5. secureport
                    DataCommon.pushStringData("args", fbuf, dc);
                    fprog.getVar();
                    DataCommon.pushStringData("secureport", fbuf, dc);
                    fbuf.writeByte(Actions.GetMember);

                    // 4. secure
                    DataCommon.pushStringData("args", fbuf, dc);
                    fprog.getVar();
                    DataCommon.pushStringData("secure", fbuf, dc);
                    fbuf.writeByte(Actions.GetMember);

                    // 3. opts
                    int optcount = 0;
                    {
                        // { op: 'invoke', oname: varname, scope: scope, 
                        //   objectreturntype: objectreturntype, 
                        //   doreq: TBD, dores: TBD}
                        DataCommon.pushStringData("op", fbuf, dc);
                        DataCommon.pushStringData("invoke", fbuf, dc);
                        optcount++;

                        DataCommon.pushStringData("objectreturntype", fbuf, dc);
                        {
                            DataCommon.pushStringData("args", fbuf, dc);
                            fprog.getVar();
                            DataCommon.pushStringData("objectreturntype", fbuf, dc);
                            fbuf.writeByte(Actions.GetMember);
                        }
                        optcount++;

                        DataCommon.pushStringData("oname", fbuf, dc);
                        {
                            DataCommon.pushStringData("args", fbuf, dc);
                            fprog.getVar();
                            DataCommon.pushStringData("attributename", fbuf, dc);
                            fbuf.writeByte(Actions.GetMember);
                        }
                        optcount++;

                        DataCommon.pushStringData("scope", fbuf, dc);
                        {
                            DataCommon.pushStringData("args", fbuf, dc);
                            fprog.getVar();
                            DataCommon.pushStringData("scope", fbuf, dc);
                            fbuf.writeByte(Actions.GetMember);
                        }
                        optcount++;

                        DataCommon.pushStringData("methodname", fbuf, dc);
                        DataCommon.pushStringData(classname + "." + methodName,
                                                  fbuf, dc);
                        optcount++;

                        DataCommon.pushStringData("classname", fbuf, dc);
                        DataCommon.pushStringData(classname, fbuf, dc);
                        optcount++;

                        if (doreq) {
                            DataCommon.pushStringData("doreq", fbuf, dc);
                            fprog.push(1);
                            optcount++;
                        }
                        if (dores) {
                            DataCommon.pushStringData("dores", fbuf, dc);
                            fprog.push(1);
                            optcount++;
                        }
                    }
                    fprog.push(optcount); // # of object values
                    fbuf.writeByte(Actions.InitObject);

                    // 2. arguments[0] (should be array of arguments)
                    DataCommon.pushStringData("arguments", fbuf, dc);
                    fprog.getVar();
                    fprog.push(0);
                    fbuf.writeByte(Actions.GetMember);

                    // 1. arguments[1] (should be delegate)
                    DataCommon.pushStringData("arguments", fbuf, dc);
                    fprog.getVar();
                    fprog.push(1);
                    fbuf.writeByte(Actions.GetMember);

                    // Number of parameters
                    fprog.push(5);

                    // call function
                    DataCommon.pushStringData("_root", fbuf, dc);
                    fprog.getVar();
                    DataCommon.pushStringData("LzJavaRPCService", fbuf, dc);
                    fbuf.writeByte(Actions.GetMember);
                    DataCommon.pushStringData("invoke", fbuf, dc);
                    fprog.callMethod();

                    fprog.pop(); // pop undefined return value

                    body.writeWord(fbuf.pos); // length of defined function body
                    body.writeFOB(fbuf);
                }
            }
            program.push(count);
            body.writeByte(Actions.InitObject);
        }


        program.push(3);
        body.writeByte(Actions.InitObject);


        //------------------------------------------------------------
        // call into the viewsystem
        program.push("this");
        program.getVar();
        program.push(2); 
        program.push("_parent"); 
        program.getVar();
        program.push("loader"); 
        body.writeByte(Actions.GetMember);
        program.push("returnData"); 
        program.callMethod();
        program.pop();

        // Collect the string dictionary data
        byte pooldata[] = DataCommon.makeStringPool(dc);
        // 'out' is the main FlashBuffer for composing the output file
        final int MISC = 64;
        FlashBuffer out = new FlashBuffer(body.getSize() + pooldata.length + MISC);
        // Write out string constant pool
        out.writeByte( Actions.ConstantPool );
        out.writeWord( pooldata.length + 2 ); // number of bytes in pool data + int (# strings)
        out.writeWord( dc.cpool.size() ); // number of strings in pool
        out.writeArray( pooldata, 0, pooldata.length); // copy the data
        // Write out the code to build nodes
        out.writeArray(body.getBuf(), 0, body.getSize());

        return new Program(out);
    }


    /**
     */
    public static FlashFile createObjectFile(String classname, String scope, Class c, 
                                             int swfversion) 
        throws IOException {
        if (mLogger.isDebugEnabled()) {
            mLogger.debug("createObjectFile(" + classname + "," + c.getName() + ")" );
        }
        // Create FlashFile object nd include action bytes
        FlashFile file = FlashFile.newFlashFile();
        Script s = new Script(1);
        file.setMainScript(s);
        file.setVersion(swfversion);
        Frame frame = s.newFrame();
        Program program = createObjectProgram(classname, scope, c);
        frame.addFlashObject(new DoAction(program));
        return file;
    }

    /**
     */
    public static byte[] createObject(String classname, String scope, int swfversion)
        throws IOException {
        if (mLogger.isDebugEnabled()) {
            mLogger.debug("createObject(" + classname + "," + scope + ")" );
        }

        Class c;
        try {
            c = Class.forName(classname);
        } catch (ClassNotFoundException e) {
            throw new RuntimeException("Can't find class " + classname);
        }

        int i = 0;
        try {
            FlashFile file = createObjectFile(classname, scope, c, swfversion);
            FlashOutput fob = file.generate();
            byte[] buf = new byte[fob.getSize()];
            System.arraycopy(fob.getBuf(), 0, buf, 0, fob.getSize());
            return buf;
        } catch (IVException e) {
            throw new ChainedException(e);
        } catch (IOException e) {
            mLogger.error("io error creating object SWF: " + e.getMessage());
            throw e;
        }
    }

}
