/* *****************************************************************************
 * StringUtils.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.utils;
import java.util.*;

/**
 * A utility class containing string utility functions.
 *
 * @author Oliver Steele
 */
public abstract class StringUtils {
    /**
     * Splits a string containing <var>n</var> occurrences of
     * <var>delim</var> into <var>n+1</var> strings that don't contain
     * <var>delim</var> (and whose occurrences in <var>str</var> are
     * bounded by <var>delim</var>).
     *
     * <p>For example, <code>split("a,b,,c", ",")<code> evaluates to
     * <code>{"a", "b", "", "c"}</code>.

     * <p><code>split("/", "/") -> ["", ""]</code>
     *
     * @param str a <code>String</code> value
     * @param delim a <code>char</code> value
     * @return a <code>String[]</code> value
     */
    public static String[] split(String str, String delim)
    {
        List lines = new Vector();
        int startPos = 0;
        while (true) {
            int endPos = indexOf(str, delim, startPos);
            if (endPos == -1) {
                if (startPos > 0 || startPos < str.length()) {
                    lines.add(str.substring(startPos));
                }
                break;
            }
            lines.add(str.substring(startPos, endPos));
            startPos = endPos + delim.length();
        }
        {
            String[] result = new String[lines.size()];
            int i = 0;
            for (Iterator e = lines.iterator(); e.hasNext(); ) {
                result[i] = (String) e.next();
                i++;
            }
            return result;
        }
    }
    
    /**
     * Returns a single string that interpolates the strings in
     * <var>lines</var> by the delimiters <var>delim</var>.
     *
     * For example, <code>join({"a","b","c"}, ",")</code> would
     * evaluate to <code>"a,b,c"</code> (if Java had a literal
     * expression notation for arrays).
     *
     * @param lines a <code>String[]</code> value
     * @param delim a <code>char</code> value
     * @return a <code>String</code> value
     */
    public static String join(String[] lines, String delim) {
        StringBuffer buffer = new StringBuffer();
        for (int i = 0; i < lines.length; i++) {
            if (i > 0) {
                buffer.append(delim);
            }
            buffer.append(lines[i]);
        }
        return buffer.toString();
    }

    /** List<Object>, String -> String */
    public static String join(List words, String delim) {
        StringBuffer buffer = new StringBuffer();
        for (Iterator iter = words.iterator(); iter.hasNext(); ) {
            buffer.append(iter.next());
            if (iter.hasNext()) {
                buffer.append(delim);
            }
        }
        return buffer.toString();
    }

    /** Return the index of the first occurrence of value in str that
     * is later than or equal to offset.
     * @param str a String
     * @param value a String
     * @param offset a String
     * @return an int
     */
    public static int indexOf(String str, String value, int offset) {
        if (value.length() == 0) {
            throw new IllegalArgumentException();
        }
        while (offset < str.length()) {
            int pos = str.indexOf(value.charAt(0), offset);
            if (pos == -1) {
                return pos;
            }
            for (int i = 1; ; i++) {
                if (i >= value.length()) {
                    return pos;
                }
                if (pos + i >= str.length() ||
                    str.charAt(pos + i) != value.charAt(i)) {
                    break;
                }
            }
            offset = pos + 1;
        }
        return -1;
    }

    /** Turn CRs into LFs.
     *
     * Note: not implemented for DOS (CRLF) line feeds.  Will turn
     * them into double lines, which will mess up multi-line string
     * literals.
     *
     * @param src a string
     * @return a string
     */
    public static String normalizeLineEndings(String src) {
        if (src.indexOf('\r') >= 0) {
            return StringUtils.join(StringUtils.split(src, "\r"), "\n");
        }
        return src;
    }

    /**
     * Parse an int, interpreting a leading 0x or # to mean
     * the text is in hexidecimal
     *
     * @param src a string
     */
    public static int parseInt(String src) {
        if (src.length() > 1 && src.charAt(0) == '#') {
            return Integer.parseInt(src.substring(1), 16);
        }
        else if (src.length() > 2 && src.charAt(0) == '0' 
                && src.charAt(1) == 'x') {
            return Integer.parseInt(src.substring(2), 16);
        } else {
            return Integer.parseInt(src);
        }
    }


    /** 
     * Replaces old character with new string.
     *
     * @param str string to replace old character with new string
     * @param oldChar old character to replace in string
     * @param newStr new string to replace old character with
     * @return string with replaced string
     */
    public static String replace(String str, char oldChar, String newStr)
    {
        // offset: length(newString) - length(oldChar)
        int os = newStr.length()-1; 
        int bufOS = 0;
        StringBuffer buf = new StringBuffer(str);
        int i = str.indexOf(oldChar);
        while (i != -1) {
            // Index of buffer needs to be adjusted by the number of times new
            // string is replaced
            buf.replace(i+bufOS, i+bufOS+1, newStr);
            bufOS += os;
            i = str.indexOf(oldChar, i+1);
        }
        return buf.toString();
    }

    /**
     * Replace index in string with given character
     * @param str string to start with
     * @param index to use
     * @param char to replace with
     * @return str with char replaced
     */
    public static String setCharAt(String str, int x, char c) 
        throws IndexOutOfBoundsException {
        
        StringBuffer buf = new StringBuffer(str);
        buf.setCharAt(x, c);
        return buf.toString();

    }

    /** 
     * Replaces old string with new string.
     *
     * @param str string to replace old character with new string
     * @param oldStr old string to replace in string
     * @param newStr new string to replace old character with
     * @return string with replaced string
     */
    public static String replace(String str, String oldStr, String newStr)
    {
        StringBuffer buf = new StringBuffer();
        int curIndex = 0;
        int len = oldStr.length();

        while(true) {
            int x = str.indexOf(oldStr, curIndex);
            if (x != -1) {
                buf.append(str.substring(curIndex, x));
                buf.append(newStr);
                curIndex = x + len;
            } else {
                buf.append(str.substring(curIndex));
                break;
            }
        } 
        
        return buf.toString();
    }


    /**
     * Expand property values in a string. Property values need to be enclosed
     * in ${}. For example, the string
     * "${file.separator}lps${file.separator}logs" will be converted to
     * "/lps/logs".
     * @param str string to expand property values
     * @return expanded string or null, if a property variable wasn't closed
     * properly or a specified property value doesn't exist.
     */
    public static String expandPropertyValues(String str)
        throws Exception
    {
        StringBuffer buf = new StringBuffer();

        int i0, i1;
        int pos = 0;
        String propName;
        String propValue;
        while (pos < str.length()) {
            i0 = str.indexOf("${", pos); 
            if (i0 == -1) {
                buf.append(str.substring(pos));
                break;
            }
            i1 = str.indexOf('}', i0);
            if (i1 == -1)
                throw new Exception("missing close bracket: '}'");

            // Append part of string we've skipped
            if (i0 > pos)
                buf.append(str.substring(pos, i0));

            propName = str.substring(i0+2, i1);
            try {
                propValue = System.getProperty(propName);
                if (propValue == null)
                    throw new Exception("System property " + propName +
                                         " does not exist");
            } catch (SecurityException e) {
                // TODO [2004-07-14 bloch]: log exception?
                propValue = "";
            }

            buf.append(propValue);

            pos = i1+1;
        }
        return buf.toString();
    }
}
