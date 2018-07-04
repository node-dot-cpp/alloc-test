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
 * Page Allocator: 
 *     - returns a requested number of allocated (or previously cached) pages
 *     - accepts a pointer to pages to be deallocated (pointer to and number of
 *       pages must be that from one of requests for allocation)
 * 
 * v.1.00    May-09-2018    Initial release
 * 
 * -------------------------------------------------------------------------------*/

 
#ifndef PAGE_ALLOCATOR_H
#define PAGE_ALLOCATOR_H

#include "iibmalloc_common.h"

#define GET_PERF_DATA

#ifdef GET_PERF_DATA
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#endif // GET_PERF_DATA


/* OS specific implementations */
class VirtualMemory
{
public:
	static size_t getPageSize();
	static size_t getAllocGranularity();

	static unsigned char* reserve(void* addr, size_t size);
	static void commit(uintptr_t addr, size_t size);
	static void decommit(uintptr_t addr, size_t size);

	static void* allocate(size_t size);
	static void deallocate(void* ptr, size_t size);
//	static void release(void* addr);

	static void* AllocateAddressSpace(size_t size);
	static void* CommitMemory(void* addr, size_t size);
	static void DecommitMemory(void* addr, size_t size);
	static void FreeAddressSpace(void* addr, size_t size);
};

struct MemoryBlockListItem
{
	MemoryBlockListItem* next;
	MemoryBlockListItem* prev;
	typedef size_t SizeT; //todo
//	static constexpr SizeT INUSE_FLAG = 1;
	SizeT size;
	SizeT sizeIndex;

	void initialize(SizeT sz, SizeT szIndex)
	{
		size = sz;
		prev = nullptr;
		next = nullptr;
		sizeIndex = szIndex;
	}

	void listInitializeEmpty()
	{
		next = this;
		prev = this;
	}
	SizeT getSizeIndex() const { return sizeIndex; }
	SizeT getSize() const {	return size; }

	void listInsertNext(MemoryBlockListItem* other)
	{
		assert(isInList());
		assert(!other->isInList());

		other->next = this->next;
		other->prev = this;
		next->prev = other;
		next = other;
	}

	MemoryBlockListItem* listGetNext()
	{
		return next;
	}
	
	MemoryBlockListItem* listGetPrev()
	{
		return prev;
	}

	bool isInList() const
	{
		if (prev == nullptr && next == nullptr)
			return false;
		else
		{
			assert(prev != nullptr);
			assert(next != nullptr);
			return true;
		}
	}

	void removeFromList()
	{
		assert(prev != nullptr);
		assert(next != nullptr);

		prev->next = next;
		next->prev = prev;

		next = nullptr;
		prev = nullptr;
	}


};


class MemoryBlockList
{
private:
	uint32_t count = 0;
	MemoryBlockListItem lst;
public:
	
	MemoryBlockList()
	{
		initialize();
	}

	void initialize()
	{
		count = 0;
		lst.listInitializeEmpty();
	}

	FORCE_INLINE
	bool empty() const { return count == 0; }
	FORCE_INLINE
		uint32_t size() const { return count; }
	FORCE_INLINE
		bool isEnd(MemoryBlockListItem* item) const { return item == &lst; }

	FORCE_INLINE
	MemoryBlockListItem* front()

	{
		return lst.listGetNext();
	}

	FORCE_INLINE
	void pushFront(MemoryBlockListItem* chk)
	{
		lst.listInsertNext(chk);
		++count;
	}

	FORCE_INLINE
	MemoryBlockListItem* popFront()
	{
		assert(!empty());

		MemoryBlockListItem* chk = lst.listGetNext();
		chk->removeFromList();
		--count;

		return chk;
	}

	FORCE_INLINE
		MemoryBlockListItem* popBack()
	{
		assert(!empty());

		MemoryBlockListItem* chk = lst.listGetPrev();
		chk->removeFromList();
		--count;

		return chk;
	}

