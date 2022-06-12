#!/bin/bash

source ./activate
make clean
cd ./vm
make
cd ./build

pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/close-bad-fd:close-bad-fd --swap-disk=4 -- -q   -f run close-bad-fd
# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/read-normal:read-normal -p ../../tests/userprog/sample.txt:sample.txt --swap-disk=4 -- -q   -f run read-normal
# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/read-boundary:read-boundary -p ../../tests/userprog/sample.txt:sample.txt --swap-disk=4 -- -q   -f run read-boundary
# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/read-bad-fd:read-bad-fd --swap-disk=4 -- -q   -f run read-bad-fd
# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/write-bad-fd:write-bad-fd --swap-disk=4 -- -q   -f run write-bad-fd



# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/fork-once:fork-once --swap-disk=4 -- -q   -f run fork-once


# pintos --gdb   --fs-disk=10 -p tests/vm/mmap-bad-off:mmap-bad-off -p ../../tests/vm/large.txt:large.txt --swap-disk=4 -- -q   -f run mmap-bad-off
# pintos --fs-disk=10 -p tests/userprog/exit:exit --swap-disk=4 -- -q   -f run exit
# pintos --fs-disk=10 -p tests/userprog/args-none:args-none --swap-disk=4 -- -q   -f run args-none
# pintos -v -k -T 600 -m 40   --fs-disk=10 -p tests/vm/swap-fork:swap-fork -p tests/vm/child-swap:child-swap --swap-disk=200 -- -q   -f run swap-fork


# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/pt-grow-stack:pt-grow-stack --swap-disk=4 -- -q   -f run pt-grow-stack
# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/pt-grow-bad:pt-grow-bad --swap-disk=4 -- -q   -f run pt-grow-bad
# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/pt-grow-stk-sc:pt-grow-stk-sc --swap-disk=4 -- -q   -f run pt-grow-stk-sc
