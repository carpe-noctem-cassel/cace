/*
 * speed.cpp
 *
 *  Created on: 25.06.2014
 *      Author: endy
 */

#include <boost/asio.hpp>
#include "boost/bind.hpp"
#include <communication/multicast/PracticalSocket.h>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <cace/CaceAcknowledge.h>
#include <cace/CaceType.h>
#include <cace.h>
#include <caceSpace.h>
#include <communication/CaceCommunication.h>
#include <communication/CommunicationWorker.h>
#include <communication/jobs/AbstractCommunicationJob.h>
#include <communication/jobs/CommandJob.h>
#include <communication/JobStateEntity.h>
#include <CaceTypes.h>
#include <gtest/gtest.h>
#include <gtest/gtest-message.h>
#include <gtest/internal/gtest-internal.h>
#include <gtest/internal/gtest-string.h>
#include <ros/init.h>
#include <timeManager/AgentCommunicationModel.h>
#include <timeManager/AgentTimeData.h>
#include <timeManager/TimeManager.h>
#include <variables/ConsensusVariable.h>
#include <variableStore/CVariableStore.h>
#include <cstdint>
#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <thread>

const int tries = 10000;

using namespace cace;
using namespace std;
using namespace std::chrono;

class IncrementalEstimator
{
public:
	double n;
	double mean;
	double m2;
	double min;
	double max;
	IncrementalEstimator()
	{
		clear();
	}

	void addData(double x)
	{
		if (x > max)
		{
			max = x;
		}
		if (x < min)
		{
			min = x;
		}

		n = n + 1.0;
		double delta = x - mean;
		mean = mean + delta / n;
		m2 = m2 + delta * (x - mean);
	}

	double getVariance()
	{
		if (n < 2)
		{
			return 0;
		}
		return m2 / (n - 1.0);
	}

	void clear()
	{
		n = 0;
		mean = 0;
		m2 = 0;
		min = numeric_limits<double>::max();
		max = numeric_limits<double>::min();
	}

	string toString()
	{
		return "m: " + to_string(mean) + "\tv: " + to_string(getVariance()) + "\tcount: " + to_string(n) + "\tmin: "
				+ to_string(min) + "\tmax: " + to_string(max);
	}

};

class TimeMeasure
{
public:
	bool first = true;
	high_resolution_clock::time_point time;
	void notifyChange(ConsensusVariable* v)
	{
		if (first)
		{
			time = high_resolution_clock::now();
			first = false;
		}
	}
};

class SpeedEval : public ::testing::Test
{
public:
	TimeMeasure V1Time;
	TimeMeasure V2Time;bool run;

	void thread()
	{
		string addr = "224.16.32.40";
		int size = 24;
		char* test = new char[size];
		while (run)
		{
			sback.sendTo(test, 1, addr, 1235);
			//cout << "." << endl;
		}
	}

protected:
	std::thread* t;
	cacemulticast::UDPSocket sback;
	Cace* cace1 = nullptr;
	Cace* cace2 = nullptr;

	SpeedEval() :
			sback(1234)
	{
		run = false;
		// You can do set-up work for each test here.
	}

	virtual ~SpeedEval()
	{
		// You can do clean-up work that doesn't throw exceptions here.
	}

// If the constructor and destructor are not enough for setting up
// and cleaning up each test, you can define the following methods:

	virtual void SetUp()
	{
		cace1 = Cace::getEmulated("", 1);
		cace2 = Cace::getEmulated("", 2);
		cace1->activeRobots.push_back(2);
		cace1->communication->startAsynchronous();
		cace2->activeRobots.push_back(1);
		cace2->communication->startAsynchronous();
		this_thread::sleep_for(chrono::milliseconds(1));
		// Code here will be called immediately after the constructor (right
		// before each test).
		run = true;
		//t = new std::thread(&SpeedEval::thread, this);
	}

	virtual void TearDown()
	{
		run = false;
		//t->join();
		//delete t;

		delete cace1;
		delete cace2;
		// Code here will be called immediately after each test (right
		// before the destructor).
	}

// Objects declared here can be used by all tests in the test case for Foo.

};

TEST_F(SpeedEval, Time)
{
//int tries = 10000000;
	int tries = 1000000;
	std::chrono::nanoseconds total_ns(0);
	for (int i = 0; i < tries; i++)
	{
		auto t0 = std::chrono::high_resolution_clock::now();
		auto t1 = std::chrono::high_resolution_clock::now();
		total_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0);
	}

	std::cout << " milliseconds: " << total_ns.count() / ((double)tries) << "ns\n";
}

