set -e
export PATH="${AJIT_PROJECT_HOME}/.local-bin:${PATH}"
CWD=$(pwd)
rm -f *.mmap aggregated.mmap main.mmap
./compile_for_ajit_uclibc.sh
cd cpu_0
rm -f *.mmap
./compile_for_ajit_uclibc.sh
cd $CWD
cd cpu_1
rm -f *.mmap
./compile_for_ajit_uclibc.sh
cd $CWD
cat core*.mmap cpu_0/core*.mmap cpu_1/core*.mmap > aggregated.mmap
cp aggregated.mmap main.mmap
