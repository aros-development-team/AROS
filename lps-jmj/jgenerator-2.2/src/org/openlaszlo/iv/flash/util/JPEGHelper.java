/*
 * $Id: JPEGHelper.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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
import org.openlaszlo.iv.flash.api.image.*;
import java.io.*;

/**
 * Simple JPEG scanner helping in extracting image's width and height.
 *
 * @author Dmitry Skavish
 */
public class JPEGHelper {

    public static final int M_SOF0  = 0xC0;
    public static final int M_SOF1  = 0xC1;
    public static final int M_SOF2  = 0xC2;
    public static final int M_SOF3  = 0xC3;
    public static final int M_SOF5  = 0xC5;
    public static final int M_SOF6  = 0xC6;
    public static final int M_SOF7  = 0xC7;
    public static final int M_SOF9  = 0xC9;
    public static final int M_SOF10 = 0xCA;
    public static final int M_SOF11 = 0xCB;
    public static final int M_SOF13 = 0xCD;
    public static final int M_SOF14 = 0xCE;
    public static final int M_SOF15 = 0xCF;
    public static final int M_SOI   = 0xD8;
    public static final int M_EOI   = 0xD9;
    public static final int M_SOS   = 0xDA;
    public static final int M_APP14 = 0xEE;

    public static JPEGBitmap.JPEGInfo getInfo( byte[] b, int p, int end ) {
        if( Util.getUByte(b[p])!=0xff || Util.getUByte(b[p+1])!=M_SOI ) return null;

        JPEGBitmap.JPEGInfo info = new JPEGBitmap.JPEGInfo();
        // Default to progressive
        info.type = 1;

        p += 2;   // skip SOI
        // Find SOF0 marker
        while( p < end ) {
            if( Util.getUByte(b[p]) == 0xff ) {
                int id = Util.getUByte(b[p+1]);
                int len = Util.getUWord(b[p+3], b[p+2]);
                switch( id ) {
                    case M_EOI:
                        return info;    // no SOF0
                    case M_SOF0:
                    case M_SOF1:
                    case M_SOF3:
                    case M_SOF5:
                    case M_SOF7:
                    case M_SOF9:
                    case M_SOF11:
                    case M_SOF13:
                    case M_SOF15:
                        // Not progressive
                        info.type = 0;
                        // Fall through
                    case M_SOF2:
                    case M_SOF6:
                    case M_SOF10:
                    case M_SOF14: {
                       info.precision = b[p+4];
                       info.height = Util.getUWord(b[p+6], b[p+5]);
                       info.width = Util.getUWord(b[p+8], b[p+7]);
                       info.num_comps = Util.getUByte(b[p+9]);
                       //compID[0] = GB(fp+10);
                       //compID[1] = GB(fp+13);
                       //compID[2] = GB(fp+16);
                       return info;
                    }
                }
                p += len+2;
            } else {
                p++;
            }
        }

        return info;
    }
}
