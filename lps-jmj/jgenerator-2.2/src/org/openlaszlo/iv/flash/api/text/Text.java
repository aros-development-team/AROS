/*
 * $Id: Text.java,v 1.5 2002/07/16 20:17:30 skavish Exp $
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
import java.util.*;
import java.awt.geom.*;

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.text.*;
import org.openlaszlo.iv.flash.context.Context;

/**
 * This class represent MM Generator undocumented text tag.
 * <P>
 * It contains vector of {@link TextItem}s.
 *
 * @author Dmitry Skavish
 */
public final class Text extends FlashDef implements TextBlock {

    public static final int PROPERTY_CONTROLLED = 0;
    public static final int MM_STYLE            = 1;
    public static final int JG_STYLE            = 2;

    private boolean withAlpha;              // true if color of change record has alpha
    private Rectangle2D bounds;             // original bounds of the text
    private Rectangle2D genBounds;          // bounds from MM Generator tag
    private int boundsStyle = PROPERTY_CONTROLLED;// jgen or MM bounds style, by default controlled by property
    private AffineTransform matrix;         // matrix of the text
    private IVVector items = new IVVector();// vector of TextItems
    private TextLayout myLayout;            // layout of this text

    public Text() {}

    public Text( boolean withAlpha )    {
        this.withAlpha = withAlpha;
    }

    public static Text newText() {
        Text t = new Text(true);
        t.matrix = new AffineTransform();
        return t;
    }

    public int getTag() {
        if( withAlpha ) return Tag.DEFINETEXT2;
        return Tag.DEFINETEXT;
    }

    // ------------------------------------------------------- //
    //                 TextBlock implementation                //

    /**
     * Layout this text
     */
    public void layout() {
        if( myLayout != null ) return;
        doLayout();
    }

    /**
     * Returns vector of {@link TextRecord}s from this text
     * of specified font.
     *
     * @param font   font of text records to be returned
     * @return text records of specified font
     */
    public IVVector getTextRecords( Font font ) {
        layout();
        return myLayout.getTextRecords( font );
    }

    /**
     * Returns vector of all {@link TextRecord}s and {@link TextStyleChangeRecord}s from this text
     *
     * @return all text records
     */
    public IVVector getAllTextRecords() {
        layout();
        return myLayout.getAllTextRecords();
    }

    /**
     * Updates records' font.
     * <P>
     * Changes one specified font into another in all records.
     * In text records also updates indexes.
     *
     * @param old_font old font
     * @param new_font new font
     */
    public void changeFont( Font old_font, Font new_font ) {
        layout();
        myLayout.changeFont(old_font, new_font);
    }

    // ------------------------------------------------------- //

    public void setBoundsStyle( int boundsStyle ) {
        this.boundsStyle = boundsStyle;
    }

    public int getBoundsStyle() {
        return boundsStyle;
    }

    protected void doLayout() {
        myLayout = new TextLayout(this,genBounds!=null?genBounds:bounds);
        myLayout.layout();
    }

    public void collectDeps( DepsCollector dc ) {
        for( int i=0; i<items.size(); i++ ) {
            TextItem t = (TextItem) items.elementAt(i);
            if( t.font != null ) dc.addDep( t.font );
        }
    }

    public void collectFonts( FontsCollector fc ) {
        for( int i=0; i<items.size(); i++ ) {
            TextItem t = (TextItem) items.elementAt(i);
            if( t.font != null ) fc.addFont( t.font, this );
        }
    }

    public void apply( Context context ) {
        if( myLayout != null ) return;
        super.apply( context );
        for( int i=0; i<items.size(); i++ ) {
            TextItem t = (TextItem) items.elementAt(i);
            t.apply( context );
        }
        // after applying context text bound could be changed, so we need to layout text
        doLayout();
    }

    public boolean isConstant() {
        for( int i=0; i<items.size(); i++ ) {
            TextItem t = (TextItem) items.elementAt(i);
            if( !t.isConstant() ) return false;
        }
        return true;
    }

    public Rectangle2D getBounds() {
        // do I have to call layout here ? probably yes
        layout();
        return bounds;
    }

    public void setGenBounds( Rectangle2D bounds ) {
        this.genBounds = bounds;
    }

    public void setBounds( Rectangle2D bounds ) {
        this.bounds = bounds;
    }

    public void setBounds( int x, int y, int width, int height ) {
        setBounds( GeomHelper.newRectangle(x,y,width,height) );
    }

