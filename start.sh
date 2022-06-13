#!/bin/bash

source ./activate
make clean
# cd ./userprog
cd ./vm
make
cd ./build


# PASS!
# pintos --fs-disk=10 -p tests/userprog/fork-once:fork-once --swap-disk=4 -- -q   -f run fork-once
# pintos --fs-disk=10 -p tests/userprog/fork-once:fork-once -- -q   -f run fork-once
# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/exec-arg:exec-arg -p tests/userprog/child-args:child-args --swap-disk=4 -- -q   -f run exec-arg
# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/fork-read:fork-read -p ../../tests/userprog/sample.txt:sample.txt --swap-disk=4 -- -q   -f run fork-read
# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/fork-close:fork-close -p ../../tests/userprog/sample.txt:sample.txt --swap-disk=4 -- -q   -f run fork-close
# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/fork-boundary:fork-boundary --swap-disk=4 -- -q   -f run fork-boundary
# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/exec-boundary:exec-boundary -p tests/userprog/child-simple:child-simple --swap-disk=4 -- -q   -f run exec-boundary
# pintos --fs-disk=10 -p tests/userprog/exec-read:exec-read -p ../../tests/userprog/sample.txt:sample.txt -p tests/userprog/child-read:child-read --swap-disk=4 -- -q   -f run exec-read



# pintos --gdb   --fs-disk=10 -p tests/vm/mmap-bad-off:mmap-bad-off -p ../../tests/vm/large.txt:large.txt --swap-disk=4 -- -q   -f run mmap-bad-off
# pintos --fs-disk=10 -p tests/userprog/exit:exit --swap-disk=4 -- -q   -f run exit
# pintos --fs-disk=10 -p tests/userprog/args-none:args-none --swap-disk=4 -- -q   -f run args-none
# pintos -v -k -T 600 -m 40   --fs-disk=10 -p tests/vm/swap-fork:swap-fork -p tests/vm/child-swap:child-swap --swap-disk=200 -- -q   -f run swap-fork


# stack grow

pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/pt-grow-stack:pt-grow-stack --swap-disk=4 -- -q   -f run pt-grow-stack < /dev/null 2> tests/vm/pt-grow-stack.errors > tests/vm/pt-grow-stack.output
perl -I../.. ../../tests/vm/pt-grow-stack.ck tests/vm/pt-grow-stack tests/vm/pt-grow-stack.result

pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/pt-grow-bad:pt-grow-bad --swap-disk=4 -- -q   -f run pt-grow-bad < /dev/null 2> tests/vm/pt-grow-bad.errors > tests/vm/pt-grow-bad.output
perl -I../.. ../../tests/vm/pt-grow-bad.ck tests/vm/pt-grow-bad tests/vm/pt-grow-bad.result

pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/pt-big-stk-obj:pt-big-stk-obj --swap-disk=4 -- -q   -f run pt-big-stk-obj < /dev/null 2> tests/vm/pt-big-stk-obj.errors > tests/vm/pt-big-stk-obj.output
perl -I../.. ../../tests/vm/pt-big-stk-obj.ck tests/vm/pt-big-stk-obj tests/vm/pt-big-stk-obj.result

pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/vm/pt-grow-stk-sc:pt-grow-stk-sc --swap-disk=4 -- -q   -f run pt-grow-stk-sc < /dev/null 2> tests/vm/pt-grow-stk-sc.errors > tests/vm/pt-grow-stk-sc.output
perl -I../.. ../../tests/vm/pt-grow-stk-sc.ck tests/vm/pt-grow-stk-sc tests/vm/pt-grow-stk-sc.result


# 이상한것
# create-empty?


# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/write-bad-fd:write-bad-fd --swap-disk=4 -- -q   -f run write-bad-fd < /dev/null 2> tests/userprog/write-bad-fd.errors > tests/userprog/write-bad-fd.output

# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/bad-read:bad-read --swap-disk=4 -- -q   -f run bad-read < /dev/null 2> tests/userprog/bad-read.errors > tests/userprog/bad-read.output
# perl -I../.. ../../tests/userprog/bad-read.ck tests/userprog/bad-read tests/userprog/bad-read.result


# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/write-bad-fd:write-bad-fd --swap-disk=4 -- -q   -f run write-bad-fd < /dev/null 2> tests/userprog/write-bad-fd.errors > tests/userprog/write-bad-fd.output
# perl -I../.. ../../tests/userprog/write-bad-fd.ck tests/userprog/write-bad-fd tests/userprog/write-bad-fd.result

# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/read-bad-fd:read-bad-fd --swap-disk=4 -- -q   -f run read-bad-fd < /dev/null 2> tests/userprog/read-bad-fd.errors > tests/userprog/read-bad-fd.output
# perl -I../.. ../../tests/userprog/read-bad-fd.ck tests/userprog/read-bad-fd tests/userprog/read-bad-fd.result
# FAIL tests/userprog/read-bad-fd

# pintos -v -k -T 60 -m 20   --fs-disk=10 -p tests/userprog/close-bad-fd:close-bad-fd --swap-disk=4 -- -q   -f run close-bad-fd < /dev/null 2> tests/userprog/close-bad-fd.errors > tests/userprog/close-bad-fd.output
# perl -I../.. ../../tests/userprog/close-bad-fd.ck tests/userprog/close-bad-fd tests/userprog/close-bad-fd.result
# FAIL tests/userprog/close-bad-fd