/*
 * $Id: Program.java,v 1.6 2002/08/02 03:15:17 skavish Exp $
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

package org.openlaszlo.iv.flash.api.action;

import java.io.PrintStream;

import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.context.Context;

/**
 * Wrapper of ActionScript bytecode.
 * <P>
 * The bytecodes are not parsed into some objects but rather
 * kept in their "bytecoded" form. Parsing the stuff is very
 * simple and straightforward and due to the nature of JGenerator
 * this is what we need most of the time.
 *
 * @author Dmitry Skavish
 */
public final class Program extends FlashObject {

    // properties
    public static final int PROP_X            =  0;
    public static final int PROP_Y            =  1;
    public static final int PROP_XSCALE       =  2;
    public static final int PROP_YSCALE       =  3;
    public static final int PROP_CURRENTFRAME =  4;
    public static final int PROP_TOTALFRAMES  =  5;
    public static final int PROP_ALPHA        =  6;
    public static final int PROP_VISIBLE      =  7;
    public static final int PROP_WIDTH        =  8;
    public static final int PROP_HEIGHT       =  9;
    public static final int PROP_ROTATION     = 10;
    public static final int PROP_TARGET       = 11;
    public static final int PROP_FRAMESLOADED = 12;
    public static final int PROP_NAME         = 13;
    public static final int PROP_DROPTARGET   = 14;
    public static final int PROP_URL          = 15;
    public static final int PROP_HIGHQUALITY  = 16;
    public static final int PROP_FOCUSRECT    = 17;
    public static final int PROP_SOUNDBUFTIME = 18;
    public static final int PROP_QUALITY      = 19;
    public static final int PROP_XMOUSE       = 20;
    public static final int PROP_YMOUSE       = 21;

    private FlashBuffer body;

    /**
     * Creates empty program of default capacity.
     */
    public Program() {
        body = new FlashBuffer(50);
    }

    /**
     * Creates program from specified flashbuffer.
     * <P>
     * Specified flashbuffer is not copied but directly used.
     *
     * @param body   flashbuffer containing actionscript bytecode
     */
    public Program( FlashBuffer body ) {
        this.body = body;
    }

    /**
     * Creates program from specified buffer of bytecodes.
     * <P>
     * The bytecodes from the buffer copied into internal flashbuffer
     *
     * @param buf    buffer containing bytecodes
     * @param start  offset in the buffer
     * @param end    end offset in the buffer
     */
    public Program( byte[] buf, int start, int end ) {
        int bufLength = end-start;
        byte[] myBuf = new byte[bufLength];
        System.arraycopy(buf, start, myBuf, 0, bufLength);
        body = new FlashBuffer( myBuf, bufLength );
    }

    public int getTag() {
        return -1;
    }

    /**
     * Applies specified context to this program
     * <P>
     * Searches for all possible generator variables in this program
     * data and applies specified context to them.<br>
     * Since changing the data may change the offsets used in the program
     * to perform gotos and jumps the special care taken to resolve forward
     * references in the bytecode.
     *
     * @param context specified generator context
     */
    public void apply( Context context ) {
        int[] new_offs = new int[getLength()];
        FlashBuffer fob = new FlashBuffer(getLength()+10);

        ForwardRef[] forward_refs = new ForwardRef[] { null };
        setPos(0);
    Loop:
        for(;;) {
            int curPos = getPos();
            boolean isFRef = new_offs[curPos] == -1;
            new_offs[curPos] = fob.getPos();
            if( isFRef ) resolveForwardReferences(forward_refs, fob);
            int code = getUByte();
            boolean hasLength = (code&0x80) != 0;
            int length = hasLength? getUWord(): 0;
            fob.writeByte(code);
            int nextPos = getPos()+length;

            switch( code ) {
                case Actions.None:
                    setPos(nextPos);
                    break Loop;
                case Actions.GotoFrame:
                    fob.writeWord(2);
                    fob.writeWord(getUWord());
                    break;
                case Actions.GetURL: {
                    String url = getString();
                    String target = getString();
                    url = context.apply(url);
                    target = context.apply(target);
                    fob.writeWord(url.length()+target.length()+2);  // 2 - length of end zeroes
                    fob.writeStringZ(url);
                    fob.writeStringZ(target);
                    break;
                }
                case Actions.WaitForFrame:
                    fob.writeWord(3);
                    fob.writeWord(getUWord());
                    fob.writeByte(getUByte());
                    break;
                case Actions.SetTarget: {
                    String target = getString();
                    target = context.apply(target);
                    fob.writeWord(target.length()+1);  // 1 - length of end zero
                    fob.writeStringZ(target);
                    break;
                }
                case Actions.GotoLabel: {
                    String label = getString();
                    label = context.apply(label);
                    fob.writeWord(label.length()+1);  // 1 - length of end zero
                    fob.writeStringZ(label);
                    break;
                }
                case Actions.PushData: {
                    // quick check whether we have vars or not
                    int start = getPos();
                    int l = length;
                    while( --l >= 0 ) {
                        if( getUByte() == '{' ) break;
                    }
                    // reset position
                    setPos( start );
                    if( l <= 0 ) { // no vars
                        fob.writeWord(length);
                        body.getTo(fob, length);
                    } else { // probably some vars, let's parse fair
                        FlashBuffer fb = new FlashBuffer( length+5 );
                        while( getPos() < nextPos ) {
                            int type = getUByte();
                            fb.writeByte( type );
                            switch( type ) {
                                case 0: // string
                                    fb.writeStringZ( context.apply( getString() ) );
                                    break;
                                case 1: // float
                                    fb.writeDWord( getUDWord() );
                                    break;
                                case 2: // null
                                    break;
                                case 3: // undefined
                                    break;
                                case 4: // register
                                    fb.writeByte( getUByte() );
                                    break;
                                case 5: // boolean
                                    fb.writeByte( getUByte() );
                                    break;
                                case 6: // double
                                    fb.writeDWord( getUDWord() );
                                    fb.writeDWord( getUDWord() );
                                    break;
                                case 7: // int
                                    fb.writeDWord( getUDWord() );
                                    break;
                                case 8: // dictionary lookup 1 byte
                                    fb.writeByte( getUByte() );
                                    break;
                                case 9: // dictionary lookup 2 bytes
                                    fb.writeWord( getUWord() );
                                    break;
                                default:
                                    // since we don't know what is this it's safer to skip everything
                                    // it has to be investigated further
                                    // after this while loop will break !
                                    body.getTo(fb, nextPos-getPos());
                                    break;
                            }
                        }
                        fob.writeWord( fb.getSize() );
                        fob.writeFOB( fb );
                    }
                    break;
                }
                case Actions.JumpIfTrue:
                case Actions.Jump: {
                    fob.writeWord(2);
                    int offset = getWord();
                    int target_off = getPos()+offset;
                    if( offset <= 0 ) {
                        int new_off = new_offs[target_off] - (fob.getPos()+2);  // has to be negative
                        fob.writeWord(new_off);
                    } else {
                        new_offs[target_off] = -1;  // mark this offset as having forward reference (just for optimization)
                        addForwardRef( forward_refs, new ForwardRef( fob.getPos(), target_off ) );
                        fob.skip(2);
                    }
                    break;
                }
                case Actions.GotoExpression:
                case Actions.GetURL2:
                case Actions.WaitForFrameExpression:
                    fob.writeWord(1);
                    fob.writeByte(getUByte());
                    break;
                case Actions.CallFrame:
                    fob.writeWord(0);
                    break;
                case Actions.ConstantPool: {
                    int num = getUWord();
                    String[] strings = new String[num];
                    int len = 2;
                    for( int i=0; i<num; i++ ) {
                        String c = getString();
                        c = context.apply(c);
                        strings[i] = c;
                        len += c.length()+1;
                    }
                    fob.writeWord(len);
                    fob.writeWord(num);
                    for( int i=0; i<num; i++ ) {
                        fob.writeStringZ(strings[i]);
                    }
                    break;
                }
                case Actions.With: {
                    String with = getString();
                    with = context.apply(with);
                    fob.writeWord(with.length()+1);
                    fob.writeStringZ(with);
                    break;
                }
                // name (STR), parmsNum (WORD), parms (STR*parmsNum), codeSize (WORD)
                // we need to generate correct codesize value
                case Actions.DefineFunction: {
                    fob.writeWord(length);  // ??? about this length, it may be dangerous
                    fob.writeStringZ(getString());
                    int num = getUWord();
                    fob.writeWord(num);
                    for( int i=0; i<num; i++ ) fob.writeStringZ(getString());
                    int codesize = getUWord();
                    int target_off = getPos()+codesize;
                    new_offs[target_off] = -1;  // mark this offset as having forward reference (just for optimization)
                    addForwardRef( forward_refs, new ForwardRef( fob.getPos(), target_off ) );
                    fob.skip(2);
                    break;
                }
                default:
                    if( hasLength ) {
                        fob.writeWord(length);
                        body.getTo(fob, length);
                    }
                    break;
            }
            setPos(nextPos);
        }
        body = fob;
    }

