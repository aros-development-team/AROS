/*
 * $Id: SoundStreamBlock.java,v 1.3 2002/07/12 07:43:41 skavish Exp $
 *
 * ===========================================================================
 *
 */

package org.openlaszlo.iv.flash.api.sound;

import java.io.PrintStream;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;

public class SoundStreamBlock extends FlashObject
{
    public DataMarker data;

    public int getTag()
    {
        return Tag.SOUNDSTREAMBLOCK;
    }

    public static SoundStreamBlock parse( Parser p )
    {
        SoundStreamBlock o = new SoundStreamBlock();

        o.data = new DataMarker( p.getBuf(), p.getPos(), p.getTagEndPos() );

        return o;
    }

    public void write( FlashOutput fob )
    {
        // sometimes sound does not play if we generate short tag
        // it seems to be a player's bug
        fob.writeLongTag( getTag(), data.length() );

        data.write(fob);
    }

    public boolean isConstant()
    {
        return true;
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier )
    {
        super.copyInto( item, copier );

        ( (SoundStreamBlock) item ).data = data;

        return item;
    }
    public FlashItem getCopy( ScriptCopier copier )
    {
        return copyInto( new SoundStreamBlock(), copier );
    }

    public void printContent( PrintStream out, String indent )
    {
        out.println( indent + "SoundStreamBlock" );
        data.printContent(out);
    }
}
