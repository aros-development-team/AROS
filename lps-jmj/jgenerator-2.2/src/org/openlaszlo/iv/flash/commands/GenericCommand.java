/*
 * $Id: GenericCommand.java,v 1.12 2002/07/17 05:13:37 skavish Exp $
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

package org.openlaszlo.iv.flash.commands;

import java.io.*;
import java.awt.geom.*;
import java.lang.reflect.*;

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.shape.*;
import org.openlaszlo.iv.flash.api.text.*;
import org.openlaszlo.iv.flash.util.*;

import org.openlaszlo.iv.flash.cache.*;
import org.openlaszlo.iv.flash.context.Context;
import org.openlaszlo.iv.flash.context.ContextFactory;

/**
 * Generic Generator command. All commands have to subclass it.
 */

public class GenericCommand extends FlashObject {

    public static final int TYPE_MOVIE      = 0x01;
    public static final int TYPE_BUTTON     = 0x02;
    public static final int TYPE_GRAPHICS   = 0x04;
    public static final int TYPE_GLOBAL     = 0x08;

    private int type;
    private int depth;
    private int flags;
    private int frameNum;

    private IVVector names = new IVVector();
    private IVVector values = new IVVector();
    private Instance instance;

    /** Needed to track if the command was created from a MX component */
    private boolean isComponent = false;

    private static final String MACROMEDIACLASS = "com.macromedia.generator.commands.";
    private static final String MXCOMPONENT_CMDNAME = "JGeneratorCommand";

    public GenericCommand() {}

    public void doCommand( FlashFile file, Context context, Script parent, int frame )
        throws IVException
    {
    }

    /**
     * Sets the isComponent flag
     *
     * @param isComponent boolean
     */
    public void setIsComponent(boolean isComponent) {
        this.isComponent = isComponent;
    }

    /**
     * Gets the isComponent flag
     *
     * @retun boolean
     */
    public boolean isComponent() {
        return isComponent;
    }

    public boolean isGlobal() {
        return (type&TYPE_GLOBAL) != 0;
    }

    /**
     * Returns command name
     *
     * @return command name
     */
    public String getCommandName() {
        String name = getClass().getName();
        int idx = name.lastIndexOf('.');
        return name.substring(idx+1);
    }

    /**
     * Returns instance
     *
     * @return
     */
    public Instance getInstance() {
        return instance;
    }

    public void setInstance( Instance instance ) {
        this.instance = instance;
    }

    /**
     * Returns an instance which is associated with commands modifying instances (like Replicate)
     * <P>
     * <OL>
     * <LI>if this command is MX component then
     *   <OL>
     *     <LI>get parameter "instancename"
     *     <LI>if this parameter is specified then search for an instance in the file with this name
     *     <LI>if found then use it
     *     <LI>if not found search in parent script for an instance which encloses this command.
     *         by enclosing I mean that its bounds encloses the comamnd bounds
     *   </OL>
     * <LI>if this command is not an MX component then just use this command instance as an instance
     * </OL>
     *
     * @param file     current flash file
     * @param context  current context
     * @param parent   parent script (where the command is placed)
     * @param frameNum frame number where the command is instantiated
     * @return the instance to which this command has to be applied
     * @exception IVException
     */
    public Instance getCommandInstance( FlashFile file, Context context, Script parent, int frameNum ) throws IVException {
        Instance inst = null;
        if( isComponent() ) {
            String instancename = getParameter(context, "instancename");
            if( instancename != null ) {
                instancename = instancename.trim();
                if( instancename.length() != 0 ) {
                    Script main = file.getMainScript();
                    inst = main.findInstance(instancename);
                }
            }
            if( inst == null ) {
                // find enclosure

                IVVector defs = new IVVector();
                IVVector instances = new IVVector();

                Timeline timeline = parent.getTimeline();
                int maxFrameNum = Math.min(timeline.getFrameCount(), frameNum+1);
                for( int i=0; i<maxFrameNum; i++ ) {
                    Frame frame = timeline.getFrameAt(i);
                    for( int k=0; k<frame.size(); k++ ) {
                        FlashObject fo = frame.getFlashObjectAt(k);
                        if( fo instanceof Instance ) {
                            Instance myInst = (Instance) fo;
                            int layer = myInst.depth;
                            FlashDef def = myInst.def;
                            if( def != null ) {
                                defs.setElementAt(def, layer);
                            }
                            instances.setElementAt(myInst, layer);
                        } else if( fo instanceof RemoveObject ) {
                            RemoveObject ro = (RemoveObject) fo;
                            int layer = ro.depth;
                            defs.setElementAt(null, layer);
                            instances.setElementAt(null, layer);
                        }
                    }
                }

                Point2D ptCmd = new Point2D.Double(-1024, -1024);
                instance.matrix.transform(ptCmd, ptCmd);

                int myDepth = instance.depth;   // depth of this command instance
                for( int i=myDepth; --i>=0; ) {
                    Instance myInst = (Instance) instances.elementAt(i);
                    if( myInst == null ) continue;
                    FlashDef def = (FlashDef) defs.elementAt(i);
                    if( def == null || !(def instanceof Script) ) continue;     // ??? error
                    Rectangle2D bounds = def.getBounds();
                    if( bounds != null && myInst.matrix != null ) {
                        bounds = GeomHelper.calcBounds(myInst.matrix, bounds);
                    }
                    if( bounds == null ) continue;
                    if( bounds.contains(ptCmd) ) {
                        inst = myInst;
                        break;
                    }
                }
            }
        } else {
            inst = instance;
        }

        if( inst == null ) {
            throw new IVException(Resource.CMDINSTNOTFOUND, new Object[] {getCommandName()});
        } else {
            inst.setCommand(this);
        }

        return inst;
    }

