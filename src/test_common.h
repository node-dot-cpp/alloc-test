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
 * Memory allocator tester -- common
 * 
 * v.1.00    Jun-22-2018    Initial release
 * 
 * -------------------------------------------------------------------------------*/


#ifndef ALLOCATOR_TEST_COMMON_H
#define ALLOCATOR_TEST_COMMON_H

#include <memory>
#include <cstdint>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#if _MSC_VER
#include <intrin.h>
#define ALIGN(n)      __declspec(align(n))
#define NOINLINE      __declspec(noinline)
#define FORCE_INLINE	__forceinline
#elif __GNUC__
#include <x86intrin.h>
#define ALIGN(n)      __attribute__ ((aligned(n))) 
#define NOINLINE      __attribute__ ((noinline))
#define	FORCE_INLINE inline __attribute__((always_inline))
#else
#define	FORCE_INLINE inline
#define NOINLINE
//#define ALIGN(n)
#warning ALIGN, FORCE_INLINE and NOINLINE may not be properly defined
#endif

int64_t GetMicrosecondCount();
size_t GetMillisecondCount();
size_t getRss();

constexpr size_t max_threads = 32;

enum MEM_ACCESS_TYPE { none, single, full };

struct ThreadTestRes
{
	size_t threadID;

	size_t innerDur;

	uint64_t rdtscBegin;
	uint64_t rdtscSetup;
	uint64_t rdtscMainLoop;
	uint64_t rdtscExit;
};

inline
void printThreadStats( const char* prefix, ThreadTestRes& res )
{
	uint64_t rdtscTotal = res.rdtscExit - res.rdtscBegin;
	printf( "%s%zd: %zdms; %zd (%.2f | %.2f | %.2f);\n", prefix, res.threadID, res.innerDur, rdtscTotal, (res.rdtscSetup - res.rdtscBegin) * 100. / rdtscTotal, (res.rdtscMainLoop - res.rdtscSetup) * 100. / rdtscTotal, (res.rdtscExit - res.rdtscMainLoop) * 100. / rdtscTotal );
}

struct TestRes
{
	size_t duration;
	size_t cumulativeDuration;
	ThreadTestRes threadRes[max_threads];
};

struct TestStartupParams
{
	size_t threadCount;
	size_t maxItems;
	size_t maxItemSize;
	size_t iterCount;
	MEM_ACCESS_TYPE mat;
};

struct TestStartupParamsAndResults
{
	TestStartupParams startupParams;
	TestRes* testRes;
};

struct ThreadStartupParamsAndResults
{
	TestStartupParams startupParams;
	size_t threadID;
	ThreadTestRes* threadRes;
};


#endif // ALLOCATOR_TEST_COMMON_H