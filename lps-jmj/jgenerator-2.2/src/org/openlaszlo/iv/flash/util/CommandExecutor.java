/*
 * $Id: CommandExecutor.java,v 1.5 2002/04/04 08:14:25 skavish Exp $
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
import org.openlaszlo.iv.flash.context.*;

import java.util.*;
import java.text.*;
import java.io.*;
import java.lang.reflect.*;

/**
 * Standard executor of context commands.
 * <P>
 * To add new command you have to add new method to this class
 * or to one of its subclasses. Name of the command will be name of the method.
 * If command was invoked without parameters then method without parameters will be
 * called, if command was invoked with several parameters then method with corresponding
 * number of parameters will be called. If none of these was succeded then method with
 * parameter Vector will be called.
 *
 * @author Dmitry Skavish
 * @see CommandContext
 */
public class CommandExecutor {

    protected FlashFile flashFile;

    /**
     * Create executor in environment of given flash file.
     *
     * @param file   environment for executor
     */
    public CommandExecutor( FlashFile flashFile ) {
        this.flashFile = flashFile;
    }

    public CommandExecutor() {
    }

    public void setFlashFile( FlashFile flashFile ) {
        this.flashFile = flashFile;
    }

    public FlashFile getFlashFile() {
        return flashFile;
    }

    /**
     * Executes command.
     *
     * @param context context from which the command was called
     * @param name    name fo the command
     * @param parms   parameters or null
     * @return result of the command execution
     */
    public String execute( Context context, String name, Vector parms ) {
        try {
            Class c = getClass();
            Method m;
            Object[] mp;
            try {
                if( parms == null ) {
                    m = c.getMethod( name, new Class[] {Context.class} );
                    mp = null;
                } else {
                    Class[] cmp = new Class[parms.size()+1];
                    cmp[0] = Context.class;
                    for( int i=1; i<cmp.length; i++ )
                        cmp[i] = String.class;
                    m = c.getMethod( name, cmp );
                    mp = new Object[cmp.length];
                    mp[0] = context;
                    for( int i=1; i<cmp.length; i++ )
                        mp[i] = parms.elementAt(i-1);
                }
            } catch( NoSuchMethodException e ) {
                m = c.getMethod( name, new Class[] {Context.class, Vector.class} );
                mp = new Object[] {context, parms};
            }
            Object res = m.invoke( this, mp );
            if( res == null ) return "";
            return res.toString();
        } catch( NoSuchMethodException e ) {
            Log.logRB(Resource.INLINECMDNOTFOUND, new Object[] {name});
            return "";
        } catch( IllegalAccessException e ) {
            Log.logRB(Resource.INLINECMDERROR, new Object[] {name}, e);
            return "";
        } catch( InvocationTargetException e ) {
            Log.logRB(Resource.INLINECMDERROR, new Object[] {name}, e);
            return "";
        }
    }

    /*---------------------------------------------------------------------------------*/
    /*                                   C O M M A N D S                               */
    /*---------------------------------------------------------------------------------*/

    /**
     * Returns current time and date
     *
     * @return current time and date
     */
    public String date( Context context ) {
        return new Date().toLocaleString();
    }

    /**
     * Returns current time and date formatted according to given template.<p>
     * Examples:
     * <UL>
     * <LI>MM/dd/yyyy hh:mm:ss z
     * </UL>
     *
     * @param format date format
     * @return current formatted date
     */
    public String date( Context context, String format ) {
        SimpleDateFormat formatter = new SimpleDateFormat(format);
        return formatter.format( new Date() );
    }

    /**
     * Return JGenerator version
     *
     * @return JGenerator version
     */
    public String version( Context context ) {
        return Util.getVersion();
    }

    /**
     * Returns substring of specified string
     *
     * @param s      specified string
     * @param from   start index
     * @return substring
     */
    public String substr( Context context, String s, String from ) {
        //System.out.println( "substr('"+s+"', '"+from+"')" );
        try {
            return s.substring( Util.toInt(from, -1) );
        } catch( Exception e ) {
            Log.logRB(e);
            return "";
        }
    }

