TEXTBASE=0x40000000
DATABASE=0x40040000
MAIN=whetstone
AAR=$AJIT_HOME/AjitPublicResources/tools/ajit_access_routines_mt
PT=$AJIT_HOME/AjitPublicResources/tools/minimal_printf_timer
SHIM=$AJIT_HOME/ajit_shim
WHETSTONE_PASSES_X100="${WHETSTONE_PASSES_X100:-1}"
WHETSTONE_DURATION="${WHETSTONE_DURATION:-100}"
DEFS=" -D AJIT -D DP -D HAS_FLOAT -D PRINTON -D CLK_FREQUENCY=80000000 -D WHETSTONE_PASSES_X100=${WHETSTONE_PASSES_X100} -D WHETSTONE_DURATION=${WHETSTONE_DURATION} "
#DEFS=" -D AJIT -D DP -D HAS_FLOAT "
SRCS=" -C ../src/ -C $SHIM/src -c $AAR/src/ajit_access_routines.c -C $PT/src -s $AAR/asm/trap_handlers.s -s ../src/init.s "
INCLUDES="-I ../src/ -I ../ -I $SHIM/include -I $AAR/include -I $PT/include -I $AJIT_UCLIBC_HEADERS -I $AJIT_LIBGCC_INSTALL_DIR/include"
# -I /usr/include " see if really needed
OPTS=" -F fschedule-insns -F fschedule-insns2 -F frename-registers "
python3 "$AJIT_PROJECT_HOME/tools/linker/makeLinkerScript.py" -t $TEXTBASE -d $DATABASE -o customLinkerScript.lnk
compileToSparcUclibc.py -o 3 $SRCS $INCLUDES -N ${MAIN} $OPTS -L customLinkerScript.lnk $DEFS
