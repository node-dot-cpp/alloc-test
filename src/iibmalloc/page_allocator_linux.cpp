/* -------------------------------------------------------------------------------
 * Copyright (c) 2018, OLogN Technologies AG
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * -------------------------------------------------------------------------------
 * 
 * Per-thread bucket allocator
 * 
 * v.1.00    May-09-2018    Initial release
 * 
 * -------------------------------------------------------------------------------*/
 
 
#include "page_allocator.h"

#include <cstdlib>
#include <cstddef>
#include <memory>
#include <cstring>
#include <limits>

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>


thread_local PageAllocatorWithCaching thg_PageAllocatorWithCaching;

// limit below is single read or write op in linux
static constexpr size_t MAX_LINUX = 0x7ffff000;
// [DI: what depends on what/] static_assert(MAX_LINUX <= MAX_CHUNK_SIZE, "Use of big chunks needs review.");

/*static*/
size_t VirtualMemory::getPageSize()
{
	long sz = sysconf(_SC_PAGESIZE);
	assert(sz != -1);
	return sz;  
}

/*static*/
size_t VirtualMemory::getAllocGranularity()
{
	// On linux page size and alloc granularity are the same thing
	return getPageSize();
}


/*static*/
uint8_t* VirtualMemory::reserve(void* addr, size_t size)
{
	return nullptr;
}

/*static*/
void VirtualMemory::commit(uintptr_t addr, size_t size)
{
	assert(false);
}

///*static*/
void VirtualMemory::decommit(uintptr_t addr, size_t size)
{
	assert(false);
}

void* VirtualMemory::allocate(size_t size)
{
	void* ptr = mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (ptr == (void*)(-1))
	{
		int e = errno;
		printf( "mmap error at allocate(%zd), error = %d (%s)\n", size, e, strerror(e) );
		throw std::bad_alloc();
	}

	return ptr;
}

void VirtualMemory::deallocate(void* ptr, size_t size)
{
	assert( size % 4096 == 0 );
	int ret = munmap(ptr, size);
 	if ( ret == -1 )
	{
		int e = errno;
		printf( "munmap error at deallocate(0x%zx, 0x%zx), error = %d (%s)\n", (size_t)(ptr), size, e, strerror(e) );
//		printf( "munmap error at deallocate(%zd), error = %d (%s)\n", size, e, strerror(e) );
		throw std::bad_alloc();
	}
}



void* VirtualMemory::AllocateAddressSpace(size_t size)
{
    void * ptr = mmap((void*)0, size, PROT_NONE, MAP_PRIVATE|MAP_ANON, -1, 0);
	if (ptr == (void*)(-1))
	{
		int e = errno;
		printf( "mmap error at AllocateAddressSpace(%zd), error = %d (%s)\n", size, e, strerror(e) );
		throw std::bad_alloc();
	}
 //   msync(ptr, size, MS_SYNC|MS_INVALIDATE);
    return ptr;
}
 
void* VirtualMemory::CommitMemory(void* addr, size_t size)
{
//    void * ptr = mmap(addr, size, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED|MAP_ANON, -1, 0);
    void * ptr = mmap(addr, size, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_PRIVATE|MAP_ANON, -1, 0);
	if (ptr == (void*)(-1))
	{
		int e = errno;
//		printf( "allocation error at CommitMemory(0x%zx, 0x%zx), error = %d (%s)\n", (size_t)(addr), size, e, strerror(e) );
		printf( "mmap error at CommitMemory(%zd), error = %d (%s)\n", size, e, strerror(e) );
		throw std::bad_alloc();
	}
//    msync(addr, size, MS_SYNC|MS_INVALIDATE);
    return ptr;
}
 
void VirtualMemory::DecommitMemory(void* addr, size_t size)
{
    // instead of unmapping the address, we're just gonna trick 
    // the TLB to mark this as a new mapped area which, due to 
    // demand paging, will not be committed until used.
 
    void * ptr = mmap(addr, size, PROT_NONE, MAP_FIXED|MAP_PRIVATE|MAP_ANON, -1, 0);
 	if (ptr == (void*)(-1))
	{
		int e = errno;
//		printf( "allocation error at DecommitMemory(0x%zx, 0x%zx), error = %d (%s)\n", (size_t)(addr), size, e, strerror(e) );
		printf( "mmap error at DecommitMemory(%zd), error = %d (%s)\n", size, e, strerror(e) );
		throw std::bad_alloc();
	}
   msync(addr, size, MS_SYNC|MS_INVALIDATE);
}
 
void VirtualMemory::FreeAddressSpace(void* addr, size_t size)
{
    int ret = msync(addr, size, MS_SYNC);
  	if ( ret == -1 )
	{
		int e = errno;
//		printf( "allocation error at FreeAddressSpace(0x%zx, 0x%zx), error = %d (%s)\n", (size_t)(addr), size, e, strerror(e) );
		printf( "msync error at FreeAddressSpace(%zd), error = %d (%s)\n", size, e, strerror(e) );
		throw std::bad_alloc();
	}
	ret = munmap(addr, size);
 	if ( ret == -1 )
	{
		int e = errno;
//		printf( "allocation error at FreeAddressSpace(0x%zx, 0x%zx), error = %d (%s)\n", (size_t)(addr), size, e, strerror(e) );
		printf( "munmap error at FreeAddressSpace(%zd), error = %d (%s)\n", size, e, strerror(e) );
		throw std::bad_alloc();
	}
}