/*
 * multicasttests.cpp
 *
 *  Created on: 23.06.2014
 *      Author: endy
 */

#include <gtest/gtest.h>
#include <gtest/gtest-message.h>
#include <gtest/internal/gtest-internal.h>
#include <gtest/internal/gtest-string.h>

#include "communication/multicast/PracticalSocket.h"
#include "communication/multicast/CaceMultiCastChannel.h"


using namespace cacemulticast;

class MulticastTests : public ::testing::Test
{
protected:
	CaceMultiCastChannel<MulticastTests>* channel;
	UDPSocket sock;
	string address;
	unsigned short port;

	MulticastTests()
	{
		address = "225.0.0.37";
		port = 12345;
	}

	virtual ~MulticastTests()
	{
		// You can do clean-up work that doesn't throw exceptions here.
	}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp()
	{
		channel = new CaceMultiCastChannel<MulticastTests>(address, port, &MulticastTests::handle, this);
		sock.joinGroup(address);
	}

	virtual void TearDown()
	{
		sock.disconnect();
		delete channel;
	}

	string received;
	int receivedBytes=-1;
	void handle(char* packet, int size)
	{
		packet[size]='\0';
		received = packet;
		receivedBytes = size;
	}

};

TEST_F(MulticastTests, ChannelRecvSend)
{
	string data = "test";
	sock.sendTo(data.c_str(), data.length(), address, port);
	//channel->publish(data.c_str(), data.length());
	this_thread::sleep_for(chrono::milliseconds(1));
	EXPECT_EQ(receivedBytes, data.length()) << "Wrong number of received bytes";
	EXPECT_EQ(received, data) << "Received String Wrong";
}

TEST_F(MulticastTests, ChannelSend)
{
	string data = "test2";
	channel->publish(data.c_str(), data.length());
	this_thread::sleep_for(chrono::milliseconds(1));
	EXPECT_EQ(receivedBytes, data.length()) << "Wrong number of received bytes";
	EXPECT_EQ(received, data) << "Received String Wrong";
}
