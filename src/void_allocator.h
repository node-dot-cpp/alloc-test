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
 * Memory allocator tester -- void allocator
 * 
 * v.1.00    Jun-22-2018    Initial release
 * 
 * -------------------------------------------------------------------------------*/


#ifndef VOID_ALLOCATOR_H
#define VOID_ALLOCATOR_H

#include "test_common.h"


class VoidAllocatorForTest
{
	CommonTestResults* testRes;
	size_t start;
	uint8_t* fakeBuffer = nullptr;
	static constexpr size_t fakeBufferSize = 0x1000000;

public:
	VoidAllocatorForTest( CommonTestResults* testRes_ ) { testRes = testRes_; }
	static constexpr bool isFake() { return true; } // thus indicating that certain checks over allocated memory should be ommited

	static constexpr const char* name() { return "void allocator"; }

	void init( size_t threadID )
	{
		start = GetMillisecondCount();
		testRes->threadID = threadID; // just as received
		testRes->rdtscBegin = __rdtsc();
		fakeBuffer = new uint8_t [fakeBufferSize];
	}

	void* allocate( size_t sz ) { assert( sz <= fakeBufferSize ); return fakeBuffer; }
	void deallocate( void* ptr ) {}

	void deinit() { if ( fakeBuffer ) delete [] fakeBuffer; fakeBuffer = nullptr; }

	void doWhateverAfterSetupPhase() { testRes->rdtscSetup = __rdtsc(); }
	void doWhateverAfterMainLoopPhase() { testRes->rdtscMainLoop = __rdtsc(); }
	void doWhateverAfterCleanupPhase()
	{
		testRes->rdtscExit = __rdtsc();
		testRes->innerDur = GetMillisecondCount() - start;
	}
};




#endif // VOID_ALLOCATOR_H