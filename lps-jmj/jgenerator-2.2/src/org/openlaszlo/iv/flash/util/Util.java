/*
 * $Id: Util.java,v 1.8 2002/07/15 02:15:03 skavish Exp $
 *
 * ===========================================================================
 *
 * The JGenerator Software License, Version 1.0
 *
 * Copyright (c) 2000 Dmitry Skavish (skavish@usa.net). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution, if
 *    any, must include the following acknowlegement:
 *    "This product includes software developed by Dmitry Skavish
 *     (skavish@usa.net, http://www.flashgap.com/)."
 *    Alternately, this acknowlegement may appear in the software itself,
 *    if and wherever such third-party acknowlegements normally appear.
 *
 * 4. The name "The JGenerator" must not be used to endorse or promote
 *    products derived from this software without prior written permission.
 *    For written permission, please contact skavish@usa.net.
 *
 * 5. Products derived from this software may not be called "The JGenerator"
 *    nor may "The JGenerator" appear in their names without prior written
 *    permission of Dmitry Skavish.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL DMITRY SKAVISH OR THE OTHER
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

package org.openlaszlo.iv.flash.util;

import org.openlaszlo.iv.flash.api.*;

import org.openlaszlo.iv.flash.cache.*;
import org.openlaszlo.iv.flash.url.*;
import org.openlaszlo.iv.flash.context.*;

import java.awt.geom.Rectangle2D;
import java.awt.geom.AffineTransform;
import java.io.*;
import java.net.*;
import java.util.*;
import java.lang.reflect.*;

/**
 * Utility class.
 *
 * @author Dmitry Skavish
 */
public class Util {

    public static String genVersion = "2.0";

    public static double javaVersion = 1.2;
    public static char fileSeparator = '/';
    public static char pathSeparator = ':';
    public static String lineSeparator = "\n";

    private static String installDir;

    /**
     * Initialize JGenerator
     * <P>
     * Usually you need to call this method from standalone applications only
     */
    public static void init() {

        // try to find jgenerator install directory
        String installDir = System.getProperty("org.openlaszlo.iv.flash.installDir");

        if( installDir == null ) installDir = System.getProperty("iv.flash.installDir");

        // try to figure out install dir from classloader
        if( installDir == null ) {
            String resource = "/"+Util.class.getName().replace('.','/')+".class";
            URL classFileURL = getResource(resource);
            String path = classFileURL.getPath();
            if( path.startsWith("file:" ) ) {
                path = path.substring(5);
                int idx = path.lastIndexOf( "/lib/" );
                if( idx != -1 ) {
                    installDir = path.substring(0,idx);
                }
            } else {
                int idx = path.lastIndexOf( "/classes/" );
                if( idx != -1 ) {
                    installDir = path.substring(0,idx);
                }
            }
        }

        if( installDir == null ) installDir = "";

        init( installDir );
    }

    /**
     * Initialize JGenerator.
     * <P>
     * Sets installation directory and reads and caches some
     * basic properties and iv.properties file
     *
     * @param installDir jgenerator installation directory
     * @param propFileName iv.properties file name
     */
    public static void init( String installDir, String propFileName ) {
        Util.installDir = translatePath(installDir);

        PropertyManager.init(propFileName);

        try {
            String jVersion = System.getProperty("java.version").substring(0, 3);
            javaVersion = toDouble(jVersion, 1.2);
            fileSeparator = System.getProperty("file.separator","/").charAt(0);
            pathSeparator = System.getProperty("path.separator",":").charAt(0);
            lineSeparator = System.getProperty("line.separator","\n");
        } catch( Throwable e ) {
            Log.error(Resource.get(Resource.CANTLOADSYSPROPS));
        }
    }

    /**
     * Initialize JGenerator.
     * <P>
     * Sets installation directory and reads and caches some
     * basic properties and iv.properties file
     *
     * @param installDir jgenerator installation directory
     */
    public static void init( String installDir ) {
        init(installDir, null);
    }

