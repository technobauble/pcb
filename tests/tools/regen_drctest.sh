#!/bin/bash


if [ -L ../pcbtest.sh ]; then
  PCB=../../../pcbtest.sh 
elif [ -L ../pcbtest ]; then
  PCB=../../../pcbtest
fi

PCB=$PCB ./run_tests.sh --regen DRCTests

if [ $? -ne 0 ]; then
  exit 1
fi

rm golden/DRCTests/*-flags-after.txt
rm golden/DRCTests/*-flags-before.txt
rm golden/DRCTests/*.pcb
rm golden/DRCTests/drctest.script

