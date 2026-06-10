#!/usr/bin/env bash
set -e

rm -f ./*.mmap
./compile_for_ajit_uclibc.sh
cp whetstone.mmap main.mmap