    public IVVector getParameterNames() {
        return names;
    }

    public void setParameter( String name, String value ) {
        for( int i=0; i<names.size(); i++ ) {
            String n = (String) names.elementAt(i);
            if( n.equals(name) ) {
                values.setElementAt(value, i);
                return;
            }
        }
        addParameter(name, value);
    }

    public void addParameter( String name, String value ) {
        names.addElement( name );
        values.addElement( value );
    }

    public String getParameter( String name ) {
        for( int i=0; i<names.size(); i++ ) {
            String n = (String) names.elementAt(i);
            if( n.equals(name) )
            {
                String value = (String) values.elementAt(i);
                if ( value == null || value.trim().length() == 0 )
                    return null;
                else
                    return value;
            }
        }
        return null;
    }

    public String getParameter( String name, String defaultValue ) {
        String res = getParameter( name );
        if( res != null ) return res;
        return defaultValue;
    }

    public boolean getBoolParameter( String name ) {
        return Util.toBool( getParameter( name ), false );
    }

    public boolean getBoolParameter( String name, boolean def ) {
        return Util.toBool( getParameter( name ), def );
    }

    public String getParameter( Context context, String name, String def ) {
        return context.apply( getParameter( name, def ) );
    }

    public String getParameter( Context context, String name ) {
        return context.apply( getParameter( name ) );
    }

    public boolean getBoolParameter( Context context, String name, boolean def ) {
        String v = context.apply( getParameter( name ) );
        return Util.toBool( v, def );
    }

    public AlphaColor getColorParameter( Context context, String name, AlphaColor def ) {
        String v = context.apply( getParameter( name ) );
        return Util.toColor( v, def );
    }

    public int getIntParameter( Context context, String name, int def ) {
        String v = context.apply( getParameter( name ) );
        return Util.toInt( v, def );
    }

    public double getDoubleParameter( Context context, String name, double def ) {
        String v = context.apply( getParameter( name ) );
        return Util.toDouble( v, def );
    }

    protected int findColumn( String columnName, String[][] data ) {
        for( int i=0; i<data[0].length; i++ ) {
            String name = data[0][i];
            if( name.equalsIgnoreCase(columnName) ) return i;
        }
        return -1;
    }

    protected boolean isDefault( String s ) {
        return Util.isDefault(s);
    }

    /**
     * Creates rectangular shape with default fill&amp;line styles.
     *
     * @param r      rectangle to be used for the shape
     * @return rectangular shape
     */
    protected Shape getMask( Rectangle2D r ) {
        Shape s = new Shape();
        s.setFillStyle0( FillStyle.newSolid( AlphaColor.white ) );
        s.setFillStyle1( 0 );
        s.setLineStyle( new LineStyle(1, AlphaColor.white) );
        s.drawRectangle(r);
        s.setBounds(r);
        return s;
    }

