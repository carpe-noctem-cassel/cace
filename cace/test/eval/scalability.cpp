/*
 * scalability.cpp
 *
 *  Created on: 24.07.2014
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
#include <communication/CaceCommunicationMultiCast.h>
#include <communication/multicast/CaceMultiCastChannel.h>
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

using namespace cace;



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




int main(int argc, char **argv)
{
	Cace **caces;
	caces = new Cace*[100];
	string name = "test";

	acceptStrategy strat = acceptStrategy::TwoWayHandShake;
	if(argc > 1) {
		if(string(argv[1])=="1") {
			strat = acceptStrategy::FireAndForget;
			cerr << "OneWay" << endl;
		}
		if(string(argv[1])=="2") {
			strat = acceptStrategy::TwoWayHandShake;
			cerr << "TwoWay" << endl;
		}
		if(string(argv[1])=="3") {
			strat = acceptStrategy::ThreeWayHandShake;
			cerr << "ThreeWay" << endl;
		}

	}

	int maxAgentCount = 30;

	for (int i = 2; i < maxAgentCount; i++)
	{
		//init
		for (int n = 0; n < i; n++)
		{
			caces[n] = Cace::getEmulated("", n + 1, false);
			caces[n]->safeStepMode = true;

			for (int m = 0; m < i; m++)
			{
				caces[n]->agentEngangement(m + 1, false);
			}
			//caces[n]->run();
		}
		for (double prop=0; prop<0.6; prop+=0.1) {
			cerr << i << " " << prop << endl;
			IncrementalEstimator ie;
			for(int tries=0; tries < 100; tries++) {
				// delete all
				for (int n = 0; n < i; n++)
				{
					caces[n]->variableStore->deleteAllVariables();
					caces[n]->worker->clearJobs();
				}

				string tname = name+to_string(tries);
				auto v = make_shared<ConsensusVariable>(tname, strat,
														std::numeric_limits<long>::max(), caces[0]->communication->getOwnID(),
														caces[0]->timeManager->getDistributedTime(),
														caces[0]->timeManager->lamportTime, CaceType::Custom);

				cacemulticast::CaceMultiCastChannel<CaceCommunicationMultiCast>::packetLossPropability = prop;
				cacemulticast::CaceMultiCastChannel<CaceCommunicationMultiCast>::traffic = 0;
				caces[0]->caceSpace->distributeVariable(v);
				caces[0]->step();
				this_thread::sleep_for(chrono::microseconds(1000));
				//step all
				bool finished = false;
				while (!finished)
				{
					for (int n = 1; n < i; n++)
					{
						caces[n]->step();
						this_thread::sleep_for(chrono::microseconds(1000));
					}

					caces[0]->step();
					this_thread::sleep_for(chrono::microseconds(3000));

					//determine finished
					finished = true;
					for (int n = 0; n < i; n++)
					{
						if (caces[n]->worker->count() > 0)
						{
							finished = false;
						}
					}
				}
				ie.addData(cacemulticast::CaceMultiCastChannel<CaceCommunicationMultiCast>::traffic);
				cerr << "." << flush;
			}
			cerr << endl;

			cout << "Instancecount: " << i << " Traffic: "
					<< ie.toString() << " " << cacemulticast::CaceMultiCastChannel<CaceCommunicationMultiCast>::packetLossPropability << endl;
		}
		// delete all
		for (int n = 1; n < i; n++)
		{
			delete caces[n];
		}

	}

}
