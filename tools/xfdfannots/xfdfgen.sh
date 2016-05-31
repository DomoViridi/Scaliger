#!/bin/bash
PGFIRST=83
PGLAST=90
# Nominal hight of a line
lineheight=19

# Measures of every text rectangle
boxwidth=12
boxhight=12

# Bottom edge of first rectangle in either margin
y0=877

# Left edge of rectangles in left-hand margin
x1=106

# Right edge of rectangles in left-hand margin
(( x2 = x1 + boxwidth ))

# Left edge of rectangles in right-hand margin
x3=500

# Right edge of rectangles in right-hand margin
(( x4 = x3 + boxwidth ))

cat << EOF0
<?xml version="1.0" encoding="UTF-8"?>
<xfdf xmlns="http://ns.adobe.com/xfdf/" xml:space="preserve">
    <annots>
EOF0

pg=$PGFIRST
while (( pg <= PGLAST ))
do
    for nr in 1 5 10 15 20 25 30 35 40
    do
        (( y1 = y0 - ( nr - 1 ) * lineheight ))
        (( y2 = y1 + boxhight ))
        echo "        <freetext width=\"0\""
        echo "            flags=\"print\""
        echo "            justification=\"right\""
        echo "            page=\"$pg\""

        if (( pg % 2 == 0 ))
        then
            echo "            rect=\"$x1,$y1,$x2,$y2\""
        else
            echo "            rect=\"$x3,$y1,$x4,$y2\""
        fi
        echo "        >"
        echo "            <contents>$nr</contents>"
        echo "            <defaultstyle>font: Helvetica 10.0pt; text-align:right; color:#FF0000</defaultstyle>"
#        echo "             <defaultstyle>font: LucidaGrande 11.000000pt; text-align:right; color:#ff0000</defaultstyle>"
        echo "        </freetext>"

    done
    ((pg += 1)) 
done

cat <<EOF9
    </annots>
    <f href="../Documents/work/Temp/1629-Geneva-1(orig)-Opus_de_emendatione_temporum_hac_postrem copy.pdf"/>
    <ids original="27E63E7C58902FD49EF0C376902BEEA7"
         modified="27E63E7C58902FD49EF0C376902BEEA7"/>
</xfdf>
EOF9

exit 0