TEST_F(SpeedEval, ConsistencyDelay)
{
	string name = "A";
	uint8_t type = CaceType::Custom;
	acceptStrategy strategy = acceptStrategy::NoDistribution;
	auto v1 = make_shared<ConsensusVariable>(name, strategy, std::numeric_limits<long>::max(),
												cace1->communication->getOwnID(),
												cace1->timeManager->getDistributedTime(),
												cace1->timeManager->lamportTime, type);
	auto v2 = make_shared<ConsensusVariable>(name, strategy, std::numeric_limits<long>::max(),
												cace1->communication->getOwnID(),
												cace1->timeManager->getDistributedTime(),
												cace1->timeManager->lamportTime, type);
	cace1->caceSpace->addVariable(v1, false);
	cace2->caceSpace->addVariable(v2, false);
	v1->changeNotify.push_back(delegate<void(ConsensusVariable*)>(&V1Time, &TimeMeasure::notifyChange));
	v2->changeNotify.push_back(delegate<void(ConsensusVariable*)>(&V2Time, &TimeMeasure::notifyChange));

//	vector<uint8_t> val;
//val.resize(5000);
	/*for (int i = 0; i < 8000; i++)
	 {
	 val.push_back(0);
	 }*/
//	v2->setValue(val);
	v2->setAcceptStrategy(acceptStrategy::TwoWayHandShake);
	nanoseconds timeRemoteArrival(0);
	nanoseconds timeFeedback(0);
	nanoseconds total(0);
	IncrementalEstimator arrival;
	IncrementalEstimator feedback;
	IncrementalEstimator consensus;

	high_resolution_clock::time_point before;

	for (int curSize = 0; curSize < 65536; curSize += 200)
	{
		/*delete cace1;
		delete cace2;

		cace1 = Cace::getEmulated("", 1);
		cace2 = Cace::getEmulated("", 2);
		cace1->activeRobots.push_back(2);
		cace1->communication->startAsynchronous();
		cace2->activeRobots.push_back(1);
		cace2->communication->startAsynchronous();
		this_thread::sleep_for(chrono::milliseconds(1));*/

		vector<uint8_t> val(curSize);
		v2->setValue(val);
		arrival.clear();
		feedback.clear();
		consensus.clear();
		for (int i = 0; i < tries; i++)
		{
			V1Time.first = true;
			V2Time.first = true;
			cace2->worker->clearJobs();
			cace2->communication->clearAllMessageLists();
			cace1->worker->clearJobs();
			cace1->communication->clearAllMessageLists();
			//val.resize(curSize);
			//v2->setValue(val);
			before = high_resolution_clock::now();
			cace2->caceSpace->distributeVariable(v2);
			//cace2->caceSpace->distributeValue(name, val, CaceType::Custom, acceptStrategy::TwoWayHandShake);
			for (int n = 0; n < 10; n++)
			{
				cace1->step();
				cace2->step();
			}
			this_thread::sleep_for(chrono::milliseconds(10));
			//timeRemoteArrival += duration_cast<std::chrono::nanoseconds>(V1Time.time - before);
			/*if ((duration_cast<std::chrono::nanoseconds>(V1Time.time - before).count() > 0
			 && duration_cast<std::chrono::nanoseconds>(V2Time.time - V1Time.time).count() > 0
			 && duration_cast<std::chrono::nanoseconds>(V2Time.time - before).count() > 0)
			 && (duration_cast<std::chrono::nanoseconds>(V1Time.time - before).count() < 400000
			 && duration_cast<std::chrono::nanoseconds>(V2Time.time - V1Time.time).count() < 400000
			 && duration_cast<std::chrono::nanoseconds>(V2Time.time - before).count() < 400000))
			 {*/

			arrival.addData(duration_cast<std::chrono::nanoseconds>(V1Time.time - before).count());
			//timeFeedback += duration_cast<std::chrono::nanoseconds>(V2Time.time - V1Time.time);
			feedback.addData(duration_cast<std::chrono::nanoseconds>(V2Time.time - V1Time.time).count());
			//total += duration_cast<std::chrono::nanoseconds>(V2Time.time - before);
			consensus.addData(duration_cast<std::chrono::nanoseconds>(V2Time.time - before).count());
			//cout << duration_cast<std::chrono::nanoseconds>(V1Time.time - before).count() << endl;
			/*}
			 else
			 {
			 cout << "#Lost Packet" << endl;
			 i--;
			 }*/
		}
		std::cout << "Cace - Size: " << curSize << "\tArrival\t" << arrival.toString() << " ns\t";
		std::cout << "Feedback: " << feedback.toString() << " ns\t";
		std::cout << "Consensus: " << consensus.toString() << " ns\t" << endl;
	}
}

class sender
{
public:
	void handle_send_to(const boost::system::error_code& error)
	{

	}
};

