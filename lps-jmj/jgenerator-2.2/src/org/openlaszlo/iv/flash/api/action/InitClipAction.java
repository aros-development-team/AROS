/*
 * $Id: InitClipAction.java,v 1.1 2002/05/30 05:04:56 skavish Exp $
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
 * InitClipAction tag.
 * <P>
 * Contains a program to initalize a clip
 *
 * @author Dmitry Skavish
 * @since flash 6
 */
public class InitClipAction extends FlashObject {

    private Program program;
    private FlashDef def;

    public InitClipAction() {}

    public InitClipAction( FlashDef def, Program program ) {
        this.def = def;
        this.program = program;
    }

    public int getTag() {
        return Tag.INITCLIPACTION;
    }

    public Program getProgram() {
        return program;
    }

    public void setProgram( Program program ) {
        this.program = program;
    }

    public FlashDef getClip() {
        return def;
    }

    public void setClip( FlashDef def ) {
        this.def = def;
    }

    public static InitClipAction parse( Parser p ) {
        FlashDef clip = p.getDef(p.getUWord());
        Program prg = new Program(p.getBuf(), p.getPos(), p.getTagEndPos());
        return new InitClipAction(clip, prg);
    }

    public void write( FlashOutput fob ) {
        fob.writeTag(Tag.INITCLIPACTION, program.getLength()+2);
        fob.writeDefID(def);
        program.write(fob);
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"InitClipAction: clipID="+def.getID() );
        program.printContent( out, indent+"   " );
    }

    public void apply( Context context ) {
        super.apply( context );
        program.apply(context);
    }

    protected boolean _isConstant() {
        return program.isConstant();
    }

    public void collectDeps( DepsCollector dc ) {
        dc.addDep(def);
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((InitClipAction)item).def = copier.copy(def);
        ((InitClipAction)item).program = (Program) program.getCopy(copier);
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new InitClipAction(), copier );
    }

}