	FORCE_INLINE
	void remove(MemoryBlockListItem* chk)
	{
		chk->removeFromList();
		--count;
	}

	size_t getCount() const { return count; }
};

struct BlockStats
{
	//alloc / dealloc ops
	uint64_t sysAllocCount = 0;
	uint64_t sysAllocSize = 0;
	uint64_t rdtscSysAllocSpent = 0;

	uint64_t sysDeallocCount = 0;
	uint64_t sysDeallocSize = 0;
	uint64_t rdtscSysDeallocSpent = 0;

	uint64_t allocRequestCount = 0;
	uint64_t allocRequestSize = 0;

	uint64_t deallocRequestCount = 0;
	uint64_t deallocRequestSize = 0;

	void printStats()
	{
		printf("Allocs %zd (%zd), ", sysAllocCount, sysAllocSize);
		printf("Deallocs %zd (%zd), ", sysDeallocCount, sysDeallocSize);

		uint64_t ct = sysAllocCount - sysDeallocCount;
		uint64_t sz = sysAllocSize - sysDeallocSize;

		printf("Diff %zd (%zd)\n\n", ct, sz);
	}

	void registerAllocRequest( size_t sz )
	{
		allocRequestSize += sz;
		++allocRequestCount;
	}
	void registerDeallocRequest( size_t sz )
	{
		deallocRequestSize += sz;
		++deallocRequestCount;
	}

	void registerSysAlloc( size_t sz, uint64_t rdtscSpent )
	{
		sysAllocSize += sz;
		rdtscSysAllocSpent += rdtscSpent;
		++sysAllocCount;
	}
	void registerSysDealloc( size_t sz, uint64_t rdtscSpent )
	{
		sysDeallocSize += sz;
		rdtscSysDeallocSpent += rdtscSpent;
		++sysDeallocCount;
	}
};

struct PageAllocator // rather a proof of concept
{
	BlockStats stats;
	uint8_t blockSizeExp = 0;

public:

	void initialize(uint8_t blockSizeExp)
	{
		this->blockSizeExp = blockSizeExp;
	}

	MemoryBlockListItem* getFreeBlock(size_t sz)
	{
		stats.registerAllocRequest( sz );

		assert(isAlignedExp(sz, blockSizeExp));

		uint64_t start = __rdtsc();
		void* ptr = VirtualMemory::allocate(sz);
		uint64_t end = __rdtsc();

		stats.registerSysAlloc( sz, end - start );

		if (ptr)
		{
			MemoryBlockListItem* chk = static_cast<MemoryBlockListItem*>(ptr);
			chk->initialize(sz, 0);
			return chk;
		}
		//todo enlarge top chunk

		throw std::bad_alloc();
	}


	void freeChunk( MemoryBlockListItem* chk )
	{
		size_t ix = chk->getSizeIndex();
		assert ( ix == 0 );

		size_t sz = chk->getSize();
		stats.registerDeallocRequest( sz );
		uint64_t start = __rdtsc();
		VirtualMemory::deallocate(chk, sz );
		uint64_t end = __rdtsc();
		stats.registerSysDealloc( sz, end - start );

	}

	const BlockStats& getStats() const { return stats; }

	void printStats()
	{
		stats.printStats();
	}

	void* AllocateAddressSpace(size_t size)
	{
		return VirtualMemory::AllocateAddressSpace( size );
	}
	void* CommitMemory(void* addr, size_t size)
	{
		return VirtualMemory::CommitMemory( addr, size);
	}
	void DecommitMemory(void* addr, size_t size)
	{
		VirtualMemory::DecommitMemory( addr, size );
	}
	void FreeAddressSpace(void* addr, size_t size)
	{
		VirtualMemory::FreeAddressSpace( addr, size );
	}
};


constexpr size_t max_cached_size = 20; // # of pages
constexpr size_t single_page_cache_size = 32;
constexpr size_t multi_page_cache_size = 4;

