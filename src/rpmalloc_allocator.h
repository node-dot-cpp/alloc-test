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
 * Memory allocator tester -- rpmalloc allocator
 * 
 * v.1.00    Jun-22-2018    Initial release
 * 
 * -------------------------------------------------------------------------------*/


#ifndef RPMALLOC_ALLOCATOR_H
#define RPMALLOC_ALLOCATOR_H

#include "test_common.h"

#include <rpmalloc.h>

#include <atomic>

class RPMallocGuard {
public:
	RPMallocGuard() {
		bool notInitialized = false;
		if (isInitialized.compare_exchange_strong(notInitialized, true))
			rpmalloc_initialize();
	}

	void ref() {}

	static std::atomic_bool isInitialized;
};

class RPMallocAllocatorForTest
{
	ThreadTestRes* testRes;

public:
	RPMallocAllocatorForTest( ThreadTestRes* testRes_ ) { testRes = testRes_; rpmalloc_guard.ref(); }
	static constexpr bool isFake() { return false; }

	static constexpr const char* name() { return "rpmalloc allocator"; }

	void init() { rpmalloc_thread_initialize(); }
	void* allocate( size_t sz ) { return rpmalloc(sz); }
	void deallocate( void* ptr ) { rpfree(ptr); }
	void deinit() { rpmalloc_thread_finalize(); }

	// next calls are to get additional stats of the allocator, etc, if desired
	void doWhateverAfterSetupPhase() {}
	void doWhateverAfterMainLoopPhase() {}
	void doWhateverAfterCleanupPhase() {}

	ThreadTestRes* getTestRes() { return testRes; }

	RPMallocGuard rpmalloc_guard;
};




#endif // RPMALLOC_ALLOCATOR_H