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


#include "iibmalloc.h"

#include <cstdlib>
#include <cstddef>
#include <memory>
#include <cstring>
#include <limits>

#include <windows.h>

thread_local SerializableAllocatorBase g_AllocManager;

void* operator new(std::size_t count)
{
	return g_AllocManager.allocate(count);
}

void* operator new[](std::size_t count)
{
	return g_AllocManager.allocate(count);
}

void operator delete(void* ptr) noexcept
{
	g_AllocManager.deallocate(ptr);
}

void operator delete[](void* ptr) noexcept
{
	g_AllocManager.deallocate(ptr);
}

#if __cplusplus >= 201703L

//We don't support alignment new/delete yet

//void* operator new(std::size_t count, std::align_val_t alignment);
//void* operator new[](std::size_t count, std::align_val_t alignment);
//void operator delete(void* ptr, std::align_val_t al) noexcept;
//void operator delete[](void* ptr, std::align_val_t al) noexcept;
#endif




//void SerializableAllocatorBase::deserialize(const std::string& fileName)
//{
//	assert(false);//TODO
//	/* On replay we avoid using heap before mmap
//		* so we have the best chances that heap address
//		* we need does not get used.
//		* So we read the header on the stack
//		*/
//	assert(!heap);
//	allocLog("SerializableAllocatorBase::deserialize fileName='{}'", fileName);
//
//	//int fd = open(fileName.c_str(), O_RDONLY);
//	//assert(fd != -1);
//	std::wstring wsTmp(fileName.begin(), fileName.end());
//	HANDLE fHandle = CreateFile(wsTmp.c_str(),
//		GENERIC_READ,
//		0,
//		NULL,
//		OPEN_EXISTING,
//		FILE_ATTRIBUTE_NORMAL,
//		NULL);
//	assert(fHandle != INVALID_HANDLE_VALUE);
//	
//	//First read header to a temporary location
//	Heap tmp;
//	//ssize_t c = read(fd, &tmp, sizeof(tmp));
//	//assert(c == sizeof(tmp));
//	DWORD readed = 0;
//	BOOL r = ReadFile(fHandle, &tmp, sizeof(tmp), &readed, NULL);
//	assert(r && readed == sizeof(tmp));
//	assert(tmp.magic == SerializableAllocatorMagic.asUint);
//// 		assert(tmp->usedSize <= bufferSize);
//	
//	//void* ptr = mmap(tmp.begin, tmp.allocSize, PROT_READ|PROT_WRITE,
//	//					MAP_PRIVATE|MAP_NORESERVE/*|MAP_FIXED*/, fd, 0);
//
//	//Now we can reserve all memory
////	void* ptr = VirtualMemory::reserve(reinterpret_cast<void*>(tmp.begin), tmp.getReservedSize());
//	
//	//Last, reset file and re-read everything
//	DWORD p = SetFilePointer(fHandle, 0, 0, FILE_BEGIN);
//	assert(p != INVALID_SET_FILE_POINTER);
//
//	
//	//BOOL rf = ReadFile(fHandle, ptr, static_cast<DWORD>(tmp.getReservedSize()), &readed, NULL);
//	//assert(rf && static_cast<size_t>(readed) == tmp.getReservedSize());
//
//	CloseHandle(fHandle);
//
////	heap = reinterpret_cast<Heap*>(ptr);
//	//allocLog("Aslr anchorPoint {}", (void*)&anchorPoint);
//	//arena->fixAslrVtables(&anchorPoint);
//	//arenaEnd = reinterpret_cast<uint8_t*>(ptr) + tmp.getReservedSize();
//}

