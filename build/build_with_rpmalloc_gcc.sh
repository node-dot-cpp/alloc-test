#!/bin/sh

gcc -c ../src/rpmalloc/rpmalloc.c -DNDEBUG -O3 -funit-at-a-time -fstrict-aliasing -o rpmalloc.o
g++ -DUSE_RPMALLOC=1 ../src/test_common.cpp ../src/allocator_tester.cpp ../src/rpmalloc_allocator.cpp -std=c++17 -g -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-empty-body -DNDEBUG -O2 -flto  -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free -L/home/mattias/projects/rpmalloc/lib/linux/release/x86-64 -I../src/rpmalloc rpmalloc.o -lpthread -o tester_rpmalloc.bin
