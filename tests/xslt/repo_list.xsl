<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="text" indent="yes"/>

	<!-- Don't output text by default. -->
	<xsl:template match="text()"/>

	<xsl:template match="td[@class='toplevel-repo']">
		<xsl:value-of select="descendant::text()"/>
		<xsl:text>&#10;</xsl:text>
	</xsl:template>

	<xsl:template match="td[@class='reposection']">
		<xsl:text>* </xsl:text>
		<xsl:value-of select="descendant::text()"/>
		<xsl:text>&#10;</xsl:text>
	</xsl:template>

	<xsl:template match="td[@class='sublevel-repo']">
		<!-- Indent with TAB. -->
		<xsl:text>&#09;</xsl:text>
		<xsl:value-of select="descendant::text()"/>
		<xsl:text>&#10;</xsl:text>
	</xsl:template>

</xsl:stylesheet>