    /**
     * Adds mask for specified instance to specified script
     * <P>
     * Shifts all layers of all instances beginning from specified one,
     * then adds mask to specified frame and then searches when specified
     * instance gets removed from the timeline and if found removes the mask
     * in the same frame.
     * <P>
     * The mask inherits transformation of the specified instance.
     *
     * @param script   script to insert the mask to
     * @param frameNum frame number to put the mask in
     * @param inst     instance to be masked
     * @param width    width of the mask
     * @param height   height of the mask
     */
    protected void addMask( Script script, int frameNum, Instance inst, int width, int height ) {
        addMask(script, frameNum, inst, GeomHelper.newRectangle(0, 0, width, height));
    }

    protected void addMask( Script script, int frameNum, Instance inst, Rectangle2D maskRect ) {
        //System.out.println( "addMask: frame="+frameNum+", inst.depth="+inst.depth );
        int maskLayer = inst.depth;
        script.reserveLayers(maskLayer, 1);
        int instLayer = maskLayer+1;
        //System.out.println( "   after reserve: maskLayer="+maskLayer+", new inst.depth="+inst.depth );
        Frame frame = script.getFrameAt(frameNum);
        Instance maskInst = frame.addInstance( getMask(maskRect), maskLayer,
            (AffineTransform) (inst.matrix!=null?inst.matrix.clone(): null), null);
        maskInst.clip = instLayer;

        // searches when the instance gets removed and if found remove the mask at the same time
        int cnt = script.getFrameCount();
        main:
        for( int i=frameNum+1; i<cnt; i++ ) {
            Frame f = script.getFrameAt(i);
            int fsz = f.size();
            for( int j=0; j<fsz; j++ ) {
                FlashObject o = (FlashObject) f.getFlashObjectAt(j);
                if( o instanceof Instance ) {
                    Instance inst1 = (Instance) o;
                    if( inst1.depth == instLayer && inst1.isMove && inst1.def != inst.def && inst1.def != null ) {
                        //System.out.println( "      frame="+i+", found instance: inst1.depth="+inst1.depth+", inst1.def="+inst1.def );
                        f.addFlashObject(new RemoveObject(maskLayer));
                        break main;
                    }
                } else if( o instanceof RemoveObject ) {
                    RemoveObject ro = (RemoveObject) o;
                    if( ro.depth == instLayer ) {
                        //System.out.println( "      frame="+i+", found remove object: ro.depth="+ro.depth );
                        f.addFlashObject(new RemoveObject(maskLayer));
                        break main;
                    }
                }
            }
        }
    }

    protected Font getFont( FlashFile file, String fontName ) {
        FlashFile defFile = file.getDefaultSymbolFile();
        if( defFile != null ) {
            return defFile.getFont( fontName );
        }
        return null;
    }

    /**
     * Creates simple text and calculates its bounds
     * <P>
     * Text should not contain newlines
     *
     * @param txt    text
     * @param font   font
     * @param height font height
     * @param color  text color
     * @return created text
     */
    protected LazyText newSimpleText( String txt, Font font, int height, AlphaColor color ) {
        LazyText text = new LazyText(true);

        TextStyleChangeRecord tsr = new TextStyleChangeRecord(font, height, color);
        tsr.setX(0);
        tsr.setY(font.ascent);
        text.addTextStyleChangeRecord(tsr);

        TextRecord tr = new TextRecord(txt.length());
        int l = txt.length();
        int x = 0;
        for( int i=0; i<l; i++ ) {
            char ch = txt.charAt(i);
            int idx = font.getIndex(ch);
            if( idx == -1 ) idx = 0;
            int adv = font.getAdvanceValue(idx);
            if( i < l-1 ) {
                char ch_next = txt.charAt(i+1);
                int kern = font.getKerning(ch, ch_next);
                adv += kern;
            }
            tr.add(ch, idx, adv);
            x += adv;
        }

        text.setBounds(GeomHelper.newRectangle(0, 0, x, font.ascent+font.descent));

        return text;
    }

