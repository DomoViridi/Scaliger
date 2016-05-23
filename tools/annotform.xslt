<?xml version="1.0" encoding="UTF-8"?>
<xsl:transform version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:x="http://ns.adobe.com/xfdf/"
    xmlns:b="http://www.w3.org/1999/xhtml"
    xmlns="http://ns.adobe.com/xfdf/"
    exclude-result-prefixes="x b"
>
<!--
 Setting the default namespace (to xfdf) prevents 'xmlns=""' attributes from appearing.
 The exclude-result-prefixes attribute stops our own defined namespaces from appearing
 in the output.
 The combination of these allows the use of litteral elements like '<annots>' without the
 transformation adding namespace attributes to them.
 -->

<xsl:output method="xml" indent="yes" encoding="UTF-8" />

<xsl:template match="x:xfdf">
    <xsl:copy>
        <xsl:attribute name="xml:space">preserve</xsl:attribute>
        <xsl:apply-templates select="x:annots" />
        <xsl:apply-templates select="x:f" />
        <xsl:apply-templates select="x:ids" />
    </xsl:copy>
</xsl:template>

<xsl:template match="x:annots">
    <annots>
        <xsl:for-each select="x:freetext">
            <freetext width="0" flags="print">
                <xsl:attribute name="page">
                    <xsl:value-of select="@page" />
                </xsl:attribute>
                <xsl:attribute name="justification">right</xsl:attribute>
                <xsl:attribute name="rect">
                    <xsl:value-of select="@rect" />
                </xsl:attribute>
                <contents>
                    <xsl:value-of select="x:contents-richtext/b:body/b:p" />
                </contents>
                <xsl:apply-templates select="x:defaultstyle" />
            </freetext>
        </xsl:for-each>
    </annots>
</xsl:template>

<xsl:template match="x:defaultstyle">
    <defaultstyle><xsl:value-of select="." /></defaultstyle>
</xsl:template>

<xsl:template match="x:f">
    <xsl:copy-of select="." />
</xsl:template>

<xsl:template match="x:ids">
    <xsl:copy-of select="." />
</xsl:template>

</xsl:transform>