struct PageAllocatorWithCaching // to be further developed for practical purposes
{
//	Chunk* topChunk = nullptr;
	std::array<MemoryBlockList, max_cached_size+1> freeBlocks;

	BlockStats stats;
	//uintptr_t blocksBegin = 0;
	//uintptr_t uninitializedBlocksBegin = 0;
	//uintptr_t blocksEnd = 0;
	uint8_t blockSizeExp = 0;

public:

	void initialize(uint8_t blockSizeExp)
	{
		this->blockSizeExp = blockSizeExp;
		for ( size_t ix=0; ix<=max_cached_size; ++ ix )
			freeBlocks[ix].initialize();
	}

	void deinitialize()
	{
		for ( size_t ix=0; ix<=max_cached_size; ++ ix )
		{
			while ( !freeBlocks[ix].empty() )
			{
				MemoryBlockListItem* chk = static_cast<MemoryBlockListItem*>(freeBlocks[ix].popFront());
				size_t sz = chk->getSize();
				uint64_t start = __rdtsc();
				VirtualMemory::deallocate(chk, sz );
				uint64_t end = __rdtsc();
				stats.registerSysDealloc( sz, end - start );
			}
		}
	}

	MemoryBlockListItem* getFreeBlock(size_t sz)
	{
		stats.registerAllocRequest( sz );

		assert(isAlignedExp(sz, blockSizeExp));

		size_t ix = (sz >> blockSizeExp)-1;
		if (ix < max_cached_size)
		{
			if (!freeBlocks[ix].empty())
			{
				MemoryBlockListItem* chk = static_cast<MemoryBlockListItem*>(freeBlocks[ix].popFront());
				chk->initialize(sz, ix);
//				assert(chk->isFree());
//				chk->setInUse();
				return chk;
			}
		}

		uint64_t start = __rdtsc();
		void* ptr = VirtualMemory::allocate(sz);
		uint64_t end = __rdtsc();
		stats.registerSysAlloc( sz, end - start );

		if (ptr)
		{
			MemoryBlockListItem* chk = static_cast<MemoryBlockListItem*>(ptr);
			chk->initialize(sz, ix);
			return chk;
		}
		//todo enlarge top chunk

		throw std::bad_alloc();
	}

	void* getFreeBlockNoCache(size_t sz)
	{
		stats.registerAllocRequest( sz );

		assert(isAlignedExp(sz, blockSizeExp));

		uint64_t start = __rdtsc();
		void* ptr = VirtualMemory::allocate(sz);
		uint64_t end = __rdtsc();
		stats.registerSysAlloc( sz, end - start );

		if (ptr)
			return ptr;

		throw std::bad_alloc();
	}


	void freeChunk( MemoryBlockListItem* chk )
	{
//		assert(!chk->isFree());
//		assert(!chk->isInList());

		size_t sz = chk->getSize();
		stats.registerDeallocRequest( sz );
//		size_t ix = chk->getSizeIndex();
		size_t ix = (sz >> blockSizeExp)-1;
		if ( ix == 0 ) // quite likely case (all bucket chunks)
		{
			if ( freeBlocks[ix].getCount() < single_page_cache_size )
			{
//				chk->setFree();
				freeBlocks[ix].pushFront(chk);
				return;
			}
		}
		else if ( ix < max_cached_size && freeBlocks[ix].getCount() < multi_page_cache_size )
		{
//			chk->setFree();
			freeBlocks[ix].pushFront(chk);
			return;
		}

		uint64_t start = __rdtsc();
		VirtualMemory::deallocate(chk, sz );
		uint64_t end = __rdtsc();
		stats.registerSysDealloc( sz, end - start );
	}

	void freeChunkNoCache( void* block, size_t sz )
	{
//		assert(!chk->isFree());
//		assert(!chk->isInList());

		stats.registerDeallocRequest( sz );

		uint64_t start = __rdtsc();
		VirtualMemory::deallocate( block, sz );
		uint64_t end = __rdtsc();
		stats.registerSysDealloc( sz, end - start );
	}