    /**
     * Gets resource's URL
     *
     * @param resource resource name
     * @return resource URL
     */
    public static URL getResource( String resource ) {
        ClassLoader classLoader = null;
        URL url = null;

        try {
            classLoader = Thread.currentThread().getContextClassLoader();
            if( classLoader != null ) {
                url = classLoader.getResource(resource);
                if( url != null ) {
                    return url;
                }
            }

            // We could not find resource. Ler us now try with the
            // classloader that loaded this class.
            classLoader = Util.class.getClassLoader();

            url = classLoader.getResource(resource);
            if( url != null ) {
                return url;
            }
        } catch( Throwable t ) {
        }

        // get the resource from the class path
        return ClassLoader.getSystemResource(resource);
    }

    /**
     * Returns true if the specified string is "default" in some language
     *
     * @param s      string to be tested for "default"
     * @return true if the specified string is "default"
     */
    public static boolean isDefault( String s ) {
        return s.equalsIgnoreCase("default")     || s.equalsIgnoreCase("\u30c7\u30d5\u30a9\u30eb\u30c8") ||
               s.equalsIgnoreCase("Standard")    || s.equalsIgnoreCase("D\u00e9faut")                    ||
               s.equalsIgnoreCase("Padr\u00e3o") || s.equalsIgnoreCase("Predeterminado")                 ||
               s.equalsIgnoreCase("Predefinita");
    }

    /**
     * Converts specified string to int.
     *
     * @param v      string to be converted
     * @param def    default value to be used if there are some problems during conversion
     * @return int value
     */
    public static int toInt( String v, int def ) {
        if( v == null ) return def;
        try {
            return Integer.valueOf( v ).intValue();
        } catch( NumberFormatException e ) {
            return def;
        }
    }

    /**
     * Converts specified string to long.
     *
     * @param v      string to be converted
     * @param def    default value to be used if there are some problems during conversion
     * @return long value
     */
    public static long toLong( String v, long def ) {
        if( v == null ) return def;
        try {
            return Long.valueOf( v ).longValue();
        } catch( NumberFormatException e ) {
            return def;
        }
    }

    /**
     * Converts specified string to double.
     *
     * @param v      string to be converted
     * @param def    default value to be used if there are some problems during conversion
     * @return double value
     */
    public static double toDouble( String v, double def ) {
        if( v == null ) return def;
        try {
            return Double.valueOf( v ).doubleValue();
        } catch( NumberFormatException e ) {
            return def;
        }
    }

    /**
     * Converts specified string to boolean.
     *
     * @param v      string to be converted
     * @param def    default value to be used if there are some problems during conversion
     * @return boolean value
     */
    public static boolean toBool( String v, boolean def ) {
        if( v == null ) return def;
        return v.equalsIgnoreCase("TRUE") || v.equalsIgnoreCase("YES") || v.equalsIgnoreCase("1") || v.equalsIgnoreCase("ON");
    }

    /**
     * Converts specified string to color.
     *
     * @param v      string to be converted
     * @param def    default value to be used if there are some problems during conversion
     * @return color
     */
    public static AlphaColor toColor( String v, AlphaColor def ) {
        if( v == null || v.length() == 0 ) return def;
        if( v.charAt(0) == '#' ) {
            try {
                long rgba = Long.parseLong( v.substring(1), 16 );
                if( v.length() <= 7 ) rgba |= 0xff000000L;  // + alpha
                //System.out.println( v+"->"+Util.d2h((int)rgba) );
                return new AlphaColor( (int) rgba );
            } catch( NumberFormatException e ) {
                return def;
            }
        } else {
            AlphaColor c = null;
            try {
                int value = (int) Double.parseDouble(v);
                c = new AlphaColor(value);
                c.setAlpha(0xFF);
            } catch (NumberFormatException ex) {
                c = AlphaColor.getColor(v);
            }
            if( c == null ) return def;
            return c;
        }
    }

