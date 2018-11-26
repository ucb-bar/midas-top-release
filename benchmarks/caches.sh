#!/bin/bash

SIZES=(16 32 64 128 256 512 1024 2048 4096 5000 7000 8192 10000 12000 16000 \
       20000 32768 45000 65536 131000 180000 262144 300000 524288 650000 800000 1000000 1200000 1600000)
STRIDES=(16)
CURPWD=$(pwd)

cd $(dirname $1)
echo "@NumDataPointsPerSet=["${#SIZES[@]}"]" > $CURPWD/caches.report.txt
for s in ${STRIDES[@]}; do
        echo "Stride: " $s
        for i in ${SIZES[@]}; do
                echo "$@ $CURPWD/pk $CURPWD/caches $i 10000000 $s"
                ./$(basename $1) ${@:2} $CURPWD/pk $CURPWD/caches $i 10000000 $s >> $CURPWD/caches.report.txt 2> $CURPWD/caches-$s-$i.err
                
        done
done
cd $CURPWD
