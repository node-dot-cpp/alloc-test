NOTE: 32-BIT PLATFORMS HAVE NOT YET BEEN TESTED!

To test tcmalloc / jemalloc / hoard / ptmalloc:

1. Make sure new_delete_allocator is selected at selector.h

2. See useful build notes at section "Testing" at http://ithare.com/testing-memory-allocators-ptmalloc2-tcmalloc-hoard-jemalloc-while-trying-to-simulate-real-world-loads/

3. Edit build_gcc.sh to enable one of the above allocators (add/edit to have one of -ljemalloc, -ltcmalloc, -lhoard, or -L path/to/libmalloc.a) (
   Note: -lhoard: see https://github.com/emeryberger/Hoard
         -L path/to/libmalloc.a assumes that libmalloc.a is already built for a target platform


To test any other allocator:

1. Create "src/my_allocator.h" file with a class representing an allocator to be tested.
   As a sample "src/new_delete_allocator.h" or "src/void_allocator.h" could be taken.
   class MyAllocator must have all member functions found in the above-mentioned samples
   as this class is used as a template parameter in the testing routine
   
2. Edit "src/selector.h" to switch to using a new allocator

3. Rebuild and run