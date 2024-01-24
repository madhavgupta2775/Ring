#ifndef MACRO
#include <random>
#include <iostream>
#include <cstring>
#endif

template <typename T>
class Random
{
private:
	std::mt19937_64 mt;
	std::uniform_int_distribution<T> nums;
public:
	Random(T l, T r) : nums{ l, r }
	{
		std::random_device rd{};
		std::seed_seq ss{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
		std::mt19937_64 mt{ ss };
	}
	T rand_num()
	{
		return nums(mt);
	}
};