    /**
     * Processes escapes in the specified string.
     * <P>
     * Escapes to be processed:<br>
     * <UL>
     * <LI>\n     - newline
     * <LI>\r     - carriage return
     * <LI>\0x... - hex value
     * <LI>\0...  - octal value
     * </UL>
     *
     * @param line   string to be processed
     * @return escape-processed string
     */
    public static String processEscapes( String line ) {

        if( line == null ) return null;
        int cur = line.indexOf('\\');
        if( cur < 0 ) return line;

        int length = line.length();
        StringBuffer sb = new StringBuffer( line );
        sb.setLength( cur++ );

        for(; cur < length; ) {
            char ch = line.charAt(cur);
            switch( ch ) {
                case 'n':
                    sb.append('\n');
                    break;
                case 'r':
                    sb.append('\r');
                    break;
                case '0':
                    cur++;
                    if( cur >= length ) sb.append( 0 );
                    else {
                        if( line.charAt(cur) == 'x' || line.charAt(cur) == 'X' ) {
                            // hex
                            int code = 0;;
                            for( int l=++cur; cur<length && cur-l<4; cur++ ) {
                                char c = line.charAt(cur);
                                if( c >= '0' && c <= '9' ) {
                                    code = (code<<4) + (c-'0');
                                } else if( c >= 'a' && c <= 'f' ) {
                                    code = (code<<4) + (c-'a'+10);
                                } else if( c >= 'A' && c <= 'F' ) {
                                    code = (code<<4) + (c-'A'+10);
                                } else {
                                    break;
                                }
                            }
                            cur--;
                            sb.append( (char) code );
                        } else {
                            // oct
                            int code = 0;;
                            for( int l=cur; cur<length && cur-l<6; cur++ ) {
                                char c = line.charAt(cur);
                                if( c >= '0' && c <= '7' ) {
                                    code = (code<<3) + (c-'0');
                                } else {
                                    break;
                                }
                            }
                            cur--;
                            sb.append( (char) code );
                        }
                    }
                    break;
                default:
                    sb.append(ch);
                    break;
            }
            cur++;
            int end = line.indexOf( '\\', cur );
            if( end < 0 ) {
                sb.append( line.substring( cur, length) );
                break;
            }
            int l = end-cur;
            if( l == 0 ) {
                cur++;
            } else {
                sb.append( line.substring( cur, end) );
                cur = end+1;
            }
        }

        return new String( sb );
    }

    /**
     * Converts byte to unsigned byte.
     */
    public static int getUByte(byte a) {
        return a & 0xff;
    }

    /**
     * Creates signed word from two bytes.
     *
     * @param a      low byte
     * @param b      high byte
     * @return signed word
     */
    public static int getWord(byte a, byte b) {
        return (a&0xff) | (b << 8);
    }

    /**
     * Creates unsigned word from two bytes.
     *
     * @param a      low byte
     * @param b      high byte
     * @return unsigned word
     */
    public static int getUWord(byte a, byte b) {
        return (a&0xff) | ((b&0xff) << 8);
    }

    /**
     * Creates signed dword from four bytes.
     *
     * @param a      0 byte (low)
     * @param b      1 byte
     * @param c      2 byte
     * @param d      3 byte (high)
     * @return signed dword
     */
    public static int getDWord(byte a, byte b, byte c, byte d) {
        return (a&0xff) | ((b&0xff) << 8) | ((c&0xff) << 16) | (d << 24);
    }

    /**
     * Creates unsigned dword from four bytes.
     *
     * @param a      0 byte (low)
     * @param b      1 byte
     * @param c      2 byte
     * @param d      3 byte (high)
     * @return unsigned dword
     */
    public static int getUDWord(byte a, byte b, byte c, byte d) {
        return (a&0xff) | ((b&0xff) << 8) | ((c&0xff) << 16) | ((d&0xff) << 24);
    }

    /**
     * Returns maximum element of array.
     *
     * @param arr    array to be searched for maximum
     * @param size   size of the array
     * @return maximim element of the array
     */
    public static int getMax( int[] arr, int size ) {
        int max = Integer.MIN_VALUE;
        for( int i=0; i<size; i++ ) {
            if( arr[i] > max ) max = arr[i];
        }
        return max;
    }

