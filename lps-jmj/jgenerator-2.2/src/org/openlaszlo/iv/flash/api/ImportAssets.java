/*
 * $Id: ImportAssets.java,v 1.2 2002/02/15 23:44:27 skavish Exp $
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
 * The ImportAssets tag imports assets from another swf file as a linked asset.
 * <P>
 * "Client" swf files can import assets like fonts that are stored in "server"
 * swfs (that are ideally cached on an end user's local system).
 * The importing swf file references the exporting swf file with a URL.
 * The URL needs to include the asset identifier string.
 * Imported assets are resolved and stored in a ScriptThread's SCharacter dictionary.
 * <BR>
 * Note that the URL can be absolute or relative.  If it's relative, it's resolved
 * relative to the location of the importing swf file.
 *
 * @author Dmitry Skavish
 * @see ExportAssets
 * @see ImportedDef
 */
public class ImportAssets extends FlashObject {

    private String url;             // the url of the exporting swf file
    private IVVector defs;          // imported definitions references (ImportedDef)
    private IVVector names;         // imported definitions names

    public ImportAssets() {}

    public int getTag() {
        return Tag.IMPORTASSETS;
    }

    public void setUrl( String url ) {
        this.url = url;
    }

    public void addAsset( String name, FlashDef def ) {
        if( defs == null ) {
            names = new IVVector();
            defs = new IVVector();
        }
        names.addElement(name);
        defs.addElement(def);
    }

    public static ImportAssets parse( Parser p ) {
        ImportAssets o = new ImportAssets();
        o.url = p.getString();
        int num = p.getUWord();
        o.defs = new IVVector(num);
        o.names = new IVVector(num);
        for( int i=0; i<num; i++ ) {
            int tagID = p.getUWord();
            ImportedDef idef = new ImportedDef();
            idef.setID(tagID);
            p.addDef(idef);
            String name = p.getString();
            o.defs.addElement( idef );
            o.names.addElement( name );
        }
        return o;
    }

    // we should not collect defs here, it has to be done by other objects
    /*  public void collectDeps( DepsCollector dc ) {
      }*/

    /*  protected void collectFonts( FontsCollector fc ) {
      }*/

    public void write( FlashOutput fob ) {
        int tagPos = fob.getPos();
        fob.skip(6);

        fob.writeStringZ(url);
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
        out.println( indent+"ImportAssets: url="+url );
        for( int i=0; i<defs.size(); i++ ) {
            FlashDef def = (FlashDef) defs.elementAt(i);
            String name = (String) names.elementAt(i);
            out.println(indent+"    name="+name+", defID="+def.getID());
        }

    }

    protected boolean _isConstant() {
        if( Util.hasVar(url) ) return false;
        for( int i=0; i<names.size(); i++ ) {
            String name = (String) names.elementAt(i);
            if( Util.hasVar(name) ) return false;
        }
        return true;
    }

    public void apply( Context context ) {
        url = context.apply(url);
        for( int i=0; i<names.size(); i++ ) {
            String name = (String) names.elementAt(i);
            name = context.apply(name);
            names.setElementAt(name, i);
        }
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((ImportAssets)item).url = url;
        ((ImportAssets)item).defs = defs.getCopy(copier);
        ((ImportAssets)item).names = (IVVector) names.clone();
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new ImportAssets(), copier );
    }
}
