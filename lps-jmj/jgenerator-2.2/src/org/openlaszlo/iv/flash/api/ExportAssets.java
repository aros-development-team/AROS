/*
 * $Id: ExportAssets.java,v 1.6 2002/07/15 22:39:32 skavish Exp $
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

package org.openlaszlo.iv.flash.api;

import java.io.PrintStream;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.commands.*;
import org.openlaszlo.iv.flash.context.Context;

/**
 * Class represents ExportAssets tag
 * <p>
 * The ExportAssets tag exports assets from the flash file so that other flash files
 * can ImportAssets as linked assets.
 * <p>
 * ExportAssets makes repetitive portions of a flash movie or a series of flash movies
 * available for import.  For example, ten flash movies that are all part of the same website can share an
 * embedded custom font if one movie embeds the font and sets the font to "export".
 * Exporting gives the font a special identifier string.  As well as sharing embedded fonts,
 * a flash can share other symbol types (linked assets) like movie clips, buttons and sounds.
 * <P>
 * Objects of this class have two vectors:<BR>
 * <ul>
 * <li>vector of FlashDefs
 * <li>vector of names under which these FlashDef's are exported
 * </ul>
 *
 * @author Dmitry Skavish
 * @see ImportAssets
 */
public class ExportAssets extends FlashObject {

    private IVVector defs;          // vector of FlashDef objects (flash definitions)
    private IVVector names;         // vector of names of definitions (from {@link #defs} vector)

    public ExportAssets() {}

    public int getTag() {
        return Tag.EXPORTASSETS;
    }

    public IVVector getDefs() {
        return defs;
    }

    public IVVector getNames() {
        return names;
    }

    public void addAsset( String name, FlashDef def ) {
        if( defs == null ) {
            names = new IVVector();
            defs = new IVVector();
        }
        names.addElement(name);
        defs.addElement(def);
    }

    public static ExportAssets parse( Parser p ) {
        ExportAssets o = new ExportAssets();
        int num = p.getUWord();
        o.defs = new IVVector(num);
        o.names = new IVVector(num);
        for( int i=0; i<num; i++ ) {
            FlashDef def = p.getDef( p.getUWord() );
            String name = p.getString();
            // if name starts with jgenerator prefix do not add it, it's our library
            //             if( !name.startsWith(PropertyManager.mxLibrarySymbolPrefix) ) {
            if (true) {
                o.defs.addElement( def );
                o.names.addElement( name );
            }
            // add definition to file export table
            // !!! possible problem with apply(), it does not update export table
            p.getFile().addDefInAssets(name.toUpperCase(), def);
        }
        if( o.defs.size() == 0 ) return null;
        return o;
    }

    public void collectDeps( DepsCollector dc ) {
        for( int i=0; i<defs.size(); i++ ) {
            FlashDef def = (FlashDef) defs.elementAt(i);
            dc.addDep(def);
        }
    }

    public void collectFonts( FontsCollector fc ) {
        for( int i=0; i<defs.size(); i++ ) {
            FlashDef def = (FlashDef) defs.elementAt(i);
            def.collectFonts(fc);
            // I am not sure about this, it has to be checked !!!!!!!!!!
            /*if( def instanceof FontDef ) {
                defs.setElementAt( fc.addFont((FontDef)def), i );
            }*/
        }
    }

    public void write( FlashOutput fob ) {
        int tagPos = fob.getPos();
        fob.skip(6);

        fob.writeWord( defs.size() );
        for( int i=0; i<defs.size(); i++ ) {
            FlashDef def = (FlashDef) defs.elementAt(i);
            String name = (String) names.elementAt(i);
            fob.writeDefID( def );
            fob.writeStringZ(name);
        }

        fob.writeLongTagAt( getTag(), fob.getPos()-tagPos-6, tagPos );
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"ExportAssets:" );
        for( int i=0; i<defs.size(); i++ ) {
            FlashDef def = (FlashDef) defs.elementAt(i);
            String name = (String) names.elementAt(i);
            out.println(indent+"    name="+name+", defID="+def.getID());
        }
    }

    protected boolean _isConstant() {
        for( int i=0; i<names.size(); i++ ) {
            String name = (String) names.elementAt(i);
            if( Util.hasVar(name) ) return false;
        }
        return true;
    }

    public void apply( Context context ) {
        for( int i=0; i<names.size(); i++ ) {
            String name = (String) names.elementAt(i);
            name = context.apply(name);
            names.setElementAt(name, i);
        }
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((ExportAssets)item).defs = defs.getCopy(copier);
        ((ExportAssets)item).names = (IVVector) names.clone();
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new ExportAssets(), copier );
    }
}