    /**
     * Returns maximum element of array.
     *
     * @param arr    array to be searched for maximum
     * @return maximim element of the array
     */
    public static int getMax( int[] arr ) {
        return getMax( arr, arr.length );
    }

    /**
     * Returns maximum of absolute values of two numbers.
     *
     * @param a      specified number
     * @param b      specified number
     * @return maximum of two numbers
     */
    public static int getMax( int a, int b ) {
        if( a < 0 ) a = -a;
        if( b < 0 ) b = -b;
        if( a >= b ) return a;
        return b;
    }

    /**
     * Returns maximum of absolute values of three numbers.
     *
     * @param a      specified number
     * @param b      specified number
     * @param c      specified number
     * @return maximum of three numbers
     */
    public static int getMax( int a, int b, int c ) {
        if( a < 0 ) a = -a;
        if( b < 0 ) b = -b;
        if( c < 0 ) c = -c;
        if( a >= b && a >= c ) return a;
        if( b >= a && b >= c ) return b;
        return c;
    }

    /**
     * Returns maximum of absolute values of four numbers.
     *
     * @param a      specified number
     * @param b      specified number
     * @param c      specified number
     * @param d      specified number
     * @return maximum of four specified numbers
     */
    public static int getMax( int a, int b, int c, int d ) {
        if( a < 0 ) a = -a;
        if( b < 0 ) b = -b;
        if( c < 0 ) c = -c;
        if( d < 0 ) d = -d;

        if( a > b ) {
            if( a > c ) {
                if( a > d ) {
                    return a;
                } else {
                    return d;
                }
            } else {
                if( c > d ) {
                    return c;
                } else {
                    return d;
                }
            }
        } else {
            if( b > c ) {
                if( b > d ) {
                    return b;
                } else {
                    return d;
                }
            } else {
                if( c > d ) {
                    return c;
                } else {
                    return d;
                }
            }
        }
    }

    /**
     * Returns number of significant bits for signed integer.
     *
     * @param v      signed integer
     * @return number of significant bits for signed integer
     */
    public static int getMinBitsS( int v ) {
        if( v < 0 ) v = -v;
        return getMinBitsU( v )+1;
    }

    private static final int[] BITS_LENGTH = {
    //  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, E, F
        0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4
    };

    /**
     * Returns number of significant bits for unsigned integer.
     *
     * @param v      unsigned integer
     * @return number of significant bits for unsigned integer
     */
    public static int getMinBitsU( int v ) {
        int n = 0;
        if( (v & ~0xffff) != 0 ) {
            n+=16; v >>>= 16;
        }
        if( (v & ~0x00ff) != 0 ) {
            n+= 8; v >>>=  8;
        }
        if( (v & ~0x000f) != 0 ) {
            n+= 4; v >>>=  4;
        }
        // for( ; v != 0; n++ ) v >>>= 1;
        n += BITS_LENGTH[v];
        return n;
    }

    /* --------------------------------------------------------------------------
     * Double, float, fixed, twips conversion helpers
     * -------------------------------------------------------------------------- */

    private static final double FRACTION_DBL = (double) 0x10000;
    private static final float  FRACTION_FLT = (float) 0x10000;

    /**
     * Converts fixed Flash value into double
     *
     * @param value  fixed Flash value
     * @return double value
     */
    public static double fixed2double( int value ) {
        return (double)value / FRACTION_DBL;
    }

    /**
     * Converts fixed Flash value into float
     *
     * @param value  fixed Flash value
     * @return float value
     */
    public static float fixed2float( int value ) {
        return (float)value / FRACTION_FLT;
    }

    /**
     * Converts double to fixed value
     *
     * @param value  double value
     * @return fixed value
     */
    public static int double2fixed( double value ) {
        return (int) (value * FRACTION_DBL);
    }

    /**
     * Converts float to fixed value
     *
     * @param value  float value
     * @return fixed value
     */
    public static int float2fixed( float value ) {
        return (int) (value * FRACTION_FLT);
    }

    /**
     * Converts twips to double
     *
     * @param value  twips value
     * @return double value
     */
    public static double twips2double( int value ) {
        return (double)value / 20.0;
    }