    /**
     * Returns vector of all text items ({@link TextItem}) of this text.
     *
     * @return vector of {@link TextItem}
     */
    public IVVector getTextItems() {
        return items;
    }

    /**
     * Sets new vector of text items ({@link TextItem}) for this text.
     *
     * @param items  new vector of {@link TextItem}
     */
    public void setTextItems( IVVector items ) {
        this.items = items;
    }

    /**
     * Adds new text item to this text
     *
     * @param item   new text item to be added
     * @return added text item
     */
    public TextItem addTextItem( TextItem item ) {
        items.addElement(item);
        return item;
    }

    public void setMatrix( AffineTransform matrix ) {
        this.matrix = matrix;
    }

    public AffineTransform getMatrix() {
        return matrix;
    }

    /**
     * Parses any DefineText tag.
     * <P>
     * If current parsed file is not generator template
     * then parse text lazy ({@link LazyText}).
     *
     * @param p         parser to parse with
     * @param withAlpha with alpha or not
     * @return LazyText or Text
     */
    public static FlashDef parse( Parser p, boolean withAlpha ) {
        if( !p.getFile().isTemplate() ) {
            return LazyText.parse( p, withAlpha );
        } else {
            Text text = new Text( withAlpha );
            // get id
            text.setID( p.getUWord() );
            // get bounds and matrix
            text.bounds = p.getRect();
            text.matrix = p.getMatrix();

            // do not parse futher, generator text tag will follow
            p.skipLastTag();

            return text;
        }
    }

    /**
     * Parses MM Generator define text tag: 0x2A.
     * <P>
     * Creates vector of TextItems
     */
    public void parseGenText( Parser p ) {
        genBounds = p.getRect();

        TextItem item = new TextItem();
        boolean wasText = false;
        for(;;) {
            int code = p.getUByte();
            if( code == 0 ) {
                items.addElement( item );
                break;
            }
            if( (code&0x80) != 0 ) {
                // control record
                if( wasText ) {
                    // save text item from previous record
                    items.addElement( item );
                    item = (TextItem) item.getCopy(null);
                    item.text = null;
                    wasText = false;
                }
                code = code&0x7f;
                switch( code ) {
                    case 0:   // bold, italic
                        item.style = p.getUByte();
                        break;
                    case 1:   // font id
                        item.font = ((FontDef)p.getDef(p.getUWord())).getFont();
                        break;
                    case 2:   // font height
                        item.height = p.getUWord();
                        break;
                    case 3:   // color with alpha
                        item.color = Color.parseRGBA(p);
                        break;
                    case 4:   // 0 - normal, 1 - superscript, 2 - subscript
                        item.script = p.getUByte();
                        break;
                    case 5:   // kerning
                        item.kerning = p.getWord();
                        break;
                    case 8:   // left, center, right, justify
                        item.align = p.getUByte();
                        break;
                    case 9:   // indent
                        item.indent = p.getWord();
                        break;
                    case 10:  // left margin
                        item.marginleft = p.getWord();
                        break;
                    case 11:  // right margin
                        item.marginright = p.getWord();
                        break;
                    case 12:  // line space
                        item.linesp = p.getWord();
                        break;
                    default:
                        // there is nothing we can do, so just eat one byte
                        Log.logRB( Resource.UNKNOWNGENTEXT, new Object[] {Util.b2h(code)} );
                        p.getUByte();
                        break;
                }
            } else {

                String encoding = p.getFile().getEncoding();
                int length = code*2;
                String mystr;

                if( item.font != null && (item.font.flags&Font.UNICODE) != 0 ) {
                    // font is Unicode, so try to convert to Unicode using specified or default
                    // encoding + we need to repair broken DBCS (if any)
                    byte[] buf = p.getTempByteBuf(length);
                    int size = gen2DBCSbytes(p.getBuf(), p.getPos(), length, buf);
                    if( encoding == null ) {
                        mystr = new String(buf, 0, size);
                    } else {
                        try {
                            mystr = new String(buf, 0, size, encoding);
                        } catch( UnsupportedEncodingException e ) {
                            Log.log(e);
                            mystr = new String(buf, 0, size);
                        }
                    }
                    //System.out.println("unicode: '"+mystr+"'\n");
                } else {
                    // font is not Unicode, so we must not convert to Unicode, just leave
                    // the characters as is + repair broken DBCS (if any)
                    char[] buf = p.getTempCharBuf(length/2);
                    int size = gen2DBCSchars(p.getBuf(), p.getPos(), length, buf);
                    mystr = new String(buf, 0, size);
                    //System.out.println("normal: '"+mystr+"'\n");
                }

                item.text = item.text == null? mystr: item.text+mystr;

                p.skip(length);

                wasText = true;
            }
        }
    }

