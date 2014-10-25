/*
 * WriteAckJob.cpp
 *
 *  Created on: 30.05.2014
 *      Author: endy
 */

#include <boost/smart_ptr/shared_ptr.hpp>
#include <communication/CaceCommunication.h>
#include <communication/jobs/WriteAckJob.h>
#include <variables/ConsensusVariable.h>
#include <variableStore/CVariableStore.h>
#include <string>

namespace cace
{

	WriteAckJob::WriteAckJob(string name, shared_ptr<ConsensusVariable> variable, vector<int> robotids,
								unsigned long lamportTime, Cace* cace, CaceCommandPtr cmd) :
			AbstractCommunicationJob(name, variable, robotids, lamportTime, cace)
	{
		this->cmd = cmd;
		if (!cace->safeStepMode)
		{
			shared_ptr<ConsensusVariable> cv = doVariableUpdate();

			caceCommunication->sendCaceWriteAck(cv->getName(), cmd->msgID, cmd->senderID);
		}
	}

	WriteAckJob::~WriteAckJob()
	{
	}

	string WriteAckJob::toString()
	{
		return "WAck " + cmd->variableName + "=" + to_string(cmd->value.size()) + "\tattempts: " + to_string(attempts)
				+ "\tMsgID: " + to_string(cmd->msgID);
	}

	bool WriteAckJob::process()
	{
		if (attempts == 0 && cace->safeStepMode)
		{
			shared_ptr<ConsensusVariable> cv = doVariableUpdate();
			caceCommunication->sendCaceWriteAck(cv->getName(), cmd->msgID, cmd->senderID);
		}
		attempts++;

		return true;
	}

	shared_ptr<ConsensusVariable> WriteAckJob::doVariableUpdate()
	{
		shared_ptr<ConsensusVariable> cv;
		CVariableStore* store = cace->variableStore;

		if (store->existsVariable(cmd->variableName))
		{
			cv = store->getVariable(cmd->variableName);
			cv->setValue(cmd->value);
			cv->setArrivalTime(cace->timeManager->getLocalTime());
			cv->setDecissionTime(cmd->decissionTime);
			cv->setValidityTime(cmd->validityTime);
			cv->setLamportAge(cmd->lamportTime);
			cv->setType(cmd->type);
			cv->setAcceptStrategy((acceptStrategy)cmd->level);
			cv->setRobotID(caceCommunication->getOwnID());
			cv->notify();
		}
		else
		{
			//If we don't know anything
			cv = make_shared<ConsensusVariable>(cmd->variableName, (acceptStrategy)cmd->level, cmd->validityTime,
												caceCommunication->getOwnID(), cmd->decissionTime, cmd->lamportTime,
												cmd->type);
			cv->setValue(cmd->value);
			cv->setArrivalTime(cace->timeManager->getLocalTime());

			store->addVariable(cv);
		}

		return cv;
	}

} /* namespace cace */
