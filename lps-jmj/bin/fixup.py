# * P_LZ_COPYRIGHT_BEGIN ******************************************************
# * Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
# * Use is subject to license terms.                                          *
# * P_LZ_COPYRIGHT_END ********************************************************

import sys;
import re;
import string;
import urllib
import xml.sax.saxutils 

if __name__ == '__main__' :
    if len( sys.argv ) > 1:
        i = open( sys.argv[ 1 ], 'r' )
    else:
        i = sys.stdin

    badchar = re.compile( "\xA0" )
    def xmlFix( s ):
        s=s.replace( '&' , "&amp;" );
        s=s.replace( '<' , "&lt;" );
        s=s.replace( '>' , "&gt;" );
        s=s.replace( '"' , "&quot;" );
        s=s.replace( "'" , "&apos;" );
        s=s.replace( "\\xAO" , "&#AO;" );
        s = badchar.sub( "&#160;" , s )
        
        return s

    avalre = re.compile( "^(.*?)=\"(.*?)\"" );
    for lin in i:
        s = ""
        m = avalre.match(  lin ) 
        #if m:
            #for i in m.groups():
                #print i;
        while m:
            s+= m.groups()[ 0 ]
            s+= "=\"" + xmlFix( urllib.unquote( m.groups()[ 1 ] ) ) +"\""
            lin = avalre.sub( "" , lin )
            #lin = re.sub("^(.*?)=\"(.*?)\"" , "" , lin )
            m = avalre.match( lin ) 

        s += lin
        print s,
