/* *****************************************************************************
 * SetUils.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.utils;
import java.util.*;

/**
 * A utility class containing set utility functions.
 *
 * @author Oliver Steele
 */
public abstract class SetUtils {
    public static boolean containsAny(Set a, Set b) {
        for (Iterator iter = b.iterator(); iter.hasNext(); ) {
            if (a.contains(iter.next())) {
                return true;
            }
        }
        return false;
    }

    public static Set intersection(Set a, Set b) {
        Set c = new HashSet();
        for (Iterator iter = b.iterator(); iter.hasNext(); ) {
            Object e = iter.next();
            if (a.contains(e)) {
                c.add(e);
            }
        }
        return c;
    }
}
