MAIN=dhrystone
AAR=$AJIT_PROJECT_HOME/tools/ajit_access_routines_mt
PT=$AJIT_PROJECT_HOME/tools/minimal_printf_timer
NDHRYSTONE_ITERS="${DHRYSTONE_ITERS:-100}"
DEFS=" -D AJIT -D DHRYOPT -D HAS_FLOAT -D NO_GLIBC -D NDHRYSTONE_ITERS=${NDHRYSTONE_ITERS}"
SRCS=" -c ../unified.c -c $AAR/src/ajit_access_routines.c -s init.s -C $PT/src -s $AAR/asm/mutexes.s "
INCLUDES=" -I ../../ -I $AAR/include -I $PT/include -I $AJIT_UCLIBC_HEADERS -I $AJIT_LIBGCC_INSTALL_DIR/include "
python3 "$AJIT_PROJECT_HOME/tools/linker/makeLinkerScript.py" -t 0x40020000 -d 0x40030000 -o customLinkerScript.lnk
compileToSparcUclibc.py -o 3 $SRCS $INCLUDES -N ${MAIN} -L customLinkerScript.lnk $DEFS
