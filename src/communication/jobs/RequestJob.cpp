/*
 * RequestJob.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include <caceSpace.h>
#include <communication/CaceCommunication.h>
#include <communication/jobs/RequestJob.h>
#include <communication/JobStateEntity.h>
#include <Configuration.h>
#include <SystemConfig.h>
#include <timeManager/TimeManager.h>
#include <variables/ConsensusVariable.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace cace
{

	RequestJob::RequestJob(string name, shared_ptr<ConsensusVariable> variable, vector<int> robotids,
							unsigned long lamportTime, Cace* cace) :
			AbstractCommunicationJob(name, variable, robotids, lamportTime, cace)
	{
		this->name = name;
		target = robotids[0];
		//trying to produce a uniq msg id
		msgID = (short)((int)(cace->timeManager->lamportTime + 100) * caceCommunication->getOwnID());

		supplementary::SystemConfig* sc = supplementary::SystemConfig::getInstance();

		maxRetrys = (*sc)["Cace"]->get<int>("Cace.MaxCommandRetrys", NULL);
		caceCommunication->sendCaceVariableRequest(target, msgID, name);
		lamportTime = cace->timeManager->lamportTime;
		for (int id : expectedRobotIDs)
		{
			entities.push_back(
					JobStateEntity(id, maxRetrys,
									cace->timeManager->getLocalTime() / cace->timeManager->timeResolutionDevisor));
		}
	}

	RequestJob::~RequestJob()
	{
	}

	string RequestJob::toString()
	{
		return "Req " + name + "\t attempts" + to_string(attempts) + "\tMissing Acks: "
				+ to_string(expectedRobotIDs.size()) + "\tMsgID: " + to_string(msgID);
	}

	bool RequestJob::process()
	{
		attempts++;

		//Check for response
		if (cace->caceSpace->checkAvailableResponse(name))
		{
			expectedRobotIDs.clear();
			return true;
		}

		remainingRetrys = 0;
		for (JobStateEntity& jse : this->entities)
		{

			for (int i : expectedRobotIDs)
			{
				if (i == jse.robotID)
				{
					remainingRetrys += max(jse.retrys, 0);
				}
				if (jse.robotID == i && jse.retrys > 0
						&& jse.lastSent + cace->timeManager->getEstimatedResendTime(i)
								< cace->timeManager->getLocalTime() / cace->timeManager->timeResolutionDevisor)
				{
					//Console.WriteLine(cace.TimeManager.GetEstimatedResetTime(i));
					caceCommunication->sendCaceVariableRequest((short)i, msgID, name, lamportTime);
					jse.lastSent = cace->timeManager->getLocalTime() / cace->timeManager->timeResolutionDevisor;
					jse.retrys--;
					if (jse.retrys <= 0)
					{
						cace->agentDisengangement(jse.robotID);
					}
				}
			}
		}

		return expectedRobotIDs.size() == 0 || remainingRetrys <= 0;
	}

	bool RequestJob::failed()
	{
		return remainingRetrys <= 0;
	}

} /* namespace cace */