    protected Text newText( FlashFile file, String msg, Font font, int size, AlphaColor color, Rectangle2D rect ) {
        Text text = Text.newText();
        TextItem item = new TextItem( msg, font, size, color );
        text.addTextItem( item );
        text.setBounds( rect );
        return text;
    }

    protected Text newText( FlashFile file, String msg, Font font, int size, AlphaColor color, int width, int height ) {
        return newText( file, msg, font, size, color, GeomHelper.newRectangle(0,0,width,height) );
    }

    protected Text newText( FlashFile file, String msg, String fontName, int size, AlphaColor color, Rectangle2D rect ) {
        Font font = getFont( file, fontName );
        if( font == null ) return null;
        return newText( file, msg, font, size, color, rect );
    }

    protected void processFlashDef( Instance inst, FlashFile file, Context context ) throws IVException {
        if( inst.def != null ) {
            inst.def.process(file, context);
        }
    }

    protected Context makeContext( Context context, String[][] data, int row )
        throws IVException
    {
        return ContextFactory.createContext( context, data, row );
    }

    public void setDepth( int depth ) {
        this.depth = depth;
    }

    public void setFlags( int flags ) {
        this.flags = flags;
    }

    public void setFrameNum( int frameNum ) {
        this.frameNum = frameNum;
    }

    public int getDepth() {
        return depth;
    }

    public int getFlags() {
        return flags;
    }

    public int getFrameNum() {
        return frameNum;
    }

    /**
     * Sets command type
     * <P>
     * One of the following:
     * <UL>
     * <LI>TYPE_MOVIE
     * <LI>TYPE_BUTTON
     * <LI>TYPE_GRAPHICS
     * <LI>TYPE_GLOBAL
     * </UL>
     *
     * @param type   command type
     */
    public void setType( int type ) {
        this.type = type;
    }

