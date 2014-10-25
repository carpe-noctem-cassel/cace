/*
 * timeEval.cpp
 *
 *  Created on: 10.07.2014
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

const int tries = 1;

using namespace cace;
using namespace std;
using namespace std::chrono;

double normalDistribution(double expectation, double varianz)
{
//Box-Muller-Methode
	double a = ((double)rand()) / ((double)RAND_MAX);
	double b = ((double)rand()) / ((double)RAND_MAX);
	return cos(2 * M_PI * a) * sqrt(-2 * log(b) * varianz) + expectation;
}

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

class TimeEval : public ::testing::Test
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
	int caceInstanceCount = 20;
	Cace** caces = nullptr;

	TimeEval() :
			sback(1234)
	{
		run = false;
		// You can do set-up work for each test here.
	}

	virtual ~TimeEval()
	{
		// You can do clean-up work that doesn't throw exceptions here.
	}

// If the constructor and destructor are not enough for setting up
// and cleaning up each test, you can define the following methods:

	virtual void SetUp()
	{
		caces = new Cace*[caceInstanceCount];
		for (int i = 1; i < caceInstanceCount + 1; i++)
		{
			caces[i - 1] = Cace::getEmulated("", i);

			for (int j = 1; j < caceInstanceCount + 1; j++)
			{
				if (i == j)
					continue;
				caces[i - 1]->activeRobots.push_back(j);
			}
			//caces[i - 1]->communication->startAsynchronous();
		}
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
		for (int i = 1; i < caceInstanceCount + 1; i++)
		{
			delete caces[i - 1];
		}
		delete caces;
		// Code here will be called immediately after each test (right
		// before the destructor).
	}

// Objects declared here can be used by all tests in the test case for Foo.

};

TEST_F(TimeEval, ConsistencyDelay)
{
	int delay = -5000000;
	for (long l = 0; l < caceInstanceCount; l++)
	{
		caces[l]->timeManager->timeEvalOffeset = 1000000000 * l;
		caces[l]->timeManager->timeReceiveEvalOffeset = delay;
	}

	cace::ctime t1, t2;

	for (long n = 0; n < 100; n += 1)
	{
		int delay = -5000000;
		for (long l = 0; l < caceInstanceCount; l++)
		{
			caces[l]->timeManager->timeEvalOffeset = 1000000000 * l;
			caces[l]->timeManager->timeReceiveEvalOffeset = delay;
			caces[l]->timeManager->clearQueues();
			caces[l]->timeManager->timeDiff = 0;
		}

		for (int i = 0; i < 50; i++)
		{
			for (long l = 0; l < caceInstanceCount; l++)
			{
				caces[l]->timeManager->timeReceiveEvalOffeset = delay
						+ (long)(normalDistribution(0, 7.5 * 7.5) * 1000000.0);
			}

			t1 = caces[0]->timeManager->getDistributedTime();
			t2 = caces[1]->timeManager->getDistributedTime();

			cout << i << "\t" << (double)((long)t1 - (long)t2) / 1000000.0;
			//cout << "\t" << (long)caces[0]->timeManager->getAgentCommunicationDelay(2) + delay;
			//cout << "\t" << (long)caces[1]->timeManager->getAgentCommunicationDelay(1) + delay;
			cout << endl;

			/*if (abs((long)t1 - (long)t2) < 100000)
			 {
			 cout << delay << "\t" << i << endl;
			 break;
			 }*/

			for (long l = 0; l < caceInstanceCount; l++)
			{
				caces[l]->communication->sendTime(caces[l]->timeManager);
				this_thread::sleep_for(chrono::milliseconds(1));
			}
		}
		//this_thread::sleep_for(chrono::milliseconds(1000));
		cout << endl;
	}

	//com->sendTime(this);
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
}