    /**
     * Gets length of the program
     *
     * @return length of the program
     */
    public int getLength() {
        return body.getSize()+1;
    }

    public void write( FlashOutput fob ) {
        fob.writeFOB( body );
        fob.writeByte(0);   // end
    }

    public void printContent( PrintStream out, String indent ) {
        int origPos = getPos();
        try {
            out.println( indent+"Actions: " );
            setPos(0);

            String[] cpool = null;

            for(;;) {
                int offset = getPos();
                int code = getUByte();
                boolean hasLength = (code&0x80) != 0;
                int length = hasLength? getUWord(): 0;
                int nextPos = getPos()+length;

                out.print( indent+"   "+Util.w2h(offset)+": "+Actions.getActionName(code)+"  " );
                switch( code ) {
                  case Actions.None:
                    out.println();
                    setPos(nextPos);
                    return;
                  case Actions.GotoFrame:
                    out.println( getUWord() );
                    break;
                  case Actions.GetURL:
                    out.println( "url='"+getString()+"' target='"+getString()+"'" );
                    break;
                  case Actions.WaitForFrame:
                    out.println( "frame="+getUWord()+" skipcount="+getUByte() );
                    break;
                  case Actions.SetTarget:
                    out.println( "target='"+getString()+"'" );
                    break;
                  case Actions.GotoLabel:
                    out.println( "label='"+getString()+"'" );
                    break;
                  case Actions.PushData: {
                      out.println( "values: " );
                      for( int l=length; l>0; ) {
                          int type = getUByte();
                          l--;
                          switch( type ) {
                            case 0:
                              String ss = getString();
                              out.println( indent+"          string='"+ss+"'" );
                              l-=ss.length()+1;
                              break;
                            case 1:
                              float flt = Float.intBitsToFloat(getUDWord());
                              out.println( indent+"          float="+flt );
                              l-=4;
                              break;
                            case 2:
                              out.println( indent+"          NULL" );
                              break;
                            case 3:
                              out.println( indent+"          undefined" );
                              break;
                            case 4:
                              out.println( indent+"          register="+getUByte() );
                              l--;
                              break;
                            case 5:
                              out.println( indent+"          boolean="+(getUByte()!=0) );
                              l--;
                              break;
                            case 6:
                              long dbits = ( ((long)getUDWord())<<32 ) | (((long)getUDWord())&0xffffffffL);
                              double dbl = Double.longBitsToDouble(dbits);
                              out.println( indent+"          double="+dbl );
                              l-=8;
                              break;
                            case 7:
                              int ival = getUDWord();
                              out.println( indent+"          int="+ival+" (hex: "+Util.d2h(ival)+")" );
                              l-=4;
                              break;
                            case 8: {
                                int idx = getUByte();
                                String val = cpool!=null&&idx<cpool.length? cpool[idx]: "<<<error>>>";
                                out.println( indent+"          pool_index="+idx+" -> "+val );
                                l--;
                                break;
                            }
                            case 9: {
                                int idx = getUWord();
                                String val = cpool!=null&&idx<cpool.length? cpool[idx]: "<<<error>>>";
                                out.println( indent+"          pool_index="+idx+" -> "+val );
                                l-=2;
                                break;
                            }
                          }
                      }
                      break;
                  }
                  case Actions.JumpIfTrue:
                  case Actions.Jump: {
                      int off = getWord();
                      out.println( "offset="+off+" (goto "+Util.w2h(getPos()+off)+")" );
                      break;
                  }
                  case Actions.GotoExpression:
                  case Actions.GetURL2:
                  case Actions.WaitForFrameExpression:
                    out.println( "byte="+getUByte() );
                    break;
                  case Actions.ConstantPool: {
                      int num = getUWord();
                      cpool = new String[num];
                      out.println( "constants="+num );
                      for( int i=0; i<num; i++ ) {
                          out.println( indent+"          constpool["+i+"]='"+(cpool[i]=getString())+"'" );
                      }
                      break;
                  }
                    // name (STR), parmsNum (WORD), parms (STR*parmsNum), codeSize (WORD)  /*, code (UI8*codeSize)*/
                  case Actions.DefineFunction: {
                      String name = getString();
                      int num = getUWord();
                      out.print( "function "+name+"( " );
                      for( int i=0; i<num; i++ ) {
                          out.print( getString() );
                          if( i != num-1 ) out.print( ", " );
                      }
                      int codesize = getUWord();
                      out.println( " ) codesize="+codesize+" (until "+Util.w2h(getPos()+codesize)+")" );
                      break;
                  }
                  case Actions.With:
                    out.println( "withblock='"+getString()+"'" );
                    break;
                  default:
                    // if( hasLength ) body.getSkip( length );  this will be done after switch
                    out.println();
                    break;
                }

                // Buffer capacity is not ensured in this function since it
                // assumes that it will be called only when the buffer is
                // complete. To prevent array overflow, check that the next
                // position is not >= current buf length. See bug 4589. -pk
                if (nextPos >= body.getBuf().length) {
                    return;
                }

                setPos(nextPos);
            }
        } finally {
            setPos(origPos);
        }
    }

