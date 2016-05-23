//
//  xfdfgen.c
//  xfdfannots
//
//  Created by Erik Groenhuis on 19-5-2016.
//  Copyright © 2016 Vertus Publications. All rights reserved.
//

#include <stdio.h>

struct rect_s {
    float width, height;
} rect = {
    12.00, 12.00
};

struct pageannots {
    int     pgfirst;        // Number of first page of this chapter
    int     pglast;         // Number of last page of this chapter
    int     lastline;       // Highest line number to use on a page
    struct margins {
        float   leftedge;    // Left edge of rectangles
        float   firstbottom; // Bottom edge of first rectangle
        float   linehight;   // Height of a single line
    } left, right;
} chapterdata[] = {
//    {  27,  27, 25, {106.000, 614.000, 19.450}, {500.000, 614.000, 19.450} }, // Prolegomena first page
//    {  28,  78, 40, {106.000, 886.000, 19.450}, {500.000, 880.000, 19.270} }, // Prolegomena
//    {  83,  83, 25, { 82.881, 641.000, 19.400}, {524.267, 641.000, 19.400} }, // Liber Primus first page
//    {  84, 142, 40, { 76.000, 887.000, 19.250}, {524.267, 880.000, 19.250} }, // Liber Primus
    { 143, 143, 25, { 82.881, 652.000, 19.300}, {524.267, 652.000, 19.300} }, // Liber Secundus first page
    { 144, 150, 40, { 82.881, 889.000, 19.250}, {524.267, 889.000, 19.250} }, // Liber Secundus
//    { 270, 270, 25, { 82.881, 647.000, 19.300}, {524.267, 647.000, 19.300} }, // Liber Tertius first page
//    { 271, 275, 40, { 82.881, 889.000, 19.250}, {524.267, 889.000, 19.250} }, // Liber Tertius

    {   0,   0,  0, {  0.000,   0.000,  0.000}, {  0.000,   0.000,  0.000} }  // terminator
};

// Output a single annotation
void note(int ch, int pg, int line)
{
    float bx0, by0, bx1, by1;

    if (pg % 2 == 0) {
        // Left page. Put notes in the left margin
        bx0 = chapterdata[ch].left.leftedge;
        by0 = chapterdata[ch].left.firstbottom - (line-1)*chapterdata[ch].left.linehight;
    } else {
        // Right page. Put notes in the right margin
        bx0 = chapterdata[ch].right.leftedge;
        by0 = chapterdata[ch].right.firstbottom - (line-1)*chapterdata[ch].right.linehight;
    }

    bx1 = bx0+rect.width;
    by1 = by0+rect.height;

    printf("        <freetext width=\"0\"\n");
    printf("            flags=\"print\"\n");
    printf("            justification=\"right\"\n");
    printf("            page=\"%d\"\n", pg);
    printf("            rect=\"%.3f,%.3f,%.3f,%.3f\"\n", bx0, by0, bx1, by1 );
    printf("        >\n");
    printf("            <contents>%d</contents>\n", line);
    printf("            <defaultstyle>font: Helvetica 10.0pt; text-align:right; color:#FF0000</defaultstyle>\n");
    printf("        </freetext>\n");
}

// Output the annotations for page number pg in chapter ch
void page(int ch, int pg)
{
    int line;

    note(ch, pg, 1);
    for(line = 5; line <= chapterdata[ch].lastline; line += 5) {
        note(ch, pg, line);
    }
}

// Output the annotations for the whole chapter nr ch
void chapter(int ch)
{
    for (int pg=chapterdata[ch].pgfirst; pg<=chapterdata[ch].pglast; pg++) {
        page(ch, pg);
    }
}

// Output the annotations for all the chapters in chapterdata
void chapters(void)
{
    int ch;

    ch=0;
    while(chapterdata[ch].pgfirst > 0){
        chapter(ch);
        ch++;
    }
}


int main()
{
    printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    printf("<xfdf xmlns=\"http://ns.adobe.com/xfdf/\" xml:space=\"preserve\">\n");
    printf("    <annots>\n");

    chapters();

    printf("    </annots>\n");
    printf("    <f href=\"1629-Geneva-1(orig)-Opus_de_emendatione_temporum_hac_postrem copy.pdf\" />\n");
    printf("    <ids original=\"27E63E7C58902FD49EF0C376902BEEA7\"\n");
    printf("         modified=\"27E63E7C58902FD49EF0C376902BEEA7\" />\n");
    printf("</xfdf>\n");
    return 0;
}