/******************************************************************************
 * LZReturnObject.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.remote;

import java.io.*;
import java.lang.reflect.*;
import java.util.*;
import java.lang.reflect.*;
import javax.servlet.http.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.utils.*;
import org.openlaszlo.xml.internal.*;
import org.apache.log4j.*;
import org.apache.commons.beanutils.PropertyUtils;

/**
 * Utility class to create object SWF based on a server object.
 */
public class LZReturnObject
{
    public static Logger mLogger = Logger.getLogger(LZReturnObject.class);
    public static final int RETTYPE_POJO = 0;
    public static final int RETTYPE_JAVA_BEAN = 1;


    int mCount = 0;
    int mSize = 4096;
    FlashBuffer mBody = new FlashBuffer(mSize);
    Program mProgram = new Program(mBody);
    DataContext mDC;
    int mSWFVersion;
    int mObjRetType;

    public LZReturnObject(String objectReturnType, int swfversion) {
        mDC = new DataContext();
        mDC.setEncoding( swfversion == 5 ? "Cp1252" : "UTF-8" );
        if (objectReturnType == null) {
            mObjRetType = RETTYPE_POJO;
        } else if ("javabean".equals(objectReturnType)){
            mObjRetType = RETTYPE_JAVA_BEAN;
        } else {
            mObjRetType = RETTYPE_POJO;
        }
    }

    void pushInteger(int i) {
        mLogger.debug("pushInteger");
        mProgram.push(i);
    }

    void pushFloat(float f) {
        mLogger.debug("pushFloat");
        mProgram.push(f);
    }

    void pushString(String s) {
        mLogger.debug("pushString");
        DataCommon.pushStringData(s, mBody, mDC);
//        mProgram.push(s);
    }

    void pushDouble(double d) {
        mLogger.debug("pushDouble");
        mBody.writeByte(Actions.PushData);
        mBody.writeWord(8+1);
        mBody.writeByte(6);
        long dbits = Double.doubleToLongBits(d);
        mBody.writeDWord((int)(dbits>>>32));
        mBody.writeDWord((int)(dbits&0xffffffffL));
    }


    void pushBoolean(boolean b) {
        mLogger.debug("pushBoolean");
        mBody.writeByte(Actions.PushData);
        mBody.writeWord(1+1);
        mBody.writeByte(5);
        mBody.writeByte(b?1:0);
    }

    void pushArray(Object object) {
        mLogger.debug("pushArray");
        int length = Array.getLength(object);
        for (int i = length - 1; 0 <= i; i--) {
            createReturnValue(Array.get(object, i));
        }
        mProgram.push(length);
        mBody.writeByte(Actions.InitArray);
    }

    void pushList(Object object) {
        mLogger.debug("pushList");
        List list = (List)object;
        int length = list.size();
        for (int i = length - 1; 0 <= i; i--) {
            createReturnValue(list.get(i));
        }
        mProgram.push(length);
        mBody.writeByte(Actions.InitArray);
    }

    void pushNull() {
        mBody.writeByte(Actions.PushData);
        mBody.writeWord(0+1);
        mBody.writeByte(2);
    }


    void pushObject(Object object) {
        Class cl = object.getClass();

        // Does this have to be a unique name?
        String varname = "__tmp" + (mCount++);
        String classname = cl.getName();

        //------------------------------------------------------------
        // varname = new Object();
        mProgram.push(varname);
        {
            // new Object()
            mProgram.push(0);        // zero arguments
            mProgram.push("Object"); // push classname
            mProgram.newObject();    // instantiate
        }
        mProgram.setVar();


        //------------------------------------------------------------
        // varname.class = classname
        mProgram.push(varname);
        mProgram.getVar();
        mProgram.push("class");
        mProgram.push(classname);
        mBody.writeByte(Actions.SetMember);

        if (mObjRetType == RETTYPE_JAVA_BEAN) {
            pushObjectJavaBean(object, varname);
        } else {
            pushObjectPOJO(object, varname);
        }
     
        //------------------------------------------------------------
        // add varname object into stack
        mProgram.push(varname);
        mProgram.getVar();
    }

