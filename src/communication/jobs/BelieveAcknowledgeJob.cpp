/*
 * BelieveAcknowledgeJob.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include <boost/smart_ptr/shared_ptr.hpp>
#include <cace/CaceShortAck.h>
#include <cace.h>
#include <communication/CaceCommunication.h>
#include <communication/jobs/BelieveAcknowledgeJob.h>
#include <CaceTypes.h>
#include <Configuration.h>
#include <SystemConfig.h>
#include <timeManager/TimeManager.h>
#include <variables/ConsensusVariable.h>
#include <variableStore/CVariableStore.h>
#include <algorithm>
#include <iterator>

namespace cace
{

	BelieveAcknowledgeJob::BelieveAcknowledgeJob(string name, shared_ptr<ConsensusVariable> variable,
													vector<int> robotids, unsigned long lamportTime, Cace* cace,
													CaceBelieveNotificationPtr notification) :
			AbstractCommunicationJob(name, variable, robotids, lamportTime, cace)
	{
		// /todo
		//int maxRetrys = SystemConfig.LocalInstance["Cace"].GetInt("Cace.MaxCommandRetrys");
		msgID = notification->msgID;
		updatedOwnBelieve = true;
		this->notification = notification;
		supplementary::SystemConfig* sc = supplementary::SystemConfig::getInstance();

		maxRetrys = (*sc)["Cace"]->get<int>("Cace.MaxCommandRetrys", NULL);

		if (!cace->safeStepMode)
		{
			//Console.WriteLine(this.ToString());
			shared_ptr<ConsensusVariable> cv = doVariableUpdate();
			if (updatedOwnBelieve)
			{
				//send ack
				if (notification->level > acceptStrategy::FireAndForget)
				{
					vector<uint8_t> val = cv->getValue();
					for (int id : expectedRobotIDs)
					{
						caceCommunication->sendCaceAcknowledge(cv->getName(), val, msgID, id, cv->getType());
						lamportTime = cace->timeManager->lamportTime;
						entities.push_back(
								JobStateEntity(
										id, maxRetrys,
										cace->timeManager->getLocalTime() / cace->timeManager->timeResolutionDevisor));
					}
				}
			}
		}
	}

	BelieveAcknowledgeJob::~BelieveAcknowledgeJob()
	{
	}

	string BelieveAcknowledgeJob::toString()
	{
		return "BAck " + notification->variableName + "=" + to_string(notification->value.size()) + "\tattempts: "
				+ to_string(attempts) + "\tMsgID: " + to_string(notification->msgID);
	}

	bool BelieveAcknowledgeJob::process()
	{
		if (attempts == 0 && cace->safeStepMode)
		{
			//Console.WriteLine(this.ToString());
			shared_ptr<ConsensusVariable> cv = doVariableUpdate();
			if (updatedOwnBelieve)
			{
				//send ack
				if (notification->level > acceptStrategy::FireAndForget)
				{
					vector<uint8_t> val = cv->getValue();
					for (int id : expectedRobotIDs)
					{
						caceCommunication->sendCaceAcknowledge(cv->getName(), val, msgID, id, cv->getType());
						lamportTime = cace->timeManager->lamportTime;
						entities.push_back(
								JobStateEntity(
										id, maxRetrys,
										cace->timeManager->getLocalTime() / cace->timeManager->timeResolutionDevisor));
					}
				}
			}
		}
		if (!updatedOwnBelieve)
			return true;

		attempts++;

		list<CaceShortAckPtr> newAcks = caceCommunication->getCaceShortAcksAndRemove(msgID);
		for (CaceShortAckPtr ca : newAcks)
		{
			for (int i = 0; i < expectedRobotIDs.size(); i++)
			{
				if (expectedRobotIDs.at(i) == ca->senderID)
				{
					expectedRobotIDs.erase(expectedRobotIDs.begin() + i);
					i--;
				}
			}
		}

		if (notification->level > acceptStrategy::FireAndForget)
		{

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
						caceCommunication->sendCaceAcknowledge(notification->variableName, notification->value, msgID,
																(short)i, notification->type, lamportTime);
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

		return expectedRobotIDs.size() == 0 || remainingRetrys <= 0;
	}

	bool BelieveAcknowledgeJob::failed()
	{
		return remainingRetrys <= 0;
	}

	shared_ptr<ConsensusVariable> BelieveAcknowledgeJob::doVariableUpdate()
	{
		shared_ptr<ConsensusVariable> cv;
		CVariableStore* store = cace->variableStore;
		if (store->existsVariable(notification->variableName))
		{
			cv = store->getVariable(notification->variableName);
			cv = store->getVariable(notification->variableName);

			bool found = false;
			for (auto var : cv->proposals)
			{
				if (var->getRobotID() == notification->senderID)
				{
					found = true;
					if (var->getLamportAge() <= notification->lamportTime)
					{
						var->setValue(notification->value);
						var->setDecissionTime(notification->decissionTime);
						var->setValidityTime(notification->validityTime);
						var->setLamportAge(notification->lamportTime);
						var->setAcceptStrategy((acceptStrategy)notification->level);
						var->setType(notification->type);
					}
				}
			}
			if (!found)
			{
				//add believe
				auto var = make_shared<ConsensusVariable>(notification->variableName,
																(acceptStrategy)notification->level,
																notification->validityTime, notification->senderID,
																notification->decissionTime, notification->lamportTime,
																notification->type);
				var->setValue(notification->value);
				var->setRobotID(notification->senderID);
				cv->proposals.push_back(var);
			}
			cv->acceptProposals(*cace, nullptr);
		}
		else
		{
			//If we don't know anything: Accept Command
			cv = make_shared<ConsensusVariable>(notification->variableName, (acceptStrategy)notification->level,
												notification->validityTime, caceCommunication->getOwnID(),
												notification->decissionTime, notification->lamportTime,
												notification->type);
			auto co = make_shared<ConsensusVariable>(notification->variableName,
															(acceptStrategy)notification->level,
															notification->validityTime, notification->senderID,
															notification->decissionTime, notification->lamportTime,
															notification->type);
			co->setValue(notification->value);
			cv->proposals.push_back(co);

			cv->acceptProposals(*cace, &notification->value);
			updatedOwnBelieve = true;
			store->addVariable(cv);
		}

		return cv;
	}

} /* namespace cace */
