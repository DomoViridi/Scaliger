//
//  xfdfgen.c
//  xfdfannots
//
//  Created by Erik Groenhuis on 19-5-2016.
//  Copyright © 2016 Vertus Publications. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Blunt solution: maximum number of chapters we can output
// (also: maximum number that can be defined in the config file)
enum {
    CHAP_DATA_MAX=100,
    MAX_CONFIG_LINELEN = 501     // Maximum nr of characters in a config file line with data
};

FILE * fout;    // The output file
FILE * fconf;   // The configuration file, holding chapter data

int configerr = 0;  // Flag for errors in the config file

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
} chapterdata[CHAP_DATA_MAX] = {
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

    fprintf(fout, "        <freetext width=\"0\"\n");
    fprintf(fout, "            flags=\"print\"\n");
    fprintf(fout, "            justification=\"right\"\n");
    fprintf(fout, "            page=\"%d\"\n", pg);
    fprintf(fout, "            rect=\"%.3f,%.3f,%.3f,%.3f\"\n", bx0, by0, bx1, by1 );
    fprintf(fout, "        >\n");
    fprintf(fout, "            <contents>%d</contents>\n", line);
    fprintf(fout, "            <defaultstyle>font: Helvetica 10.0pt; text-align:right; color:#FF0000</defaultstyle>\n");
    fprintf(fout, "        </freetext>\n");
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

// Output all the annotations for a whole file
void outfile(void)
{
    fprintf(fout, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(fout, "<xfdf xmlns=\"http://ns.adobe.com/xfdf/\" xml:space=\"preserve\">\n");
    fprintf(fout, "    <annots>\n");

    chapters();

    fprintf(fout, "    </annots>\n");
    fprintf(fout, "    <f href=\"1629-Geneva-1(orig)-Opus_de_emendatione_temporum_hac_postrem copy.pdf\" />\n");
    fprintf(fout, "    <ids original=\"27E63E7C58902FD49EF0C376902BEEA7\"\n");
    fprintf(fout, "         modified=\"27E63E7C58902FD49EF0C376902BEEA7\" />\n");
    fprintf(fout, "</xfdf>\n");
}

// Report an error in the token we are parsing
void tokenerr(char * Expected, int line, int pos, char * found, int quoteit, char * wholeline)
{
    configerr = 1;  // Flag the error

    fprintf(stderr, "%s expected at line %d, pos %d. ", Expected, line, pos);
    if (quoteit) {
        fprintf(stderr, "Found \"%s\"\n", found);
    } else {
        fprintf(stderr, "Found %s\n", found);
    }
    fprintf(stderr, "%s", wholeline);

    // Add a newline if it is missing from the line
    if(wholeline[strlen(wholeline)-1] != '\n') {
        fprintf(stderr, "\n");
    }

    // Mark the position of the error
    for(int i=1; i<pos; i++) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "^\n");
}

// Parse an integer or float value from a string passed in 'str'.
// An integer is a string of digits, nothing more.
// A float is a string of digits, followed by a period and another string of digits.
// Integer example: 456
// Float example: 234.567
// If an integer is found, it's value is returned through ival, and the function returns 1.
// If a float is found, it's value is returned through fval, and the function returns 2.
// If anything else is found, the function returns a negative value.
int getnumber(char * str, int *ival, float *fval)
{
    long ivalue = 0L;
    double fvalue = 0.0;
    int rtype = 1;  // Return type. Defaults to integer
    double divider = 1.0;    // Fractional divider

    if(*str == '\0') {
        return -1;
    }
    while(isdigit(*str)) {
        ivalue = 10L * ivalue + (*str - '0');
        str++;
    }
    if(*str == '.') {   // Expect a float
        rtype = 2;
        str++; // Skip the dot
        while(isdigit(*str)) {
            ivalue = 10L * ivalue + (*str - '0');
            str++;
            divider *= 10.0;
        }
        fvalue = (double)ivalue/divider;
    }
    if(*str != '\0') return -2; // Junk follows the number
    if (rtype == 1) {
        *ival = (int)ivalue;
    } else {
        *fval = (float)fvalue;
    }
    return rtype;
}

// Parse an integer in 'token'.
// Report an error if it's a float or not a string of digits or empty
int getint(int linenr, int pos, char * token, char * wholeline)
{
    int intval = 0;
    float fltval = 0.0;
    int ntype;

    ntype  = getnumber(token, &intval, &fltval);
    if(ntype == -2) { // Found rubbish
        tokenerr("Integer", linenr, pos, token, 1, wholeline);
    } else if (ntype == -1) {   // Found nothing
        tokenerr("Integer", linenr, pos, "nothing", 0, wholeline);
    } else if (ntype == 2) {    // Found float
        tokenerr("Integer", linenr, pos, "float", 0, wholeline);
    }
    return intval;
}

// Parse a float in 'token'.
// Report an error if it's an integer or not a string of digits or empty
float getfloat(int linenr, int pos, char * token, char * wholeline)
{
    int intval = 0;
    float fltval = 0.0;
    int ntype;

    ntype  = getnumber(token, &intval, &fltval);
    if(ntype == -2) { // Found rubbish
        tokenerr("Float", linenr, pos, token, 1, wholeline);
    } else if (ntype == -1) {   // Found nothing
        tokenerr("Float", linenr, pos, "nothing", 0, wholeline);
    } else if (ntype == 1) {    // Found integer
        tokenerr("Float", linenr, pos, "integer", 0, wholeline);
    }
    return fltval;
}

// Parse the configuration file
// Return != 0 on error
int parseconfig(char * fconfname)
{
    char * retval;
    char lb[MAX_CONFIG_LINELEN];
    char origln[MAX_CONFIG_LINELEN];
    int chpnr = 0; // Chapter number
    char * token; // Token pointer
    int confline = 0;   // Line number in the config file
    int pos;
    char * separators = " \t,{}\n";

    while(!feof(fconf)) {
        lb[0] = '\0'; // Clear the buffer
        // Clear out the data of the next chapter
        //chapterdata[chpnr] = {   0,   0,  0, {  0.000,   0.000,  0.000}, {  0.000,   0.000,  0.000} };
        chapterdata[chpnr].pgfirst = 0;
        chapterdata[chpnr].pglast = 0;
        chapterdata[chpnr].lastline = 0;
        chapterdata[chpnr].left.leftedge = 0.0;
        chapterdata[chpnr].left.firstbottom = 0.0;
        chapterdata[chpnr].left.linehight = 0.0;
        chapterdata[chpnr].right.leftedge = 0.0;
        chapterdata[chpnr].right.firstbottom = 0.0;
        chapterdata[chpnr].right.linehight = 0.0;

        retval = fgets(lb, MAX_CONFIG_LINELEN, fconf);
        confline++;
        if(retval == NULL) {
            if(feof(fconf)) { // Reached the end of the config file
                return configerr;
            }
            // Not EOF but no data: Error while reading line
            fprintf(stderr, "Error while reading line %d in configuration file: %s\n", confline, fconfname);
            perror(NULL);
            configerr = 1;
            return 1;
        }

        // We have succesfully read a line

        // Detect lines that are longer than the limit
        if(lb[strlen(lb)-1] != '\n') {
            char c;
            // Advance to newline so next fgets() call reads the next line, not the rest of the current line
            c = fgetc(fconf);
            while(c != '\n') {
                if (c == EOF) {
                    configerr = 1;
                    fprintf(stderr, "Configuration file does not end in newline.\n");
                    return configerr;
                }
                c = fgetc(fconf);
            }
            // This comes after the while loop. Otherwise this error will be printed if the last line in the file
            // is not terminated with a newline.
            fprintf(stderr, "Line %d: too long (more than %d characters). Rest ignored.\n", confline, MAX_CONFIG_LINELEN-1);
        }

        // Ignore comment lines and empty lines
        if (    lb[0] == '#' ||                     // '#' at start of line
                lb[0] == '%' ||                     // '%' at start of line
                (lb[0] == '/' && lb[1] == '/') ||   // '//' at start of line
                lb[0] == '\0'
            ) {
            continue; // Get the next line
        }

        // Ignore blank lines (with only spaces, tabs and a possible final newline)
        if(strspn(lb, " \t\n") == strlen(lb)) {
            continue; // Get the next line
        }

        // We have a non-comment line
        strncpy(origln, lb, MAX_CONFIG_LINELEN); // Save a copy of the original line before strtok() chops it up

        token = strtok(lb, separators);
        if (token == NULL) {
            fprintf(stderr, "Line %d: found none of 9 values\n", confline);
            configerr = 1;
            continue;
        }
        // Ignore lines that do not start with a number. Only give one error for those lines
        pos = (int)(token-lb) + 1;
        if (!isdigit(token[0])) {
            tokenerr("Data line with numbers", confline, pos, token, 1, origln);
            continue;
        }
        chapterdata[chpnr].pgfirst = getint(confline, pos, token, origln);

        token = strtok(NULL, separators);
        if (token == NULL) {
            fprintf(stderr, "Line %d: found only 1 of 9 values\n", confline);
            configerr = 1;
            continue;
        }
        pos = (int)(token-lb) + 1;
        chapterdata[chpnr].pglast = getint(confline, pos, token, origln);

        token = strtok(NULL, separators);
        if (token == NULL) {
            fprintf(stderr, "Line %d: found only 2 of 9 values\n", confline);
            configerr = 1;
            continue;
        }
        pos = (int)(token-lb) + 1;
        chapterdata[chpnr].lastline = getint(confline, pos, token, origln);

        token = strtok(NULL, separators);
        if (token == NULL) {
            fprintf(stderr, "Line %d: found only 3 of 9 values\n", confline);
            configerr = 1;
            continue;
        }
        pos = (int)(token-lb) + 1;
        chapterdata[chpnr].left.leftedge = getfloat(confline, pos, token, origln);

        token = strtok(NULL, separators);
        if (token == NULL) {
            fprintf(stderr, "Line %d: found only 4 of 9 values\n", confline);
            configerr = 1;
            continue;
        }
        pos = (int)(token-lb) + 1;
        chapterdata[chpnr].left.firstbottom = getfloat(confline, pos, token, origln);

        token = strtok(NULL, separators);
        if (token == NULL) {
            fprintf(stderr, "Line %d: found only 5 of 9 values\n", confline);
            configerr = 1;
            continue;
        }
        pos = (int)(token-lb) + 1;
        chapterdata[chpnr].left.linehight = getfloat(confline, pos, token, origln);

        token = strtok(NULL, separators);
        if (token == NULL) {
            fprintf(stderr, "Line %d: found only 6 of 9 values\n", confline);
            configerr = 1;
            continue;
        }
        pos = (int)(token-lb) + 1;
        chapterdata[chpnr].right.leftedge = getfloat(confline, pos, token, origln);

        token = strtok(NULL, separators);
        if (token == NULL) {
            fprintf(stderr, "Line %d: found only 7 of 9 values\n", confline);
            configerr = 1;
            continue;
        }
        pos = (int)(token-lb) + 1;
        chapterdata[chpnr].right.firstbottom = getfloat(confline, pos, token, origln);

        token = strtok(NULL, separators);
        if (token == NULL) {
            fprintf(stderr, "Line %d: found only 8 of 9 values\n", confline);
            configerr = 1;
            continue;
        }
        pos = (int)(token-lb) + 1;
        chapterdata[chpnr].right.linehight = getfloat(confline, pos, token, origln);

        // Catch extra data on the line that is not comment
        token = strtok(NULL, separators);
        if (token != NULL) { // There is another token
            if ( token[0] == '#' ||                         // '#' at start of line
                 token[0] == '%' ||                         // '%' at start of line
                 (token[0] == '/' && token[1] == '/') ||    // '//' at start of line
                 token[0] == '\0'
               ) { /* Ok */ } else {
                pos = (int)(token-lb) + 1;
                tokenerr("Either comment or nothing", confline, pos, token, 1, origln);
            }
        }

        chpnr++;    // We had a chapter data line. On to the next chapter
    }


    return configerr;
}

int main(int argc, char *argv[])
{
    char * fconfname;
    int parseresult;

    fout = stdout; // Use stdout as default output stream

    if(argc == 2) {
        // Single argument is taken as configuration file name
        fconfname = argv[1];
        fconf = fopen(fconfname, "r");
        if(fconf == NULL) {
            fprintf(stderr, "Configuration file not found: %s\n", fconfname);
            perror(NULL);
            exit(1);
        }
        if(feof(fconf)) {
            fprintf(stderr, "Configuration file is empty: %s\n", fconfname);
        }
        parseresult = parseconfig(fconfname);
        fclose(fconf);
        if(parseresult != 0) {
            exit(parseresult);
        }
    }
    outfile();
    return 0;
}