    /**
     * Returns substring of specified string
     *
     * @param s      specified string
     * @param from   start index
     * @param to     to index
     * @return substring
     */
    public String substr( Context context, String s, String from, String to ) {
        //System.out.println( "substr('"+s+"', '"+from+"', '"+to+"')" );
        try {
            return s.substring( Util.toInt(from, -1), Util.toInt(to, -1) );
        } catch( Exception e ) {
            Log.logRB(e);
            return "";
        }
    }

    /**
     * Returns length of the specified string
     *
     * @param s      specified string
     * @return length of the specified string
     */
    public String len( Context context, String s ) {
        return Integer.toString(s.length());
    }

    /**
     * Converts hex to decimal: 20 -> 32
     *
     * @param s      string in hex representation
     * @return decimal representation
     */
    public String h2d( Context context, String s ) {
        return Integer.toString(Integer.parseInt(s, 16));
    }

    /**
     * Converts decimal to hex: 32 -> 20
     *
     * @param s      string in decimal representation
     * @return hex representation
     */
    public String d2h( Context context, String s ) {
        return Integer.toHexString(Integer.parseInt(s));
    }

    /**
     * Converts decimal to binary: 4 -> 100
     *
     * @param s      string in decimal representation
     * @return binary representation
     */
    public String d2b( Context context, String s ) {
        return Integer.toBinaryString(Integer.parseInt(s));
    }

    /**
     * Converts color to its web color representation
     * <P>
     * For example:
     * <P>
     * red -> #ffff0000
     * green -> #ff00ff00
     *
     * @param s      color specified either by name or by webcolor
     * @return web representation of color
     */
    public String color2web( Context context, String s ) {
        AlphaColor c = Util.toColor(s, AlphaColor.black);
        StringBuffer sb = new StringBuffer(10);
        sb.append('#');
        sb.append(Util.b2h(c.getAlpha()));
        sb.append(Util.b2h(c.getRed()));
        sb.append(Util.b2h(c.getGreen()));
        sb.append(Util.b2h(c.getBlue()));
        return sb.toString();
    }

    /**
     * Returns red component of specified color in decimal
     *
     * @param s      specified color either by its name or by web color
     * @return red component
     */
    public String red( Context context, String s ) {
        AlphaColor c = Util.toColor(s, AlphaColor.black);
        return Integer.toString(c.getRed());
    }

    /**
     * Returns green component of specified color in decimal
     *
     * @param s      specified color either by its name or by web color
     * @return green component
     */
    public String green( Context context, String s ) {
        AlphaColor c = Util.toColor(s, AlphaColor.black);
        return Integer.toString(c.getGreen());
    }

    /**
     * Returns blue component of specified color in decimal
     *
     * @param s      specified color either by its name or by web color
     * @return blue component
     */
    public String blue( Context context, String s ) {
        AlphaColor c = Util.toColor(s, AlphaColor.black);
        return Integer.toString(c.getBlue());
    }

    /**
     * Returns alpha component of specified color in decimal
     *
     * @param s      specified color either by its name or by web color
     * @return alpha component
     */
    public String alpha( Context context, String s ) {
        AlphaColor c = Util.toColor(s, AlphaColor.black);
        return Integer.toString(c.getAlpha());
    }

    /**
     * Loads and executes javascript file
     *
     * @param fileName file with javascript
     * @return whatever was printed during execution of javascript
     */
    public String js( Context context, String fileName ) {
        String s = fileName;
        int idx = s.indexOf('?');
        if( idx != -1 ) {
            fileName = s.substring(0, idx);
        }
        fileName = fileName.trim();

        File file = new File(fileName);
        if( !file.isAbsolute() )
            file = new File(flashFile.getFileDir(), file.getPath());
        fileName = file.getAbsolutePath();

        Hashtable parms = null;
        if( idx >= 0 ) parms = Util.parseUrlParms(s, idx);

        String[] args = new String[parms!=null?parms.size():0];
        if( parms != null ) {
            int i=0;
            for( Enumeration e = parms.keys(); e.hasMoreElements(); ) {
                args[i++] = (String) e.nextElement();
            }
        }

        String res = Util.executeJSFile(context, fileName, args);
        return res;
    }

}



