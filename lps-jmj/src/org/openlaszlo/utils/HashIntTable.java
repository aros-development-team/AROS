/* *****************************************************************************
 * HashIntTable.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/


/**
 * Redistribution and use of this software and associated documentation
 * ("Software"), with or without modification, are permitted provided
 * that the following conditions are met:
 *
 * 1. Redistributions of source code must retain copyright
 *    statements and notices.  Redistributions must also contain a
 *    copy of this document.
 *
 * 2. Redistributions in binary form must reproduce the
 *    above copyright notice, this list of conditions and the
 *    following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. The name "Exolab" must not be used to endorse or promote
 *    products derived from this Software without prior written
 *    permission of Intalio.  For written permission,
 *    please contact info@exolab.org.
 *
 * 4. Products derived from this Software may not be called "Exolab"
 *    nor may "Exolab" appear in their names without prior written
 *    permission of Intalio. Exolab is a registered
 *    trademark of Intalio.
 *
 * 5. Due credit should be given to the Exolab Project
 *    (http://www.exolab.org/).
 *
 * THIS SOFTWARE IS PROVIDED BY INTALIO AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * INTALIO OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright 2000 (C) Intalio Inc. All Rights Reserved.
 *
 */


package org.openlaszlo.utils;

import java.util.ConcurrentModificationException;
import java.util.Enumeration;
import java.util.NoSuchElementException;

///////////////////////////////////////////////////////////////////////////////
// HashIntTable
///////////////////////////////////////////////////////////////////////////////

/**
 * This class provides unsynchronized hash get, put, remove
 * and increment of int values. If multiple threads are accessing
 * the table then access to the table must be synchronized.
 *
 * @author <a href="mohammed@intalio.com">Riad Mohammed</a>
 */
public final class HashIntTable 
{
    /**
     * The array of entries
     */
    Entry[] table = null;

    /**
     * The default value of the HashIntTable if the
     * key does not exist
     */
    private /*final*/ int defaultValue; //javac error

    /**
     * The number of entries in the table
     */
    private int numberOfEntries = 0;

    /**
     * The modification count
     */
    private int modificationCount = 0;
    

    /**
     * Create the HashIntTable using the default
     * size
     */
    public HashIntTable()
    {
        this(101, 0);
    }


    /**
     * Create the HashIntTable with the specified
     * size.
     *
     * @param size the size (must be greater than zero)
     */
    public HashIntTable(int size, int defaultValue)
    {
        if (size <= 0) {
            throw new IllegalArgumentException("The argument 'size' is not greater than 0.");
        }

        this.table = new Entry[size];
        this.defaultValue = defaultValue;
    }

    /**
     * Return the size of the HashIntTable.
     *
     * @return the size of the HashIntTable.
     */
    public int size()
    {
        return numberOfEntries;
    }


    /**
     * Return the enumeration of keys.
     * <P>
     * If the size of the HashIntTable changes
     * (via put, remove, increment) while the
     * keys are being enuemrated a 
     * ConcurrentModificationException is thrown
     * by the enumeration.
     *
     * @return the enumeration of keys.
     */
    public Enumeration keys()
    {
        return new KeysEnumeration(modificationCount, table, this);
    }

    /**
     * Get the value of the specified key. If the key does not
     * exist in the table return the default value.
     *
     * @param key the key. Cannot be null.
     * @return the value of the key in the table
     */
    public int get(Object key)
    {
        if (null == key) {
            throw new IllegalArgumentException("The arguments 'key' is null.");
        }

        // get the hash code of the key
        int hashCode = key.hashCode();
        // get the entry at the index
        Entry entry = table[(hashCode & 0x7FFFFFFF) % table.length];

        // loop over the entries looking for a match
        while (null != entry) {
            if ((entry.hashCode == hashCode) &&
                (entry.key.equals(key))) {
                return entry.value;
            }
            // try the next entry
            entry = entry.next;
        }
        // failed - return defalut value
        return defaultValue;
    }

    /**
     * Check if key exists
     *
     * @param key the key. Cannot be null.
     * @return the value of the key in the table
     */
    public boolean containsKey(Object key)
    {
        if (null == key) {
            throw new IllegalArgumentException("The arguments 'key' is null.");
        }

        // get the hash code of the key
        int hashCode = key.hashCode();
        // get the entry at the index
        Entry entry = table[(hashCode & 0x7FFFFFFF) % table.length];

        // loop over the entries looking for a match
        while (null != entry) {
            if ((entry.hashCode == hashCode) &&
                (entry.key.equals(key))) {
                return true;
            }
            // try the next entry
            entry = entry.next;
        }
        // failed - return false
        return false;
    }



