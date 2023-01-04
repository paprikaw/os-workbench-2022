#!/bin/sh
export LD_LIBRARY_PATH=..
cd ..
make
cd tests/
make clean
make
 gdb libco-test-64
# valgrind -s --leak-check=full ./libco-test-64
# valgrind  ./libco-test-64
# valgrind --leak-check=full --show-leak-kinds=all ./libco-test-64