    /**
     * Returns command type
     * <P>
     * One of the following:
     * <UL>
     * <LI>TYPE_MOVIE
     * <LI>TYPE_BUTTON
     * <LI>TYPE_GRAPHICS
     * <LI>TYPE_GLOBAL
     * </UL>
     *
     * @return command type
     */
    public int getType() {
        return type;
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"Generator Command: "+getClass().getName() );
        out.println( indent+"  Parameters: " );
        for( int i=0; i<values.size(); i++ ) {
            String name = (String) names.elementAt(i);
            String value = (String) values.elementAt(i);
            out.println( indent+"    "+name+"='"+value+"'" );
        }
    }

    public void write( FlashOutput fob ) {
        int tagpos = fob.getPos();
        fob.skip(6);    // skip tag

        fob.writeByte(type);
        fob.writeByte(0);
        fob.writeWord(depth);

        if( (type&TYPE_GRAPHICS) != 0 ) {
            fob.writeWord(flags);
            fob.writeWord(frameNum);
        }

        StringBuffer sb = new StringBuffer(100);
        sb.append(getClass().getName());

        // add parameters
        for( int i=0; i<names.size(); i++ ) {
            String n = (String) names.elementAt(i);
            String v = (String) values.elementAt(i);
            sb.append(' ');
            sb.append(n);
            sb.append("=\"");
            if( v != null ) {
                for( int j=0; j<v.length(); j++ ) {
                    char ch = v.charAt(j);
                    switch(ch) {
                        case '\n':
                            sb.append("/n/r");
                            break;
                        case '"':
                            sb.append("\\\"");
                            break;
                        default:
                            sb.append(ch);
                            break;
                    }
                }
            }
            sb.append('"');
        }

        fob.writeStringZ(sb.toString());

        fob.writeLongTagAt(getTag(), fob.getPos()-tagpos-6, tagpos);
    }

    public static GenericCommand parse( Parser p ) {
        // 0x01 - movie
        // 0x02 - button
        // 0x04 - graphics
        // 0x08 - global
        int type = p.getUByte();

        p.getUByte(); // skip 1 byte

        int depth = p.getUWord();
        int flags = 0;
        int frameNum = 0;
        if( (type&TYPE_GRAPHICS) != 0 ) { // graphics
            // 0x0010 - loop
            // 0x0020 - play once
            // 0x0040 - single frame
            flags = p.getUWord();
            frameNum = p.getUWord();
        }

        String command = p.getString();
        int clen = command.length();
        int idx = command.indexOf(' ');
        String className;
        if( idx == -1 ) {
            className = command;
            idx = clen;
        } else {
            className = command.substring(0,idx);
        }

        if( className.startsWith(MACROMEDIACLASS) ) {
            className = "org.openlaszlo.iv.flash.commands."+
                        className.substring(MACROMEDIACLASS.length());
        }
        Class cmdClazz = CommandCache.getCommandClass(className);
        if( cmdClazz == null ) {
            Log.logRB( Resource.CMDNOTFOUND, new Object[] {className} );
            return null;
        }
        GenericCommand cmd = null;
        try {
            cmd = (GenericCommand) cmdClazz.newInstance();
        } catch( Exception e ) {
            Log.logRB(Resource.CMDNOTCREATED, new Object[] {className}, e);
            return null;
        }

        cmd.setType(type);
        cmd.setDepth(depth);
        cmd.setFlags(flags);
        cmd.setFrameNum(frameNum);

        /* parse parameters, idx = index of blank
         *
         * format:
         *  commandname name1="value1" name2="value/n/rvalue/n/rvalue\"value\"" name3="value3"
         */
        StringBuffer sb = new StringBuffer();
        for( ; idx+1<clen; ) {
            int startName = idx+1;
            int endName = command.indexOf('=', startName);
            String name = command.substring(startName, endName);
            idx = endName+1;
            sb.setLength(0);
            boolean quotes = false;
        ValueLoop:
            while( idx < clen ) {
                char ch = command.charAt(idx);
                switch( ch ) {
                    case '\\':
                        sb.append( command.charAt(idx+1) );
                        idx += 2;
                        break;
                    case '/':
                        if( clen-idx >= 4 && command.charAt(idx+2)=='/' &&
                            ( (command.charAt(idx+1)=='n' && command.charAt(idx+3)=='r' ) ||
                              (command.charAt(idx+1)=='r' && command.charAt(idx+3)=='n' )
                            )
                          ) {
                            sb.append('\n');
                            idx += 4;
                        } else {
                            sb.append(ch);
                            idx++;
                        }
                        break;
                    case '"':
                        quotes = !quotes;
                        idx++;
                        break;
                    case ' ':
                        if( !quotes ) break ValueLoop;
                    default:
                        sb.append(ch);
                        idx++;
                        break;
                }
            }
            String value = new String(sb);
            cmd.addParameter( name, value );
        }

        return cmd;
    }

    /**
     * Check the ClipActions to retrieve variable of Commands<br>
     * if JGeneratorCommand variable exists in the clipEvent action
     *
     * @param actions ClipActions the ClipActions of the instance to test
     * @return java.util.Hashtable the variables
     */
    public static GenericCommand checkAndParseMX( Instance instance ) {
        if( instance.actions == null ) return null;

        IVVector acts = instance.actions.getActions();

        GenericCommand cmd = null;

        // collect variables
        for( int i = 0; i < acts.size(); i++ ) {
            ClipAction action = (ClipAction) acts.elementAt(i);

            if( action.getFlags() == ClipAction.MXCOMPONENT ) {

                Program prog = action.getProgram();

                FlashBuffer body = prog.body();
                body.setPos(0);
                boolean firstAction = true;
                String[] strings = null;

                String name = null;
                String value = null;

                // loop to retrieve all the variables
                mainLoop:
                for (;;) {
                    int offset = body.getPos();
                    int code = body.getUByte();
                    boolean hasLength = (code&0x80) != 0;
                    int length = hasLength? body.getUWord(): 0;
                    int nextPos = body.getPos()+length;

                    switch( code ) {
                        case Actions.None:
                            body.setPos(nextPos);
                            break mainLoop;
                        case Actions.PushData: {
                            for( int l=length; l>0; ) {
                                int type = body.getUByte();
                                l--;
                                switch( type ) {
                                    case 0:
                                        String ss = body.getString();
                                        if (name == null)
                                            name = ss;
                                        else
                                            value = ss;
                                        l-=ss.length()+1;
                                        break;
                                    case 1:
                                        value = new Float(Float.intBitsToFloat(body.getUDWord())).toString();
                                        l-=4;
                                        break;
                                    case 2:
                                    case 3:
                                        break;
                                    case 4:
                                        l--;
                                        break;
                                    case 5:
                                        value = new Boolean( body.getUByte()!=0 ).toString();
                                        l--;
                                        break;
                                    case 6:
                                        long dbits = ( ((long)body.getUDWord())<<32 ) | (((long)body.getUDWord())&0xffffffffL);
                                        value = new Double(Double.longBitsToDouble(dbits)).toString();
                                        l-=8;
                                        break;
                                    case 7:
                                        value = new Integer(body.getUDWord()).toString();
                                        l-=4;
                                        break;
                                    case 8:
                                        int index = body.getUByte();
                                        if ( strings != null && index < strings.length ) {
                                            if (name == null)
                                                name = strings[index];
                                            else
                                                value = strings[index];
                                        }
                                        l--;
                                        break;
                                }
                            }
                            // if this is first action it has to be: push "JGeneratorCommand", otherwise
                            // this is not our component
                            if( firstAction && !MXCOMPONENT_CMDNAME.equals(name) )
                                break mainLoop;
                            break;
                        }
                        case Actions.ConstantPool: {
                            int num = body.getUWord();
                            strings = new String[num];
                            for( int j=0; j<num; j++ ) {
                                strings[j] = body.getString();
                            }
                            // if this is first action it has to have "JGeneratorCommand" in first element,
                            // otherwise this is not our component
                            if( firstAction && !MXCOMPONENT_CMDNAME.equals(strings[0]) )
                                break mainLoop;
                            break;
                        }
                        case Actions.SetVariable:
                            if( MXCOMPONENT_CMDNAME.equals(name) ) {
                                // create jgenerator command from its name
                                String className = "org.openlaszlo.iv.flash.commands." + value + "Command";

                                Class cmdClazz = CommandCache.getCommandClass(className);
                                if( cmdClazz == null ) {
                                    Log.logRB( Resource.CMDNOTFOUND, new Object[] {className} );
                                    return null;
                                }

                                // instanciate it
                                try {
                                    cmd = (GenericCommand) cmdClazz.newInstance();
                                } catch( Exception e ) {
                                    Log.logRB(Resource.CMDNOTCREATED, new Object[] {className}, e);
                                    return null;
                                }

                                cmd.setDepth(instance.depth);
                                cmd.setType(cmd.isGlobal()?TYPE_GLOBAL:TYPE_MOVIE);
                                // indicate we created it from a MX component
                                cmd.setIsComponent(true);

                                // Embed it inside a MovieClip
                                Script script = new Script(1);
                                instance.def = script;
                                // we must translate the instance matrix because it is 2048x2048 centered
                                instance.matrix.translate(1024,1024);

                                // set the command
                                instance.setCommand(cmd);
                                cmd.setInstance(instance);

                            } else {
                                if( cmd != null )
                                    cmd.setParameter(name, value);
                                else
                                    break mainLoop;
                            }
                            name = null;
                            break;
                        default:
                            // all other actions mean this is not our component
                            break mainLoop;
                    }
                    firstAction = false;
                }
            }

            if( cmd != null ) {
                // this actions represented jgenerator mx component, remove it from list of clipactions
                acts.removeElementAt(i);
                instance.actions.setMask( instance.actions.getMask()&~ClipAction.MXCOMPONENT );
                break;  // no need for further processing
            }
        }

        if( acts.size() == 0 ) instance.actions = null;

        return cmd;
    }

    public int getTag() {
        return Tag.TEMPLATECOMMAND;
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((GenericCommand)item).type = type;
        ((GenericCommand)item).depth = depth;
        ((GenericCommand)item).flags = flags;
        ((GenericCommand)item).frameNum = frameNum;
        ((GenericCommand)item).instance = instance;
        ((GenericCommand)item).isComponent = isComponent;
        ((GenericCommand)item).names = new IVVector(names);
        ((GenericCommand)item).values = new IVVector(values);
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        try {
            Constructor c = getClass().getConstructor( new Class [] {} );
            GenericCommand o = (GenericCommand) c.newInstance( new Object[] {} );
            return copyInto( o, copier );
        } catch( Exception e ) {
            Log.log(e);
            return null;
        }

    }

}