    /**
     * Remove the value for specified key
     *
     * @param key the key. Cannot be null.
     * @return the old value. If the key does not exist in the
     *      table return the default value.
     */
    public int remove(Object key)
    {
        if (null == key) {
            throw new IllegalArgumentException("The arguments 'key' is null.");
        }

        // get the hash code of the key
        int hashCode = key.hashCode();
        // the index in the table of the key
        int index = (hashCode & 0x7FFFFFFF) % table.length;
        // get the entry at the index
        Entry entry = table[index];
        // the previous entry to the current entry
        Entry previousEntry = null;

        // loop over the entries looking for a match
        while (null != entry) {
            if ((entry.hashCode == hashCode) &&
                (entry.key.equals(key))) {
                // remove the current entry
                if (null == previousEntry) {
                    table[index] = entry.next;            
                }
                else {
                    previousEntry.next = entry.next;
                }
                // decrement the size
                --numberOfEntries;
                // increment the mod count
                ++modificationCount;
                return entry.value;
            }
            // set the previous entry
            previousEntry = entry;
            // try the next entry
            entry = entry.next;
        }
        // failed - return default value
        return defaultValue;
    }


    /**
     * Associate the specified key with the
     * specified value.
     *
     * @param key the key. Cannot be null
     * @param value the new value
     * @return the existing value. If the
     *      key does not exist in the table
     *      return the default value.
     */
    public int put(Object key, int value)
    {
        if (null == key) {
            throw new IllegalArgumentException("The arguments 'key' is null.");
        }

        // get the hash code of the key
        int hashCode = key.hashCode();
        // the index in the table of the key
        int index = (hashCode & 0x7FFFFFFF) % table.length;
        // the current entry examined - get the entry at the index
        Entry entry = table[index];
        // the previous entry to the current entry
        Entry previousEntry = null;

        // loop over the entries looking for a match
        while (null != entry) {
            if ((entry.hashCode == hashCode) &&
                (entry.key.equals(key))) {
                // get the old value
                int oldValue = entry.value;
                // set the new value
                entry.value = value;
                return oldValue;
            }
            // set the previous entry
            previousEntry = entry;
            // try the next entry
            entry = entry.next;
        }

        // add a new entry
        if (null == previousEntry) {
            table[index] = new Entry(key, hashCode, value);    
        }
        else {
            previousEntry.next= new Entry(key, hashCode, value);
        }

        // increment the size
        ++numberOfEntries;
        // increment the mod count
        ++modificationCount;
        // return value
        return value;
    }


    /**
     * Increment the value associated with the specified key
     * by the specified amount.
     * If key does not exist in the table then increment the
     * default value and store the result
     *
     * @param key the key. Cannot be null
     * @param increment the increment
     * @return the incremented value
     */
    public int increment(Object key, int increment)
    {
        if (null == key) {
            throw new IllegalArgumentException("The arguments 'key' is null.");
        }

        // get the hash code of the key
        int hashCode = key.hashCode();
        // the index in the table of the key
        int index = (hashCode & 0x7FFFFFFF) % table.length;
        // the current entry examined - get the entry at the index
        Entry entry = table[index];
        // the previous entry to the current entry
        Entry previousEntry = null;

        // loop over the entries looking for a match
        while (null != entry) {
            if ((entry.hashCode == hashCode) &&
                (entry.key.equals(key))) {
                // set the new value
                entry.value += increment;
                return entry.value;
            }
            // set the previous entry
            previousEntry = entry;
            // try the next entry
            entry = entry.next;
        }

        // set the new value
        int value = defaultValue + increment;

        // add a new entry
        if (null == previousEntry) {
            table[index] = new Entry(key, hashCode, value);    
        }
        else {
            previousEntry.next= new Entry(key, hashCode, value);
        }

        // increment the size
        ++numberOfEntries;
        // increment the mod count
        ++modificationCount;
        // return value
        return value;
    }


