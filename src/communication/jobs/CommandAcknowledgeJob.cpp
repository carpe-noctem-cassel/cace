/*
 * CommandAcknowledgeJob.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include <boost/smart_ptr/shared_ptr.hpp>
#include <cace/CaceShortAck.h>
#include <cace.h>
#include <communication/CaceCommunication.h>
#include <communication/jobs/CommandAcknowledgeJob.h>
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
	int CommandAcknowledgeJob::maxRetrys = 10;

	CommandAcknowledgeJob::CommandAcknowledgeJob(string name, shared_ptr<ConsensusVariable> variable,
													vector<int> robotids, unsigned long lamportTime, Cace* cace,
													CaceCommandPtr cmd) :
			AbstractCommunicationJob(name, variable, robotids, lamportTime, cace)
	{
		msgID = (short)cmd->msgID;
		updatedOwnBelieve = true;
		command = cmd;
		/*supplementary::SystemConfig* sc = supplementary::SystemConfig::getInstance();

		maxRetrys = (*sc)["Cace"]->get<int>("Cace.MaxCommandRetrys", NULL);*/

		if (!cace->safeStepMode)
		{
			//Console.WriteLine(this.ToString());
			doVariableUpdate();
			if (updatedOwnBelieve)
			{
				//send ack
				if (command->level > acceptStrategy::FireAndForget)
				{
					vector<uint8_t> val = cv->getValue();
					caceCommunication->sendCaceAcknowledge(cv->getName(), val, msgID, 0, cv->getType());
					lamportTime = cace->timeManager->lamportTime;
					for (int id : expectedRobotIDs)
					{
						entities.push_back(
								JobStateEntity(
										id, maxRetrys,
										cace->timeManager->getLocalTime() / cace->timeManager->timeResolutionDevisor));
					}
				}
			}
		}
	}

	CommandAcknowledgeJob::~CommandAcknowledgeJob()
	{
	}

	string CommandAcknowledgeJob::toString()
	{
		return "Ack " + command->variableName + "=" + cv->valueAsString() + "\tattempts: " + to_string(attempts)
				+ "\tCommand MsgID: " + to_string(command->msgID);
	}

	bool CommandAcknowledgeJob::process()
	{
		if (attempts == 0 && cace->safeStepMode)
		{
			doVariableUpdate();
			if (!updatedOwnBelieve)
				return true;
			//send ack
			if (command->level > acceptStrategy::FireAndForget)
			{
				vector<uint8_t> val = cv->getValue();
				caceCommunication->sendCaceAcknowledge(cv->getName(), val, msgID, 0, cv->getType());
				lamportTime = cace->timeManager->lamportTime;

				for (int id : expectedRobotIDs)
				{
					entities.push_back(
							JobStateEntity(
									id, maxRetrys,
									cace->timeManager->getLocalTime() / cace->timeManager->timeResolutionDevisor));
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

		if (command->level > acceptStrategy::FireAndForget)
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
						vector<uint8_t> val = cv->getValue();
						caceCommunication->sendCaceAcknowledge(cv->getName(), val, msgID, (short)i, cv->getType(),
																lamportTime);
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
				|| command->level <= (short)acceptStrategy::TwoWayHandShake;
	}

	bool CommandAcknowledgeJob::failed()
	{
		return remainingRetrys <= 0;
	}

	shared_ptr<ConsensusVariable> CommandAcknowledgeJob::doVariableUpdate()
	{
		CVariableStore* store = cace->variableStore;
		cv = store->getVariable(command->variableName);
		if (store->existsVariable(command->variableName))
		{
			cv = store->getVariable(command->variableName);

			bool found = false;
			for (auto var : cv->proposals)
			{
				if (var->getRobotID() == command->senderID)
				{
					found = true;
					if (var->getLamportAge() <= command->lamportTime)
					{
						var->setValue(command->value);
						var->setDecissionTime(command->decissionTime);
						var->setValidityTime(command->validityTime);
						var->setLamportAge(command->lamportTime);
						var->setAcceptStrategy((acceptStrategy)command->level);
						var->setType(command->type);
					}
				}
			}
			if (!found)
			{
				//add believe
				auto var = make_shared<ConsensusVariable>(command->variableName, (acceptStrategy)command->level,
																command->validityTime, command->senderID,
																command->decissionTime, command->lamportTime,
																command->type);
				var->setValue(command->value);
				var->setRobotID(command->senderID);
				cv->proposals.push_back(var);
			}
			cv->acceptProposals(*cace, nullptr);
		}
		else
		{
			//If we don't know anything: Accept Command
			cv = make_shared<ConsensusVariable>(command->variableName, (acceptStrategy)command->level,
												command->validityTime, caceCommunication->getOwnID(),
												command->decissionTime, command->lamportTime, command->type);
			auto co = make_shared<ConsensusVariable>(command->variableName, (acceptStrategy)command->level,
															command->validityTime, command->senderID,
															command->decissionTime, command->lamportTime,
															command->type);
			co->setValue(command->value);
			cv->proposals.push_back(co);
			cv->acceptProposals(*cace, &command->value);
			updatedOwnBelieve = true;
			store->addVariable(cv);
		}

		return cv;
	}

} /* namespace cace */
