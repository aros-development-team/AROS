/*
 * $Id: TextItem.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
 *
 * ==========================================================================
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

package org.openlaszlo.iv.flash.api.text;

import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.context.Context;
import java.io.*;

/**
 * This class contains info for one text style run
 * from MM Generator undocumented text tag.
 * <P>
 * The objects of this class are not supposed to be written
 * to flash buffer (i.e. generated). They are used by
 * layout managers (for example {@link org.openlaszlo.iv.flash.util.TextLayout})
 * to lay out text. The layout managers produce {@link TextRecord}
 * and {@link TextStyleChangeRecord}.
 *
 * @author Dmitry Skavish
 */
public final class TextItem extends FlashItem {

    public Font font = null;      // font
    public Color color = null;    // color
    public int height;            // font height
    public int style  = 0;        // 0x01 - bold, 0x02 - italic
    public int align  = 0;        // 0-left, 1-right, 2-center, 3-justify
    public int indent = 0;        // indentation
    public int marginleft  = 0;   // margin left
    public int marginright = 0;   // margin right
    public int linesp = 0;        // line space
    public int kerning = 0;       // kerning
    public int script = 0;        // 0-normal, 1-superscript, 2-subscript
    public String text;           // text

    public TextItem() {}

    public TextItem( String text, Font font, int height, Color color ) {
        this.text = text;
        this.font = font;
        this.height = height;
        this.color = color;
    }

    public void apply( Context context ) {
        text = context.apply(text);
        text = text.replace( (char) 9, ' ' );
    }

    public boolean isConstant() {
        return !Util.hasVar(text);
    }

    public void write( FlashOutput fob ) {
        // ......
    }


    public void printContent( PrintStream out, String indent ) {
        // ......
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        TextItem t = (TextItem) item;
        t.font = font;
        t.color = (Color) color.getCopy(copier);
        t.height = height;
        t.style = style;
        t.align = align;
        t.indent = indent;
        t.marginleft = marginleft;
        t.marginright = marginright;
        t.linesp = linesp;
        t.kerning = kerning;
        t.script = script;
        t.text = text;
        return t;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new TextItem(), copier );
    }

}

