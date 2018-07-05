#!/bin/sh

clang -c ../src/rpmalloc/rpmalloc.c -O3 -funit-at-a-time -fstrict-aliasing -o rpmalloc.o
clang++ -DUSE_RPMALLOC=1 ../src/test_common.cpp ../src/allocator_tester.cpp ../src/rpmalloc_allocator.cpp -std=c++1z -g -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-empty-body -DNDEBUG -O3 -I ../src/rpmalloc -flto rpmalloc.o -lpthread -o tester_rpmalloc.bin
