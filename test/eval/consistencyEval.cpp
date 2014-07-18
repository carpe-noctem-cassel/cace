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
	~TimeMeasure()
	{
	}
	Cace *cace;bool initiator;
	shared_ptr<ConsensusVariable> v1;
	unsigned long time;
	int count = 0;
	IncrementalEstimator ie1;
	IncrementalEstimator ie2;
	long lastConsistentValue, lastConsensedValue;
	void notifyChange(ConsensusVariable* v)
	{
		time = cace->timeManager->getLocalTime();

		long sendingtime;
		v->getValue(&sendingtime);

		if (initiator && v->isAcknowledged(*cace))
		{
			ie1.addData(time - sendingtime);
			cout << "ConsensusAchieved: " << ie1.toString() << endl;
			count++;
		}
		if (!initiator)
		{
			long curConsistentCommandedValue = std::numeric_limits<long>::min();
			long curConsistentAckValue = std::numeric_limits<long>::min();
			for (auto prop : v->proposals)
			{
				if (prop->getRobotID() == 1)
				{
					prop->getValue(&curConsistentCommandedValue);
				}
				else
				{
					prop->getValue(&curConsistentAckValue);
				}
			}

			bool newcmd = false;
			bool newack = false;
			if (lastConsistentValue < curConsistentCommandedValue)
			{
				lastConsistentValue = curConsistentCommandedValue;
				newcmd = true;
			}
			if (lastConsensedValue < curConsistentAckValue)
			{
				lastConsensedValue = curConsistentAckValue;
				newack = true;
			}

			if (newcmd)
			{
				ie1.addData(time - sendingtime);
				cout << "ConsistencyAchieved: " << ie1.toString() << endl;
			}
			if (!v->checkConflict(*cace) && (newcmd || newack))
			{

				ie2.addData(time - sendingtime);
				cout << "ConsensusAchieved: " << ie2.toString() << endl;
			}

		}
	}
};

int main(int argc, char **argv)
{
	TimeMeasure V1Time;
	if (argc > 1)
	{
		V1Time.cace = Cace::getEmulated("", 1);
	}
	else
	{
		V1Time.cace = Cace::get();
	}
	V1Time.cace->agentEngangement(1, false);
	V1Time.cace->agentEngangement(5, false);
	V1Time.cace->agentEngangement(6, false);

	string name = "test";
	V1Time.v1 = make_shared<ConsensusVariable>(name, acceptStrategy::NoDistribution, std::numeric_limits<long>::max(),
												V1Time.cace->communication->getOwnID(),
												V1Time.cace->timeManager->getDistributedTime(),
												V1Time.cace->timeManager->lamportTime, CaceType::Custom);
	V1Time.cace->caceSpace->addVariable(V1Time.v1, false);
	V1Time.v1->changeNotify.push_back(delegate<void(ConsensusVariable*)>(&V1Time, &TimeMeasure::notifyChange));

	V1Time.v1->setAcceptStrategy(acceptStrategy::ThreeWayHandShake);
	//V1Time.v1->setAcceptStrategy(acceptStrategy::TwoWayHandShake);
	V1Time.cace->run();
	V1Time.cace->communication->startAsynchronous();
	V1Time.cace->safeStepMode = false;
	V1Time.count = 0;
	V1Time.ie1.clear();
	V1Time.ie2.clear();
	V1Time.lastConsensedValue = 0;
	V1Time.lastConsistentValue = 0;

	this_thread::sleep_for(chrono::milliseconds(30000));

	if (argc > 1)
	{
		V1Time.initiator = true;
		// "sender"
		for (int i = 0; i < 100; i++)
		{
			V1Time.v1->setValue((long)V1Time.cace->timeManager->getLocalTime());
			V1Time.cace->caceSpace->distributeVariable(V1Time.v1);
			while (V1Time.cace->worker->count() != 0)
			{
				this_thread::sleep_for(chrono::milliseconds(100));
			}
			this_thread::sleep_for(chrono::milliseconds(500));
		}
	}
	else
	{
		V1Time.initiator = false;
		while (true)
		{
			this_thread::sleep_for(chrono::milliseconds(1));
			//V1Time.cace->step();
		}
		// "receiver"
	}
}
