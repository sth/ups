#!/bin/sh

if [ $# -eq 0 ]; then
   FILE=`xgetfile -title "Open program..."`
else
   FILE=$1
fi

aterm -title UPS -e ups $FILE