    /**
     * Converts twips to float
     *
     * @param value  twips value
     * @return float value
     */
    public static float twips2float( int value ) {
        return (float)value / 20.0f;
    }

    /**
     * Converts double to twips
     *
     * @param value  double value
     * @return twips value
     */
    public static int double2twips( double value ) {
        return (int) (value * 20.0);
    }

    /**
     * Converts float to twips
     *
     * @param value  float value
     * @return twips value
     */
    public static int float2twips( float value ) {
        return (int) (value * 20.0f);
    }

    private static final char[] digits = {
        '0' , '1' , '2' , '3' , '4' , '5' ,
        '6' , '7' , '8' , '9' , 'a' , 'b' ,
        'c' , 'd' , 'e' , 'f'
    };

    /**
     * Returns hexadecimal representation of specified byte
     *
     * @param v      specified byte
     * @return hex repsentation of specified byte
     */
    public static String b2h( int v ) {
        char[] r = new char[2];
        r[0] = digits[ (v&0xf0)>>4 ];
        r[1] = digits[ (v&0x0f)    ];
        return new String( r );
    }

    /**
     * Returns hexadecimal representation of specified word
     *
     * @param v      specified word
     * @return hex repsentation of specified word
     */
    public static String w2h( int v ) {
        char[] r = new char[4];
        r[0] = digits[ (v&0xf000)>>12 ];
        r[1] = digits[ (v&0x0f00)>>8  ];
        r[2] = digits[ (v&0x00f0)>>4  ];
        r[3] = digits[ (v&0x000f)     ];
        return new String( r );
    }

    /**
     * Returns hexadecimal representation of specified dword
     *
     * @param v      specified dword
     * @return hex repsentation of specified dword
     */
    public static String d2h( int v ) {
        char[] r = new char[8];
        r[0] = digits[ (v&0xf0000000)>>>28 ];
        r[1] = digits[ (v&0x0f000000)>>24 ];
        r[2] = digits[ (v&0x00f00000)>>20 ];
        r[3] = digits[ (v&0x000f0000)>>16 ];
        r[4] = digits[ (v&0x0000f000)>>12 ];
        r[5] = digits[ (v&0x00000f00)>>8  ];
        r[6] = digits[ (v&0x000000f0)>>4  ];
        r[7] = digits[ (v&0x0000000f)     ];
        return new String( r );
    }

    /**
     * Returns specified char if it's printable or '.'
     *
     * @param ch     char to be converted to printable char
     * @return printable char
     */
    public static char toPrint( char ch ) {
        if( Character.isISOControl(ch) ) return '.';
        return ch;
    }

    private static void _dump( byte[] buffer, int pos, int size, PrintStream out ) {
        if( size == 0 ) return;
        for( int i=0; i<16; i++ ) {
            if( i >= size ) {
                out.print( ".. " );
            } else {
                out.print( b2h(buffer[pos+i])+' ' );
            }
        }
        for( int i=0; i<16; i++ ) {
            if( i >= size ) {
                out.print( " " );
            } else {
                out.print( toPrint( (char) buffer[pos+i] ) );
            }
        }
    }

    /**
     * Prints hex dump of part of byte array to specified stream
     *
     * @param buffer buffer to be dumped
     * @param start  offset in the buffer to start the dump from
     * @param size   size of the dumped area
     * @param out    stream to print the dump to
     */
    public static void dump( byte[] buffer, int start, int size, PrintStream out ) {
        StringBuffer sb = new StringBuffer();
        int pos = 0;
        for( int i=0; i<size/16; i++ ) {
            _dump( buffer, start+pos, 16, out );
            pos += 16;
            out.println();
        }
        _dump( buffer, start+pos, size-pos, out );
        out.println();
    }

    /**
     * Returns whether specified string has generator variable or not.
     *
     * @param s      string to be searched for generator variable
     * @return true if the specified string has generator variable
     */
    public static boolean hasVar( String s ) {
        return s != null && s.indexOf('{') != -1;
    }

