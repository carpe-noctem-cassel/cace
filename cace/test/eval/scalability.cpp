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

int main(int argc, char **argv)
{
	Cace **caces;
	caces = new Cace*[100];
	string name = "test";

	for (int i = 2; i < 100; i++)
	{
		//init
		cacemulticast::CaceMultiCastChannel<CaceCommunicationMultiCast>::traffic = 0;
		for (int n = 0; n < i; n++)
		{
			caces[n] = Cace::getEmulated("", n + 1, false);
			caces[n]->safeStepMode = true;

			for (int m = 0; m < i; m++)
			{
				caces[n]->agentEngangement(m + 1, false);
			}
		}

		auto v = make_shared<ConsensusVariable>(name, acceptStrategy::ThreeWayHandShake,
												std::numeric_limits<long>::max(), caces[0]->communication->getOwnID(),
												caces[0]->timeManager->getDistributedTime(),
												caces[0]->timeManager->lamportTime, CaceType::Custom);
		caces[0]->caceSpace->distributeVariable(v);

		//step all
		bool finished = false;
		while (!finished)
		{
			for (int n = 0; n < i; n++)
			{
				caces[n]->step();
			}
			this_thread::sleep_for(chrono::milliseconds(1));

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

		cout << "Instancecount: " << i << " Traffic: "
				<< cacemulticast::CaceMultiCastChannel<CaceCommunicationMultiCast>::traffic << endl;
		// delete all
		for (int n = 1; n < i; n++)
		{
			delete caces[n];
		}
	}
}