TEST_F(SpeedEval, MulticastDelay)
{
	sender s;
	string addr = "224.16.32.40";
	unsigned short p1 = 1234;
	unsigned short p2 = 1234;
	cacemulticast::UDPSocket s1(p1);
	cacemulticast::UDPSocket s2(p2);
	s1.setMulticastTTL(1);
	s1.joinGroup(addr);
	s2.setMulticastTTL(1);
	s2.joinGroup(addr);

	int size = 100000;
	char* test = new char[size];
	char* recv = new char[size];
	nanoseconds total_ns(0);
	//int tries = 1000;
	IncrementalEstimator ie;

	for (int curSize = 30000; curSize < 65536; curSize += 200)
	{
		ie.clear();
		for (int i = 0; i < tries; i++)
		{
			auto t0 = std::chrono::high_resolution_clock::now();
			s1.sendTo(test, curSize, addr, p2);
			s2.recvFrom(recv, curSize, addr, p1);
			auto t1 = std::chrono::high_resolution_clock::now();

			if (std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() > 0)
			{
				ie.addData(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
			}
			this_thread::sleep_for(chrono::milliseconds(10));
		}
		std::cout << "Std. Socket - Size: " << curSize << "\t" << ie.toString() << " ns" << endl;
	}
	delete[] test;
	delete[] recv;

	/*	int sockfd, n;
	 struct sockaddr_in servaddr, cliaddr;
	 char sendline[1000];
	 char recvline[1000];

	 sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	 bzero(&servaddr, sizeof(servaddr));
	 servaddr.sin_family = AF_INET;
	 servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	 servaddr.sin_port = htons(32000);

	 for (int i = 0; i < 20; i++)
	 {
	 auto t0 = std::chrono::high_resolution_clock::now();
	 sendto(sockfd, sendline, 1, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
	 auto t1 = std::chrono::high_resolution_clock::now();
	 std::cout << "Std. Socket nanoseconds " << duration_cast<std::chrono::nanoseconds>(t1 - t0).count() << "ns"
	 << endl;
	 //this_thread::sleep_for(chrono::milliseconds(100));
	 }
	 /*n = recvfrom(sockfd, recvline, 10000, 0, NULL, NULL);
	 recvline[n] = 0;
	 fputs(recvline, stdout);
	 */
}

// Run all the tests that were declared with TEST()
int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);

	ros::init(argc, argv, "Cace");

	int ret = RUN_ALL_TESTS();
	cout << "All tests Completed" << endl;
	ros::shutdown();

	return ret;
	/*cout << "Before" << endl;

	 TimeMeasure V1Time;
	 TimeMeasure V2Time;

	 Cace* cace1 = Cace::getEmulated("", 1);
	 Cace* cace2 = Cace::getEmulated("", 2);
	 cace1->activeRobots.push_back(2);
	 cace1->communication->startAsynchronous();
	 cace2->activeRobots.push_back(1);
	 cace2->communication->startAsynchronous();
	 this_thread::sleep_for(chrono::milliseconds(1));

	 string name = "A";
	 uint8_t type = CaceType::CDouble;
	 acceptStrategy strategy = acceptStrategy::NoDistribution;
	 auto v1 = make_shared<ConsensusVariable>(name, strategy, std::numeric_limits<long>::max(),
	 cace1->communication->getOwnID(),
	 cace1->timeManager->getDistributedTime(),
	 cace1->timeManager->lamportTime, type);
	 auto v2 = make_shared<ConsensusVariable>(name, strategy, std::numeric_limits<long>::max(),
	 cace1->communication->getOwnID(),
	 cace1->timeManager->getDistributedTime(),
	 cace1->timeManager->lamportTime, type);
	 cace1->caceSpace->addVariable(v1, false);
	 cace2->caceSpace->addVariable(v2, false);
	 v1->changeNotify.push_back(delegate<void(ConsensusVariable*)>(&V1Time, &TimeMeasure::notifyChange));
	 v2->changeNotify.push_back(delegate<void(ConsensusVariable*)>(&V2Time, &TimeMeasure::notifyChange));

	 vector<uint8_t> val={'a'};
	 //val.resize(5000);
	 //for (int i = 0; i < 8000; i++)
	 // {
	 // val.push_back(0);
	 // }
	 v2->setValue(val);
	 v2->setAcceptStrategy(acceptStrategy::TwoWayHandShake);
	 high_resolution_clock::time_point before = high_resolution_clock::now();
	 //cace2->caceSpace->distributeVariable(v2);
	 vector<int>* all = cace2->getActiveRobots();
	 cace2->worker->appendJob(
	 new CommandJob(v2->getName(), v2, v2->getValue(), *all, cace2->timeManager->lamportTime, cace2));

	 //cace2->caceSpace->distributeValue(name, val, CaceType::Custom, acceptStrategy::TwoWayHandShake);
	 //cace1->step();
	 //cace2->step();
	 //cace1->step();
	 this_thread::sleep_for(chrono::milliseconds(5000));
	 nanoseconds timeRemoteArrival = duration_cast<std::chrono::nanoseconds>(V1Time.time - before);
	 nanoseconds timeFeedback = duration_cast<std::chrono::nanoseconds>(V2Time.time - V1Time.time);
	 nanoseconds total = duration_cast<std::chrono::nanoseconds>(V2Time.time - before);
	 std::cout << " milliseconds: " << timeRemoteArrival.count() << "ns\n";
	 std::cout << " milliseconds: " << timeFeedback.count() << "ns\n";
	 std::cout << " milliseconds: " << total.count() << "ns\n";
	 return 1;*/
}
