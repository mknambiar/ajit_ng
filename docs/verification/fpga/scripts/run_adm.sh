# set the environment
cd /home/ajit/ajit-toolchain
source set_ajit_home
source ajit_env
# go to work directory..
#  THIS IS SPECIFIC TO THE EXAMPLE.
cd $AJIT_HOME/...etc...
# generate the adm script.
adm_script_gen 2 1 0x40000000 cortos_build/main.mmap.remapped  results.txt  adm_script.txt ignore_log.txt
# calibrate the UART
calibrateUart -u $TTY 1000000
#  Supply all your arguments here (paths in container)
ajit_debug_monitor_mt -n 2 -t 1 -m ./cortos_build/main.mmap.remapped -O -f $AJIT_HOME/..etc../adm_script.txt | tee adm.log 
# thats it.
exit
