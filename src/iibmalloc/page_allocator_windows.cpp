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
 * iibmalloc allocator
 * 
 * v.1.00    May-09-2018    Initial release
 * 
 * -------------------------------------------------------------------------------*/


#include "iibmalloc_page_allocator.h"

#include <cstdlib>
#include <cstddef>
#include <memory>
#include <cstring>
#include <limits>

#include <windows.h>

//thread_local PageAllocatorWithCaching thg_PageAllocatorWithCaching;

template< typename... ARGS >
void allocLog(const char* formatStr, const ARGS&... args)
{
	constexpr size_t string_max = 255;
	char buff[string_max+1];
	snprintf(buff, string_max, formatStr, args...);
	printf("%s\n", buff);
}

/*static*/
size_t VirtualMemory::getPageSize()
{
	SYSTEM_INFO siSysInfo;
	GetSystemInfo(&siSysInfo);

	return static_cast<size_t>(siSysInfo.dwPageSize);
}

/*static*/
size_t VirtualMemory::getAllocGranularity()
{
	SYSTEM_INFO siSysInfo;
	GetSystemInfo(&siSysInfo);

	return static_cast<size_t>(siSysInfo.dwAllocationGranularity);
}

/*static*/
uint8_t* VirtualMemory::reserve(void* addr, size_t size)
{
	void* ptr = VirtualAlloc(addr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (!addr)
	{
		allocLog("VirtualMemory::reserve %zd 0x%x", (size_t)(ptr), size);
	}
	else if (addr == ptr)
	{
		allocLog("VirtualMemory::reserve %zd 0x%x", (size_t)(ptr), size);
	}
	else
	{
		allocLog("VirtualMemory::reserve  %zd 0x%x FAILED", (size_t)(addr), size);

		MEMORY_BASIC_INFORMATION info;
		SIZE_T s = VirtualQuery(addr, &info, size);


		allocLog("BaseAddress=%zd", (size_t)(info.BaseAddress));
		allocLog("AllocationBase=%zd", (size_t)(info.AllocationBase));
		allocLog("RegionSize=%zd", info.RegionSize);
		allocLog("State=%d", info.State);

		allocLog("Failed to get the required address range");
		assert(false);
		return nullptr;
	}

	printf( "  allocating %zd\n", size );
	return static_cast<uint8_t*>(ptr);
}

/*static*/
void VirtualMemory::commit(uintptr_t addr, size_t size)
{
	// TODO: revise necessity
	void* ptr = VirtualAlloc(reinterpret_cast<void*>(addr), size, MEM_COMMIT, PAGE_READWRITE);
	assert(ptr);
}

/*static*/
void VirtualMemory::decommit(uintptr_t addr, size_t size)
{
	// TODO: revise necessity
	BOOL r = VirtualFree(reinterpret_cast<void*>(addr), size, MEM_DECOMMIT);
	assert(r);
}

void* VirtualMemory::allocate(size_t size)
{
	void* ret = VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if ( ret != nullptr ) // hopefully, likely branch
		return ret;
	else
	{
		printf( "Reserving and commiting memory failed for size %zd (%zx), error = %d\n", size, size, GetLastError() );
		throw std::bad_alloc();
		return ret;
	}
}

void VirtualMemory::deallocate(void* ptr, size_t size)
{
	bool ret = VirtualFree(ptr, 0, MEM_RELEASE);
	if ( ret ) // hopefully, likely branch
		return;
	else
	{
		printf( "Releasing memory failed for size %zd (%zx) at address 0x%zx, error = %d\n", size, size, (size_t)ptr, GetLastError() );
		throw std::bad_alloc();
		return;
	}
}


void* VirtualMemory::AllocateAddressSpace(size_t size)
{
    void* ret = VirtualAlloc(NULL, size, MEM_RESERVE , PAGE_NOACCESS);
	if ( ret != nullptr ) // hopefully, likely branch
		return ret;
	else
	{
		printf( "Reserving memory failed for size %zd (%zx), error = %d\n", size, size, GetLastError() );
		throw std::bad_alloc();
		return ret;
	}
}
 
void* VirtualMemory::CommitMemory(void* addr, size_t size)
{
	void* ret = VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
	if ( ret != nullptr ) // hopefully, likely branch
		return ret;
	else
	{
		printf( "Commiting memory failed for size %zd (%zx) and addr 0x%zx, error = %d\n", size, size, (size_t)addr, GetLastError() );
		throw std::bad_alloc();
		return ret;
	}
}
 
void VirtualMemory::DecommitMemory(void* addr, size_t size)
{
    BOOL ret = VirtualFree((void*)addr, size, MEM_DECOMMIT);
	if ( ret ) // hopefully, likely branch
		return;
	else
	{
		printf( "Decommiting memory failed for size %zd (%zx) at address 0x%zx, error = %d\n", size, size, (size_t)addr, GetLastError() );
		throw std::bad_alloc();
		return;
	}
}
 
void VirtualMemory::FreeAddressSpace(void* addr, size_t size)
{
    BOOL ret = VirtualFree((void*)addr, 0, MEM_RELEASE);
	if ( ret ) // hopefully, likely branch
		return;
	else
	{
		printf( "Releasing memory failed for size %zd (%zx) at address 0x%zx, error = %d\n", size, size, (size_t)addr, GetLastError() );
		throw std::bad_alloc();
		return;
	}
}
