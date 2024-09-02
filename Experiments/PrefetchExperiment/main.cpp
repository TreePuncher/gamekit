#include <containers.hpp>
#include <vector>
#include <cstdlib>
#include <print>
#include <chrono>

constexpr size_t testSize = 100'000'000;

void RunLinearTest()
{
	std::vector<int> ints{};

	for (size_t i = 0; i < testSize; i++)
		ints.push_back(rand());

	auto begin = std::chrono::high_resolution_clock::now();

	int64_t accumulator = 0;

	for (auto I : ints)
		accumulator += I;

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = end - begin;

	std::print("{}\n", accumulator);
	std::print("No random iteration, time taken: {}\n", duration);
}


void RunNaieveTest()
{
	std::vector<int> ints		{};
	std::vector<int> indexes	{};
	std::vector<int> scrambled	{};

	for (size_t i = 0; i < testSize; i++)
		ints.push_back(rand());

	// Get random order to read ints
	for (size_t i = 0; i < testSize; i++)
		indexes.push_back(i);

	while (indexes.size() > 2)
	{
		auto idx = rand() % indexes.size();
		auto i = indexes[idx];
		indexes[idx] = indexes.back();
		indexes.pop_back();

		scrambled.push_back(i);
	}

	while (indexes.size())
	{
		scrambled.push_back(indexes.back());
		indexes.pop_back();
	}

	auto begin = std::chrono::high_resolution_clock::now();

	int64_t accumulator = 0;

	for (auto I : scrambled)
		accumulator += ints[scrambled[I]];

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = end - begin;

	std::print("{}\n", accumulator);
	std::print("No prefetch, time taken: {}\n", duration);
}


template<size_t prefetchSize = 1>
auto RunPreFetchTest()
{
	std::vector<int> ints		{};
	std::vector<int> indexes	{};
	std::vector<int> scrambled	{};

	for (size_t i = 0; i < testSize; i++)
		ints.push_back(rand());


	// Get random order to read ints
	for (size_t i = 0; i < testSize; i++)
		indexes.push_back(i);

	while (indexes.size() > 2)
	{
		auto idx = rand() % indexes.size();
		auto i = indexes[idx];
		indexes[idx] = indexes.back();
		indexes.pop_back();

		scrambled.push_back(i);
	}

	while (indexes.size())
	{
		scrambled.push_back(indexes.back());
		indexes.pop_back();
	}

	FlexKit::CircularBuffer<int, 1> prefetchQueue;

	auto begin = std::chrono::high_resolution_clock::now();

	int64_t accumulator = prefetchSize;
	auto* data_ptr = ints.data();
	for (auto i : scrambled)
	{
		_mm_prefetch((char*)data_ptr + i, _MM_HINT_T0);

		prefetchQueue.push_back(i, [&](auto tailValue)
			{
				accumulator += data_ptr[i];
			});
	}

	for (auto i : prefetchQueue)
		accumulator += data_ptr[i];

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = end - begin;

	std::print("{}\n", accumulator);
	std::print("Prefetch {} elements ahead, time taken: {}\n", prefetchSize, duration);

	return duration;
}


int main()
{
	using namespace std::chrono_literals;

	std::print("Test set size {}\n", testSize);

	std::chrono::high_resolution_clock::duration times[12];

	for(auto& t : times)
		t = 0ns;

	for (int i = 0; i < 10; i++)
	{
		std::print("Trial {}\n", i);
		RunLinearTest();
		RunNaieveTest();
		times[0] += RunPreFetchTest<1>();
		times[1] += RunPreFetchTest<2>();
		times[2] += RunPreFetchTest<4>();
		times[3] += RunPreFetchTest<8>();
		times[4] += RunPreFetchTest<16>();
		times[5] += RunPreFetchTest<32>();
		times[6] += RunPreFetchTest<64>();
		times[7] += RunPreFetchTest<128>();
		times[8] += RunPreFetchTest<256>();
		times[9] += RunPreFetchTest<512>();
		times[10] += RunPreFetchTest<1024>();
		times[11] += RunPreFetchTest<2048>();

		for (auto&& [idx, t] : std::views::enumerate(times))
			std::print("rolling average for prefetch {} size: {}\n", 0x01 << idx, t / (i + 1));
	}

	return 0;
}