    /**
     * Converts array of bytes in Macromedia Generator character format
     * into array of bytes suitable to be processed by encoding converters.
     * <P>
     * Chars are considered to be in broken DBCS, result is suitable to be
     * converted to UNICODE.
     *
     * @param buf    array of bytes in MM Generator char format
     * @param pos    position in this array
     * @param length length of this array
     * @param outbuf output buffer suitable to be converted to Unicode
     * @return size of output buffer
     */
    public static int gen2DBCSbytes( byte[] buf, int pos, int length, byte[] outbuf ) {
        int size = 0;
        for( int i=0; i<length; i+=2 ) {
            byte hi = buf[pos++];
            byte lo = buf[pos++];
            if( lo == 0 ) {
                outbuf[size++] = hi;
            } else {
                outbuf[size+1] = hi;
                outbuf[size] = lo;
                size+=2;
            }
        }
        return size;
    }

    /**
     * Converts array of bytes in Macromedia Generator character format
     * into array of DBCS bytes
     * <P>
     * Chars are considered to be in broken DBCS, result is DBCS
     *
     * @param buf    array of bytes in MM Generator char format
     * @param pos    position in this array
     * @param length length of this array
     * @param outbuf output buffer, array od DBCS chars
     * @return size of output buffer
     */
    public static int gen2DBCSchars( byte[] buf, int pos, int length, char[] outbuf ) {
        int size = 0;
        for( int i=0; i<length; i+=2 ) {
            int hi = buf[pos++]&0xff;
            int lo = buf[pos++]&0xff;
            if( lo == 0 ) {
                outbuf[size++] = (char) hi;
            } else {
                outbuf[size++] = (char) ((hi<<8) | lo);
            }
        }
        return size;
    }

    public void write( FlashOutput fob ) {
        write(fob, this);
    }

    public void write( FlashOutput fob, FlashDef def ) {
        int start = fob.getPos();     // save for tag
        fob.skip( 6 );    // 6 - long tag
        fob.writeDefID(def);

        layout();
        fob.write(bounds);
        fob.write(matrix);

        myLayout.write(fob);

        int size = fob.getPos()-start-6;  // 6 - long tag
        // we need to generate Tag.DEFINETEXT2 all the time tag because from generator text we get rgba colors
        fob.writeLongTagAt(Tag.DEFINETEXT2, size, start );
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"Text: id="+getID() );
        out.println( indent+"    bounds: "+bounds );
        out.println( indent+"    genBounds: "+genBounds );
        out.println( indent+"    "+matrix );

        TextItem last = new TextItem();
        for( int i=0; i<items.size(); i++ ) {
            TextItem t = (TextItem) items.elementAt(i);
            out.println( indent+"    TextItem: '"+t.text+"'" );
            if( t.font        != last.font        ) out.println( indent+"      font  : '"+t.font.getFontName()+"'" );
            if( t.height      != last.height      ) out.println( indent+"      height: "+t.height );
            if( t.style       != last.style       ) out.println( indent+"      style : "+t.style );
            if( t.align       != last.align       ) out.println( indent+"      align : "+t.align );
            if( t.indent      != last.indent      ) out.println( indent+"      indent: "+t.indent );
            if( t.linesp      != last.linesp      ) out.println( indent+"      linesp: "+t.linesp );
            if( t.kerning     != last.kerning     ) out.println( indent+"      kerning: "+t.kerning );
            if( t.script      != last.script      ) out.println( indent+"      script: "+t.script );
            if( t.marginleft  != last.marginleft  ) out.println( indent+"      margin left: "+t.marginleft );
            if( t.marginright != last.marginright ) out.println( indent+"      margin right : "+t.marginright );
            if( t.color       != last.color       ) t.color.printContent(out, indent+"      ");
            last = t;
        }
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((Text)item).withAlpha = withAlpha;
        ((Text)item).bounds = (Rectangle2D) bounds.clone();
        ((Text)item).genBounds = genBounds==null?null:(Rectangle2D)genBounds.clone();
        ((Text)item).matrix = (AffineTransform) matrix.clone();
        ((Text)item).items = items.getCopy(copier);
        ((Text)item).myLayout = myLayout==null?null:myLayout.getCopy(copier);
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new Text(), copier );
    }
}
