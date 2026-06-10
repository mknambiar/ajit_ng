MAIN=coremark
AAR=$AJIT_ACCESS_ROUTINES_MT
COREMARK_ROOT=../../ajit_test
INCLUDES="-I ${COREMARK_ROOT} -I ${COREMARK_ROOT}/include/ -I ${COREMARK_ROOT}/env/ -I $AAR/include -I $AJIT_UCLIBC_HEADERS -I $AJIT_LIBGCC_INSTALL_DIR/include"
SRCS="-C ${COREMARK_ROOT}/env/ -C ${COREMARK_ROOT}/src/ -c $AAR/src/ajit_access_routines.c -s ./init.s -s ../trap_handlers.s -s $AAR/asm/mutexes.s "
COREMARK_ITERS="${COREMARK_ITERS:-2000}"
DEFS="-D NO_GLIBC -D PERFORMANCE_RUN=1 -D ITERATIONS=${COREMARK_ITERS} -D CORE_DEBUG=0 -D COMPILER_REQUIRES_SORT_RETURN=1"

#Step 1: Generate the Linker Script
makeLinkerScript.py -t 0x40040000 -d 0x40050000 -o customLinkerScript.lnk

#Step 2: Compile the application using uclibc
compileToSparcUclibc.py -o 2 -U -N ${MAIN} $INCLUDES $SRCS -L customLinkerScript.lnk $DEFS -F 'fgcse-sm' -F 'funroll-loops'  -F 'finline-functions'
