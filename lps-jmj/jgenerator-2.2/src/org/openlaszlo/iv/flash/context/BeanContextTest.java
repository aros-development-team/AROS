package org.openlaszlo.iv.flash.context;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import java.util.HashMap;
import java.util.List;
import java.util.ListIterator;
import java.util.ArrayList;

public class BeanContextTest extends TestCase
{
    public BeanContextTest( String testName )
    {
        super( testName );
    }

    public static void main( String[] args )
    {
        junit.textui.TestRunner.run( suite() );
    }

    public static Test suite()
    {
        TestSuite suite = new TestSuite( BeanContextTest.class );

        return suite;
    }

//    Object as root is currently not handled by Jexl
//
//    public void testGetValueWithObjectAsRoot()
//    {
//        BeanContext ctx = new BeanContext( null, new Foo() );
//
//        assertEquals( "1", ctx.getValue( "intField" ) );
//        assertEquals( "foo.stringField", ctx.getValue( "stringField" ) );
//        assertEquals( "arrayItemZero", ctx.getValue( "arrayField[1]" ) );
//    }

    public void testGetValueWithHashMapAsRoot()
    {
        HashMap map = new HashMap();

        map.put( "object", new Foo() );

        BeanContext ctx = new BeanContext( null, map );

        assertEquals( "1", ctx.getValue( "object.intField" ) );
        assertEquals( "foo.stringField", ctx.getValue( "object.stringField" ) );
        assertEquals( "arrayItemZero", ctx.getValue( "object.arrayField.0" ) );
        assertEquals( "arrayItemOne", ctx.getValue( "object.arrayField[1]" ) );

        assertEquals( "15", ctx.getValue( "object.stringField.length()" ) );
    }

    public void testGetValueList()
    {
        List inList = new ArrayList();

        inList.add( new Foo() );
        inList.add( new Foo() );
        inList.add( new Foo() );
        inList.add( new Foo() );

        BeanContext context = new BeanContext();

        context.put( "list", inList );

        List l;

        l = context.getValueList( "list" );

        assertEquals( 4, l.size() );

        ListIterator iter = l.listIterator();
        BeanContext childContext;

        while ( iter.hasNext() )
        {
            childContext = ( BeanContext ) iter.next();

            assertEquals( "Instance of Foo",
                          childContext.getValue( "root" ) );

            assertEquals( "foo.stringField",
                          childContext.getValue( "root.stringField" ) );
        }
    }

    public void testApply()
    {
        HashMap map = new HashMap();

        map.put( "object1", new Foo() );
        map.put( "object2", new Foo() );
        map.put( "object3", new Foo() );
        map.put( "object4", new Foo() );

        BeanContext context = new BeanContext( null, map );

        String initial = "'{object1.stringField}', '{object2.intField}', '{object3.stringField.length()}'";
        String expected = "'foo.stringField', '1', '15'";

        String actual = context.apply( initial );

        assertEquals( expected, actual );
    }

    // "Bean" for tests

    public static class Foo
    {
        private int intField = 1;
        private String stringField = "foo.stringField";
        private String[] arrayField =
            {"arrayItemZero", "arrayItemOne", "arrayItemTwo"};

        public int getIntField()
        {
            return intField;
        }

        public String getStringField()
        {
            return stringField;
        }

        public String[] getArrayField()
        {
            return arrayField;
        }

        public String toString()
        {
            return "Instance of Foo";
        }
    }
}
