set -e
CWD=$(pwd)
rm -f *.mmap
./compile_for_ajit_uclibc.sh
cd cpu_0
rm -f *.mmap
./compile_for_ajit_uclibc.sh
cd $CWD
cd cpu_1
rm -f *.mmap
./compile_for_ajit_uclibc.sh
cd $CWD
cat dhrystone_wrap.mmap cpu_0/dhrystone.mmap cpu_1/dhrystone.mmap > aggregated.mmap
cp aggregated.mmap main.mmap