    /**
     * Returns concatenation of path and filename.
     *
     * @param path   path
     * @param name   filename
     * @return concatenation of specified path and specified filename
     */
    public static String concatFileNames( String path, String name ) {
        if( path.length() == 0 ) return name;
        char ch = path.charAt( path.length()-1 );
        if( ch != '\\' && ch != '/' ) path += fileSeparator;
        if( name.charAt(0) == '/' || name.charAt(0) == '\\' ) return path+name.substring(1);
        return path+name;
    }

    /**
     * Returns directory where jgenerator is installed
     *
     * @return directory where jgenerator is installed
     */
    public static String getInstallDir() {
        if( installDir == null ) {
            init();
        }
        return installDir;
    }

    /**
     * Returns jgenerator file by its name.
     * <P>
     * If name is relative returns absolute path constructing
     * with {@link #installDir} and the specified name
     *
     * @param name   file name
     * @return jgenerator absolute file
     */
    public static File getSysFile( String name ) {
        name = translatePath( name );
        File file = new File( name );
        if( file.isAbsolute() ) return file;
        return new File( installDir, name );
    }

    /**
     * Translates file separator in the specified string into
     * file separator of the system jgenerator is running on.
     *
     * @param path   path to be translated
     * @return translated path
     */
    public static String translatePath( String path ) {
        if( fileSeparator == '\\' ) {
            path = path.replace('/', '\\');
        } else if( fileSeparator == '/' ) {
            path = path.replace('\\', '/');
        }
        return path;
    }

    /**
     * Creates URL from specified file.
     *
     * @param file   file to be used to create URL
     * @return created URL from File
     * @exception MalformedURLException
     */
    public static URL toURL( File file ) throws MalformedURLException {
        String path = file.getAbsolutePath();
        if( fileSeparator != '/' ) {
            path = path.replace( fileSeparator, '/' );
        }
        if( path.charAt(0) != '/' ) {
            path = '/'+path;
        }
        if( !path.endsWith("/") && file.isDirectory() ) {
            path = path+'/';
        }
        return new URL("file", "", path);
    }

    /**
     * Reads data from specified url into FlashBuffer.
     *
     * @param url    IVUrl to read from
     * @return FlashBuffer with data read
     * @exception IOException
     */
    public static FlashBuffer readUrl( IVUrl url ) throws IOException {
        FlashBuffer fob = null;
        InputStream is = null;
        try {
            is = url.getInputStream();
            fob = new FlashBuffer( is );
        } finally {
            try {
                if( is != null ) is.close();
            } catch( Exception e ) {}
        }

        return fob;
    }

    /**
     * Creates new instance of an specified class
     *
     * @param className specified class name
     * @param parmsCls  parameters types of constructor
     * @param parms     parameters of constructor
     * @return new instance
     * @exception Exception
     */
    public static Object newInstance( String className, Class[] parmsCls, Object[] parms ) throws Exception {
        Class clazz = Class.forName(className);
        if( parmsCls == null ) {
            return clazz.newInstance();
        } else {
            Constructor constr = clazz.getConstructor(parmsCls);
            return constr.newInstance(parms);
        }
    }

    /**
     * Executes specified javascript text in given Context
     *
     * @param context specified context
     * @param js_text javascript text
     * @param parms   parameters for javascript
     * @return result of "printing" in javascript
     */
    public static String executeJSString( Context context, String js_text, String[] parms ) {
        return executeJSHelperMethod("execString", context, js_text, parms);
    }

    /**
     * Executes specified javascript file in given Context
     *
     * @param context specified context
     * @param js_text javascript text
     * @param parms   parameters for javascript
     * @return result of "printing" in javascript
     */
    public static String executeJSFile( Context context, String fileName, String[] parms ) {
        return executeJSHelperMethod("execFile", context, fileName, parms);
    }

