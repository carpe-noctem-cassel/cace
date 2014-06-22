/*
 * serialization.cpp
 *
 *  Created on: 22.06.2014
 *      Author: endy
 */

#include <gtest/gtest.h>
#include "variables/serializer/serialize.h"

TEST(get_size, Ints)
{
	uint16_t v1 = 2u;
	EXPECT_EQ(sizeof(decltype(v1)), get_size(v1));
	EXPECT_EQ(2u, get_size(v1));

	uint32_t v2 = 10u;
	EXPECT_EQ(sizeof(decltype(v2)), get_size(v2));
	EXPECT_EQ(4u, get_size(v2));
}

TEST(get_size, Vector)
{
	std::vector<uint8_t> v1 {0u, 1u, 2u};
	EXPECT_EQ(sizeof(size_t) + v1.size(), get_size(v1));

	std::vector<uint64_t> v2 {0u, 8u};
	EXPECT_EQ(sizeof(size_t) + v2.size() * sizeof(uint64_t), get_size(v2));
}

TEST(get_size, String)
{
	std::string s1 = "hello";
	EXPECT_EQ(sizeof(size_t) + s1.length(), get_size(s1));

	std::string s2;
	EXPECT_EQ(sizeof(size_t), get_size(s2));
}

TEST(get_size, Tuple)
{
	auto t1 = std::make_tuple(0ul, std::string("hi"));
	EXPECT_EQ(sizeof(unsigned long) + sizeof(size_t) + 2u, get_size(t1));

	auto t2 = std::make_tuple(std::vector<uint16_t> {'a', 'b', 'c'}, 10, 20);
	EXPECT_EQ(sizeof(size_t) + 3 * 2 + sizeof(int) * 2, get_size(t2));
}

TEST(Serialize, Ints)
{
	uint32_t v1 = 10;
	StreamType res;
	serialize(v1, res);

	EXPECT_EQ(sizeof(uint32_t), res.size());
	EXPECT_EQ(res, std::vector<uint8_t>( {0xA, 0, 0, 0}));
	res.clear();

	uint64_t v2 = 64;
	serialize(v2, res);
	EXPECT_EQ(sizeof(uint64_t), res.size());
	EXPECT_EQ(res, std::vector<uint8_t>( {0x40, 0, 0, 0, 0, 0, 0, 0}));
	res.clear();

	int v3 = -1;
	serialize(v3, res);
	EXPECT_EQ(sizeof(int), res.size());
	EXPECT_EQ(res, std::vector<uint8_t>( {0xFF, 0xFF, 0xFF, 0xFF}));
}

TEST(Serialize, Vector)
{
	auto t1 = std::vector<int> {1, 2};
	StreamType res;
	serialize(t1, res);
	EXPECT_EQ(sizeof(decltype(t1)::value_type)*t1.size()+sizeof(size_t), res.size());
	EXPECT_EQ(res, std::vector<uint8_t>(
	{/*size(*/2, 0, 0, 0, 0, 0, 0, 0,/*)*/1, 0, 0, 0, 2, 0, 0, 0}));
	res.clear();

	auto t2 = std::vector<std::vector<uint8_t>>
	{
		{	1,2},
		{	3,4}};
	serialize(t2,res);
	EXPECT_EQ(get_size(t2), res.size());
	EXPECT_EQ(28u, res.size());
	EXPECT_EQ(res, std::vector<uint8_t>(
	{/*size(*/2, 0, 0, 0, 0, 0, 0, 0, /*) size(*/2, 0, 0, 0, 0, 0, 0, 0, /*)*/1, 2,
		/*size(*/2, 0, 0, 0, 0, 0, 0, 0, /*)*/3, 4}));
}

TEST(Serialize, IntTuple)
{

	auto t1 = std::make_tuple(1, 2);
	StreamType res;
	serialize(t1, res);
	EXPECT_EQ(sizeof(decltype(t1)), res.size());
	EXPECT_EQ(res, std::vector<uint8_t>( {1, 0, 0, 0, 2, 0, 0, 0}));
	res.clear();

	auto t2 = std::make_tuple(256, 256 * 2, 256 * 3);
	serialize(t2, res);
	EXPECT_EQ(sizeof(decltype(t2)), res.size());
	EXPECT_EQ(res, std::vector<uint8_t>( {0, 1, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0}));
	res.clear();

	auto t3 = std::tuple<uint32_t, uint64_t>(0, 1);
	serialize(t3, res);
	EXPECT_EQ(get_size(t3), res.size());
	EXPECT_EQ(res, std::vector<uint8_t>( {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0}));
}

TEST(Serialize, TupleVec)
{
	auto t1 = std::tuple<int, int, std::vector<uint8_t>>(10, 20, std::vector<uint8_t> {1, 2});
	StreamType res;
	serialize(t1, res);
	EXPECT_EQ(18u, res.size());
	EXPECT_EQ(res, std::vector<uint8_t>( {
	/*get<0>*/10,
											0, 0, 0,
											/*get<1>*/20,
											0, 0, 0,
											/*size(*/2,
											0, 0, 0, 0, 0, 0, 0,/*)*/1, 2}));
}

TEST(Serialize, String)
{
	std::string s = "string";
	StreamType res;
	serialize(s, res);
	EXPECT_EQ(14u, get_size(s));
	EXPECT_EQ(14u, res.size());
	EXPECT_EQ(res, std::vector<uint8_t>( {/*size(*/6, 0, 0, 0, 0, 0, 0, 0,/*)*/'s', 't', 'r', 'i', 'n', 'g'}));
}

TEST(Deserialize, Ints)
{
	int v1 = deserialize<uint32_t>( {0xA, 0, 0, 0});
	EXPECT_EQ(v1, 10);

	auto v2 = deserialize<uint64_t>( {0x40, 0, 0, 0, 0, 0, 0, 0});
	EXPECT_EQ(v2, 64u);

	auto v3 = deserialize<int>( {0xFF, 0xFF, 0xFF, 0xFF});
	EXPECT_EQ(v3, -1);
}

TEST(Deserialize, Vector)
{
	auto v1 = deserialize<std::vector<int>>( {2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0});
	EXPECT_EQ(v1, std::vector<int>( {1, 2}));

	auto v2 = deserialize<std::vector<std::vector<uint8_t>>>(
	{	2, 0, 0, 0, 0, 0, 0, 0, /* size */
		2, 0, 0, 0, 0, 0, 0, 0, /* size */1, 2,
		2, 0, 0, 0, 0, 0, 0, 0, /* size */3, 4
	});
	EXPECT_EQ(v2, std::vector<std::vector<uint8_t>>(
	{
		{	1,2},
		{	3,4}}));
}

TEST(Deserialize, IntTuple)
{
	auto t1 = deserialize<std::tuple<int, int>>( {1, 0, 0, 0, 2, 0, 0, 0});
	EXPECT_EQ(t1, std::make_tuple(1, 2));

	auto t2 = deserialize<std::tuple<int, int, char>>( {1, 0, 0, 0, 2, 0, 0, 0, 3});
	EXPECT_EQ(t2, std::make_tuple(1, 2, 3));
}

TEST(Deserialize, TupleVec)
{
	auto t = deserialize<std::tuple<int, int, std::vector<uint8_t>>>(
	{
		10, 0, 0, 0, /* get<0> */
		20, 0, 0, 0, /* get<1> */
		2, 0, 0, 0, 0, /* size */0, 0, 0, 1, 2 /* get<2> */
	});
	EXPECT_EQ(t, std::make_tuple(10,20,std::vector<uint8_t>(
					{	1,2})));
}
