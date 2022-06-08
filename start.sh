#!/bin/bash

source ./activate
make clean
cd ./vm
make
cd ./build


pintos --gdb   --fs-disk=10 -p tests/vm/mmap-bad-off:mmap-bad-off -p ../../tests/vm/large.txt:large.txt --swap-disk=4 -- -q   -f run mmap-bad-off