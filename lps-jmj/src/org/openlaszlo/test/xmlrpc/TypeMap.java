/******************************************************************************
 * TypeMap.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.test.xmlrpc;

import java.util.*;

public class TypeMap
{
	Calendar mycal = Calendar.getInstance();

	public class Box
	{
		public int dimensions[];
		public long bgcolor;
		public String name;

		public Box(int h, int w, long bc, String str)
		{
			dimensions = new int[2];
			dimensions[0] = w;
			dimensions[1] = h;
			bgcolor = bc;
			name = str;
		}
	};

	public TypeMap()
	{}

	// METHODS


	public boolean getBool()

	{
		return mycal.getFirstDayOfWeek() == Calendar.SUNDAY;
	}

	public Map getProperties()
	{
		return System.getProperties();
	}

	public List getList()
	{
		Locale lc[] = Calendar.getAvailableLocales();
		List loc_lst = Arrays.asList(lc);

		Vector cl = new Vector();

		for (int i = 0, n = loc_lst.size(); i < n; ++i)
		{
			String dc = ((Locale)loc_lst.get(i)).getDisplayCountry();
			if (!dc.equals("")) cl.add(dc);
		}

		return cl;
	}

	public String getString()
	{
		return mycal.getTime().toString();
	}

	public int getInt()
	{
		return mycal.get(Calendar.YEAR);
	}

	public double getDouble()
	{
		return Math.sin(Math.PI/4);
	}

	public Box getObject()
	{
		return new Box(100, 200, 0xe055cd, "rect1");
	}

	public ObjectMapper getMappedObject()
	{
		return new ObjectMapper(100, 200, 0xe055cd, "rect1");
	}

	public void getVoid()
	{
		System.err.println("getVoid called!");
	}

	// Argument mapping testers


	public boolean callWithArg(Object obj, String obj_type)
	{
		boolean match = false;
		try
		{
			if (Class.forName(obj_type) == obj.getClass())
				match = true;
		}
		catch (Throwable ex)
		{
		}
		return match;
	}

	public boolean callWithInt(int n)
	{
		return true;
	}

	public boolean callWithDouble(double d)
	{
		return true;
	}

	public boolean callWithBoolean(boolean b)
	{
		return true;
	}

	public boolean callWithList(List l)
	{
		return true;
	}

	public boolean callWithMap(Map m)
	{
		return true;
	}
}