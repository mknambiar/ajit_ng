#!/bin/sh
set -e

MAIN=main
python=python3
#python=./.venv/bin/python
#readelf=./.venv/bin/readelf.py

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "ERROR: missing required tool '$1' on PATH." >&2
    echo "Source ./set_ajit_home and ./ajit_env, and ensure AjitPublicResources/tools is built." >&2
    echo "Typical fix: cd \$AJIT_HOME/AjitPublicResources/tools && scons" >&2
    exit 1
  fi
}

require_cmd genVmapAsm
require_cmd generateMemoryMap_Byte
require_cmd remapMemmap
require_cmd compileToSparcShim.py

# STEP 1: setup page tables.
genVmapAsm vmap.txt setup_page_tables.s

# STEP 2: compile to generate elf file.
# NOTE: the use of `-U` to enable uclibc
compileToSparcShim.py -I ./ -I $AJIT_SHIM_DIR/include -C $AJIT_SHIM_DIR/src -s init.s -s setup_page_tables.s -s trap_handlers.s -c ${MAIN}.c -N ${MAIN} -L LinkerScript.lnk -a gcc -D AJIT -U

# STEP 3: Read elf to generate mmap file.
echo "\nNOW READING ELF...\n"

# generate the mmap file for deployment on the processor
# NOTE: the use of `./pt_load_sections.py` script to extract loadable modules
#sparc64-linux-gnu-readelf `pt_load_sections.py ${MAIN}.elf` ${MAIN}.elf | grep 0x > ${MAIN}.hex 
#sparc64-linux-gnu-readelf `pt_load_sections.py ${MAIN}.elf` ${MAIN}.elf | grep 0x > ${MAIN}.hex 
generateMemoryMap_Byte ${MAIN}.hex > ${MAIN}.mmap
remapMemmap vmap.txt ${MAIN}.mmap ${MAIN}.mmap.remapped
