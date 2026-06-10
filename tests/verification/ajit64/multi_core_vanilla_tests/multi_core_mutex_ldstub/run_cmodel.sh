#!/bin/sh

# run AJIT C Model on the generated mmap file
MAIN=mem_bw
ajit_C_system_model -n 4 -m ${MAIN}.mmap.remapped -w ${MAIN}.wtrace > serial.out 2> run.log 
