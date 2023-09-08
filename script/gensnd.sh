#!/bin/sh

if [ $# -lt 2 ]; then
  echo "usage: $0 <file> <output>"
  exit 1
fi

ffmpeg -i $1 -f u8 -ar 48000 -ab 48k -ac 1 $2
