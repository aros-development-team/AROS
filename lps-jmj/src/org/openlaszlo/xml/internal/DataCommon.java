/* ****************************************************************************
 * DataCommon.java
 *
 * Compile XML directly to SWF bytecodes.
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.xml.internal;

import java.io.*;
import java.util.*;

import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.compiler.CompilationError;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.utils.HashIntTable;

import org.apache.log4j.*;

/**
 * Common data compiler routines between DataCompiler (XML compiler) and DataBuilder (Java API)
 *
 * @author Henry Minsky
 * @version 1.0
 */
public abstract class DataCommon {
    /* Logger */
    static public String ROOT_NODE_INSTANTIATOR_FN      = "_rootndi";
    static public String RESULTSET_NODE_INSTANTIATOR_FN = "_resultsetndi";
    static public String BODY_NODE_INSTANTIATOR_FN      = "_bodyndi";
    static public String NODE_INSTANTIATOR_FN      = "_m";
    static public String TEXT_INSTANTIATOR_FN      = "_t";
    static public String ROOT_NODE_FINAL_FN        = "_finishndi";

    /**
     * Push a reference to string data on stack. maybeInternString
     * uses a heuristic to decide whether to add the string constant
     * pool, so this may push a string literal or a dictionary ref.
     */
    static public final void pushStringData(String s, FlashBuffer body, DataContext dc) {
        int idx = maybeInternString(s, dc);
        if (idx != -1) {
            //System.out.println("intern IDX '"+s+"'="+idx);
            // PUSH idx
            body.writeByte(Actions.PushData);
            body.writeWord(2);
            writeOffset(idx, body);
        } else {
            //            System.out.println("intern LITERAL '"+s+"'");
            // PUSH literal attrname
            // {0x96, lenlo, lenhi, 0x00, char, char, char, ...0x00, }
            body.writeByte(Actions.PushData);
            body.writeWord(getByteLength(s, dc.encoding) + 2); // account for type code and zero terminator
            body.writeByte(0); // type code 0 ; null terminated string type
            body.writeStringZ(s, dc.encoding);
        }
    }

    static public int getByteLength (String s, String encoding) {
        try {
            byte buf[] = s.getBytes(encoding);
            return buf.length;
        } catch (UnsupportedEncodingException e) {
            throw new RuntimeException("Got UnsupportedEncodingException for charset "+encoding
                                       +" on string '"+s+"': "+e.getMessage());
        }
    }


    /**
     * Push a reference to string data on stack. maybeInternString
     * uses a heuristic to decide whether to add the string constant
     * pool, so this may push a string literal or a dictionary ref.
     *
     * This version of the method uses the faster _writeXXX() methods,
     * which do not do bounds checks on the FlashBuffer. So this should only be
     * used in situations where we know the FlashBuffer is already allocated with
     * enough space for any data we may push.
     */
    static public final void _pushStringData(String s, FlashBuffer body, DataContext dc) {
        int idx = maybeInternString(s, dc);
        if (idx != -1) {
            //System.out.println("intern IDX '"+s+"'="+idx);
            // PUSH idx
            body._writeByte(Actions.PushData);
            body._writeWord(2);
            writeOffset(idx, body);
        } else {
            //            System.out.println("intern LITERAL '"+s+"'");
            // PUSH literal attrname
            // {0x96, lenlo, lenhi, 0x00, char, char, char, ...0x00, }
            body._writeByte(Actions.PushData);
            body._writeWord(getByteLength(s, dc.encoding) + 2); // account for type code and zero terminator
            body._writeByte(0); // type code 0 ; null terminated string type
            body._writeStringZ(s, dc.encoding);
        }
    }

    /**
     * Push a reference to string data on stack. maybeInternString
     * uses a heuristic to decide whether to add the string constant
     * pool, so this may push a string literal or a dictionary ref.
     *
     *<p>
     *
     * This method is used when composing the arguments for a PUSH
     * merges several args on the stack with a single PUSH
     * instruction, and writes out only the data or constant-pool
     * index reference, as opposed to the pushStringData() method
     * which also writes out the PUSH instruction itsef.
     *
     */

    static public final void pushMergedStringData(String s, FlashBuffer body, DataContext dc) {
        int idx = maybeInternString(s, dc);
        if (idx != -1) {
            //System.out.println("intern IDX '"+s+"'="+idx);
            writeOffset(idx, body);
        } else {
            //            System.out.println("intern LITERAL '"+s+"'");
            body.writeByte(0); // type code 0 ; null terminated
            // string type
            body.writeStringZ(s, dc.encoding);
        }
    }


