#!/bin/sh

TSTR=`sed -e':x
             /\\\\$/ { N
                      s/\\\\\n/ /g
                      bx
                     }' tests.list`
echo "$TSTR" >> tests_list_without_continued_lines.txt
all_tests=`echo $TSTR | ${AWK} 'BEGIN{FS="|"} /^#/{next} {print $1}' | sed 's; ;;g'`
echo $all_tests >> test_names.txt

