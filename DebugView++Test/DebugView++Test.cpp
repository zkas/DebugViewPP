// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"

#define BOOST_TEST_MODULE DebugView++Lib Unit Test
#include <boost/test/unit_test_gui.hpp>
#include <random>

#include "Win32Lib/utilities.h"
#include "DebugView++Lib/ProcessInfo.h"
#include "IndexedStorageLib/IndexedStorage.h"
#include "Win32Lib/Win32Lib.h"
#include "DebugView++Lib/DBWinBuffer.h"
#include "DebugView++Lib/LogSources.h"
#include "DebugView++Lib/TestSource.h"
#include "DebugView++Lib/LineBuffer.h"

namespace fusion {
namespace debugviewpp {

BOOST_AUTO_TEST_SUITE(DebugViewPlusPlusLib)

std::string GetTestString(int i)
{
	return stringbuilder() << "BB_TEST_ABCDEFGHI_EE_" << i;
}

class TestLineBuffer : public LineBuffer
{
public:
	TestLineBuffer(size_t size) : LineBuffer(size) {}

	void WaitForReaderTimeout()
	{
		throw std::exception("read timeout");
	}
};

BOOST_AUTO_TEST_CASE(LineBufferTest)
{
	TestLineBuffer buffer(500);
	Timer timer;

	for (int j=0; j< 1000; ++j)
	{
		for (int i=0; i<17; ++i)
		{
			buffer.Add(timer.Get(), GetSystemTimeAsFileTime(), 0, "test");
		}

		auto lines = buffer.GetLines();
		BOOST_REQUIRE_EQUAL(lines.size(), 17);
	}
}

//BOOST_AUTO_TEST_CASE(IndexedStorageRandomAccess)
void testDisabled2()
{
	using namespace indexedstorage;

	size_t testSize = 10000;
	auto testMax = testSize - 1;

	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(0, testMax);
	SnappyStorage s;
	for (size_t i = 0; i < testSize; ++i)
		s.Add(GetTestString(i));

	for (size_t i = 0; i < testSize; ++i)
	{
		size_t j = distribution(generator);  // generates number in the range 0..testMax 
		BOOST_REQUIRE_EQUAL(s[j], GetTestString(j));
	}
}

//BOOST_AUTO_TEST_CASE(IndexedStorageCompression)
void testDisabled()
{
	using namespace indexedstorage;

	// the memory allocator will mess up test results is < 64 kb is allocated
	// make sure SnappyStorage allocates at least ~500kb for reproducable results
	
	// this test is indicative only, on average the SnappyStorage should allocate at most 50% of memory compared to a normal vector.
	// since GetTestString returns an overly simpe-to-compress string, it will appear to perform insanely good.

	size_t testSize = 100000;	
	VectorStorage v;
	SnappyStorage s;

	size_t m0 = ProcessInfo::GetPrivateBytes();

	for (size_t i = 0; i < testSize; ++i)
		v.Add(GetTestString(i));

	size_t m1 = ProcessInfo::GetPrivateBytes();
	size_t usedByVector = m1 - m0;
	BOOST_MESSAGE("VectorStorage requires: " << usedByVector/1024 << " kB");

	for (size_t i = 0; i < testSize; ++i)
		s.Add(GetTestString(i));

	size_t m2 = ProcessInfo::GetPrivateBytes();
	size_t usedBySnappy = m2 - m1;

	BOOST_MESSAGE("SnappyStorage requires: " << usedBySnappy/1024 << " kB (" << (100*usedBySnappy)/usedByVector << "%)");
	BOOST_REQUIRE_GT(0.50*usedByVector, usedBySnappy);
}

BOOST_AUTO_TEST_CASE(LogSourcesTest)
{
	LogSources logsources(false);
	auto logsource = logsources.AddTestSource();

	BOOST_MESSAGE("add line");
	logsource->Add(0, "TEST1");
	BOOST_MESSAGE("add line");
	logsource->Add(0, "TEST2");
	BOOST_MESSAGE("add line");
	logsource->Add(0, "TEST3\n");
	BOOST_MESSAGE("3 lines added.");

	auto lines = logsources.GetLines();
	BOOST_MESSAGE("received: " << lines.size() << " lines.");

	BOOST_REQUIRE_EQUAL(lines.size(), 3);

	for (auto it = lines.begin(); it != lines.end(); ++it)
	{
		auto line = *it;
		BOOST_MESSAGE("line: " << line.message);
	}

	const int testsize = 10000;
	BOOST_MESSAGE("Write " << testsize << " lines...");
	for (int i=0; i < testsize; ++i)
	{
		logsource->Add(0, "TESTSTRING 1234\n");
	}

	auto morelines = logsources.GetLines();
	BOOST_MESSAGE("received: " << morelines.size() << " lines.");
	BOOST_REQUIRE_EQUAL(morelines.size(), testsize);
}


BOOST_AUTO_TEST_SUITE_END()

} // namespace debugviewpp 
} // namespace fusion
