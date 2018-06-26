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
 * Memory allocator tester -- main
 * 
 * v.1.00    Jun-22-2018    Initial release
 * 
 * -------------------------------------------------------------------------------*/

#include "selector.h"
#include "allocator_tester.h"

thread_local unsigned long long rnd_seed = 0;



void* runRandomTest( void* params )
{
	assert( params != nullptr );
	ThreadStartupParamsAndResults* testParams = reinterpret_cast<ThreadStartupParamsAndResults*>( params );
	MyAllocatorT allocator( testParams->threadRes );
	printf( "    running thread %zd with %s [rnd_seed = %llu] ...\n", testParams->threadID, allocator.name(), rnd_seed );
	switch ( testParams->startupParams.mat )
	{
		case MEM_ACCESS_TYPE::none:
			randomPos_RandomSize<MyAllocatorT,MEM_ACCESS_TYPE::none>( allocator, testParams->startupParams.iterCount, testParams->startupParams.maxItems, testParams->startupParams.maxItemSize, testParams->threadID );
			break;
		case MEM_ACCESS_TYPE::full:
			randomPos_RandomSize<MyAllocatorT,MEM_ACCESS_TYPE::full>( allocator, testParams->startupParams.iterCount, testParams->startupParams.maxItems, testParams->startupParams.maxItemSize, testParams->threadID );
			break;
		case MEM_ACCESS_TYPE::single:
			randomPos_RandomSize<MyAllocatorT,MEM_ACCESS_TYPE::single>( allocator, testParams->startupParams.iterCount, testParams->startupParams.maxItems, testParams->startupParams.maxItemSize, testParams->threadID );
			break;
	}

	return nullptr;
}

void runTest( TestStartupParamsAndResults* startupParams )
{
	size_t threadCount = startupParams->startupParams.threadCount;

	size_t start = GetMillisecondCount();

	ThreadStartupParamsAndResults testParams[max_threads];
	std::thread threads[ max_threads ];

	for ( size_t i=0; i<threadCount; ++i )
	{
		memcpy( testParams + i, startupParams, sizeof(TestStartupParams) );
		testParams[i].threadID = i;
		testParams[i].threadRes = startupParams->testRes->threadRes + i;
	}

	// run threads
	for ( size_t i=0; i<threadCount; ++i )
	{
		printf( "about to run thread %zd...\n", i );
		std::thread t1( runRandomTest, (void*)(testParams + i) );
		threads[i] = std::move( t1 );
		printf( "    ...done\n" );
	}
	// join threads
	for ( size_t i=0; i<threadCount; ++i )
	{
		printf( "joining thread %zd...\n", i );
		threads[i].join();
		printf( "    ...done\n" );
	}

	size_t end = GetMillisecondCount();
	startupParams->testRes->duration = end - start;
	printf( "%zd threads made %zd alloc/dealloc operations in %zd ms (%zd ms per 1 million)\n", threadCount, startupParams->startupParams.iterCount * threadCount, end - start, (end - start) * 1000000 / (startupParams->startupParams.iterCount * threadCount) );
	startupParams->testRes->cumulativeDuration = 0;
	startupParams->testRes->rssMax = 0;
	startupParams->testRes->allocatedAfterSetupSz = 0;
	for ( size_t i=0; i<threadCount; ++i )
	{
		startupParams->testRes->cumulativeDuration += startupParams->testRes->threadRes[i].innerDur;
		startupParams->testRes->allocatedAfterSetupSz += startupParams->testRes->threadRes[i].allocatedAfterSetupSz;
		if ( startupParams->testRes->rssMax < startupParams->testRes->threadRes[i].rssMax )
			startupParams->testRes->rssMax = startupParams->testRes->threadRes[i].rssMax;
	}
	startupParams->testRes->cumulativeDuration /= threadCount;
}

int main()
{ 
	TestRes testRes[max_threads];

	memset( testRes, 0, sizeof( testRes ) );

	TestStartupParamsAndResults params;
	params.startupParams.iterCount = 100000000;
	params.startupParams.maxItemSize = 16;
//		params.startupParams.maxItems = 23 << 20;
	params.startupParams.mat = MEM_ACCESS_TYPE::full;

	size_t threadCountMax = 23;

	for ( params.startupParams.threadCount=1; params.startupParams.threadCount<=threadCountMax; ++(params.startupParams.threadCount) )
	{
		params.startupParams.maxItems = (1 << 25) / params.startupParams.threadCount;
		params.testRes = testRes + params.startupParams.threadCount;
		runTest( &params );
	}

	printf( "Test summary:\n" );
	for ( size_t threadCount=1; threadCount<=threadCountMax; ++threadCount )
	{
		TestRes& tr = testRes[threadCount];
		printf( "%zd,%zd\n", threadCount, tr.duration );
		printf( "Per-thread stats:\n" );
		for ( size_t i=0;i<threadCount;++i )
		{
			printf( "   %zd:\n", i );
			printThreadStats( "\t", tr.threadRes[i] );
		}
	}
	printf( "\n" );
	printf( "Short test summary for USE_RANDOMPOS_RANDOMSIZE:\n" );
	for ( size_t threadCount=1; threadCount<=threadCountMax; ++threadCount )
			printf( "%zd,%zd,%zd,%zd\n", threadCount, testRes[threadCount].duration, testRes[threadCount].rssMax, testRes[threadCount].allocatedAfterSetupSz );

	printf( "Short test summary for USE_RANDOMPOS_RANDOMSIZE (alt computations):\n" );
	for ( size_t threadCount=1; threadCount<=threadCountMax; ++threadCount )
			printf( "%zd,%zd,%zd,%zd\n", threadCount, testRes[threadCount].cumulativeDuration, testRes[threadCount].rssMax, testRes[threadCount].allocatedAfterSetupSz );

	return 0;
}