    /**
     * Create SWF for an object that conforms to JavaBean spec.
     */
    void pushObjectPOJO(Object object, String varname) {
        Class cl = object.getClass();
        Field[] fields = cl.getFields();
        for (int i=0; i < fields.length; i++) {
            if (! Modifier.isPublic(fields[i].getModifiers()))
                continue;

            String fieldName = fields[i].getName();
            Object value;
            try {
                value = fields[i].get(object);
            } catch (IllegalAccessException e) {
                mLogger.error("IllegalAccessException", e);
                continue;
            }
            if (mLogger.isDebugEnabled()) {
                mLogger.debug("add field name " + fieldName + ", " + 
                              (value != null ? value.getClass() : null) );
            }
            mProgram.push(varname);
            mProgram.getVar();
            mProgram.push(fieldName);
            createReturnValue(value);
            mBody.writeByte(Actions.SetMember);
        }
    }

    /**
     * Create SWF for an object that conforms to JavaBean spec.
     */
    void pushObjectJavaBean(Object object, String varname) {
        //------------------------------------------------------------
        // Just get the fields from the objects and add it to this object
        Map beanProps = null;
        try {
            //Use jakarta-commons beanutils to inspect the object
            beanProps = PropertyUtils.describe(object);
        } catch (IllegalAccessException e) {
            mLogger.error("IllegalAccessException",e);
        } catch (InvocationTargetException e) {
            mLogger.error("InvocationTargetException",e);
        } catch (NoSuchMethodException e) {
            mLogger.error("NoSuchMethodException",e);
        }
                    
        if (beanProps != null) {
            Set keys = beanProps.keySet();
            Iterator iter = keys.iterator();
            while(iter.hasNext()){
                String fieldName  = (String)iter.next();
                //Don't add the class property as it is already set by the method
                if(!"class".equals(fieldName)) {
                    Object value = beanProps.get(fieldName);
                    if (mLogger.isDebugEnabled()) {
                        mLogger.debug("add field name " + fieldName + ", " + 
                                      ((value!=null)?value.getClass():null));
                    }
                    mProgram.push(varname);
                    mProgram.getVar();
                    mProgram.push((String)fieldName);
                    createReturnValue(value);
                    mBody.writeByte(Actions.SetMember);
                }
            }
        }
    }


    void pushMap(Map map) {

        // Does this have to be a unique name?
        String varname = "__tmp" + (mCount++);

        //------------------------------------------------------------
        // varname = new Object();
        mProgram.push(varname);
        {
            // new Object()
            mProgram.push(0);        // zero arguments
            mProgram.push("Object"); // push classname
            mProgram.newObject();    // instantiate
        }
        mProgram.setVar();

        Iterator iter = map.keySet().iterator();

        while (iter.hasNext()) {
            String key = (String)iter.next();

            //------------------------------------------------------------
            // varname.class = classname
            mProgram.push(varname);
            mProgram.getVar();
            mProgram.push(key);
            createReturnValue(map.get(key));
            mBody.writeByte(Actions.SetMember);
        }


        //------------------------------------------------------------
        // add varname object into stack
        mProgram.push(varname);
        mProgram.getVar();
    }

