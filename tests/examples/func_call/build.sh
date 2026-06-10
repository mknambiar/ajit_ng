#!/bin/sh

MAIN=main
python=python3

# STEP 1: setup page tables.
genVmapAsm vmap.txt setup_page_tables.s

# STEP 2: compile to generate elf file.
# NOTE: the use of `-U` to enable uclibc
compileToSparcShim.py -I ./ -I $AJIT_SHIM_DIR/include -C $AJIT_SHIM_DIR/src -s init.s -s setup_page_tables.s -s trap_handlers.s -c ${MAIN}.c -N ${MAIN} -L LinkerScript.lnk -a gcc -D AJIT -U

# STEP 3: Read elf to generate mmap file.
echo "\nNOW READING ELF...\n"

# generate the mmap file for deployment on the processor
# NOTE: the use of `./pt_load_sections.py` script to extract loadable modules
#sparc-linux-readelf `pt_load_sections.py ${MAIN}.elf` ${MAIN}.elf | grep 0x > ${MAIN}.hex 
generateMemoryMap_Byte ${MAIN}.hex > ${MAIN}.mmap
remapMemmap vmap.txt ${MAIN}.mmap ${MAIN}.mmap.remapped

