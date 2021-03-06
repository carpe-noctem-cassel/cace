/*
 * BelieveNotificationJob.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include <cace.h>
#include <communication/CaceCommunication.h>
#include <communication/jobs/BelieveNotificationJob.h>
#include <CaceTypes.h>
#include <Configuration.h>
#include <SystemConfig.h>
#include <timeManager/TimeManager.h>
#include <variables/ConsensusVariable.h>
#include <algorithm>
#include <iterator>

namespace cace
{

	BelieveNotificationJob::BelieveNotificationJob(string name, shared_ptr<ConsensusVariable> variable,
													vector<uint8_t> value, vector<int> robotids,
													unsigned long lamportTime, Cace* cace) :
			AbstractCommunicationJob(name, variable, robotids, lamportTime, cace)
	{
		remainingRetrys=1;
		//trying to produce a uniq msg id
		msgID = (short)((int)(cace->timeManager->lamportTime + 100) * caceCommunication->getOwnID());

		supplementary::SystemConfig* sc = supplementary::SystemConfig::getInstance();

		maxRetrys = (*sc)["Cace"]->get<int>("Cace.MaxCommandRetrys", NULL);

		if (variable->getAcceptStrategy() > acceptStrategy::NoDistribution)
		{
			for (int id : robotids)
			{
				caceCommunication->sendCaceBelieveNotification(variable, id, msgID);
				lamportTime = cace->timeManager->lamportTime;
				entities.push_back(
						JobStateEntity(id, maxRetrys,
										cace->timeManager->getLocalTime() / TimeManager::timeResolutionDevisor));
			}
		}

	}

	BelieveNotificationJob::~BelieveNotificationJob()
	{
	}

	string BelieveNotificationJob::toString()
	{
		return string("BNot ") + "\tMsgID: " + to_string(msgID);
	}

	bool BelieveNotificationJob::process()
	{
		attempts++;
		//caceCommunication->getAcknowledgesAndRemove(msgID);

		//Check for variable update (if something "newer" updated our variable we are not interessted in delivering our massge anymore!?)
		for (int i = 0; i < expectedRobotIDs.size(); i++)
		{
			for (auto var : variable->proposals)
			{
				if (var->getRobotID() == expectedRobotIDs.at(i))
				{
					if (var->getLamportAge() >= lamportTime)
					{
						expectedRobotIDs.erase((expectedRobotIDs.begin() + i));
						i--;
						break;
					}
				}
			}
		}

		if (variable->getAcceptStrategy() > acceptStrategy::FireAndForget)
		{
			//After TimeBeforeCommandRetry milliseconds - Resend Command

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
						caceCommunication->sendCaceBelieveNotification(variable, msgID, (short)i, lamportTime);
						jse.lastSent = cace->timeManager->getLocalTime() / cace->timeManager->timeResolutionDevisor;
						jse.retrys--;
						if (jse.retrys <= 0)
						{
							cace->agentDisengangement(jse.robotID);
						}
					}
				}
			}
		}

		return expectedRobotIDs.size() == 0 || remainingRetrys <= 0
				|| variable->getAcceptStrategy() <= acceptStrategy::NoDistribution;
	}

	bool BelieveNotificationJob::failed()
	{
		return remainingRetrys <= 0;
	}

} /* namespace cace */
