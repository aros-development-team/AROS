/*
 * $Id: TextField.java,v 1.4 2002/07/15 22:39:32 skavish Exp $
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

import java.io.*;
import java.awt.geom.*;

import org.openlaszlo.iv.flash.parser.Parser;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.context.Context;

/**
 * Text field.
 *
 * @author Dmitry Skavish
 */
public final class TextField extends FlashDef {

    public static final int HASLAYOUT    = 0x2000;
    public static final int NOSELECT     = 0x1000;
    public static final int BORDER       = 0x0800;
    public static final int HTML         = 0x0200;
    public static final int USEOUTLINES  = 0x0100;
    public static final int HASTEXT      = 0x0080;
    public static final int WORDWRAP     = 0x0040;
    public static final int MULTILINE    = 0x0020;
    public static final int PASSWORD     = 0x0010;
    public static final int READONLY     = 0x0008;
    public static final int HASTEXTCOLOR = 0x0004;
    public static final int HASMAXLENGTH = 0x0002;
    public static final int HASFONT      = 0x0001;

    private Rectangle2D bounds; // text field's bounds
    private Font font;          // font
    private int height;         // font's height in twixels
    private AlphaColor color;   // text color
    private int maxlength;      // maximum length of the text
    private int align;          // 0-left,1-right,2-center,3-justify
    private int leftmargin;     // left margin in twixels
    private int rightmargin;    // right margin in twixels
    private int indent;         // indentation in twixels
    private int leading;        // leading in twixels
    private String varName;     // variable name
    private String initText;    // initial text
    private int flags;          // text field's flags

    public TextField()  {}

    /**
     * Creates TextField
     *
     * @param initText initial text (optional)
     * @param varName  variable name
     * @param font     font of the field (optional)
     * @param height   height of the font in twixels
     * @param color    color of the text field (optional)
     */
    public TextField( String initText, String varName, Font font, int height, AlphaColor color ) {
        this.initText = initText;
        this.varName = varName;
        this.font = font;
        this.height = height;
        this.color = color;
        if( font != null ) flags = HASFONT;
        if( color != null ) flags |= HASTEXTCOLOR;
        if( initText != null ) flags |= HASTEXT;
    }

    /**
     * Returns maximum length of this text field
     *
     * @return maximum length
     */
    public int getMaxLength() {
        return maxlength;
    }

    /**
     * Sets new maximum length to this text field
     *
     * @param maxLenth new maximum length
     */
    public void setMaxLength( int maxlength ) {
        this.maxlength = maxlength;
        if( maxlength >= 0 ) flags |= HASMAXLENGTH;
        else flags &= ~HASMAXLENGTH;
    }

    /**
     * Sets new font and height
     *
     * @param font   new font to be set
     * @param height fonts height in twixels
     */
    public void setFont( Font font, int height ) {
        this.font = font;
        this.height = height;
        if( font != null ) flags |= HASFONT;
        else flags &= ~HASFONT;
    }

    /**
     * Returns font
     *
     * @return this textfield font
     */
    public Font getFont() {
        return font;
    }

    /**
     * Returns font height
     *
     * @return font height
     */
    public int getHeight() {
        return height;
    }

    /**
     * Returns variable name
     *
     * @return variable name
     */
    public String getVarName() {
        return varName;
    }

    /**
     * Sets new variable name
     *
     * @param varName new variable name
     */
    public void setVarName( String varName ) {
        this.varName = varName;
    }

    /**
     * Returns initial text
     *
     * @return inittial text
     */
    public String getInitText() {
        return initText;
    }

    /**
     * Sets new initial text to this text field
     *
     * @param initText new initial text
     */
    public void setInitText( String initText ) {
        this.initText = initText;
        if( initText != null ) flags |= HASTEXT;
        else flags &= ~HASTEXT;
    }

