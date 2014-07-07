/*
 * WriteJob.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include <cace.h>
#include <communication/CaceCommunication.h>
#include <communication/jobs/WriteJob.h>
#include <Configuration.h>
#include <SystemConfig.h>
#include <timeManager/TimeManager.h>
#include <algorithm>
#include <iterator>

namespace cace
{

	WriteJob::WriteJob(string& name, shared_ptr<ConsensusVariable> variable, vector<uint8_t> value,
						vector<int>& robotids, unsigned long lamportTime, Cace* cace) :
			AbstractCommunicationJob(name, variable, robotids, lamportTime, cace)
	{
		remainingRetrys=1;
		this->name = name;
		this->target = robotids.at(0);
		msgID = (short)((int)(cace->timeManager->lamportTime + 100) * caceCommunication->getOwnID());

		supplementary::SystemConfig* sc = supplementary::SystemConfig::getInstance();

		maxRetrys = (*sc)["Cace"]->get<int>("Cace.MaxCommandRetrys", NULL);

		caceCommunication->sendWriteCommand(variable, msgID, target);
		lamportTime = cace->timeManager->lamportTime;
		for (int& id : robotids)
		{
			entities.push_back(
					JobStateEntity(id, maxRetrys,
									cace->timeManager->getLocalTime() / TimeManager::timeResolutionDevisor));
		}

	}

	WriteJob::~WriteJob()
	{
	}

	string WriteJob::toString()
	{
		return "Wrt " + variable->getName() + "=" + variable->valueAsString() + "\t attempts" + to_string(attempts)
				+ "\tMissing Acks: " + to_string(expectedRobotIDs.size()) + "\tMsgID: " + to_string(msgID);
	}

	bool WriteJob::process()
	{
		attempts++;
		list<CaceShortAckPtr> newAcks = caceCommunication->getWriteAcknowledgesAndRemove(msgID);
		acks.splice(acks.end(), newAcks);

		//Check for variable update (if something "newer" updated our variable we are not interessted in delivering our massge anymore!?)
		for (int i = 0; i < expectedRobotIDs.size(); i++)
		{
			for (auto it = acks.begin(); it != acks.end(); it++)
			{
				CaceShortAckPtr ack = *it;
				if (ack->senderID == expectedRobotIDs.at(i))
				{
					expectedRobotIDs.erase((expectedRobotIDs.begin() + i));
					i--;
				}
			}
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
					caceCommunication->sendWriteCommand(variable, msgID, (short)i);
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

	bool WriteJob::failed()
	{
		return remainingRetrys <= 0;
	}

} /* namespace cace */
