package org.openlaszlo.iv.flash.context;

import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.url.*;
import org.openlaszlo.iv.flash.xml.*;

import org.w3c.dom.Node;
import java.util.*;
import junit.framework.*;

public class XMLContextTest extends TestCase
{
    String xml;
    Node node;

    public void testGetValue() throws IVException, java.io.IOException
    {
        XMLContext ctx = XMLContext.newXMLContext( null, node );

        assertEquals( "foo contents", ctx.getValue( "root/foo" ) );
        assertEquals( "bar contents", ctx.getValue( "root/bar" ) );

        assertEquals( "foo attribute", ctx.getValue( "root/foo/@attr" ) );
        assertEquals( "bar attribute", ctx.getValue( "root/bar/@attr" ) );

        // An expression which selects a nodeset should return the first item
        // when a value is expected.

        assertEquals( "item 1 contents", ctx.getValue( "root/list/item" ) );
    }

    public void testGetValueList() throws IVException, java.io.IOException
    {
        XMLContext context = XMLContext.newXMLContext( null, node );

        List l = context.getValueList( "root/list/item" );

        assertEquals( 4, l.size() );

        ListIterator iter = l.listIterator();
        XMLContext childContext;
        int itemIndex = 1;

        while( iter.hasNext() )
        {
            childContext = ( XMLContext ) iter.next();

            assertEquals( "item " + itemIndex + " contents",
                          childContext.getValue( "." ) );

            itemIndex++;
        }
    }

    public void testApply() throws IVException, java.io.IOException
    {
        XMLContext ctx = XMLContext.newXMLContext( null, node );

        String initial = "foo: '{root/foo}', bar: '{root/bar}', item: '{root/list/item}'";
        String expected = "foo: 'foo contents', bar: 'bar contents', item: 'item 1 contents'";

        String actual = ctx.apply( initial );

        assertEquals( expected, actual );
    }

    public XMLContextTest(java.lang.String testName) throws Exception
    {
        super(testName);

        // Need to init JGen

        Util.init();

        // And load the test document into a node.

        node = XMLHelper.getNode( IVUrl.newUrl(
            Util.getInstallDir() + "/test_data/XMLContextTest.xml", null ) );
    }

    public static void main(java.lang.String[] args)
    {
        junit.textui.TestRunner.run( suite() );
    }

    public static Test suite()
    {
        TestSuite suite = new TestSuite( XMLContextTest.class );

        return suite;
    }
}
