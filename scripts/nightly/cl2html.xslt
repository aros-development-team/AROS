<?xml version='1.0' encoding="iso-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0" >
    <xsl:output encoding="iso-8859-15" method="html" indent="yes" doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN" />
    <xsl:template match="/changelog">
        <html>
            <head>
                <title>CVS-ChangeLog for the <xsl:value-of select="$module"/> module on <xsl:value-of select="$date"/></title>
                <style type="text/css">
                    h3 { background-color: #E6E6FA; color: #000000; padding: 2px; }
	            .rev { color: #808080 }
                </style>
            </head>
            <body>
                <table>
                    <xsl:for-each select="entry">
	                <tr>
	                    <td colspan="3">
                                <h3>
	                            <xsl:text>Files modified by </xsl:text>
                                    <xsl:value-of select="concat(author, ': ', date, ' (', time, ')')" />
                                </h3>
	                    </td>
	                </tr>
	                <tr>
	                    <td colspan="3">
	                        <xsl:value-of select="msg"/>
	                    </td>
	                </tr>
	                <tr height="25px"></tr>
	                <tr>
	                    <td><strong><xsl:text>File name</xsl:text></strong></td>
		            <td><strong><xsl:text>Revision</xsl:text></strong></td>
		            <td><strong><xsl:text>Status</xsl:text></strong></td>
	                </tr>
                        <xsl:for-each select="file">
		            <tr>
                                <td><xsl:value-of select="name"/></td>
                                <td><xsl:value-of select="revision"/></td>
                                <td>
		                    <xsl:choose>
		                        <xsl:when test="cvsstate='dead'">
		                            <font color="#990000">
			                        <xsl:text>DELETED</xsl:text>
			                    </font>
			                </xsl:when>
			                <xsl:otherwise>
			                    <xsl:value-of select="cvsstate"/>
		                        </xsl:otherwise>
		                    </xsl:choose>
	                        </td>
		            </tr>
                        </xsl:for-each>
	            <tr height="50px"></tr>
                </xsl:for-each>
	    </table>
            <?php
                define("_BBCLONE_DIR", "../../../mybbclone/");
                define("COUNTER", _BBCLONE_DIR."index.php");
                if (file_exists(COUNTER)) include_once(COUNTER);
            ?>
            </body>
        </html>
    </xsl:template>
</xsl:stylesheet>