    protected boolean _isConstant() {
        setPos(0);

        for(;;) {
            int code = getUByte();
            boolean hasLength = (code&0x80) != 0;
            int length = hasLength? getUWord(): 0;
            int pos = getPos();

            switch( code ) {
                case Actions.None:
                    return true;
                case Actions.GetURL:
                    if( Util.hasVar(getString()) || Util.hasVar(getString()) ) return false;
                    break;
                case Actions.SetTarget:
                case Actions.GotoLabel:
                    if( Util.hasVar(getString()) ) return false;
                    break;
                case Actions.PushData: {
                    // quick check whether we have vars or not
                    int l = length;
                    while( --l >= 0 ) {
                        if( getUByte() == '{' ) return false;
                    }
/*                    if( getUByte() == 0 ) {
                        if( Util.hasVar(getString()) ) return false;
                    }*/
                    break;
                }
                case Actions.ConstantPool: {
                    int num = getUWord();
                    for( int i=0; i<num; i++ ) {
                        if( Util.hasVar(getString()) ) return false;
                    }
                    break;
                }
                case Actions.With:
                    if( Util.hasVar(getString()) ) return false;
                    break;
            }
            setPos( pos+length );
        }
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return new Program( body.getCopy() );
    }

    public FlashBuffer body() {
        return body;
    }

    // ------------------------------------------------------------------------- //
    // API                                                                       //
    // ------------------------------------------------------------------------- //

    /**
     * End of the program
     * @since flash 3
     */
    public void none() {
        body.writeByte( Actions.None );
    }

    /**
     * Instructs the player to go to the next frame in the current movie.
     * @since flash 3
     */
    public void nextFrame() {
        body.writeByte( Actions.NextFrame );
    }

    /**
     * Instructs the player to go to the previous frame in the current movie.
     * @since flash 3
     */
    public void prevFrame() {
        body.writeByte( Actions.PrevFrame );
    }

    /**
     * Instructs the player to start playing at the current frame.
     * @since flash 3
     */
    public void play() {
        body.writeByte( Actions.Play );
    }

    /**
     * Instructs the player to stop playing the movie at the current frame.
     * @since flash 3
     */
    public void stop() {
        body.writeByte( Actions.Stop );
    }

    /**
     * Gets a variable’s value.
     * <P>
     * <OL>
     * <LI>Pops name off the stack, which is a string naming the variable to get.
     * <LI>Pushes the value of the variable to the stack.
     * </OL>
     * @since flash 4
     */
    public void eval() {
        body.writeByte( Actions.Eval );
    }

    /**
     * An equivalent of {@link #eval}.
     *
     * @see #eval
     * @since flash 4
     */
    public void getVar() {
        eval();
    }

    /**
     * Sets a variable.
     * <P>
     * <OL>
     * <LI>Pops value off the stack.
     * <LI>Pops name off the stack, which is a string naming the variable to set.
     * <LI>Sets the variable name in the current execution context to value.
     * </OL>
     * A variable in another execution context may be referenced by prefixing the variable name with
     * the target path and a colon. For example: /A/B:FOO references variable FOO in movie
     * clip with target path /A/B.
     * @since flash 4
     */
    public void setVar() {
        body.writeByte( Actions.SetVariable );
    }

    /**
     * Toggle the display between high and low quality.
     * @since flash 3
     */
    public void toggleQuality() {
        body.writeByte( Actions.ToggleQuality );
    }

    /**
     * Instructs the player to stop playing all sounds.
     * @since flash 3
     */
    public void stopSounds() {
        body.writeByte( Actions.StopSounds );
    }

    /**
     * Adds two numbers.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are converted to floating-point; non-numeric values evaluate to 0.
     * <LI>The numbers are added.
     * <LI>Pushes the result, A+B, to the stack.
     * </OL>
     * @since flash 4
     */
    public void add() {
        body.writeByte( Actions.Add );
    }

    /**
     * Subtracts two numbers.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are converted to floating-point; non-numeric values evaluate to 0.
     * <LI>A is subtracted from B.
     * <LI>Pushes the result, A-B, to the stack.
     * </OL>
     * @since flash 4
     */
    public void subtract() {
        body.writeByte( Actions.Subtract );
    }

