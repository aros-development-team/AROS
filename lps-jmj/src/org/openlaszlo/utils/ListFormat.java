/* ****************************************************************************
 * ChainedException.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.utils;

import java.text.*;
import java.util.*;

public class ListFormat extends Format {
    protected final String junction;
    
    public ListFormat(String junction) {
        this.junction = junction;
    }
    
    public ListFormat() {
        this("and");
    }
    
    public StringBuffer format(Object obj, StringBuffer buffer,
                               FieldPosition pos)
    {
        List list = (List) obj;
        int listSize = list.size();
        for (ListIterator iter = list.listIterator(); iter.hasNext(); ) {
            if (iter.hasPrevious()) {
                if (listSize > 2)
                    buffer.append(',');
                buffer.append(' ');
                if (iter.nextIndex() == listSize-1) {
                    buffer.append(junction);
                    buffer.append(' ');
                }
            }
            buffer.append(iter.next());
        }
        return buffer;
    }

    public Object parseObject(String source, ParsePosition parsePosition) {
        throw new RuntimeException("unimplemented functionality");
    }
}