    /**
     * Sets new flags to this text field
     *
     * @param flags  new flags to be set
     */
    public void setFlags( int flags ) {
        this.flags = flags;
    }

    /**
     * Returns current flags of this text field
     *
     * @return current flags
     */
    public int getFlags() {
        return flags;
    }

    /**
     * Adds specified flags to this text field flags
     *
     * @param flags  additional flags
     * @return new flags
     */
    public int addFlags( int flags ) {
        this.flags |= flags;
        return flags;
    }

    /**
     * Removes specified flags
     *
     * @param flags  flags to be removed
     * @return new flags
     */
    public int removeFlags( int flags ) {
        this.flags &= ~flags;
        return flags;
    }

    /**
     * Sets bounds to this text field
     *
     * @param bounds text field bounds
     */
    public void setBounds( Rectangle2D bounds ) {
        this.bounds = bounds;
    }

    /**
     * Returns bounds of this text field
     *
     * @return bounds
     */
    public Rectangle2D getBounds() {
        return bounds;
    }

    /**
     * Sets layout of the text in this text field
     *
     * @param align       alignment (0-left,1-right,2-center,3-justify)
     * @param leftmargin  left margin in twixels
     * @param rightmargin right margin in twixels
     * @param indent      indentation in twixels
     * @param leading     leading in twixels
     */
    public void setLayout( int align, int leftmargin, int rightmargin, int indent, int leading ) {
        this.align = align;
        this.leftmargin = leftmargin;
        this.rightmargin = rightmargin;
        this.indent = indent;
        this.leading = leading;
        flags |= HASLAYOUT;
    }

    public int getAlign() { return align; }
    public int getLeftMargin() { return leftmargin; }
    public int getRightMargin() { return rightmargin; }
    public int getIndent() { return indent; }
    public int getLeading() { return leading; }

    /**
     * Sets color
     *
     * @param color  text color
     */
    public void setColor( AlphaColor color ) {
        this.color = color;
        if( color != null ) flags |= HASTEXTCOLOR;
        else flags &= ~HASTEXTCOLOR;
    }

    /**
     * Returns text color
     *
     * @return text color
     */
    public AlphaColor getColor() {
        return color;
    }

    public int getTag() {
        return Tag.DEFINEEDITTEXT;
    }

    public static TextField parse( Parser p ) {
        TextField o = new TextField();
        // get id
        o.setID( p.getUWord() );
        // get bounds
        o.bounds = p.getRect();

        // get flags
        int flags = o.flags = p.getUWord();

        if( (flags&HASFONT) != 0 ) {
            o.font = ((FontDef) p.getDef( p.getUWord() )).getFont();
            o.height = p.getUWord();
        }

        if( (flags&HASTEXTCOLOR) != 0 ) {
            o.color = Color.parseRGBA(p);
        }

        if( (flags&HASMAXLENGTH) != 0 ) {
            o.maxlength = p.getUWord();
        }

        if( (flags&HASLAYOUT) != 0 ) {
            o.align = p.getUByte();
            o.leftmargin = p.getUWord();
            o.rightmargin = p.getUWord();
            o.indent = p.getUWord();
            o.leading = p.getUWord();
        }

        o.varName = p.getString();

        if( (flags&HASTEXT) != 0 ) {
            o.initText = p.getString();
        }

        return o;
    }

    public void collectDeps( DepsCollector dc ) {
        if( (flags&HASFONT) != 0 ) {
            dc.addDep( font );
        }
    }

    public void collectFonts( FontsCollector fc ) {
        if( (flags&HASFONT) != 0 ) {
            FontDef fdef = fc.addFont( font, null );
            if( (flags&USEOUTLINES) != 0 ) {
                if( !PropertyManager.mxLibraryFontID.equalsIgnoreCase(initText) ) {
                    fdef.setWriteLayout(true);
                    fdef.setWriteAllChars(true);    // we don't know what characters will be needed, so we direct to write all
                }
            }
        }
    }