    /**
     * Push a reference to string data on stack. maybeInternString
     * uses a heuristic to decide whether to add the string constant
     * pool, so this may push a string literal or a dictionary ref.
     *
     *<p>
     * 
     *
     * This version of the method uses the faster _writeXXX() methods,
     * which do not do bounds checks on the FlashBuffer. So this should only be
     * used in situations where we know the FlashBuffer is already allocated with
     * enough space for any data we may push.
     */

    static public final void _pushMergedStringData(String s, FlashBuffer body, DataContext dc) {
        int idx = maybeInternString(s, dc);
        if (idx != -1) {
            //System.out.println("intern IDX '"+s+"'="+idx);
            _writeOffset(idx, body);
        } else {
            //            System.out.println("intern LITERAL '"+s+"'");
            body._writeByte(0); // type code 0 ; null terminated
            // string type
            body._writeStringZ(s, dc.encoding);
        }
    }

    /**
     * Push a reference to string data on stack. Always attempts to intern in the string 
     * pool, so this may push a string literal or a dictionary ref.
     *
     * <p>
     *
     * This method is designed to be called when pushing references to strings
     * which we are pretty sure will occur again, such as tag names or attribute names.
     * We try to put them into the string pool immediately, as opposed to using the
     * heuristic of waiting until the string is seen again before interning it.
     *
     * @return number of bytes written to buffer
     */
    static public final void pushMergedStringDataSymbol(String s, FlashBuffer body, DataContext dc) {
        int idx = internString(s, dc);
        if (idx != -1) {
            //System.out.println("intern IDX '"+s+"'="+idx);
            writeOffset(idx, body);
        } else {
            //            System.out.println("intern LITERAL '"+s+"'");
            body.writeByte(0); // type code 0 ; null terminated
            // string type
            body.writeStringZ(s, dc.encoding);
        }
    }


    /**
     * Push a reference to string data on stack. Attempts to intern in the string pool
     * pool, so this may push a string literal or a dictionary ref. This routine uses the _writeXXX routines,
     * which bypass FlashBuffer's ensureCapacity(), so the FlashBuffer must have been allocated with enough
     * space for these writes, or an ArrayBoundsException may happen.
     *
     * @return number of bytes written to buffer
     */
    static public final void _pushMergedStringDataSymbol(String s, FlashBuffer body, DataContext dc) {
        int idx = internString(s, dc);
        if (idx != -1) {
            //System.out.println("intern IDX '"+s+"'="+idx);
            writeOffset(idx, body);
        } else {
            //            System.out.println("intern LITERAL '"+s+"'");
            body._writeByte(0); // type code 0 ; null terminated
            // string type
            body._writeStringZ(s, dc.encoding);
        }
    }


    /**
     * Copy data from a a byte array into a FlashBuffer 
     */
    static public final void writeFlashData(FlashBuffer body, byte[] data, int destPos) {
        //arraycopy(Object src, int srcPos, Object dest, int destPos, int length) 
        byte[] buf = body.getBuf();
        System.arraycopy(data, 0, buf, destPos, data.length);
        body.setPos(destPos + data.length);
    }


    /**
     * Return byte array containing the strings from the constant pool, sorted and zero terminated.
     */ 
    final static public byte[] makeStringPool(DataContext dc) {
        HashIntTable pool = dc.cpool;
        // Array for sorting
        int nstrings = pool.size();
        //System.out.println("makeStringPool: nstrings = "+nstrings);
        if (nstrings > (1<<16 - 1)) {
            throw new RuntimeException("more than 64k strings in constant pool");
        }
        String[] sortArray = new String[nstrings];
        Enumeration keys = pool.keys();
        int poolsize = 0;
        // Sort strings by numerical index
        while ( keys.hasMoreElements() ) {
            String key = (String) keys.nextElement();
            int v = pool.get(key);
            //System.out.println("sorting: key="+key+", index="+v);
            sortArray[v] = key;
            poolsize += (getByteLength(key, dc.encoding) + 1); // account for zero terminator
        }

        byte pooldata[] = new byte[poolsize];
        int pos = 0;
        for (int i = 0; i < nstrings; i++) {
            String s = sortArray[i];
            //System.out.println("s="+s);
            // Convert to byte values in the desired charset encoding
            byte chars[];
            try {
                chars = s.getBytes(dc.encoding);
            } catch (UnsupportedEncodingException e) {
                chars = s.getBytes();
            }
            int len = chars.length;
            //arraycopy(Object src, int srcPos, Object dest, int destPos, int length) 
            System.arraycopy(chars, 0, pooldata, pos, len);
            pos += len;
            pooldata[pos++] = 0; // null terminate the string
        }
        return pooldata;
    }