    /**
     * Recurse through this function to create return value
     */
    void createReturnValue(Object object) {
        mLogger.debug("createReturnValue");
        if (object == null) {
            pushNull();
            return;
        }

        Class cl = object.getClass();
        if (cl.isArray()) {
            pushArray(object);
        } else if (List.class.isInstance(object)) {
            pushList(object);
        } else if (Map.class.isInstance(object)) {
            pushMap((Map)object);
        } else if (cl == Integer.class) {
            pushInteger(((Integer)object).intValue());
        } else if (cl == Long.class) {
            //------------------------------------------------------------
            // From: http://developer.irt.org/script/1031.htm
            //
            // In JavaScript all numbers are floating-point numbers.
            //
            // JavaScript uses the standard 8 byte IEEE floating-point numeric
            // format, which means the range is from:
            //
            // +/- 1.7976931348623157x10^308 - very large, and +/- 5x10^-324 -
            // very small.
            //
            // As JavaScript uses floating-point numbers the accuracy is only
            // assured for integers between: -9,007,199,254,740,992 (-2^53) and
            // 9,007,199,254,740,992 (2^53)
            //
            // All the above from "JavaScript The Definitive Guide" - O'Reilly. 
            // 
            //------------------------------------------------------------
            // Java long:
            // 8 bytes signed (two's complement). Ranges from
            // -9,223,372,036,854,775,808 to +9,223,372,036,854,775,807.
            //------------------------------------------------------------
            
            // possible rounding inaccuracy
            pushInteger(((Long)object).intValue());

        } else if (cl == Short.class) {
            pushInteger(((Short)object).intValue());
        } else if (cl == Byte.class) {
            // push as number for now
            pushInteger(((Byte)object).intValue());
        } else if (cl == Character.class) {
            pushString(((Character)object).toString());
        } else if (cl == Float.class) {
            pushFloat(((Float)object).floatValue());
        } else if (cl == Double.class) {
            pushDouble(((Double)object).doubleValue());
        } else if (cl == Boolean.class) {
            pushBoolean(((Boolean)object).booleanValue());
        } else if (cl == String.class) {
            pushString((String)object);
        } else {
            pushObject(object);
        }
    }


    /**
     *
     */
    public Program createObjectProgram(Object object) {
        mLogger.debug("createObjectProgram(" + object + ")" );

        createReturnValue(object);

        //------------------------------------------------------------
        // call into the viewsystem
        mProgram.push("this");
        mProgram.getVar();
        mProgram.push(2);
        mProgram.push("_parent");
        mProgram.getVar();
        mProgram.push("loader");
        mBody.writeByte(Actions.GetMember);
        mProgram.push("returnData");
        mProgram.callMethod();
        mProgram.pop();

        // Borrowed from DataCompiler. -pk

        // Collect the string dictionary data
        byte pooldata[] = DataCommon.makeStringPool(mDC);

        // Room at the end of the buffer for maybe some callback code to the
        // runtime to say we're done.
        final int MISC = 4096;

        // 'out' is the main FlashBuffer for composing the output file
        FlashBuffer out = new FlashBuffer(mBody.getSize() + pooldata.length + MISC);
        // Write out string constant pool
        out._writeByte( Actions.ConstantPool );
        out._writeWord( pooldata.length + 2 ); // number of bytes in pool data + int (# strings)
        out._writeWord( mDC.cpool.size() ); // number of strings in pool
        out.writeArray( pooldata, 0, pooldata.length); // copy the data

        // Write out the code to build nodes
        out.writeArray(mBody.getBuf(), 0, mBody.getSize());
        return new Program(out);
    }


    /**
     */
    public static FlashFile createObjectFile(Object object, String objectReturnType, 
                                             int swfversion)
        throws IOException {
        mLogger.debug("createObjectFile(" + object + ", " + swfversion + ")" );
        // Create FlashFile object nd include action bytes
        FlashFile file = FlashFile.newFlashFile();
        Script s = new Script(1);
        file.setMainScript(s);
        file.setVersion(swfversion);
        Frame frame = s.newFrame();
        Program program = new LZReturnObject(objectReturnType, swfversion)
            .createObjectProgram(object);
        frame.addFlashObject(new DoAction(program));
        return file;
    }

    /**
     * @param objectReturnType One of 'pojo' (returns public member values) or
     * 'javabean' (returns members that have associated getters). Will default
     * to 'pojo'.
     */
    public static byte[] createObject(Object object, String objectReturnType,
                                      int swfversion)
        throws IOException {
        mLogger.debug("createObject(" + object + ")" );

        int i = 0;
        try {
            FlashFile file = createObjectFile(object, objectReturnType,
                                              swfversion);
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