    /**
     * Multiplies two numbers.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are converted to floating-point; non-numeric values evaluate to 0.
     * <LI>The numbers are multiplied.
     * <LI>Pushes the result, A*B, to the stack.
     * </OL>
     * @since flash 4
     */
    public void multiply() {
        body.writeByte( Actions.Multiply );
    }

    /**
     * Divides two numbers.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are converted to floating-point; non-numeric values evaluate to 0.
     * <LI>B is divided by A.
     * <LI>Pushes the result, B/A, to the stack.
     * <LI>If A is zero, the result is the string #ERROR#.
     * </OL>
     * Note: When playing a Flash 5 .SWF, NaN, Infinity or –Infinity is pushed to the stack instead of #ERROR#.
     * @since flash 4
     */
    public void divide() {
        body.writeByte( Actions.Divide );
    }

    /**
     * Tests two numbers for equality.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are converted to floating-point; non-numeric values evaluate to 0.
     * <LI>The numbers are compared for equality.
     * <LI>If the numbers are equal, a 1 (TRUE) is pushed to the stack.
     * <LI>Otherwise, a 0 is pushed to the stack.
     * </OL>
     * Note: When playing a Macromedia Flash 5 .SWF, true is pushed to the stack instead of 1,
     * and false is pushed to the stack instead of 0.
     * @since flash 4
     */
    public void equal() {
        body.writeByte( Actions.Equal );
    }

    /**
     * Tests if a number is less than another number.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are converted to floating-point; non-numeric values evaluate to 0.
     * <LI>The numbers are compared for equality.
     * <LI>If B < A, a 1 is pushed to the stack; otherwise, a 0 is pushed to the stack.
     * </OL>
     * Note: When playing a Macromedia Flash 5 .SWF, true is pushed to the stack instead of 1,
     * and false is pushed to the stack instead of 0.
     * @since flash 4
     */
    public void lessThan() {
        body.writeByte( Actions.LessThan );
    }

    /**
     * Performs a logical AND of two numbers.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are converted to floating-point; non-numeric values evaluate to 0.
     * <LI>If both numbers are nonzero, a 1 is pushed to the stack; otherwise, a 0 is pushed to the stack.
     * </OL>
     * Note: When playing a Macromedia Flash 5 .SWF, true is pushed to the stack instead of 1,
     * and false is pushed to the stack instead of 0.
     * @since flash 4
     */
    public void logicalAnd() {
        body.writeByte( Actions.LogicalAnd );
    }
    /**
     * Performs a logical OR of two numbers.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are converted to floating-point; non-numeric values evaluate to 0.
     * <LI>If either numbers is nonzero, a 1 is pushed to the stack; otherwise, a 0 is pushed to the stack.
     * </OL>
     * Note: When playing a Macromedia Flash 5 .SWF, true is pushed to the stack instead of 1,
     * and false is pushed to the stack instead of 0.
     * @since flash 4
     */
    public void logicalOr() {
        body.writeByte( Actions.LogicalOr );
    }

    /**
     * Performs a logical NOT of a number.
     * <P>
     * Note that in Macromedia Flash 5 .SWF files, the ActionNot action
     * converts its argument to a boolean, and pushes a result of type boolean.
     * In Macromedia Flash 4 .SWF files, the argument and result are numbers.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are converted to floating-point; non-numeric values evaluate to 0.
     * <LI>The numbers are compared for equality.
     * <LI>If the numbers are equal, a 1 (TRUE) is pushed to the stack.
     * <LI>Otherwise, a 0 is pushed to the stack.
     * </OL>
     * Note: When playing a Macromedia Flash 5 .SWF, true is pushed to the stack instead of 1,
     * and false is pushed to the stack instead of 0.
     * @since flash 4
     */
    public void logicalNot() {
        body.writeByte( Actions.LogicalNot );
    }

    /**
     * Tests two strings for equality.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>A and B are compared as strings. The comparison is case-sensitive.
     * <LI>If the strings are equal, a 1 (TRUE) is pushed to the stack.
     * <LI>Otherwise, a 0 is pushed to the stack.
     * </OL>
     * Note: When playing a Macromedia Flash 5 .SWF, true is pushed to the stack instead of 1,
     * and false is pushed to the stack instead of 0.
     * @since flash 4
     */
    public void stringEqual() {
        body.writeByte( Actions.StringEqual );
    }

    /**
     * Computes the length of a string.
     * <P>
     * <OL>
     * <LI>Pops a string off the stack.
     * <LI>The length of the string is calculated and pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void stringLength() {
        body.writeByte( Actions.StringLength );
    }

    /**
     * Tests if a string is less than another string.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>If B < A using a byte-by-byte comparison, a 1 is pushed to the stack;
     *     otherwise, a 0 is pushed to the stack.
     * </OL>
     * Note: When playing a Macromedia Flash 5 .SWF, true is pushed to the stack instead of 1,
     * and false is pushed to the stack instead of 0.
     * @since flash 4
     */
    public void stringLessThan() {
        body.writeByte( Actions.StringLessThan );
    }

    /**
     * Extracts a substring from a string.
     * <P>
     * <OL>
     * <LI>Pops number <B>count</B> off the stack.
     * <LI>Pops number <B>index</B> off the stack.
     * <LI>Pops string <B>string</B> off the stack.
     * <LI>The substring of <B>string</B> starting at the <B>index</B>’th character and
     *     <B>count</B> characters in length is pushed to the stack.
     * <LI>If either <B>index</B> or <B>count</B> do not evaluate to integers, the result
     *     is the empty string.
     * </OL>
     * @since flash 4
     */
    public void subString() {
        body.writeByte( Actions.SubString );
    }

    /**
     * Concatenates two strings.
     * <P>
     * <OL>
     * <LI>Pops value A off the stack.
     * <LI>Pops value B off the stack.
     * <LI>The concatenation BA is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void addString() {
        body.writeByte( Actions.StringConcat );
    }

    /**
     * Computes the length of a string, multi-byte aware.
     * <P>
     * <OL>
     * <LI>Pops a string off the stack.
     * <LI>The length of the string in characters is calculated and pushed to the stack.
     * <LI>This is a multi-byte aware version of ActionStringLength.
     *     On systems with double-byte support, a double-byte character is counted as a single character.
     * </OL>
     * @since flash 4
     */
    public void mbLength() {
        body.writeByte( Actions.MBLength );
    }

