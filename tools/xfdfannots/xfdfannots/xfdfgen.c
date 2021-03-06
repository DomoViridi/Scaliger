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

// Destination file name
char * dest_file;

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
    if (dest_file != NULL) {
        fprintf(fout, "    <f href=\"%s\" />\n", dest_file);
    }
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

        // Check for lines that start with 'F'
        // Skip initial whitespace
        pos = 0;
        while(isspace(lb[pos])) pos++;
        if (lb[pos] == 'F') {
            unsigned long dflen; // Length of the destination file name
            // Command must be a solitary 'F', followed by whitespace and a file name
            pos++;
            if (!isspace(lb[pos])) {
                fprintf(stderr, "Line %d: no whitespace after 'F' command\n", confline);
                configerr = 1;
                continue;
            }

            // We have an 'F' command followed by whitespace;
            // Skip whitespace to the file name (newline counts as whitespace)
            while(isspace(lb[pos])) pos++;

            // There must be a filename here (which may contain spaces)
            if (lb[pos] == '\0') {
                // No file name found
                fprintf(stderr, "Line %d: no filename found after 'F' command\n", confline);
                configerr = 1;
                continue;
            }
            // We found a string, which will be taken to be the destination file name
            // This includes everything up to the end-of-line (which is the end of the lb string)
            // If there was an earlier F command, then ignore this one and give a warning
            if (dest_file != NULL) {
                fprintf(stderr, "Line %d: repeated 'F' command; only the first will be used. This line will be ignored\n", confline);
                // no error
                continue;
            }
            // Note: the lb[] buffer still contains the terminating \n
            dflen=strlen(lb+pos); // The number of characters including the \n
            dest_file = malloc(dflen);
            if (dest_file == NULL) {
                fprintf(stderr, "Line %d: out of memory: no room for file name given in 'F' command\n", confline);
                configerr = 1;
                continue;
            }
            strncpy(dest_file, lb+pos, dflen);   // save a copy of the file name
            // Replace the terminating newline with a string terminator
            dest_file[dflen-1] ='\0';

            // We succesfully parsed a line with an 'F' command
            // Go on to the next line
            continue;
        }

        // Not a line with a command. Interpret as a data line
        // Chop the line up into tokens
        token = strtok(lb, separators);
        if (token == NULL) {
            fprintf(stderr, "Line %d: found no information\n", confline);
            configerr = 1;
            continue;
        }

        // First token must be a number
        pos = (int)(token-lb) + 1;
        // Only give one error for lines that do not start with a number
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

void usage(void)
{
    fprintf(stderr, "Usage: %s <configuration file>\n", getprogname());
    fprintf(stderr,
            "This program reads the configuration file and writes an XFDF file to stdout\n"
            "which describes a set of annotations which can be added to a PDF file.\n"
            "These annotations form a column of numbers on each page, starting with 1,\n"
            "then 5, then continuing in increments of 5.\n"
            "The configuration file describes where these numbers are placed on the pages.\n"
            "See the sample configuration file for more information.\n"
            );
}

int main(int argc, char *argv[])
{
    char * fconfname;
    int parseresult;

    fout = stdout; // Use stdout as default output stream

    // Initialise the program name
    setprogname(argv[0]);

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
        outfile();
    } else {
        // Wrong nr of arguments: show usage and exit with error code
        fprintf(stderr, "No configuration file given\n");
        usage();
        exit(1);
    }
    return 0;
}