    public static void main (String args[]) {
        HashIntTable table = new HashIntTable();
        Object[] keys = new Object[4];

        for (int i = keys.length; --i >= 0;) {
            final String name = "Key" + i;
            keys[i] = new Object(){public String toString(){return name;}};
        }
        
        System.out.println("size " + table.size());
        System.out.println("get " + keys[0]);
        System.out.println("= " + table.get(keys[0]));

        for (int i = keys.length; --i >= 0;) {
            table.put(keys[i], i);
        }

        System.out.println("size " + table.size());

        for (int i = keys.length; --i >= 0;) {
            System.out.println("get " + keys[i]);
            System.out.println("= " + table.get(keys[i]));
        }

        for (int i = keys.length; --i >= 0;) {
            table.increment(keys[i], 100);
        }

        System.out.println("size " + table.size());

        for (int i = keys.length; --i >= 0;) {
            System.out.println("get " + keys[i]);
            System.out.println("= " + table.get(keys[i]));
        }

        for (int i = keys.length; --i >= 0;) {
            System.out.println("remove " + keys[i]);
            System.out.println("= " + table.remove(keys[i]));
        }

        System.out.println("size " + table.size());

        for (int i = keys.length; --i >= 0;) {
            System.out.println("get " + keys[i]);
            System.out.println("= " + table.get(keys[i]));
        }

        for (int i = keys.length; --i >= 0;) {
            System.out.println("remove " + keys[i]);
            System.out.println("= " + table.remove(keys[i]));
        }

        System.out.println("size " + table.size());

        for (int i = keys.length; --i >= 0;) {
            System.out.println("get " + keys[i]);
            System.out.println("= " + table.get(keys[i]));
        }

        for (int i = keys.length; --i >= 0;) {
            table.increment(keys[i], 100);
        }

        System.out.println("size " + table.size());

        for (int i = keys.length; --i >= 0;) {
            System.out.println("get " + keys[i]);
            System.out.println("= " + table.get(keys[i]));
        }

        for (int i = keys.length; --i >= 0;) {
            table.put(keys[i], i);
        }

        System.out.println("size " + table.size());

        for (int i = keys.length; --i >= 0;) {
            System.out.println("get " + keys[i]);
            System.out.println("= " + table.get(keys[i]));
        }

        for (Enumeration e = table.keys(); e.hasMoreElements();) {
            System.out.println("key " + e.nextElement());
        }
    }
    
}

/**
 * The Enumeration of keys in the HashIntTable
 */
class KeysEnumeration
    implements Enumeration {
    /**
     * The expected modification count of
     * the underlying HashIntTable
     */
    final int expectedModificationCount;
    
    
    /**
     * The current index in the table whose
     * entries are being examined
     */
    private int index = -1;
    
    
    /**
     * The current entry being examined
     */
    Entry entry = null;
    

    Entry[] table;
    HashIntTable ht;
    
    /**
     * Create the KeysEnumeration
     *
     * @param modificationCount the current
     *      modification count of the
     *      underlying HashIntTable
     */
    KeysEnumeration(int modificationCount, Entry[] table, HashIntTable ht)
    {
        this.expectedModificationCount = modificationCount;
        this.table =  table;
        this.ht = ht;
    }
    
    
    /**
     * Return true if there is a next element to return in the
     * enumeration
     *
     * @return true if there is a next element to return in the
     *      enumeration
     */
    public boolean hasMoreElements()
    {
        if (null == entry) {
            for (; ++index < table.length;) {
                entry = table[index];
                
                if (null != entry) {
                    return true;    
                }
            }
        }
        
        return null != entry;
    }
    
    /**
     * Return the next element in the enumeration
     *
     * @return the next element in the enumeration
     * @throws NoSuchElementException if there is no
     *      next element in the enumeration
     */
    public Object nextElement()
    {
        if (!hasMoreElements()) {
            throw new NoSuchElementException("No more elements in exception.");    
        }
        
        // get the key from the entry
        Object key = entry.key;
        // set the entry to the next entry
        entry = entry.next;
        
        return key;
    }
}

/**
 * The class that stores the association between the
 * key and the int, with a pointer to the next Entry
 */
class Entry {
    /**
     * The key
     */
    final Object key;
    
    /**
     * The hash code of the key
     */
    final int hashCode;
    
    /**
     * The value
     */
    int value;
    
    /**
     * The next entry
     */
    Entry next = null;
    
    /**
     * Create the Entry with the specified
     * arguments
     *
     * @param key the key
     * @param value the value
     */
    Entry(Object key, int hashCode, int value)
    {
        this.key = key;
        this.hashCode = hashCode;
        this.value = value;
    }
}