    /**
     * Converts ASCII to character code, multi-byte aware.
     * <P>
     * <OL>
     * <LI>Pops value off the stack.
     * <LI>The value is converted from a number to the corresponding character.
     *     If the character is a 16-bit value (>= 256), a double-byte character
     *     is constructed with the first byte containing the high-order byte,
     *     and the second byte containing the low-order byte.
     * <LI>The resulting character is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void mbChr() {
        body.writeByte( Actions.MBChr );
    }

    /**
     * Converts character code to ASCII, multi-byte aware.
     * <P>
     * <OL>
     * <LI>Pops value off the stack.
     * <LI>The first character of value is converted to a numeric character code.
     *     If the first character of value is a double-byte character, a 16-bit value
     *     is constructed with the first byte as the high order byte and the second byte
     *     as the low order byte.
     * <LI>The resulting character code is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void mbOrd() {
        body.writeByte( Actions.MBOrd );
    }

    /**
     * Extracts a substring from a string, multi-byte aware.
     * <P>
     * <OL>
     * <LI>Pops number <B>count</B> off the stack.
     * <LI>Pops number <B>index</B> off the stack.
     * <LI>Pops string <B>string</B> off the stack.
     * <LI>The substring of <B>string</B> starting at the <B>index</B>’th character and
     *     <B>count</B> characters in length is pushed to the stack.
     * <LI>If either <B>index</B> or <B>count</B> do not evaluate to integers, the result
     *     is the empty string.
     * <LI>This is a multi-byte aware version of ActionStringExtract. index and count are
     *     treated as character indices, counting double-byte characters as single characters.
     * </OL>
     * @since flash 4
     */
    public void mbSubString() {
        body.writeByte( Actions.MBSubString );
    }

    /**
     * Converts to integer.
     * <P>
     * <OL>
     * <LI>Pops a value off the stack.
     * <LI>The value is converted to a number.
     * <LI>Next, any digits after the decimal point are discarded, resulting in an integer.
     * <LI>The resulting integer is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void toInt() {
        body.writeByte( Actions.Int );
    }

    /**
     * Converts character code to ASCII.
     * <P>
     * <OL>
     * <LI>Pops value off the stack.
     * <LI>The first character of value is converted to a numeric ASCII character code.
     * <LI>The resulting character code is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void ord() {
        body.writeByte( Actions.Ord );
    }

    /**
     * Converts ASCII to character code.
     * <P>
     * <OL>
     * <LI>Pops value off the stack.
     * <LI>The value is converted from a number to the corresponding ASCII character..
     * <LI>The resulting character is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void chr() {
        body.writeByte( Actions.Chr );
    }

    /**
     * Set current context, stack-based
     * <P>
     * <OL>
     * <LI>Pops target off the stack and makes it the current active context.
     * <LI>This action behaves exactly like the original ActionSetTarget from
     *     Macromedia Flash 3, but is stack-based to enable the target path to be the
     *     result of expression evaluation.
     * </OL>
     * @since flash 4
     */
    public void setTarget() {
        body.writeByte( Actions.SetTargetExpression );
    }

    /**
     * Gets a movie property
     * <P>
     * <OL>
     * <LI>Pops index off the stack.
     * <LI>Pops target off the stack.
     * <LI>Retrieves the value of the property enumerated as index from
     *     the movie clip with target path target and pushes the value to the stack.
     * </OL>
     * @since flash 4
     */
    public void getProperty() {
        body.writeByte( Actions.GetProperty );
    }

    /**
     * Sets a movie property.
     * <P>
     * <OL>
     * <LI>Pops value off the stack.
     * <LI>Pops index off the stack.
     * <LI>Pops target off the stack.
     * <LI>Sets the property enumerated as index in the movie clip
     *     with target path target to the value value.
     * </OL>
     * @since flash 4
     */
    public void setProperty() {
        body.writeByte( Actions.SetProperty );
    }

    /**
     * Clones a sprite.
     * <P>
     * <OL>
     * <LI>Pops depth off the stack.
     * <LI>Pops target off the stack.
     * <LI>Pops source off the stack.
     * <LI>Duplicates movie clip source, giving the new instance
     *     the name target, at z-order depth depth.
     * </OL>
     * @since flash 4
     */
    public void cloneClip() {
        body.writeByte( Actions.DuplicateClip );
    }

    /**
     * Removes a clone sprite.
     * <P>
     * <OL>
     * <LI>Pops target off the stack.
     * <LI>Removes the clone movie clip identified by target path target.
     * </OL>
     * @since flash 4
     */
    public void removeClip() {
        body.writeByte( Actions.RemoveClip );
    }

    /**
     * Starts dragging a movie clip.
     * <P>
     * <OL>
     * <LI>Pops target off the stack. target identifies the movie clip to be dragged.
     * <LI>Pops lockcenter off the stack. If lockcenter evaluates to a nonzero value,
     *     the center of the dragged movie clip is locked to the mouse position.
     *     Otherwise, the movie clip moves relatively to the mouse position when
     *     the drag started.
     * <LI>Pops constrain off the stack.
     * <LI>If constrain evaluates to a nonzero value:
     *     <OL>
     *     <LI>Pops y2 off the stack.
     *     <LI>Pops x2 off the stack.
     *     <LI>Pops y1 off the stack.
     *     <LI>Pops x1 off the stack.
     *     </OL>
     * </OL>
     * @since flash 4
     */
    public void startDrag() {
        body.writeByte( Actions.StartDragMovie );
    }

    /**
     * Ends drag operation.
     * <P>
     * <OL>
     * <LI>Ends the drag operation in progress, if any.
     * </OL>
     * @since flash 4
     */
    public void endDrag() {
        body.writeByte( Actions.StopDragMovie );
    }