	const BlockStats& getStats() const { return stats; }

	void printStats()
	{
		stats.printStats();
	}

	void* AllocateAddressSpace(size_t size)
	{
		return VirtualMemory::AllocateAddressSpace( size );
	}
	void* CommitMemory(void* addr, size_t size)
	{
		stats.registerAllocRequest( size );
		void* ret = VirtualMemory::CommitMemory( addr, size);
		if (ret == (void*)(-1))
		{
			printf( "Committing failed at %zd (%zx) (0x%zx bytes in total)\n", stats.allocRequestCount, stats.allocRequestCount, stats.allocRequestSize );
		}
		return ret;
	}
	void DecommitMemory(void* addr, size_t size)
	{
		VirtualMemory::DecommitMemory( addr, size );
	}
	void FreeAddressSpace(void* addr, size_t size)
	{
		VirtualMemory::FreeAddressSpace( addr, size );
	}
};

struct PageAllocatorNoCachingForTestPurposes // to be further developed for practical purposes
{
	uint8_t* basePtr = nullptr;
	uint8_t* currentPtr = nullptr;
	BlockStats stats;
	uint8_t blockSizeExp = 0;
	size_t allocFixedSize;

public:

	void initialize(uint8_t blockSizeExp)
	{
		this->blockSizeExp = blockSizeExp;
		allocFixedSize = (1 << 30);
		uint64_t start = __rdtsc();
		basePtr = reinterpret_cast<uint8_t*>( VirtualMemory::allocate(allocFixedSize) );
		uint64_t end = __rdtsc();
		stats.registerSysAlloc( allocFixedSize, end - start );
		currentPtr = basePtr;
	}

	void deinitialize()
	{
		uint64_t start = __rdtsc();
		VirtualMemory::deallocate( basePtr, allocFixedSize );
		uint64_t end = __rdtsc();
		stats.registerSysDealloc( allocFixedSize, end - start );
		currentPtr = nullptr;
	}

	MemoryBlockListItem* getFreeBlock(size_t sz)
	{
		MemoryBlockListItem* ret = reinterpret_cast<MemoryBlockListItem*>(currentPtr);
		currentPtr += sz;
		if ( currentPtr > basePtr + allocFixedSize )
		{
			printf( "Total amount of memory requested exceeds 0x%zx\n", allocFixedSize );
			throw std::bad_alloc();
		}
		return ret;
	}

	void* getFreeBlockNoCache(size_t sz)
	{
		MemoryBlockListItem* ret = reinterpret_cast<MemoryBlockListItem*>(currentPtr);
		currentPtr += sz;
		if ( currentPtr > basePtr + allocFixedSize )
		{
			printf( "Total amount of memory requested exceeds 0x%zx\n", allocFixedSize );
			throw std::bad_alloc();
		}
		return ret;
	}


	void freeChunk( MemoryBlockListItem* chk )
	{
//		throw std::bad_alloc();
	}

	void freeChunkNoCache( void* block, size_t sz )
	{
//		throw std::bad_alloc();
	}

	const BlockStats& getStats() const { return stats; }

	void printStats()
	{
		stats.printStats();
	}

	void* AllocateAddressSpace(size_t size)
	{
		void* ret = currentPtr;
		currentPtr += size;
		if ( currentPtr > basePtr + allocFixedSize )
		{
			printf( "Total amount of memory requested exceeds 0x%zx\n", allocFixedSize );
			throw std::bad_alloc();
		}
		return ret;
	}
	void* CommitMemory(void* addr, size_t size)
	{
		return addr;
	}
	void DecommitMemory(void* addr, size_t size)
	{
	}
	void FreeAddressSpace(void* addr, size_t size)
	{
	}
};


#endif //PAGE_ALLOCATOR_H
