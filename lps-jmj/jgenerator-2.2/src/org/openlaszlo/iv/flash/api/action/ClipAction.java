/*
 * $Id: ClipAction.java,v 1.3 2002/05/16 05:08:41 skavish Exp $
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
 * Program to be executed when certain specified clip events are occured
 *
 * @author Dmitry Skavish
 */
public class ClipAction extends FlashItem {

    public static final int LOAD        = 0x0001;
    public static final int ENTER_FRAME = 0x0002;
    public static final int UNLOAD      = 0x0004;
    public static final int MOUSE_MOVE  = 0x0008;
    public static final int MOUSE_DOWN  = 0x0010;
    public static final int MOUSE_UP    = 0x0020;
    public static final int KEY_DOWN    = 0x0040;
    public static final int KEY_UP      = 0x0080;
    public static final int DATA        = 0x0100;
    public static final int MXCOMPONENT = 0x0200;

    private int flags;          // events
    private Program program;    // program to be executed

    public ClipAction() {}

    /**
     * Creates clip actions
     *
     * @param flags   events to activate the specified program
     * @param program program to be executed when specified events are occured
     */
    public ClipAction( int flags, Program program ) {
        this.flags = flags;
        this.program = program;
    }

    public void setFlags( int flags ) {
        this.flags = flags;
    }

    public void setProgram( Program program ) {
        this.program = program;
    }

    public int getFlags() {
        return flags;
    }

    public Program getProgram() {
        return program;
    }

    public void apply( Context context ) {
        program.apply( context );
    }

    public boolean isConstant() {
        return program.isConstant();
    }

    public void write( FlashOutput fob ) {
        int version = fob.getFlashFile().getVersion();

        if( version > 5 ) fob.writeDWord(flags);
        else fob.writeWord(flags);

        fob.writeDWord( program.getLength() );
        program.write(fob);
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"    ClipAction: flags=0x"+Util.w2h(flags) );
        program.printContent(out, indent+"        ");
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return new ClipAction( flags, (Program) program.getCopy(copier) );
    }

}