    /**
     * Calculates a random number.
     * <P>
     * <OL>
     * <LI>Pops maximum off the stack.
     * <LI>Calculates a random number, an integer in the range 0 ... (maximum-1)
     * <LI>This random number is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void random() {
        body.writeByte( Actions.Random );
    }

    /**
     * Reports milliseconds since player started.
     * <P>
     * <OL>
     * <LI>Calculates the number of milliseconds since the Player was started (an integer).
     * <LI>This number is pushed to the stack.
     * </OL>
     * @since flash 4
     */
    public void getTimer() {
        body.writeByte( Actions.GetTimer );
    }

    /**
     * Instructs the player to go to the specified frame in the current movie.
     *
     * @param frame  frame number
     * @since flash 3
     */
    public void gotoFrame( int frame ) {
        body.writeByte( Actions.GotoFrame );
        body.writeWord(2);
        body.writeWord(frame);
    }

    /**
     * Instructs the player to get the URL specified by UrlString.
     * The URL can be of any type, including an HTML file, an image
     * or another SWF movie. If the movie is playing in a browser,
     * the URL will be displayed in the frame specified by TargetString.
     * The special target names _level0 and _level1 are used to load another
     * SWF movie into levels 0 and 1 respectively.
     *
     * @param url    specified url
     * @param target target
     * @since flash 3
     */
    public void getURL( String url, String target ) {
        body.writeByte( Actions.GetURL );
        body.writeWord((url==null?0:url.length())+(target==null?0:target.length())+2);  // 2 - length of end zeroes
        body.writeStringZ(url==null?"":url);
        body.writeStringZ(target==null?"":target);
    }

    /**
     * Instructs the player to wait until the specified frame,
     * otherwise skip the specified number of actions.
     *
     * @param frame  specified frame
     * @param skip   specified number of actions to skip
     * @since flash 3
     */
    public void waitForFrame( int frame, int skip ) {
        body.writeByte( Actions.WaitForFrame );
        body.writeWord(3);
        body.writeWord(frame);
        body.writeByte(skip);
    }

    /**
     * Instructs the player to change the context of subsequent actions,
     * so they apply to a named object (TargetName) rather than the current movie.
     * <P>
     * For example, the SetTarget action can be used to control the timeline of a sprite
     * object.  The following sequence of actions sends a sprite called "spinner" to the
     * first frame in its timeline:<BR>
     * <OL>
     * <LI>SetTarget "spinner"
     * <LI>GotoFrame zero
     * <LI>SetTarget "" (empty string)
     * <LI>End of actions. (Action code = 0)
     * </OL>
     * <P>
     * All actions following SetTarget "spinner" apply to the spinner
     * object until SetTarget "", which sets the action context back to
     * the current movie.
     * For a complete discussion of target names see DefineSprite.
     *
     * @param target name of the target
     * @since flash 3
     */
    public void setTarget( String target ) {
        body.writeByte( Actions.SetTarget );
        body.writeWord(target.length()+1);  // 1 - length of end zero
        body.writeStringZ(target);
    }

    /**
     * Instructs the player to go to frame associated with the specified label.
     * A label can be attached to a frame with the FrameLabel tag.
     *
     * @param label  specified frame label
     * @since flash 3
     */
    public void gotoLabel( String label ) {
        body.writeByte( Actions.GotoLabel );
        body.writeWord(label.length()+1);  // 1 - length of end zero
        body.writeStringZ(label);
    }

    /**
     * Pops a value from the stack.
     * @since flash 4
     */
    public void pop() {
        body.writeByte( Actions.Pop );
    }

    /**
     * Pushes a string to the stack.
     *
     * @param data   string to push
     * @since flash 4
     */
    public void push( String data ) {
        body.writeByte( Actions.PushData );
        body.writeWord(data.length()+1+1);
        body.writeByte(0);
        body.writeStringZ(data);
    }

    /**
     * Pushes a float to the stack.
     *
     * @param data   float to push
     * @since flash 4
     */
    public void push( float data ) {
        body.writeByte( Actions.PushData );
        body.writeWord(4+1);
        body.writeByte(1);
        body.writeDWord( Float.floatToIntBits(data) );
    }

    /**
     * Pushes an int to the stack.
     *
     * @param data int to push
     * @since flash 4
     */
    public void push( int data ) {
        body.writeByte( Actions.PushData );
        body.writeWord(4+1);
        body.writeByte(7);
        body.writeDWord( data );
    }

    /**
     * Pushes constant index to the stack
     *
     * @since flash 5
     */
    public void push( Short const_idx ) {
        body.writeByte(Actions.PushData);
        int idx = const_idx.intValue();
        if( idx > 255 ) {
            body.writeWord(3);
            body.writeByte(9);
            body.writeWord(idx);
        } else {
            body.writeWord(2);
            body.writeByte(8);
            body.writeByte(idx);
        }
    }

    /**
     * Pushes array of data to the stack
     * <P>
     * <ul>
     * <li>String  - is pushed as string
     * <li>Integer - is pushed as int
     * <li>Double  - is pushed as double
     * <li>Float   - is pushed as float
     * <li>Boolean - is pushed as bool
     * <li>Byte    - is pushed as index in constant pool
     * <li>Short   - is pushed as index in constant pool
     * <li>null    - is pushed as null
     *
     * @param data   array of data
     * @since flash 5
     */
    public void push( Object[] data ) {
        FlashBuffer fb = new FlashBuffer(40);
        for( int i=0; i<data.length; i++ ) {
            Object o = data[i];
            if( o instanceof String ) {
                fb.writeByte(0);
                fb.writeStringZ((String)o);
            } else if( o instanceof Float ) {
                fb.writeByte(1);
                fb.writeDWord( Float.floatToIntBits(((Float)o).floatValue()) );
            } else if( o instanceof Boolean ) {
                fb.writeByte(5);
                fb.writeByte(((Boolean)o).booleanValue()?1:0);
            } else if( o instanceof Double ) {
                fb.writeByte(6);
                long dbits = Double.doubleToLongBits(((Double)o).doubleValue());
                fb.writeDWord((int)(dbits>>>32));
                fb.writeDWord((int)(dbits&0xffffffffL));
            } else if( o instanceof Integer ) {
                fb.writeByte(7);
                fb.writeDWord(((Integer)o).intValue());
            } else if( o instanceof Short || o instanceof Byte ) {
                int idx = ((Number)o).intValue();
                if( idx > 255 ) {
                    fb.writeByte(9);
                    fb.writeWord(idx);
                } else {
                    fb.writeByte(8);
                    fb.writeByte(idx);
                }
            } else if( o == null ) {
                fb.writeByte(2);
            }
        }
        body.writeByte(Actions.PushData);
        body.writeWord(fb.getSize());
        body.writeFOB(fb);
    }

