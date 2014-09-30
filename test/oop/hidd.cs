/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
    This is an example for a pseudolanguage to use our OOP
    system in C.
*/

class HIDD
{
    version 41.0;

    /* This is a global constant for this class */
    const ClassTable = 0;

    /*
	FindHIDD takes three arguments: A string with the type
	and the subtype of the HIDD (eg. "gfx"/"printer")
	and a list of attributes.

	It returns an array of HIDDs which match the specified
	attributes.
    */
    Array findHIDD
    (
	String	 type,
	String	 subType,
	TagArray attrs
    )
    {
	hiddTableLock.lock ();

	Array hiddTable = classTable.get (type);
	Array hidds = Array (hiddTable.len ());

	ULONG t;
	for (t=0; hiddTable[t]; t++)
	{
	    if (!strcasecmp (hiddTable[t].subType, subType)
		&& hiddTable[t].match (attrs)
	    )
	    {
		hidds.append (hiddTable[t])
	    }
	}

	hiddTableLock.unlock ();

	return hidds;
    }

    /* These are global class variables */
    static HashTable hiddTable = HashTable (256, HashTable.calcStringHashCode);
    static Semaphore hiddTableLock = Semaphore ();
}

class Window
{
    /* Position and size of the window. The attribute can be read
       and written from outside. */
    [RW] UWORD x, y, width, height;
    [RW] UWORD minWidth, minHeight, maxWidth, maxHeight;

    /* Before writing to width, execute this code */
    pre W width
    {
	if (width < 0)
	    Raise IllegalValue ("width (%d) too small, must be > 0", width);

	if (width < minWidth)
	    /* This could also raise an error */
	    width = minWidth;

	if (width > maxWidth)
	    width = maxWidth;
    }

    post W width
    {
	this.resize (this.width, this.height);
    }

    ...
}