    public void write( FlashOutput fob ) {
        if( (flags&USEOUTLINES) != 0 && PropertyManager.mxLibraryFontID.equalsIgnoreCase(initText) ) {
            int start = fob.getPos();     // save for tag
            fob.skip(2);
            fob.writeDefID(this);
            fob.write(GeomHelper.newRectangle(0, 0, 1, 1));
            fob.writeWord(0);
            fob.writeStringZ("");
            int size = fob.getPos()-start-2;  // 2 - short tag
            fob.writeShortTagAt(getTag(), size, start);
        } else {
            // if varName.length + initText.length > 22, then generate long tag
            int l = varName.length() + ((flags&HASTEXT)!=0?initText.length():0);
            int start = fob.getPos();     // save for tag

            if( l > 22 ) {
                fob.skip(6);    // 6 - long tag
            } else {
                fob.skip(2);
            }

            fob.writeDefID(this);
            fob.write(bounds);
            fob.writeWord(flags);

            if( (flags&HASFONT) != 0 ) {
                fob.writeFontID( font );
                fob.writeWord(height);
            }

            if( (flags&HASTEXTCOLOR) != 0 ) {
                color.write(fob);
            }

            if( (flags&HASMAXLENGTH) != 0 ) {
                fob.writeWord(maxlength);
            }

            if( (flags&HASLAYOUT) != 0 ) {
                fob.writeByte(align);
                fob.writeWord(leftmargin);
                fob.writeWord(rightmargin);
                fob.writeWord(indent);
                fob.writeWord(leading);
            }

            fob.writeStringZ(varName);

            if( (flags&HASTEXT) != 0 ) {
                fob.writeStringZ(initText);
            }

            if( l > 22 ) {
                int size = fob.getPos()-start-6;  // 6 - long tag
                fob.writeLongTagAt( getTag(), size, start );
            } else {
                int size = fob.getPos()-start-2;  // 2 - short tag
                fob.writeShortTagAt( getTag(), size, start );
            }
        }
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"TextField: id="+getID()+", flags="+Util.w2h(flags) );
        out.println( indent+"    "+bounds );

        if( (flags&HASFONT) != 0 ) {
            out.println( indent+"    font="+font.getFontName()+" height="+height );
        }

        if( (flags&HASTEXTCOLOR) != 0 ) {
            color.printContent(out, indent+"    ");
        }

        if( (flags&HASMAXLENGTH) != 0 ) {
            out.println( indent+"    maxlength="+maxlength );
        }

        if( (flags&HASLAYOUT) != 0 ) {
            out.println( indent+"    align="+align );
            out.println( indent+"    leftmargin="+leftmargin );
            out.println( indent+"    rightmargin="+rightmargin );
            out.println( indent+"    indent="+indent );
            out.println( indent+"    leading="+leading );
        }

        out.println( indent+"    varName='"+varName+"'" );

        if( (flags&HASTEXT) != 0 ) {
            out.println( indent+"    initText='"+initText+"'" );
        }
    }

    public void apply( Context context ) {
        super.apply( context );
        varName = context.apply( varName );
        initText = context.apply( initText );
    }

    protected boolean _isConstant() {
        return !Util.hasVar(varName) && !Util.hasVar(initText);
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((TextField)item).bounds = (Rectangle2D) bounds.clone();
        ((TextField)item).font = font;
        ((TextField)item).height = height;
        ((TextField)item).color = (AlphaColor) color.getCopy(copier);
        ((TextField)item).maxlength = maxlength;
        ((TextField)item).align = align;
        ((TextField)item).leftmargin = leftmargin;
        ((TextField)item).rightmargin = rightmargin;
        ((TextField)item).indent = indent;
        ((TextField)item).leading = leading;
        ((TextField)item).varName = varName;
        ((TextField)item).initText = initText;
        ((TextField)item).flags = flags;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new TextField(), copier );
    }

}