    /**
     * Pushes an object to the stack
     * <P>
     * <ul>
     * <li>String  - is pushed as string
     * <li>Integer - is pushed as int
     * <li>Double  - is pushed as double
     * <li>Float   - is pushed as float
     * <li>Boolean - is pushed as bool
     * <li>Byte    - is pushed as index in constant pool
     * <li>Short   - is pushed as index in constant pool
     * <li>null    - is pushed as null
     *
     * @param o data
     * @since flash 5
     */
    public void push( Object o ) {
        body.writeByte(Actions.PushData);
        if( o instanceof String ) {
            body.writeWord(((String)o).length()+2);
            body.writeByte(0);
            body.writeStringZ((String)o);
        } else if( o instanceof Float ) {
            body.writeWord(4+1);
            body.writeByte(1);
            body.writeDWord( Float.floatToIntBits(((Float)o).floatValue()) );
        } else if( o instanceof Boolean ) {
            body.writeWord(1+1);
            body.writeByte(5);
            body.writeByte(((Boolean)o).booleanValue()?1:0);
        } else if( o instanceof Double ) {
            body.writeWord(8+1);
            body.writeByte(6);
            long dbits = Double.doubleToLongBits(((Double)o).doubleValue());
            body.writeDWord((int)(dbits>>>32));
            body.writeDWord((int)(dbits&0xffffffffL));
        } else if( o instanceof Integer ) {
            body.writeWord(4+1);
            body.writeByte(7);
            body.writeDWord(((Integer)o).intValue());
        } else if( o instanceof Short || o instanceof Byte ) {
            int idx = ((Number)o).intValue();
            if( idx > 255 ) {
                body.writeWord(2+1);
                body.writeByte(9);
                body.writeWord(idx);
            } else {
                body.writeWord(1+1);
                body.writeByte(8);
                body.writeByte(idx);
            }
        } else if( o == null ) {
            body.writeWord(0+1);
            body.writeByte(2);
        }
    }

    /**
     * Unconditional branch.
     * <P>
     * <OL>
     * <LI>BranchOffset bytes are added to the instruction pointer in the execution stream.
     * <LI>The offsets is a signed quantity, enabling branches from –32,768 bytes to 32,767 bytes.
     * <LI>An offset of 0 points to the action directly after the ActionJump action.
     * </OL>
     *
     * @param offset specified offset
     * @since flash 4
     */
    public void jump( int offset ) {
        body.writeByte( Actions.Jump );
        body.writeWord(2);
        body.writeWord(offset);
    }

    /**
     * Conditional Test and Branch.
     * <P>
     * <OL>
     * <LI>Pops Condition, a number, off the stack.
     * <LI>Tests if Condition is nonzero: If Condition is nonzero,
     *     BranchOffset bytes are added to the instruction pointer in the execution stream.
     * </OL>
     * Note: When playing a Macromedia Flash 5 .SWF, Condition is converted to a boolean and compared to true, not 0.
     * The offset is a signed quantity, enabling branches from –32768 bytes to 32767 bytes.
     * An offset of 0 points to the action directly after the ActionIf action.
     *
     * @param offset specified offset
     * @since flash 4
     */
    public void jumpIfTrue( int offset ) {
        body.writeByte( Actions.JumpIfTrue );
        body.writeWord(2);
        body.writeWord(offset);
    }

    /**
     * Calls a subroutine.
     * <P>
     * <OL>
     * <LI>Pops a value off the stack.
     * <LI>This value should be either a string matching a frame label, or a number
     *     indicating a frame number.
     * <LI>The value may be prefixed by a target string identifying the movie clip that
     *     contains the frame being called.
     * <LI>If the frame is successfully located, the actions in the target frame are executed.
     *     After the actions in the target frame are executed, execution resumes at the instruction
     *     after the ActionCall instruction.
     * <LI>If the frame cannot be found, nothing happens.
     * <LI>NOTE: This action's tag (0x9E) has the high bit set, which will waste a few bytes in SWF file size.
     *     This is a bug.
     * </OL>
     * @since flash 4
     */
    public void callFrame() {
        body.writeByte( Actions.CallFrame );
        body.writeWord(0);
    }

    /**
     * Get URL, stack-based.
     * <P>
     * <OL>
     * <LI>Pops window off the stack. window specifies the target window,
     *     which may be an empty string to indicate the current window.
     * <LI>Pops url off the stack. url which specifies the URL to be retrieved.
     * </OL>
     *
     * @param method Method specifies the method to use for the HTTP request. If (method and 0x40) != 0 then target is movie clip target,
     *               NOT browser window target!
     *               <UL>
     *               <LI>A value of 0 indicates that this is not a form request,
     *               so the movie clip’s variables should not be encoded and submitted.
     *               <LI>A value of 1 specifies a HTTP GET request.
     *               <LI>A value of 2 specifies a HTTP POST request.
     *               <LI>If method is 1 (GET) or 2 (POST), the variables in the current
     *               movie clip are submitted to the URL using the standard
     *               x-www-urlencoded encoding and the HTTP request method specified by method.
     *               </UL>
     * @since flash 4
     */
    public void getURL( int method ) {
        body.writeByte( Actions.GetURL2 );
        body.writeWord(1);
        body.writeByte(method);
    }

    /**
     * Get URL using GET method, stack-based.
     * <P>
     * <OL>
     * <LI>Pops window off the stack. window specifies the target window,
     *     which may be an empty string to indicate the current window.
     * <LI>Pops url off the stack. url which specifies the URL to be retrieved.
     * </OL>
     *
     * @see #getURL(int)
     * @since flash 4
     */
    public void getURL_GET() {
        getURL(1);
    }