    /**
     * Intern a string in the constant pool.
     * @param s the string
     * @param dc
     * @return offset into string pool
     */
    static public final int addStringConstant(String s, DataContext dc) {
        int size = dc.cpool.size();
        dc.cpool.put(s, size);
        dc.pool_data_length += (getByteLength(s, dc.encoding) + 1); 
        return size;
    }

    /**
     * @return the index of the string in the constant pool.
     */
    static public final int getStringIndex(String s, DataContext dc) {
        return dc.cpool.get(s);
    }
        
    /**
     * Intern a string in the constant pool, but only if it has been seen once before.
     * @param s the string
     * @param dc
     * @return the offset into string pool, or -1 if we decided not to intern it yet
     */
    static public final int maybeInternString(String s, DataContext dc) {
        int idx = dc.cpool.get(s);
        if (idx >= 0) {
            return idx;
        }
        if (dc.pool_data_length >= 65532) {
            return -1;
        }
        boolean seen = dc.cpool_first.containsKey(s);
        if (seen) {
            int size = dc.cpool.size();
            // We can only have 64k of data (including zero terminators on strings)
            dc.pool_data_length += (getByteLength(s, dc.encoding) + 1);
            // 65536 - 4 bytes for # strings
            if (dc.pool_data_length >= 65532) {
                return -1;
            }
            dc.cpool.put(s, size);
            return size;
        } else {
            dc.cpool_first.put(s, 1);
            return(-1);
        }
    }

    /**
     * Intern a string in the constant pool
     * @param s the string
     * @param dc
     * @return the offset into string pool, or -1 if the string pool is full
     */
    static public final int internString(String s, DataContext dc) {
        int idx = dc.cpool.get(s);
        if (idx >= 0) {
            return idx;
        }
        if (dc.pool_data_length >= 65532) {
            return -1;
        }
        int size = dc.cpool.size();
        // We can only have 64k of data (including zero terminators on strings)
        dc.pool_data_length += (getByteLength(s, dc.encoding) + 1);
        // 65536 - 4 bytes for # strings
        if (dc.pool_data_length >= 65532) {
            return -1;
        }
        dc.cpool.put(s, size);
        return size;
    }

    /**
     * Writes a string offset from the constant pool to a flash buffer
     *
     * @param o  dictionary string index to write to flash buffer
     */
    static public final void writeOffset(int o, FlashBuffer fb) {
        if (o > 255) {
            fb.writeByte(9);
            fb.writeWord(o);
        } else {
            fb.writeByte(8);
            fb.writeByte(o);
        }
    }
    

    /**
     * Writes a string offset from the constant pool to a flash buffer. This uses _writeXXX
     * which bypasses the FlashBuffer ensureCapacity() check (for speed) so you must make sure
     * the FlashBuffer is large enough to write these bytes.
     *
     * @param o  dictionary string index to write to flash buffer
     */
    static public final void _writeOffset(int o, FlashBuffer fb) {
        if (o > 255) {
            fb._writeByte(9);
            fb._writeWord(o);
        } else {
            fb._writeByte(8);
            fb._writeByte(o);
        }
    }
    
    /**
     * For debugging, to print out a Program as hex values.
    */
    
    static public void printProgram (Program program) {
        //program.printContent(System.out, " ");
        FlashBuffer body = program.body();
        byte[] buf = body.getBuf();
        System.out.print("char data[] = {\n   ");
        for (int i=0; i < body.getSize(); i++) {
            if (i % 10 == 0) {
                System.out.print("\n    ");
            }
            if (i < buf.length-1) { 
                System.out.print("(byte) 0x"+Integer.toHexString((buf[i]>>4 ) & 0xf) + Integer.toHexString(buf[i] & 0xf) + ", ");
            } else {
                System.out.print("(byte) 0x"+Integer.toHexString((buf[i]>>4 ) & 0xf) + Integer.toHexString(buf[i] & 0xf));
            }
        }
                System.out.println("\n};");
    }
}

