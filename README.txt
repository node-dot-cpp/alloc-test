To test an allocator:

1. Create "src/my_allocator.h" file with a class representing an allocator to be tested.
   As a sample "src/new_delete_allocator.h" or "src/void_allocator.h" could be taken.
   class MyAllocator must have all member functions found in the above-mentioned samples
   as this class is used as a template parameter in the testing routine
   
2. Edit "src/selector.h" to switch to using a new allocator

3. Rebuild and run