    /**
     * Get URL using POST method, stack-based.
     * <P>
     * <OL>
     * <LI>Pops window off the stack. window specifies the target window,
     *     which may be an empty string to indicate the current window.
     * <LI>Pops url off the stack. url which specifies the URL to be retrieved.
     * </OL>
     *
     * @see #getURL(int)
     * @since flash 4
     */
    public void getURL_POST() {
        getURL(2);
    }

    /**
     * Go to frame and stop, stack-based.
     * <P>
     * <OL>
     * <LI>Pops <b>frame</b> off the stack.
     * <LI>If <b>frame</b> is a number, the next frame of the movie to be displayed
     *     will be the <b>frame</b>’th frame in the current movie clip.
     * <LI>If <b>frame</b> is a string, <b>frame</b> is treated as a frame label.
     *     If the specified label exists in the current movie clip,
     *     the labeled frame will become the current frame. Otherwise, the action is ignored.
     * <LI>Either a frame or a number may be prefixed by a target path, e.g. /MovieClip:3 or /MovieClip:FrameLabel
     * </OL>
     * @since flash 4
     */
    public void gotoFrameAndStop() {
        body.writeByte( Actions.GotoExpression );
        body.writeWord(1);
        body.writeByte(0x00);
    }

    /**
     * Go to frame and play, stack-based.
     * <P>
     * <OL>
     * <LI>Pops <b>frame</b> off the stack.
     * <LI>If <b>frame</b> is a number, the next frame of the movie to be displayed
     *     will be the <b>frame</b>’th frame in the current movie clip.
     * <LI>If <b>frame</b> is a string, <b>frame</b> is treated as a frame label.
     *     If the specified label exists in the current movie clip,
     *     the labeled frame will become the current frame. Otherwise, the action is ignored.
     * <LI>Either a frame or a number may be prefixed by a target path, e.g. /MovieClip:3 or /MovieClip:FrameLabel
     * </OL>
     * @since flash 4
     */
    public void gotoFrameAndPlay() {
        body.writeByte( Actions.GotoExpression );
        body.writeWord(1);
        body.writeByte(0x80);
    }

    /**
     * Wait for a frame to be loaded, stack-based.
     * <P>
     * <OL>
     * <LI>Pops frame off the stack.
     * <LI>If the frame identified by frame has been loaded, SkipCount
     *     actions following the current one are skipped.
     * <LI>frame is evaluated in the same way as the {@link #gotoFrameAndPlay} action.
     * </OL>
     *
     * @param skip   specified number of actions to skip
     * @since flash 4
     */
    public void waitForFrameAndSkip( int skip ) {
        body.writeByte( Actions.WaitForFrameExpression );
        body.writeWord(1);
        body.writeByte(skip);
    }

    /* Some flash 5 actions */

     /**
     * Creates constant pool
     *
     * @param constants array of constants to be created
     * @since flash 5
     */
    public void addConstantPool( String[] constants ) {
        int size = 2;
        for( int i=0; i<constants.length; i++ ) {
            size += constants[i].length()+1;
        }
        body.writeByte(Actions.ConstantPool);
        body.writeWord(size);
        body.writeWord(constants.length);
        for( int i=0; i<constants.length; i++ ) {
            body.writeStringZ(constants[i]);
        }
    }

    /**
     * New Object
     * @since flash 5
     */
    public void newObject() {
        body.writeByte(Actions.NewObject);
    }

   /**
     * Calls a function.
     * Stack state must be [ arguments, argCount, methodName ]
     *
     * @since flash 5
     */
    public void callFunction() {
        body.writeByte( Actions.CallFunction );
    }

    /**
     * Calls a method of an object.
     * Stack state must be [ arguments, argCount, object, methodName ]
     *
     * @since flash 5
     */
    public void callMethod() {
        body.writeByte( Actions.CallMethod );
    }

    /**
     * Get member.
     * Stack state must be [ object, member name ]
     *
     * @since flash 5
     */
    public void getMember() {
        body.writeByte( Actions.GetMember );
    }

    /**
     * Set member.
     * Stack state must be [ object, member name, value ]
     *
     * @since flash 5
     */
    public void setMember() {
        body.writeByte( Actions.SetMember );
    }

    /* --------------------------------------------------------------------------------
     * Generating stuff
     * -------------------------------------------------------------------------------- */
    static class ForwardRef {
        ForwardRef next;
        int jumpOffset;
        int targetOffset;
        ForwardRef( int jumpOffset, int targetOffset ) {
            this.jumpOffset = jumpOffset;
            this.targetOffset = targetOffset;
        }
    }

    private void addForwardRef( ForwardRef[] forward_refs, ForwardRef fref ) {
        fref.next = forward_refs[0];
        forward_refs[0] = fref;
    }

    // resolve forward references
    private void resolveForwardReferences( ForwardRef[] forward_refs, FlashBuffer fob ) {
        ForwardRef pred = null;
        ForwardRef cur = forward_refs[0];
        int curPos = getPos();
        while( cur != null ) {
            if( cur.targetOffset == curPos ) {
                fob.writeWordAt( fob.getPos()-cur.jumpOffset-2, cur.jumpOffset );
                cur = cur.next;
                if( pred == null ) {
                    forward_refs[0] = cur;
                } else {
                    pred.next = cur;
                }
                continue;
            }
            pred = cur;
            cur = cur.next;
        }
    }

    // --------------------------------------------------------------------------------- //
    //                                     R E A D E R                                   //
    // --------------------------------------------------------------------------------- //

    protected final String getString() {
        return body.getString();
    }
    protected final int getByte() {
        return body.getByte();
    }
    protected final int getUByte() {
        return body.getUByte();
    }
    protected final int getWord() {
        return body.getWord();
    }
    protected final int getUWord() {
        return body.getUWord();
    }
    protected final int getDWord() {
        return body.getDWord();
    }
    protected final int getUDWord() {
        return body.getUDWord();
    }
    protected final int getPos() {
        return body.getPos();
    }
    protected final void setPos( int pos ) {
        body.setPos(pos);
    }

}