    private static String executeJSHelperMethod( String method, Context context, String s, String[] parms ) {
        FlashBuffer fb = new FlashBuffer(400);
        try {
            PrintStream out = new PrintStream(fb.getOutputStream());
            Class clazz = Class.forName("org.openlaszlo.iv.flash.js.JSHelper");
            Method exec = clazz.getMethod(method,
                                    new Class[] {String.class, String[].class,
                                                 PrintStream.class, Context.class});
            exec.invoke(null, new Object[] {s, parms, out, context});
            out.flush();
            return fb.toString();
        } catch( Throwable e ) {
            Log.log(e);
            return null;
        }
    }

    /**
     * Parses parameters of url-string begining after the specified index (usually index of '?')
     * <p>
     * Parameters are pairs: name=value, separated by ampersand
     *
     * @param s      url
     * @param idx    specified index
     * @return hashtable of parameter
     * @exception IVException
     */
    public static Hashtable parseUrlParms( String s, int idx ) {
        s = s.substring(idx+1);
        Hashtable parms = new Hashtable();
        StringTokenizer st = new StringTokenizer(s,"&");
        while( st.hasMoreTokens() ) {
            String token = st.nextToken();
            idx = token.indexOf('=');
            if( idx == -1 ) {
                parms.put(decodeURLEncoded(token).toLowerCase(), "");
            } else {
                String name = decodeURLEncoded(token.substring(0, idx)).toLowerCase();
                String value = decodeURLEncoded(token.substring(idx+1));
                parms.put(name, value);
            }
        }
        return parms;
    }

    /**
     * Decodes url-encoded string
     *
     * @param s      url-encoded string
     * @return decoded string
     */
    public static String decodeURLEncoded( String s ) {
        if( s.indexOf('+')<0 && s.indexOf('%')<0 ) return s;

        StringBuffer sb = new StringBuffer(s.length());
        for( int i=0; i<s.length(); i++ ) {
            char ch = s.charAt(i);
            if( ch == '+' ) {
                sb.append(' ');
            } else if( ch == '%' ) {
                ch = (char) Integer.parseInt(s.substring(i+1,i+3), 16);
                sb.append(ch);
                i+=2;
            } else {
                sb.append(ch);
            }
        }
        return new String(sb);
    }

    /**
     * Returns Reader of specified IVUrl using encoding either
     * from specified IVUrl or specified FlashFile
     *
     * @param file   file
     * @param url    url
     * @return Reader
     */
    public static LineReader getUrlReader( FlashFile file, IVUrl url ) throws IOException {
        String encoding;

        String url_encoding = url.getEncoding();
        if( url_encoding != null ) {
            encoding = url_encoding;
        } else {
            encoding = file == null? null: file.getEncoding();
            if( encoding == null ) {
                encoding = PropertyManager.defaultEncoding;
            }
        }

        return getInputStreamReader(url.getInputStream(), encoding);
    }

    /**
     * Returns Reader of specified String using encoding from specified FlashFile
     *
     * @param file   file
     * @param buf    buffer
     * @param offset offset in the buffer
     * @param length buffer length
     * @return reader
     * @exception IOException
     */
    public static LineReader getArrayReader( FlashFile file, byte[] buf, int offset, int length ) throws IOException {

        String encoding = file == null? null: file.getEncoding();
        if( encoding == null ) {
            encoding = PropertyManager.defaultEncoding;
        }

        return getInputStreamReader(new ByteArrayInputStream(buf, offset, length), encoding);
    }

    /**
     * Returns BufferedReader of specified InputStream and encoding
     *
     * @param is       input stream
     * @param encoding encoding (may be null)
     * @return reader
     */
    public static LineReader getInputStreamReader( InputStream is, String encoding ) throws IOException {
        Reader reader = null;
        if( encoding != null ) {
            try {
                reader = new InputStreamReader(is, encoding);
            } catch( UnsupportedEncodingException e ) {
                Log.log(e);
                // fall through
            }
        }

        if( reader == null ) {
            reader = new InputStreamReader(is);
        }

        return new BufferedLineReader(new BufferedReader(reader));
    }

    /**
     * Returns version of JGenerator.
     *
     * @return version of JGenerator
     */
    public static String getVersion() {
        return genVersion;
    }

}
